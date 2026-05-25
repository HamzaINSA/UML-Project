#include "PerformanceMonitor.h"

#include <iostream>

using namespace std;

namespace airwatcher {

// demarre le chronomètre pour une méthode donnée
void PerformanceMonitor::demarrer(const string& nomMethode) {
    debuts_[nomMethode] = Clock::now();
}

// arreter le chronomètre pour une méthode donnée, calculer la durée et l'afficher
long PerformanceMonitor::arreter(const string& nomMethode) {
    // récupérer le temps de début pour cette méthode
    auto it = debuts_.find(nomMethode);
    // si la méthode n'a pas été démarrée (pas dans le tableau debuts_), on retourne 0
    if (it == debuts_.end()) return 0;

    // on calcule la durée en millisecondes depuis le début
    auto duree = chrono::duration_cast<chrono::milliseconds>(Clock::now() - it->second).count();
    // supprimer le temps de début de la méthode du tableau debuts_ 
    debuts_.erase(it);
    // stocker la durée dans le tableau dernieres_ pour pouvoir la récupérer plus tard
    long ms = static_cast<long>(duree);
    dernieres_[nomMethode] = ms;
    // afficher la durée pour cette méthode
    cout << "[perf] " << nomMethode << " : " << ms << " ms\n";
    return ms;
}

// retourne la dernière durée mesurée pour une méthode donnée, ou 0 si aucune mesure n'est disponible
long PerformanceMonitor::getDerniereDuree(const string& nomMethode) const {
    auto it = dernieres_.find(nomMethode);
    return (it == dernieres_.end()) ? 0 : it->second;
}

} // namespace airwatcher
