#ifndef AIRWATCHER_RAPPORT_H
#define AIRWATCHER_RAPPORT_H

#include <string>
#include <vector>

namespace airwatcher {

enum class EtatCapteur {
    FONCTIONNEL,
    DEFAILLANT,
    INTROUVABLE
};

class Rapport {
public:
    Rapport() = default;
    Rapport(EtatCapteur etat, std::vector<std::string> anomalies = {});

    EtatCapteur                      getEtat() const      { return etat_; }
    const std::vector<std::string>&  getAnomalies() const { return anomalies_; }

    static std::string toString(EtatCapteur etat);

private:
    EtatCapteur              etat_ = EtatCapteur::INTROUVABLE;
    std::vector<std::string> anomalies_;
};

} // namespace airwatcher

#endif
