#ifndef AIRWATCHER_ADMINISTRATION_SERVICE_H
#define AIRWATCHER_ADMINISTRATION_SERVICE_H

#include <string>
#include <vector>

#include "../model/Mesure.h"
#include "../model/Rapport.h"

using namespace std;

namespace airwatcher {

class DataReader;
class PerformanceMonitor;

class AdministrationService {
public:
    AdministrationService(DataReader& data, PerformanceMonitor& perf);

    // Pseudocode scenario 1.
    Rapport analyserCapteur(const string& idCapteur);

    // Pseudocode scenario 6.
    bool classifierFiabilite(const string& idCapteur);

    // Comptage de points : +1 par requête utilisant des données du capteur.
    void attribuerPoints(const vector<string>& idsParticuliers);
    void desactiverAttributionPoints(const string& idParticulier);

private:
    DataReader&         data_;
    PerformanceMonitor& perf_;

    static double moyenne(const vector<Mesure>& mesures, const string& attribut);
    static double ecartType(const vector<Mesure>& mesures, const string& attribut);
    static bool   varianceConstante(const vector<Mesure>& mesures, const string& attribut);
    static bool   horsBornes(const string& attribut, double valeur);
};

} // namespace airwatcher

#endif
