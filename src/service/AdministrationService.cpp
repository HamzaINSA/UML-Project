#include "AdministrationService.h"

#include <array>
#include <cmath>

#include "../model/Mesure.h"
#include "../model/Particulier.h"
#include "../repository/DataReader.h"
#include "PerformanceMonitor.h"

namespace airwatcher {

namespace {
const std::array<std::string, 4> kAttributs = {"O3", "SO2", "NO2", "PM10"};

constexpr double kRayonVoisinage = 10.0;     // km
constexpr double kK              = 2.0;      // facteur d'écart-type
constexpr int    kSeuilTolerance = 2;
constexpr double kVarianceMin    = 1e-3;

struct SeuilEcart { double s; };
double seuilEcart(const std::string& a) {
//on definie ici des seuils maximum pour chaque attribut 
    if (a == "O3")   return 40.0;
    if (a == "SO2")  return 30.0;
    if (a == "NO2")  return 30.0;
    if (a == "PM10") return 20.0;
    return 50.0;
}
}  // namespace

AdministrationService::AdministrationService(DataReader& data, PerformanceMonitor& perf)
    : data_(data), perf_(perf) {}

double AdministrationService::moyenne(const std::vector<Mesure>& mesures, const std::string& attribut) {
    double s = 0.0;
    int n = 0;
    for (const auto& m : mesures) {
        if (m.getAttributId() == attribut && m.estValide()) { s += m.getValeur(); ++n; }
    }
    return n == 0 ? 0.0 : s / n;
}

double AdministrationService::ecartType(const std::vector<Mesure>& mesures, const std::string& attribut) {
    double m = moyenne(mesures, attribut);
    //s = somme des carrés des écarts à la moyenne pour les mesures valides de cet attribut
    double s = 0.0;
    //n = nombre de mesures valides pour cet attribut
    int n = 0;
    for (const auto& x : mesures) {
        if (x.getAttributId() == attribut && x.estValide()) {
            double d = x.getValeur() - m;
            s += d * d;
            ++n;
        }
    }
    //on retourne la racine de la moyenne de ces carrés d'écarts (écart-type)
    return n == 0 ? 0.0 : std::sqrt(s / n);
}

bool AdministrationService::varianceConstante(const std::vector<Mesure>& mesures, const std::string& attribut) {
    // On considère que la variance est anormalement faible si on a au moins 3 mesures valides et que l'écart-type est inférieur à un seuil.
    int n = 0;
    for (const auto& m : mesures) if (m.getAttributId() == attribut && m.estValide()) ++n;
    if (n < 3) return false;
    //si l'ecart type de toutes les mesures pour cet attribut est inferieur au seuil de variance minimale, alors on considère que la variance est anormalement faible et on retourne true
    return ecartType(mesures, attribut) < kVarianceMin;
}

bool AdministrationService::horsBornes(const std::string& attribut, double v) {
    //on verifie que notre valeur n'est pas superieure a une borne fixe par defaut (ces bornes peuvent ne pas etre realistes)
    if (v < 0) return true;
    if (attribut == "O3")   return v > 600.0;
    if (attribut == "SO2")  return v > 1000.0;
    if (attribut == "NO2")  return v > 800.0;
    if (attribut == "PM10") return v > 500.0;
    return false;
}

Rapport AdministrationService::analyserCapteur(const std::string& idCapteur) {
    perf_.demarrer("analyserCapteur");

    Capteur* capteur = data_.getCapteur(idCapteur);
    if (!capteur) {
        perf_.arreter("analyserCapteur");
        return Rapport(EtatCapteur::INTROUVABLE);
    }

    auto mesures     = data_.getMesuresCapteur(idCapteur);
    auto mesuresVois = data_.getMesuresCapteursVoisins(idCapteur);
    bool aVoisins    = !mesuresVois.empty();

    std::vector<std::string> anomalies;
    // Vérifie les anomalies sur les mesures du capteur, en comparant mesures et mesuresVoisins.
    for (const auto& attribut : kAttributs) {
        double moyenneCapteur = moyenne(mesures, attribut);
        // verifie que chaque attribut n'est pas hors bornes 

        if (horsBornes(attribut, moyenneCapteur)) {
            //si jamais un attribut est hors bornes, on ajoute une anomalie pour cet attribut
            anomalies.push_back("Valeur hors bornes : " + attribut);
        }
        //si jamais le capteur a des voisins
        if (aVoisins) {
            //on calcule l'écart entre la moyenne du capteur et la moyenne de ses voisins pour cet attribut
            double ecart = std::fabs(moyenneCapteur - moyenne(mesuresVois, attribut));
            //si l'ecart est plus grand que le seuil d'ecart pour cet attribut, on ajoute une anomalie pour cet attribut
            if (ecart > seuilEcart(attribut)) {
                //si jamais l'écart est supérieur au seuil d'écart pour cet attribut, on ajoute une anomalie pour cet attribut
                anomalies.push_back("Ecart anormal vs voisins : " + attribut);
            }
        }
        //on verifie que la variance des mesures du capteur pour cet attribut n'est pas anormalement faible (constante)
        if (varianceConstante(mesures, attribut)) {
            anomalies.push_back("Valeurs constantes suspectes : " + attribut);
        }
    }

    Rapport rapport;
    if (anomalies.empty()) {
        rapport = Rapport(EtatCapteur::FONCTIONNEL);
    } else {
        rapport = Rapport(EtatCapteur::DEFAILLANT, std::move(anomalies));
    }

    perf_.arreter("analyserCapteur");
    return rapport;
}

bool AdministrationService::classifierFiabilite(const std::string& idCapteur) {
    perf_.demarrer("classifierFiabilite");

    Capteur* capteur = data_.getCapteur(idCapteur);
    if (!capteur) {
        perf_.arreter("classifierFiabilite");
        return false;
    }
    Particulier* particulier = nullptr;
    if (capteur->aProprietairePrive()) {
        particulier = data_.getParticulier(capteur->getProprietaireId());
    }

    auto mesuresPrive       = data_.getMesuresCapteur(idCapteur);
    auto mesuresVoisins     = data_.getMesuresCapteursVoisins(idCapteur);
    
    //(void)data_.getCapteursOfficielsVoisins(idCapteur, kRayonVoisinage);

    int nbAnomalies = 0;

    // Vérifie les anomalies sur les mesures du capteur, en comparant mesuresPrive et mesuresVoisins.
    for (const auto& attribut : kAttributs) {
        double moyennePrive     = moyenne(mesuresPrive, attribut);
        double moyenneVoisinage = moyenne(mesuresVoisins, attribut);
        double et               = ecartType(mesuresVoisins, attribut);

        if (et > 0.0 && std::fabs(moyennePrive - moyenneVoisinage) > kK * et) {
            ++nbAnomalies;
        }
        if (varianceConstante(mesuresPrive, attribut)) {
            ++nbAnomalies;
        }
        if (horsBornes(attribut, moyennePrive)) {
            ++nbAnomalies;
        }
    }

    bool estFiable;
    // si le nombre d'anomalies est inférieur ou égal au seuil de tolérance, on considère que le capteur est fiable, sinon il est non fiable
    if (nbAnomalies <= kSeuilTolerance) {
        capteur->setEstFiable(true);
        if (particulier) particulier->setEstFiable(true);
        data_.marquerCapteur(idCapteur, true);
        data_.sauvegarderCapteur(*capteur);
        estFiable = true;
    } else {
        capteur->setEstFiable(false);
        // si le capteur est non-fiable alors son propriétaire est considéré comme non-fiable
        if (particulier) particulier->setEstFiable(false);
        data_.marquerCapteur(idCapteur, false);
        data_.marquerMesuresInvalides(idCapteur);
        data_.sauvegarderCapteur(*capteur);
        if (particulier) desactiverAttributionPoints(particulier->getId());
        estFiable = false;
    }

    perf_.arreter("classifierFiabilite");
    return estFiable;
}

void AdministrationService::attribuerPoints(const std::vector<std::string>& idsParticuliers) {
    for (const auto& id : idsParticuliers) {
        Particulier* p = data_.getParticulier(id);
        if (!p) continue;
        if (!p->estFiable()) continue;
        if (!p->attributionPointsActive()) continue;
        p->ajouterPoints(1);
    }
}

void AdministrationService::desactiverAttributionPoints(const std::string& idParticulier) {
    Particulier* p = data_.getParticulier(idParticulier);
    if (p) p->setAttributionPointsActive(false);
}

} // namespace airwatcher
