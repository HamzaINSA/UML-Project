#include "Utilisateur.h"

namespace airwatcher {

Utilisateur::Utilisateur(std::string id, std::string mdp, std::string nom, std::string email)
    : id_(std::move(id))
    , mdp_(std::move(mdp))
    , nom_(std::move(nom))
    , email_(std::move(email)) {}

} // namespace airwatcher
