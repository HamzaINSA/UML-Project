#ifndef AIRWATCHER_FOURNISSEUR_H
#define AIRWATCHER_FOURNISSEUR_H

#include <string>

#include "Utilisateur.h"

namespace airwatcher {

class Fournisseur : public Utilisateur {
public:
    Fournisseur() = default;
    Fournisseur(std::string id, std::string mdp, std::string nom, std::string email,
                std::string nomEntreprise = "", std::string numeroSiret = "",
                std::string contactSupport = "");

    RoleUtilisateur getRole() const override { return RoleUtilisateur::FOURNISSEUR; }

    const std::string& getNomEntreprise()  const { return nomEntreprise_; }
    const std::string& getNumeroSiret()    const { return numeroSiret_; }
    const std::string& getContactSupport() const { return contactSupport_; }

private:
    std::string nomEntreprise_;
    std::string numeroSiret_;
    std::string contactSupport_;
};

} // namespace airwatcher

#endif
