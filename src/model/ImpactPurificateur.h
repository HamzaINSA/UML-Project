#ifndef AIRWATCHER_IMPACT_PURIFICATEUR_H
#define AIRWATCHER_IMPACT_PURIFICATEUR_H

namespace airwatcher {

class ImpactPurificateur {
public:
    ImpactPurificateur() = default;
    ImpactPurificateur(double delta, double rayonAction, bool succes = true);

    double getDelta() const       { return delta_; }
    double getRayonAction() const { return rayonAction_; }
    bool   estSucces() const      { return succes_; }

    static ImpactPurificateur echec();

private:
    double delta_ = 0.0;
    double rayonAction_ = 0.0;
    bool   succes_ = true;
};

} // namespace airwatcher

#endif
