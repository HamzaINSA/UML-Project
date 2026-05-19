#ifndef AIRWATCHER_AGENCE_H
#define AIRWATCHER_AGENCE_H

#include <string>

#include "Utilisateur.h"

namespace airwatcher {

class Agence : public Utilisateur {
public:
    Agence() = default;
    Agence(std::string id, std::string mdp, std::string nom, std::string email,
           std::string serviceReference = "", std::string zoneResponsabilite = "");

    RoleUtilisateur getRole() const override { return RoleUtilisateur::AGENCE; }

    const std::string& getServiceReference()   const { return serviceReference_; }
    const std::string& getZoneResponsabilite() const { return zoneResponsabilite_; }

private:
    std::string serviceReference_;
    std::string zoneResponsabilite_;
};

} // namespace airwatcher

#endif
