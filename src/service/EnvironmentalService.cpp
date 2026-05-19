#include "EnvironmentalService.h"

#include "../model/Purificateur.h"
#include "../repository/DataReader.h"
#include "AirQualityService.h"
#include "PerformanceMonitor.h"

namespace airwatcher {

namespace {
constexpr double kRayonMax                       = 100.0; // km
constexpr double kSeuilAmeliorationSignificative = 0.5;   // points ATMO
}  // namespace

EnvironmentalService::EnvironmentalService(DataReader& data, PerformanceMonitor& perf,
                                           AirQualityService& airQuality)
    : data_(data), perf_(perf), airQuality_(airQuality) {}

double EnvironmentalService::calculerDelta(double indiceAvant, double indicePendant) const {
    return indiceAvant - indicePendant;  // positif = amélioration
}

double EnvironmentalService::estimerRayonAction(const Purificateur& purif) {
    double maxRayon = 0.0;

    for (const auto& c : data_.getCapteurs()) {
        double dist = DataReader::distanceKm(
            purif.getLatitude(), purif.getLongitude(),
            c.getLatitude(),     c.getLongitude());
        // On considère que le purificateur n'a pas d'impact au-delà de kRayonMax km.
        if (dist > kRayonMax) continue; 

        auto mesAvant   = data_.getMesuresPeriode(c.getId(), DateTime::min(), purif.getDateDebut());
        auto mesPendant = data_.getMesuresPeriode(c.getId(), purif.getDateDebut(), purif.getDateFin());

        double avant   = airQuality_.calculerIndiceATMO(mesAvant);
        double pendant = airQuality_.calculerIndiceATMO(mesPendant);

        if (avant - pendant >= kSeuilAmeliorationSignificative && dist > maxRayon)
            maxRayon = dist;
    }

    return maxRayon;
}

ImpactPurificateur EnvironmentalService::mesurerImpactPurificateur(const std::string& idPurificateur) {
    perf_.demarrer("mesurerImpactPurificateur");

    Purificateur* purif = data_.getPurificateur(idPurificateur);
    if (!purif) {
        perf_.arreter("mesurerImpactPurificateur");
        return ImpactPurificateur::echec();
    }

    double rayonAction = estimerRayonAction(*purif);
    printf("Rayon d'action estimé : %.2f km\n", rayonAction);
    if (rayonAction <= 0.0) {
        perf_.arreter("mesurerImpactPurificateur");
        return ImpactPurificateur::echec();
    }

    double indiceAvant = airQuality_.calculerMoyenneZoneJusquaDateDebut(
        purif->getLatitude(), purif->getLongitude(),
        rayonAction, purif->getDateDebut());

    double indicePendant = airQuality_.calculerMoyenneZonePendant(
        purif->getLatitude(), purif->getLongitude(),
        rayonAction, purif->getDateDebut(), purif->getDateFin());

    double delta = calculerDelta(indiceAvant, indicePendant);

    perf_.arreter("mesurerImpactPurificateur");
    return ImpactPurificateur(delta, rayonAction);
}

} // namespace airwatcher
