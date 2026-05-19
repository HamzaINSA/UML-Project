#ifndef AIRWATCHER_ADMINISTRATION_SERVICE_H
#define AIRWATCHER_ADMINISTRATION_SERVICE_H

#include <string>
#include <vector>

#include "../model/Mesure.h"
#include "../model/Rapport.h"

namespace airwatcher {

class DataReader;
class PerformanceMonitor;

class AdministrationService {
public:
    AdministrationService(DataReader& data, PerformanceMonitor& perf);

    // Pseudocode scenario 1.
    Rapport analyserCapteur(const std::string& idCapteur);

    // Pseudocode scenario 6.
    bool classifierFiabilite(const std::string& idCapteur);

    // Comptage de points : +1 par requête utilisant des données du capteur.
    void attribuerPoints(const std::vector<std::string>& idsParticuliers);
    void desactiverAttributionPoints(const std::string& idParticulier);

private:
    DataReader&         data_;
    PerformanceMonitor& perf_;

    static double moyenne(const std::vector<Mesure>& mesures, const std::string& attribut);
    static double ecartType(const std::vector<Mesure>& mesures, const std::string& attribut);
    static bool   varianceConstante(const std::vector<Mesure>& mesures, const std::string& attribut);
    static bool   horsBornes(const std::string& attribut, double valeur);
};

} // namespace airwatcher

#endif
