#ifndef AIRWATCHER_AIR_QUALITY_SERVICE_H
#define AIRWATCHER_AIR_QUALITY_SERVICE_H

#include <string>
#include <utility>
#include <vector>

#include "../model/Capteur.h"
#include "../model/DateTime.h"
#include "../model/Mesure.h"

using namespace std;

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
    double calculerMoyenneAQI(double lat, double lon, double rayon,const DateTime& debut, const DateTime& fin);

    // Variantes utilisées par EnvironmentalService.
    double calculerMoyenneZonePendant(double lat, double lon, double rayon,const DateTime& debut, const DateTime& fin);
    double calculerMoyenneZoneJusquaDateDebut(double lat, double lon, double rayon,const DateTime& dateLimite);

    // Pseudocode scenario 4.
    double estimerQualiteZone(double lat, double lon, double rayon,
                              const DateTime& debut, const DateTime& fin);

    // Pseudocode scenario 3.
    vector<CapteurScore> comparerCapteurs(const string& idRef,
                                               const DateTime& debut, const DateTime& fin);

    // Calcul ATMO.
    double calculerIndiceATMO(const vector<Mesure>& mesures) const;

    double calculerScoreSimilarite(const vector<Mesure>& mesuresRef,const vector<Mesure>& mesures) const;

    vector<pair<Capteur*, vector<Mesure>>> trouverCapteursProchesPourInterpolation(double lat, double lon) const;
    double interpolerPondereeParDistance(const vector<pair<Capteur*, vector<Mesure>>>& proches,double lat, double lon) const;

private:
    DataReader&            data_;
    PerformanceMonitor&    perf_;
    AdministrationService* admin_ = nullptr;

    void   trierParScoreCroissant(vector<CapteurScore>& classement) const;
    static double moyenneAttribut(const vector<Mesure>& mesures,const string& attributId);
    static int sousIndiceATMO(const string& attribut, double valeur);
};

} // namespace airwatcher

#endif
