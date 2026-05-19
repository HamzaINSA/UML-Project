#include "Rapport.h"

namespace airwatcher {

Rapport::Rapport(EtatCapteur etat, std::vector<std::string> anomalies)
    : etat_(etat), anomalies_(std::move(anomalies)) {}

std::string Rapport::toString(EtatCapteur etat) {
    switch (etat) {
        case EtatCapteur::FONCTIONNEL: return "FONCTIONNEL";
        case EtatCapteur::DEFAILLANT:  return "DEFAILLANT";
        case EtatCapteur::INTROUVABLE: return "INTROUVABLE";
    }
    return "INCONNU";
}

} // namespace airwatcher
