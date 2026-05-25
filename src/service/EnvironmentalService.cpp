#include "EnvironmentalService.h"

#include "../model/Purificateur.h"
#include "../repository/DataReader.h"
#include "AirQualityService.h"
#include "PerformanceMonitor.h"

using namespace std;

namespace airwatcher {

namespace {
    // Rayon maximal en kilomètres au-delà duquel on considère
    // que le purificateur n'a aucun impact sur les capteurs.
    const double kRayonMax = 100.0;

    // Seuil minimal de différence d'indice ATMO pour qu'une amélioration
    // soit considérée comme significative.
    const double kSeuilAmeliorationSignificative = 0.5;
}  // namespace anonyme

// ============================================================
// Constructeur
// ============================================================

// Initialise le service avec le dépôt de données, le moniteur de performance
// et le service de qualité de l'air.
EnvironmentalService::EnvironmentalService(DataReader& data, PerformanceMonitor& perf,
                                           AirQualityService& airQuality)
    : data_(data), perf_(perf), airQuality_(airQuality) {}

// ============================================================
// Calcul du delta d'indice ATMO
// ============================================================

// Calcule la différence entre l'indice avant et pendant le fonctionnement
// du purificateur. Une valeur positive indique une amélioration de la qualité de l'air.
double EnvironmentalService::calculerDelta(double indiceAvant, double indicePendant) const {
    return indiceAvant - indicePendant;
}

// ============================================================
// Estimation du rayon d'action
// ============================================================

// Estime le rayon d'action d'un purificateur en cherchant le capteur
// le plus éloigné pour lequel une amélioration significative est observable.
// Seuls les capteurs dans un rayon de kRayonMax km sont pris en compte.
double EnvironmentalService::estimerRayonAction(const Purificateur& purificateur) {
    double rayonMaximal = 0.0;

    // pour chaque capteur
    for (const Capteur& capteur : data_.getCapteurs()) {

        // calculer la distance entre le purificateur et le capteur
        double distance = DataReader::distanceKm(purificateur.getLatitude(), purificateur.getLongitude(),capteur.getLatitude(), capteur.getLongitude());

        // si le capteur est au dela de la zone limite, on ne le garde pas
        if (distance > kRayonMax) {
            continue;
        }

        // récupérer les mesures avant le début du fonctionnement du purificateur
        vector<Mesure> mesuresAvant = data_.getMesuresPeriode(capteur.getId(), DateTime::min(), purificateur.getDateDebut());

        // récupérer les mesures pendant le fonctionnement du purificateur
        vector<Mesure> mesuresPendant = data_.getMesuresPeriode(capteur.getId(), purificateur.getDateDebut(), purificateur.getDateFin());

        // Calcul de l'indice ATMO moyen avant et pendant le fonctionnement du purificateur.
        double indiceAvant   = airQuality_.calculerIndiceATMO(mesuresAvant);
        double indicePendant = airQuality_.calculerIndiceATMO(mesuresPendant);

        // Si l'amélioration est significative et que ce capteur est plus éloigné
        // que le maximum courant, on met à jour le rayon d'action.
        if (indiceAvant - indicePendant >= kSeuilAmeliorationSignificative && distance > rayonMaximal){
            rayonMaximal = distance;
        }
    }

    return rayonMaximal;
}

// ============================================================
// Mesure de l'impact d'un purificateur
// ============================================================

// Mesure l'impact global d'un purificateur identifié sur la qualité de l'air.
// Retourne un objet ImpactPurificateur contenant le delta ATMO et le rayon d'action,
// ou un résultat d'échec si le purificateur est introuvable ou sans impact mesurable.
ImpactPurificateur EnvironmentalService::mesurerImpactPurificateur(const string& idPurificateur) {
    perf_.demarrer("mesurerImpactPurificateur");

    // récupérer le purificateur à partir de son ID
    Purificateur* purificateur = data_.getPurificateur(idPurificateur);

    if (purificateur == nullptr) {
        perf_.arreter("mesurerImpactPurificateur");
        return ImpactPurificateur::echec();
    }

    // estimer le rayon d'action du purificateur en analysant les données des capteurs
    double rayonAction = estimerRayonAction(*purificateur);

    printf("Rayon d'action estimé : %.2f km\n", rayonAction);

    // Si aucun capteur n'a enregistré d'amélioration significative,
    // le rayon d'action est nul et on retourne un échec.
    if (rayonAction <= 0.0) {
        perf_.arreter("mesurerImpactPurificateur");
        return ImpactPurificateur::echec();
    }

    // Calcul de l'indice ATMO moyen avant le début du fonctionnement du purificateur
    double indiceAvant = airQuality_.calculerMoyenneZoneJusquaDateDebut(purificateur->getLatitude(), purificateur->getLongitude(),rayonAction, purificateur->getDateDebut());

    // Calcul de l'indice ATMO moyen pendant le fonctionnement du purificateur
    double indicePendant = airQuality_.calculerMoyenneZonePendant(purificateur->getLatitude(), purificateur->getLongitude(),rayonAction, purificateur->getDateDebut(), purificateur->getDateFin());

    // faier la différence des deux
    double delta = calculerDelta(indiceAvant, indicePendant);

    perf_.arreter("mesurerImpactPurificateur");
    return ImpactPurificateur(delta, rayonAction);
}

}  // namespace airwatcher