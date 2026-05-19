#ifndef AIRWATCHER_UTILISATEUR_H
#define AIRWATCHER_UTILISATEUR_H

#include <string>

namespace airwatcher {

enum class RoleUtilisateur {
    AGENCE,
    PARTICULIER,
    FOURNISSEUR
};

class Utilisateur {
public:
    Utilisateur() = default;
    Utilisateur(std::string id, std::string mdp, std::string nom, std::string email);
    virtual ~Utilisateur() = default;

    virtual RoleUtilisateur getRole() const = 0;

    const std::string& getId()    const { return id_; }
    const std::string& getMdp()   const { return mdp_; }
    const std::string& getNom()   const { return nom_; }
    const std::string& getEmail() const { return email_; }

    bool verifierMdp(const std::string& mdp) const { return mdp_ == mdp; }

protected:
    std::string id_;
    std::string mdp_;
    std::string nom_;
    std::string email_;
};

} // namespace airwatcher

#endif
