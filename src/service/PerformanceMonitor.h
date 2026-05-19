#ifndef AIRWATCHER_PERFORMANCE_MONITOR_H
#define AIRWATCHER_PERFORMANCE_MONITOR_H

#include <chrono>
#include <string>
#include <unordered_map>

namespace airwatcher {

class PerformanceMonitor {
public:
    void demarrer(const std::string& nomMethode);
    long arreter(const std::string& nomMethode);

    // Récupère la dernière durée mesurée (en ms) pour une méthode.
    long getDerniereDuree(const std::string& nomMethode) const;

private:
    using Clock = std::chrono::steady_clock;
    std::unordered_map<std::string, Clock::time_point> debuts_;
    std::unordered_map<std::string, long>              dernieres_;
};

} // namespace airwatcher

#endif
