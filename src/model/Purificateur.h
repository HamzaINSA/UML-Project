#ifndef AIRWATCHER_PURIFICATEUR_H
#define AIRWATCHER_PURIFICATEUR_H

#include <string>

#include "DateTime.h"

namespace airwatcher {

class Purificateur {
public:
    Purificateur() = default;
    Purificateur(std::string id, double latitude, double longitude,
                 DateTime dateDebut, DateTime dateFin,
                 std::string fournisseurId = "");

    const std::string& getId() const            { return id_; }
    double             getLatitude() const      { return latitude_; }
    double             getLongitude() const     { return longitude_; }
    const DateTime&    getDateDebut() const     { return dateDebut_; }
    const DateTime&    getDateFin() const       { return dateFin_; }
    const std::string& getFournisseurId() const { return fournisseurId_; }

private:
    std::string id_;
    double      latitude_ = 0.0;
    double      longitude_ = 0.0;
    DateTime    dateDebut_;
    DateTime    dateFin_;
    std::string fournisseurId_;
};

} // namespace airwatcher

#endif
