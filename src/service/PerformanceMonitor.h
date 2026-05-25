#ifndef AIRWATCHER_PERFORMANCE_MONITOR_H
#define AIRWATCHER_PERFORMANCE_MONITOR_H

#include <chrono>
#include <string>
#include <unordered_map>

using namespace std;
namespace airwatcher {

class PerformanceMonitor {
public:
    void demarrer(const string& nomMethode);
    long arreter(const string& nomMethode);

    // Récupère la dernière durée mesurée (en ms) pour une méthode.
    long getDerniereDuree(const string& nomMethode) const;

private:
    using Clock = chrono::steady_clock;
    unordered_map<string, Clock::time_point> debuts_;
    unordered_map<string, long>              dernieres_;
};

} // namespace airwatcher

#endif
