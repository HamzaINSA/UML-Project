#include "EnvironmentalService.h"

#include "../model/Purificateur.h"
#include "../repository/DataReader.h"
#include "AirQualityService.h"
#include "PerformanceMonitor.h"

namespace airwatcher {

namespace {
constexpr double kRayonMin                       = 1.0;   // km
constexpr double kRayonMax                       = 20.0;  // km
constexpr double kPas                            = 1.0;   // km
constexpr double kSeuilAmeliorationSignificative = 0.5;   // points ATMO
}  // namespace

EnvironmentalService::EnvironmentalService(DataReader& data, PerformanceMonitor& perf,
                                           AirQualityService& airQuality)
    : data_(data), perf_(perf), airQuality_(airQuality) {}

double EnvironmentalService::calculerDelta(double indiceAvant, double indicePendant) const {
    return indiceAvant - indicePendant;  // positif = amélioration
}

double EnvironmentalService::estimerRayonAction(const Purificateur& purif) {
    double rayonRetenu = 0.0;
    for (double r = kRayonMin; r <= kRayonMax; r += kPas) {
        double avant = airQuality_.calculerMoyenneZoneJusquaDateDebut(
            purif.getLatitude(), purif.getLongitude(), r, purif.getDateDebut());
        double pendant = airQuality_.calculerMoyenneZonePendant(
            purif.getLatitude(), purif.getLongitude(), r,
            purif.getDateDebut(), purif.getDateFin());
        if (avant - pendant >= kSeuilAmeliorationSignificative) {
            rayonRetenu = r;
        } else {
            break;
        }
    }
    return rayonRetenu;
}

ImpactPurificateur EnvironmentalService::mesurerImpactPurificateur(const std::string& idPurificateur) {
    perf_.demarrer("mesurerImpactPurificateur");

    Purificateur* purif = data_.getPurificateur(idPurificateur);
    if (!purif) {
        perf_.arreter("mesurerImpactPurificateur");
        return ImpactPurificateur::echec();
    }

    double rayonAction = estimerRayonAction(*purif);
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
