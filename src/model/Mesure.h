#ifndef AIRWATCHER_MESURE_H
#define AIRWATCHER_MESURE_H

#include <string>

#include "DateTime.h"

namespace airwatcher {

class Mesure {
public:
    Mesure() = default;
    Mesure(DateTime timestamp, std::string capteurId, std::string attributId,
           double valeur, bool estValide = true);

    const DateTime&    getTimestamp()  const { return timestamp_; }
    const std::string& getCapteurId()  const { return capteurId_; }
    const std::string& getAttributId() const { return attributId_; }
    double             getValeur()     const { return valeur_; }
    bool               estValide()     const { return estValide_; }

    void setEstValide(bool v) { estValide_ = v; }

private:
    DateTime    timestamp_;
    std::string capteurId_;
    std::string attributId_;
    double      valeur_ = 0.0;
    bool        estValide_ = true;
};

} // namespace airwatcher

#endif
