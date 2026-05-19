#include "AttributMesure.h"

namespace airwatcher {

AttributMesure::AttributMesure(std::string id, std::string unite, std::string description)
    : id_(std::move(id))
    , unite_(std::move(unite))
    , description_(std::move(description)) {}

} // namespace airwatcher
