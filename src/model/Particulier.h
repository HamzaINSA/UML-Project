#ifndef AIRWATCHER_PARTICULIER_H
#define AIRWATCHER_PARTICULIER_H

#include <string>

#include "Utilisateur.h"

namespace airwatcher {

class Particulier : public Utilisateur {
public:
    Particulier() = default;
    Particulier(std::string id, std::string mdp, std::string nom, std::string email,
                std::string adresse = "", int points = 0, bool estFiable = true,
                bool attributionPointsActive = true);

    RoleUtilisateur getRole() const override { return RoleUtilisateur::PARTICULIER; }

    int                getPoints() const                  { return points_; }
    bool               estFiable() const                  { return estFiable_; }
    bool               attributionPointsActive() const    { return attributionPointsActive_; }
    const std::string& getAdresse() const                 { return adresse_; }

    void ajouterPoints(int p)             { points_ += p; }
    void setEstFiable(bool v)             { estFiable_ = v; }
    void setAttributionPointsActive(bool v){ attributionPointsActive_ = v; }

private:
    std::string adresse_;
    int         points_ = 0;
    bool        estFiable_ = true;
    bool        attributionPointsActive_ = true;
};

} // namespace airwatcher

#endif
