#include "Mesure.h"

namespace airwatcher {

Mesure::Mesure(DateTime timestamp, std::string capteurId, std::string attributId,
               double valeur, bool estValide)
    : timestamp_(std::move(timestamp))
    , capteurId_(std::move(capteurId))
    , attributId_(std::move(attributId))
    , valeur_(valeur)
    , estValide_(estValide) {}

} // namespace airwatcher
