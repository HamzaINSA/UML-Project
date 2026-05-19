#include "DataReader.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace airwatcher {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthRadiusKm = 6371.0;

double toRad(double deg) { return deg * kPi / 180.0; }
}  // namespace

DataReader::DataReader(std::string dataDir) : dataDir_(std::move(dataDir)) {}

double DataReader::distanceKm(double lat1, double lon1, double lat2, double lon2) {
    double dLat = toRad(lat2 - lat1);
    double dLon = toRad(lon2 - lon1);
    double a = std::sin(dLat / 2) * std::sin(dLat / 2)
             + std::cos(toRad(lat1)) * std::cos(toRad(lat2))
               * std::sin(dLon / 2) * std::sin(dLon / 2);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return kEarthRadiusKm * c;
}

std::vector<std::string> DataReader::splitLine(const std::string& line, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : line) {
        if (c == sep) {
            out.push_back(cur);
            cur.clear();
        } else if (c != '\r') {
            cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

void DataReader::loadDataFromCSV() {
    loadAttributes();
    loadSensors();
    loadUsers();        // associe propriétaires aux capteurs
    loadMeasurements();
    loadProviders();
    loadCleaners();
}

void DataReader::loadAttributes() {
    std::ifstream f(dataDir_ + "/attributes.csv");
    if (!f) {
        throw std::runtime_error("Impossible d'ouvrir : " + dataDir_ + "/attributes.csv");
    }
    std::string line;
    bool header = true;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        if (header) { header = false; if (line.find("AttributeID") != std::string::npos) continue; }
        auto cols = splitLine(line);
        if (cols.size() < 3) continue;
        attributs_.emplace_back(cols[0], cols[1], cols[2]);
    }
}

void DataReader::loadSensors() {
    std::ifstream f(dataDir_ + "/sensors.csv");
    if (!f) {
        throw std::runtime_error("Impossible d'ouvrir : " + dataDir_ + "/sensors.csv");
    }
    std::string line;
    bool header = true;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        if (header) { header = false; if (line.find("SensorID") != std::string::npos) continue; }
        auto cols = splitLine(line);
        if (cols.size() < 3) continue;
        try {
            double lat = std::stod(cols[1]);
            double lon = std::stod(cols[2]);
            capteurs_.emplace_back(cols[0], lat, lon, true, "");
        } catch (...) {
            std::cerr << "Ligne sensors.csv ignorée : " << line << '\n';
        }
    }
}

void DataReader::loadMeasurements() {
    std::ifstream f(dataDir_ + "/measurements.csv");
    if (!f) {
        throw std::runtime_error("Impossible d'ouvrir : " + dataDir_ + "/measurements.csv");
    }
    std::string line;
    bool header = true;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        if (header) { header = false; if (line.find("Timestamp") != std::string::npos) continue; }
        auto cols = splitLine(line);
        if (cols.size() < 4) continue;
        try {
            DateTime ts = DateTime::parse(cols[0]);
            double v = std::stod(cols[3]);
            mesures_.emplace_back(ts, cols[1], cols[2], v, true);
        } catch (...) {
            std::cerr << "Ligne measurements.csv ignorée : " << line << '\n';
        }
    }
}

void DataReader::loadCleaners() {
    std::ifstream f(dataDir_ + "/cleaners.csv");
    if (!f) {
        throw std::runtime_error("Impossible d'ouvrir : " + dataDir_ + "/cleaners.csv");
    }
    // Index fournisseur par cleaner.
    std::unordered_map<std::string, std::string> cleanerToFournisseur;
    {
        std::ifstream pf(dataDir_ + "/providers.csv");
        if (pf) {
            std::string l;
            bool h = true;
            while (std::getline(pf, l)) {
                if (l.empty()) continue;
                if (h) { h = false; if (l.find("ProviderID") != std::string::npos) continue; }
                auto cols = splitLine(l);
                if (cols.size() < 2) continue;
                cleanerToFournisseur[cols[1]] = cols[0];
            }
        }
    }
    std::string line;
    bool header = true;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        if (header) { header = false; if (line.find("CleanerID") != std::string::npos) continue; }
        auto cols = splitLine(line);
        if (cols.size() < 5) continue;
        try {
            double lat = std::stod(cols[1]);
            double lon = std::stod(cols[2]);
            DateTime debut = DateTime::parse(cols[3]);
            DateTime fin   = DateTime::parse(cols[4]);
            std::string fournisseurId;
            auto it = cleanerToFournisseur.find(cols[0]);
            if (it != cleanerToFournisseur.end()) fournisseurId = it->second;
            purificateurs_.emplace_back(cols[0], lat, lon, debut, fin, fournisseurId);
        } catch (...) {
            std::cerr << "Ligne cleaners.csv ignorée : " << line << '\n';
        }
    }
}

void DataReader::loadProviders() {
    // Création d'un Fournisseur générique pour chaque ProviderID rencontré.
    std::ifstream f(dataDir_ + "/providers.csv");
    if (!f) return;
    std::string line;
    bool header = true;
    std::unordered_set<std::string> seen;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        if (header) { header = false; if (line.find("ProviderID") != std::string::npos) continue; }
        auto cols = splitLine(line);
        if (cols.empty()) continue;
        const std::string& id = cols[0];
        if (seen.count(id)) continue;
        seen.insert(id);
        utilisateurs_.push_back(std::make_shared<Fournisseur>(
            id, "fournisseur", "Fournisseur " + id, id + "@fournisseur.fr",
            "Entreprise " + id, "00000000000000", id + "@support.fr"));
    }
}

void DataReader::loadUsers() {
    std::ifstream f(dataDir_ + "/users.csv");
    if (!f) {
        throw std::runtime_error("Impossible d'ouvrir : " + dataDir_ + "/users.csv");
    }
    std::string line;
    bool header = true;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        if (header) { header = false; if (line.find("UserID") != std::string::npos) continue; }
        auto cols = splitLine(line);
        if (cols.size() < 2) continue;
        const std::string& uid = cols[0];
        const std::string& sid = cols[1];

        // Crée le Particulier s'il n'existe pas encore.
        bool existe = false;
        for (const auto& u : utilisateurs_) {
            if (u->getId() == uid) { existe = true; break; }
        }
        if (!existe) {
            utilisateurs_.push_back(std::make_shared<Particulier>(
                uid, "particulier", "Particulier " + uid, uid + "@user.fr",
                "Adresse inconnue", 0, true, true));
        }

        // Lie le capteur à ce propriétaire.
        for (auto& c : capteurs_) {
            if (c.getId() == sid) {
                c.setProprietaireId(uid);
                break;
            }
        }
    }

    // Une agence par défaut pour la connexion.
    utilisateurs_.push_back(std::make_shared<Agence>(
        "agence", "agence", "Agence centrale", "agence@gouv.fr",
        "Surveillance ATMO", "France"));
}

// ---------------- Capteurs ----------------

Capteur* DataReader::getCapteur(const std::string& idCapteur) {
    for (auto& c : capteurs_) if (c.getId() == idCapteur) return &c;
    return nullptr;
}

const Capteur* DataReader::getCapteur(const std::string& idCapteur) const {
    for (const auto& c : capteurs_) if (c.getId() == idCapteur) return &c;
    return nullptr;
}

std::vector<Capteur*> DataReader::getTousCapteurs() {
    std::vector<Capteur*> out;
    out.reserve(capteurs_.size());
    for (auto& c : capteurs_) out.push_back(&c);
    return out;
}

std::vector<Capteur*> DataReader::getTousCapteursLatLon(double lat, double lon, double tolKm) {
    std::vector<Capteur*> out;
    for (auto& c : capteurs_) {
        if (distanceKm(lat, lon, c.getLatitude(), c.getLongitude()) <= tolKm) {
            out.push_back(&c);
        }
    }
    return out;
}

std::vector<Capteur*> DataReader::getAutresCapteurs(const std::string& idRef) {
    std::vector<Capteur*> out;
    for (auto& c : capteurs_) if (c.getId() != idRef) out.push_back(&c);
    return out;
}

std::vector<Capteur*> DataReader::getCapteursDansRayon(double lat, double lon, double rayonKm) {
    std::vector<Capteur*> out;
    for (auto& c : capteurs_) {
        if (distanceKm(lat, lon, c.getLatitude(), c.getLongitude()) <= rayonKm) {
            out.push_back(&c);
        }
    }
    return out;
}

std::vector<Capteur*> DataReader::getCapteursValidesDansRayon(double lat, double lon, double rayonKm) {
    std::vector<Capteur*> out;
    for (auto& c : capteurs_) {
        if (!c.estFiable()) continue;
        if (c.aProprietairePrive()) {
            const Particulier* p = nullptr;
            for (const auto& u : utilisateurs_) {
                if (u->getId() == c.getProprietaireId()
                    && u->getRole() == RoleUtilisateur::PARTICULIER) {
                    p = static_cast<const Particulier*>(u.get());
                    break;
                }
            }
            if (p && !p->estFiable()) continue;  // exclus si propriétaire non fiable
        }
        if (distanceKm(lat, lon, c.getLatitude(), c.getLongitude()) <= rayonKm) {
            out.push_back(&c);
        }
    }
    return out;
}

std::vector<Capteur*> DataReader::getCapteursVoisins(const std::string& idCapteur, double rayonKm) {
    Capteur* ref = getCapteur(idCapteur);
    if (!ref) return {};
    std::vector<Capteur*> out;
    for (auto& c : capteurs_) {
        if (c.getId() == idCapteur) continue;
        if (distanceKm(ref->getLatitude(), ref->getLongitude(),
                       c.getLatitude(), c.getLongitude()) <= rayonKm) {
            out.push_back(&c);
        }
    }
    return out;
}

std::vector<Capteur*> DataReader::getCapteursOfficielsVoisins(const std::string& idCapteur, double rayonKm) {
    auto voisins = getCapteursVoisins(idCapteur, rayonKm);
    std::vector<Capteur*> out;
    for (auto* c : voisins) {
        if (!c->aProprietairePrive()) out.push_back(c);
    }
    return out;
}

// ---------------- Mesures ----------------

std::vector<Mesure> DataReader::getMesuresCapteur(const std::string& idCapteur) const {
    std::vector<Mesure> out;
    for (const auto& m : mesures_) if (m.getCapteurId() == idCapteur) out.push_back(m);
    return out;
}

std::vector<Mesure> DataReader::getMesuresPeriode(const std::string& idCapteur,
                                                  const DateTime& debut, const DateTime& fin) const {
    std::vector<Mesure> out;
    for (const auto& m : mesures_) {
        if (m.getCapteurId() != idCapteur) continue;
        if (m.getTimestamp() < debut) continue;
        if (m.getTimestamp() > fin) continue;
        out.push_back(m);
    }
    return out;
}

std::vector<Mesure> DataReader::getMesuresAuTimestamp(const std::string& idCapteur,
                                                      const DateTime& timestamp,
                                                      long toleranceSec) const {
    std::vector<Mesure> out;
    long long t = static_cast<long long>(timestamp.toEpoch());
    for (const auto& m : mesures_) {
        if (m.getCapteurId() != idCapteur) continue;
        long long diff = static_cast<long long>(m.getTimestamp().toEpoch()) - t;
        if (diff < 0) diff = -diff;
        if (diff <= toleranceSec) {
            out.push_back(m);
        }
    }
    return out;
}

std::vector<Mesure> DataReader::getMesuresCapteursVoisins(const std::string& idCapteur,
                                                          double rayonKm) const {
    const Capteur* ref = getCapteur(idCapteur);
    if (!ref) return {};
    std::unordered_set<std::string> idsVoisins;
    for (const auto& c : capteurs_) {
        if (c.getId() == idCapteur) continue;
        if (distanceKm(ref->getLatitude(), ref->getLongitude(),
                       c.getLatitude(), c.getLongitude()) <= rayonKm) {
            idsVoisins.insert(c.getId());
        }
    }
    std::vector<Mesure> out;
    for (const auto& m : mesures_) {
        if (idsVoisins.count(m.getCapteurId())) {
            out.push_back(m);
        }
    }
    return out;
}

// ---------------- Purificateurs ----------------

Purificateur* DataReader::getPurificateur(const std::string& idPurificateur) {
    for (auto& p : purificateurs_) if (p.getId() == idPurificateur) return &p;
    return nullptr;
}

// ---------------- Mutations ----------------

void DataReader::marquerCapteur(const std::string& idCapteur, bool estFiable) {
    if (auto* c = getCapteur(idCapteur)) c->setEstFiable(estFiable);
}

void DataReader::marquerMesuresInvalides(const std::string& idCapteur) {
    for (auto& m : mesures_) if (m.getCapteurId() == idCapteur) m.setEstValide(false);
}

void DataReader::sauvegarderCapteur(const Capteur& capteur) {
    if (auto* c = getCapteur(capteur.getId())) {
        *c = capteur;
    } else {
        capteurs_.push_back(capteur);
    }
}

// ---------------- Ajouts ----------------

void DataReader::addCapteur(const Capteur& c)                  { capteurs_.push_back(c); }
void DataReader::addUtilisateur(std::shared_ptr<Utilisateur> u){ utilisateurs_.push_back(std::move(u)); }
void DataReader::addPurificateur(const Purificateur& p)        { purificateurs_.push_back(p); }
void DataReader::addMesure(const Mesure& m)                    { mesures_.push_back(m); }

// ---------------- Utilisateurs ----------------

std::shared_ptr<Utilisateur> DataReader::getUtilisateur(const std::string& id) const {
    for (const auto& u : utilisateurs_) if (u->getId() == id) return u;
    return nullptr;
}

Particulier* DataReader::getParticulier(const std::string& id) {
    for (auto& u : utilisateurs_) {
        if (u->getId() == id && u->getRole() == RoleUtilisateur::PARTICULIER) {
            return static_cast<Particulier*>(u.get());
        }
    }
    return nullptr;
}

} // namespace airwatcher
