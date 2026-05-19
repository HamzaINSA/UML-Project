#include "AirQualityService.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <set>

#include "../model/Particulier.h"
#include "../repository/DataReader.h"
#include "AdministrationService.h"
#include "PerformanceMonitor.h"

namespace airwatcher {

namespace {
constexpr int kNbVoisinsInterpolation = 5;
const std::array<std::string, 4> kAttributs = {"O3", "SO2", "NO2", "PM10"};
}  // namespace

AirQualityService::AirQualityService(DataReader& data, PerformanceMonitor& perf)
    : data_(data), perf_(perf) {}

double AirQualityService::moyenneAttribut(const std::vector<Mesure>& mesures,
                                          const std::string& attributId) {
    double somme = 0.0;
    int n = 0;
    for (const auto& m : mesures) {
        if (m.getAttributId() == attributId && m.estValide()) {
            somme += m.getValeur();
            ++n;
        }
    }
    return n == 0 ? 0.0 : somme / n;
}

// Sous-indices ATMO 1..10 selon les seuils français usuels (µg/m³).
int AirQualityService::sousIndiceATMO(const std::string& attribut, double v) {
    if (attribut == "O3") {
        const double t[10] = {30, 55, 80, 105, 130, 150, 180, 210, 240, std::numeric_limits<double>::infinity()};
        for (int i = 0; i < 10; ++i) if (v <= t[i]) return i + 1;
    } else if (attribut == "NO2") {
        const double t[10] = {30, 55, 85, 110, 135, 165, 200, 275, 400, std::numeric_limits<double>::infinity()};
        for (int i = 0; i < 10; ++i) if (v <= t[i]) return i + 1;
    } else if (attribut == "SO2") {
        const double t[10] = {40, 80, 120, 160, 200, 250, 300, 400, 500, std::numeric_limits<double>::infinity()};
        for (int i = 0; i < 10; ++i) if (v <= t[i]) return i + 1;
    } else if (attribut == "PM10") {
        const double t[10] = {7, 14, 20, 27, 34, 41, 49, 64, 79, std::numeric_limits<double>::infinity()};
        for (int i = 0; i < 10; ++i) if (v <= t[i]) return i + 1;
    }
    return 0;
}

double AirQualityService::calculerIndiceATMO(const std::vector<Mesure>& mesures) const {
    int maxIndice = 0;
    for (const auto& a : kAttributs) {
        double moy = moyenneAttribut(mesures, a);
        if (moy <= 0.0) continue;
        int s = sousIndiceATMO(a, moy);
        if (s > maxIndice) maxIndice = s;
    }
    return static_cast<double>(maxIndice);
}


double AirQualityService::calculerScoreSimilarite(const std::vector<Mesure>& mesuresRef,
                                                  const std::vector<Mesure>& mesures) const {
    double sommeCarres = 0.0;
    for (const auto& a : kAttributs) {
        double mRef = moyenneAttribut(mesuresRef, a);
        double mC   = moyenneAttribut(mesures, a);
        double d = mRef - mC;
        sommeCarres += d * d;
    }
    return std::sqrt(sommeCarres);
}

double AirQualityService::calculerMoyenneAQI(double lat, double lon, double rayon,
                                             const DateTime& debut, const DateTime& fin) {
    perf_.demarrer("calculerMoyenneAQI");

    auto capteursZone = data_.getCapteursValidesDansRayon(lat, lon, rayon);
    std::vector<Mesure> toutesMesures;
    std::set<std::string> particuliersImpliques;

    for (auto* c : capteursZone) {
        auto mesures = data_.getMesuresPeriode(c->getId(), debut, fin);
        std::vector<Mesure> filtrees;
        for (auto& m : mesures) if (m.estValide()) filtrees.push_back(m);
        if (filtrees.empty()) continue;

        toutesMesures.insert(toutesMesures.end(), filtrees.begin(), filtrees.end());

        if (c->aProprietairePrive()) {
            particuliersImpliques.insert(c->getProprietaireId());
        }
    }

    double indiceATMO = calculerIndiceATMO(toutesMesures);

    if (admin_ && !particuliersImpliques.empty()) {
        std::vector<std::string> ids(particuliersImpliques.begin(), particuliersImpliques.end());
        admin_->attribuerPoints(ids);
    }

    perf_.arreter("calculerMoyenneAQI");
    return indiceATMO;
}

double AirQualityService::calculerMoyenneZonePendant(double lat, double lon, double rayon,
                                                     const DateTime& debut, const DateTime& fin) {
    return calculerMoyenneAQI(lat, lon, rayon, debut, fin);
}

double AirQualityService::calculerMoyenneZoneJusquaDateDebut(double lat, double lon, double rayon,
                                                             const DateTime& dateLimite) {
    return calculerMoyenneAQI(lat, lon, rayon, DateTime::min(), dateLimite);
}

std::vector<CapteurScore> AirQualityService::comparerCapteurs(const std::string& idRef,
                                                              const DateTime& debut,
                                                              const DateTime& fin) {
    perf_.demarrer("comparerCapteurs");

    auto mesuresRef     = data_.getMesuresPeriode(idRef, debut, fin);
    auto autresCapteurs = data_.getAutresCapteurs(idRef);
    std::vector<CapteurScore> classement;

    for (auto* c : autresCapteurs) {
        if (!c->estFiable()) continue;
        auto mesuresC = data_.getMesuresPeriode(c->getId(), debut, fin);
        bool hasValid = false;
        for (const auto& m : mesuresC) if (m.estValide()) { hasValid = true; break; }
        if (!hasValid) continue;
        double score = calculerScoreSimilarite(mesuresRef, mesuresC);
        classement.push_back({c, score});
    }

    trierParScoreCroissant(classement);

    perf_.arreter("comparerCapteurs");
    return classement;
}

void AirQualityService::trierParScoreCroissant(std::vector<CapteurScore>& classement) const {
    std::sort(classement.begin(), classement.end(),
              [](const CapteurScore& a, const CapteurScore& b) { return a.score < b.score; });
}

double AirQualityService::interpolerPondereeParDistance(
    const std::vector<std::pair<Capteur*, std::vector<Mesure>>>& proches,
    double lat, double lon) const {
    double sommeNum = 0.0;
    double sommeDen = 0.0;
    for (const auto& [c, mesures] : proches) {
        double d = DataReader::distanceKm(lat, lon, c->getLatitude(), c->getLongitude());
        if (d < 1e-6) {
            return calculerIndiceATMO(mesures);  // capteur exactement au point
        }
        double w = 1.0 / (d * d);
        double indice = calculerIndiceATMO(mesures);
        if (indice <= 0.0) continue;
        sommeNum += w * indice;
        sommeDen += w;
    }
    return sommeDen == 0.0 ? 0.0 : sommeNum / sommeDen;
}

double AirQualityService::estimerQualiteZone(double lat, double lon, double rayon,
                                             const DateTime& debut, const DateTime& fin) {
    return calculerMoyenneAQI(lat, lon, rayon, debut, fin);
}

} // namespace airwatcher
