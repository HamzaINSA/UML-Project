#include "Particulier.h"

namespace airwatcher {

Particulier::Particulier(std::string id, std::string mdp, std::string nom, std::string email,
                         std::string adresse, int points, bool estFiable,
                         bool attributionPointsActive)
    : Utilisateur(std::move(id), std::move(mdp), std::move(nom), std::move(email))
    , adresse_(std::move(adresse))
    , points_(points)
    , estFiable_(estFiable)
    , attributionPointsActive_(attributionPointsActive) {}

} // namespace airwatcher
