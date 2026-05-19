#include "Capteur.h"

namespace airwatcher {

Capteur::Capteur(std::string id, double latitude, double longitude,
                 bool estFiable, std::string proprietaireId)
    : id_(std::move(id))
    , latitude_(latitude)
    , longitude_(longitude)
    , estFiable_(estFiable)
    , proprietaireId_(std::move(proprietaireId)) {}

} // namespace airwatcher
