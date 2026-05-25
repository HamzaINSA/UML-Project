#ifndef AIRWATCHER_ENVIRONMENTAL_SERVICE_H
#define AIRWATCHER_ENVIRONMENTAL_SERVICE_H

#include <string>

#include "../model/ImpactPurificateur.h"

using namespace std;

namespace airwatcher {

class DataReader;
class PerformanceMonitor;
class AirQualityService;
class Purificateur;

class EnvironmentalService {
public:
    EnvironmentalService(DataReader& data, PerformanceMonitor& perf,
                         AirQualityService& airQuality);

    // Pseudocode scenario 5.
    ImpactPurificateur mesurerImpactPurificateur(const string& idPurificateur);

private:
    DataReader&         data_;
    PerformanceMonitor& perf_;
    AirQualityService&  airQuality_;

    double calculerDelta(double indiceAvant, double indicePendant) const;
    double estimerRayonAction(const Purificateur& purif);
};

} // namespace airwatcher

#endif
