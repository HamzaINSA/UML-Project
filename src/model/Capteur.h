#ifndef AIRWATCHER_CAPTEUR_H
#define AIRWATCHER_CAPTEUR_H

#include <string>

namespace airwatcher {

class Capteur {
public:
    Capteur() = default;
    Capteur(std::string id, double latitude, double longitude,
            bool estFiable = true, std::string proprietaireId = "");

    const std::string& getId() const             { return id_; }
    double             getLatitude() const       { return latitude_; }
    double             getLongitude() const      { return longitude_; }
    bool               estFiable() const         { return estFiable_; }
    const std::string& getProprietaireId() const { return proprietaireId_; }
    bool               aProprietairePrive() const { return !proprietaireId_.empty(); }

    void setEstFiable(bool v)                          { estFiable_ = v; }
    void setProprietaireId(const std::string& id)      { proprietaireId_ = id; }

private:
    std::string id_;
    double      latitude_ = 0.0;
    double      longitude_ = 0.0;
    bool        estFiable_ = true;
    // Vide si capteur officiel ; sinon id du Particulier propriétaire.
    std::string proprietaireId_;
};

} // namespace airwatcher

#endif
