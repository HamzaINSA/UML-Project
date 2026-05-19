#include "Purificateur.h"

namespace airwatcher {

Purificateur::Purificateur(std::string id, double latitude, double longitude,
                           DateTime dateDebut, DateTime dateFin,
                           std::string fournisseurId)
    : id_(std::move(id))
    , latitude_(latitude)
    , longitude_(longitude)
    , dateDebut_(std::move(dateDebut))
    , dateFin_(std::move(dateFin))
    , fournisseurId_(std::move(fournisseurId)) {}

} // namespace airwatcher
