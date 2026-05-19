#ifndef AIRWATCHER_PARAMETRES_H
#define AIRWATCHER_PARAMETRES_H

#include <string>

#include "DateTime.h"

namespace airwatcher {

struct ParametresZone {
    double   latitude  = 0.0;
    double   longitude = 0.0;
    double   rayon     = 0.0;
    DateTime debut;
    DateTime fin;
};


struct ParametresComparaison {
    std::string idCapteur;
    DateTime    debut;
    DateTime    fin;
};

struct ParametresFiabilite {
    std::string idParticulier;
    std::string idCapteur;
};

} // namespace airwatcher

#endif
