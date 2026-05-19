#ifndef AIRWATCHER_AIR_QUALITY_SERVICE_H
#define AIRWATCHER_AIR_QUALITY_SERVICE_H

#include <string>
#include <utility>
#include <vector>

#include "../model/Capteur.h"
#include "../model/DateTime.h"
#include "../model/Mesure.h"

namespace airwatcher {

class DataReader;
class PerformanceMonitor;
class AdministrationService;

// Résultat de comparerCapteurs : capteur + score de similarité.
struct CapteurScore {
    Capteur* capteur;
    double   score;
};

class AirQualityService {
public:
    AirQualityService(DataReader& data, PerformanceMonitor& perf);

    void setAdministrationService(AdministrationService* admin) { admin_ = admin; }

    // Pseudocode scenario 2.
    double calculerMoyenneAQI(double lat, double lon, double rayon,
                              const DateTime& debut, const DateTime& fin);

    // Variantes utilisées par EnvironmentalService.
    double calculerMoyenneZonePendant(double lat, double lon, double rayon,
                                      const DateTime& debut, const DateTime& fin);
    double calculerMoyenneZoneJusquaDateDebut(double lat, double lon, double rayon,
                                              const DateTime& dateLimite);

    // Pseudocode scenario 4.
    double estimerQualiteZone(double lat, double lon, double rayon,
                              const DateTime& debut, const DateTime& fin);

    // Pseudocode scenario 3.
    std::vector<CapteurScore> comparerCapteurs(const std::string& idRef,
                                               const DateTime& debut, const DateTime& fin);

    // Calcul ATMO.
    double calculerIndiceATMO(const std::vector<Mesure>& mesures) const;

    double calculerScoreSimilarite(const std::vector<Mesure>& mesuresRef,
                                   const std::vector<Mesure>& mesures) const;

private:
    DataReader&            data_;
    PerformanceMonitor&    perf_;
    AdministrationService* admin_ = nullptr;

    void   trierParScoreCroissant(std::vector<CapteurScore>& classement) const;
    double interpolerPondereeParDistance(
        const std::vector<std::pair<Capteur*, std::vector<Mesure>>>& proches,
        double lat, double lon) const;

    static double moyenneAttribut(const std::vector<Mesure>& mesures,
                                  const std::string& attributId);
    static int    sousIndiceATMO(const std::string& attribut, double valeur);
};

} // namespace airwatcher

#endif
