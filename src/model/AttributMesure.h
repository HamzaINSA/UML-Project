#ifndef AIRWATCHER_ATTRIBUT_MESURE_H
#define AIRWATCHER_ATTRIBUT_MESURE_H

#include <string>

namespace airwatcher {

class AttributMesure {
public:
    AttributMesure() = default;
    AttributMesure(std::string id, std::string unite, std::string description);

    const std::string& getId() const          { return id_; }
    const std::string& getUnite() const       { return unite_; }
    const std::string& getDescription() const { return description_; }

private:
    std::string id_;
    std::string unite_;
    std::string description_;
};

} // namespace airwatcher

#endif
