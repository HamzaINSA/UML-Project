#include "PerformanceMonitor.h"

#include <iostream>

namespace airwatcher {

void PerformanceMonitor::demarrer(const std::string& nomMethode) {
    debuts_[nomMethode] = Clock::now();
}

long PerformanceMonitor::arreter(const std::string& nomMethode) {
    auto it = debuts_.find(nomMethode);
    if (it == debuts_.end()) return 0;
    auto duree = std::chrono::duration_cast<std::chrono::milliseconds>(
                     Clock::now() - it->second).count();
    debuts_.erase(it);
    long ms = static_cast<long>(duree);
    dernieres_[nomMethode] = ms;
    std::cout << "[perf] " << nomMethode << " : " << ms << " ms\n";
    return ms;
}

long PerformanceMonitor::getDerniereDuree(const std::string& nomMethode) const {
    auto it = dernieres_.find(nomMethode);
    return (it == dernieres_.end()) ? 0 : it->second;
}

} // namespace airwatcher
