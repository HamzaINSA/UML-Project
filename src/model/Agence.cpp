#include "Agence.h"

namespace airwatcher {

Agence::Agence(std::string id, std::string mdp, std::string nom, std::string email,
               std::string serviceReference, std::string zoneResponsabilite)
    : Utilisateur(std::move(id), std::move(mdp), std::move(nom), std::move(email))
    , serviceReference_(std::move(serviceReference))
    , zoneResponsabilite_(std::move(zoneResponsabilite)) {}

} // namespace airwatcher
