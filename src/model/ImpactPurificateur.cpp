#include "ImpactPurificateur.h"

namespace airwatcher {

ImpactPurificateur::ImpactPurificateur(double delta, double rayonAction, bool succes)
    : delta_(delta), rayonAction_(rayonAction), succes_(succes) {}

ImpactPurificateur ImpactPurificateur::echec() {
    return ImpactPurificateur(0.0, 0.0, false);
}

} // namespace airwatcher
