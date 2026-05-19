#include "Fournisseur.h"

namespace airwatcher {

Fournisseur::Fournisseur(std::string id, std::string mdp, std::string nom, std::string email,
                         std::string nomEntreprise, std::string numeroSiret,
                         std::string contactSupport)
    : Utilisateur(std::move(id), std::move(mdp), std::move(nom), std::move(email))
    , nomEntreprise_(std::move(nomEntreprise))
    , numeroSiret_(std::move(numeroSiret))
    , contactSupport_(std::move(contactSupport)) {}

} // namespace airwatcher
