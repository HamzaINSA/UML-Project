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

using namespace std;

namespace airwatcher {

namespace {
    // Pie
    const double kPi = 3.14159265358979323846;

    // Rayon moyen de la Terre en kilomètres.
    const double kEarthRadiusKm = 6371.0;

    // Convertit un angle en degrés vers des radians.
    double toRad(double deg) {
        return deg * kPi / 180.0;
    }
}  // namespace anonyme

// ============================================================
// Constructeur
// ============================================================

DataReader::DataReader(string dataDir) : dataDir_(move(dataDir)) {} // move = optimisation pour éviter une copie inutile de la chaîne de caractères (on transfère la propriété de la chaîne dataDir à l'objet DataReader)
// ============================================================
// Calcul de distance
// ============================================================

// Calcule la distance en kilomètres entre deux points géographiques
// à l'aide de la formule de Haversine.
double DataReader::distanceKm(double lat1, double lon1, double lat2, double lon2) {
    double dLat = toRad(lat2 - lat1);
    double dLon = toRad(lon2 - lon1);
    double a = sin(dLat / 2) * sin(dLat / 2)
             + cos(toRad(lat1)) * cos(toRad(lat2))
               * sin(dLon / 2) * sin(dLon / 2);
    double c = 2.0 * atan2(sqrt(a), sqrt(1 - a));
    return kEarthRadiusKm * c;
}

// ============================================================
// Utilitaire de découpage de ligne CSV
// ============================================================

// Découpe une ligne de texte selon un séparateur donné et retourne les colonnes.
// Les caractères '\r' sont ignorés pour gérer les fins de ligne Windows (CRLF).
vector<string> DataReader::splitLine(const string& line, char sep) {
    vector<string> resultat;
    string colonne;

    for (char caractere : line) {
        if (caractere == sep) {
            // Fin de la colonne courante : on l'ajoute au résultat.
            resultat.push_back(colonne);
            colonne.clear();
        } else if (caractere != '\r') {
            // On ignore le retour chariot Windows.
            colonne += caractere;
        }
    }

    // On ajoute la dernière colonne (pas de séparateur final).
    resultat.push_back(colonne);
    return resultat;
}

// ============================================================
// Chargement global des données
// ============================================================

// Charge l'ensemble des données depuis les fichiers CSV du répertoire configuré.
void DataReader::loadDataFromCSV() {
    loadAttributes();
    loadSensors();
    loadUsers();         // Associe les propriétaires aux capteurs.
    loadMeasurements();
    loadProviders();
    loadCleaners();
}

// ============================================================
// Chargement des attributs
// ============================================================

// Charge les attributs de qualité de l'air depuis le fichier attributes.csv.
void DataReader::loadAttributes() {
    ifstream fichier(dataDir_ + "/attributes.csv");

    if (!fichier) {
        throw runtime_error("Impossible d'ouvrir : " + dataDir_ + "/attributes.csv");
    }

    string ligne;
    bool estEntete = true;

    while (getline(fichier, ligne)) {
        if (ligne.empty()) {
            continue;
        }

        // On ignore la ligne d'en-tête si elle contient "AttributeID".
        if (estEntete) {
            estEntete = false;
            if (ligne.find("AttributeID") != string::npos) {
                continue;
            }
        }

        vector<string> colonnes = splitLine(ligne);

        if (colonnes.size() < 3) {
            continue;
        }

        attributs_.emplace_back(colonnes[0], colonnes[1], colonnes[2]);
    }
}

// ============================================================
// Chargement des capteurs
// ============================================================

// Charge les capteurs depuis le fichier sensors.csv.
void DataReader::loadSensors() {
    ifstream fichier(dataDir_ + "/sensors.csv");

    if (!fichier) {
        throw runtime_error("Impossible d'ouvrir : " + dataDir_ + "/sensors.csv");
    }

    string ligne;
    bool estEntete = true;

    while (getline(fichier, ligne)) {
        if (ligne.empty()) {
            continue;
        }

        // On ignore la ligne d'en-tête si elle contient "SensorID".
        if (estEntete) {
            estEntete = false;
            if (ligne.find("SensorID") != string::npos) {
                continue;
            }
        }

        vector<string> colonnes = splitLine(ligne);

        if (colonnes.size() < 3) {
            continue;
        }

        try {
            double latitude  = stod(colonnes[1]);
            double longitude = stod(colonnes[2]);
            // Le capteur est créé comme fiable par défaut, sans propriétaire assigné.
            capteurs_.emplace_back(colonnes[0], latitude, longitude, true, "");
        } catch (...) {
            cerr << "Ligne sensors.csv ignorée : " << ligne << '\n';
        }
    }
}

// ============================================================
// Chargement des mesures
// ============================================================

// Charge les mesures depuis le fichier measurements.csv.
void DataReader::loadMeasurements() {
    ifstream fichier(dataDir_ + "/measurements.csv");

    if (!fichier) {
        throw runtime_error("Impossible d'ouvrir : " + dataDir_ + "/measurements.csv");
    }

    string ligne;
    bool estEntete = true;

    while (getline(fichier, ligne)) {
        if (ligne.empty()) {
            continue;
        }

        // On ignore la ligne d'en-tête si elle contient "Timestamp".
        if (estEntete) {
            estEntete = false;
            if (ligne.find("Timestamp") != string::npos) {
                continue;
            }
        }

        vector<string> colonnes = splitLine(ligne);

        if (colonnes.size() < 4) {
            continue;
        }

        try {
            DateTime horodatage = DateTime::parse(colonnes[0]);
            double valeur       = stod(colonnes[3]);
            // La mesure est créée comme valide par défaut.
            mesures_.emplace_back(horodatage, colonnes[1], colonnes[2], valeur, true);
        } catch (...) {
            cerr << "Ligne measurements.csv ignorée : " << ligne << '\n';
        }
    }
}

// ============================================================
// Chargement des purificateurs (cleaners)
// ============================================================

// Charge les purificateurs depuis le fichier cleaners.csv et
// leur associe un fournisseur en croisant avec providers.csv.
void DataReader::loadCleaners() {
    ifstream fichierCleaners(dataDir_ + "/cleaners.csv");

    if (!fichierCleaners) {
        throw runtime_error("Impossible d'ouvrir : " + dataDir_ + "/cleaners.csv");
    }

    // Construction d'un index : identifiant cleaner → identifiant fournisseur.
    unordered_map<string, string> cleanerVersFournisseur;
    {
        ifstream fichierProviders(dataDir_ + "/providers.csv");

        if (fichierProviders) {
            string ligneProvider;
            bool estEntete = true;

            while (getline(fichierProviders, ligneProvider)) {
                if (ligneProvider.empty()) {
                    continue;
                }

                if (estEntete) {
                    estEntete = false;
                    if (ligneProvider.find("ProviderID") != string::npos) {
                        continue;
                    }
                }

                vector<string> colonnes = splitLine(ligneProvider);

                if (colonnes.size() < 2) {
                    continue;
                }

                // colonnes[0] = identifiant fournisseur, colonnes[1] = identifiant cleaner.
                cleanerVersFournisseur[colonnes[1]] = colonnes[0];
            }
        }
    }

    // Lecture des purificateurs.
    string ligne;
    bool estEntete = true;

    while (getline(fichierCleaners, ligne)) {
        if (ligne.empty()) {
            continue;
        }

        if (estEntete) {
            estEntete = false;
            if (ligne.find("CleanerID") != string::npos) {
                continue;
            }
        }

        vector<string> colonnes = splitLine(ligne);

        if (colonnes.size() < 5) {
            continue;
        }

        try {
            double latitude    = stod(colonnes[1]);
            double longitude   = stod(colonnes[2]);
            DateTime debut     = DateTime::parse(colonnes[3]);
            DateTime fin       = DateTime::parse(colonnes[4]);
            string fournisseurId;

            // On recherche le fournisseur associé à ce purificateur.
            unordered_map<string, string>::iterator it = cleanerVersFournisseur.find(colonnes[0]);
            if (it != cleanerVersFournisseur.end()) {
                fournisseurId = it->second;
            }

            purificateurs_.emplace_back(colonnes[0], latitude, longitude, debut, fin, fournisseurId);
        } catch (...) {
            cerr << "Ligne cleaners.csv ignorée : " << ligne << '\n';
        }
    }
}

// ============================================================
// Chargement des fournisseurs
// ============================================================

// Crée un objet Fournisseur générique pour chaque identifiant
// rencontré dans providers.csv.
void DataReader::loadProviders() {
    ifstream fichier(dataDir_ + "/providers.csv");

    if (!fichier) {
        // Le fichier providers est facultatif : on retourne silencieusement.
        return;
    }

    string ligne;
    bool estEntete = true;
    unordered_set<string> identifiantsDeja;

    while (getline(fichier, ligne)) {
        if (ligne.empty()) {
            continue;
        }

        if (estEntete) {
            estEntete = false;
            if (ligne.find("ProviderID") != string::npos) {
                continue;
            }
        }

        vector<string> colonnes = splitLine(ligne);

        if (colonnes.empty()) {
            continue;
        }

        const string& identifiant = colonnes[0];

        // On évite les doublons de fournisseur.
        if (identifiantsDeja.count(identifiant)) {
            continue;
        }

        identifiantsDeja.insert(identifiant);

        // Création d'un fournisseur avec des données génériques basées sur son identifiant.
        utilisateurs_.push_back(make_shared<Fournisseur>(
            identifiant,
            "fournisseur",
            "Fournisseur " + identifiant,
            identifiant + "@fournisseur.fr",
            "Entreprise " + identifiant,
            "00000000000000",
            identifiant + "@support.fr"
        ));
    }
}

// ============================================================
// Chargement des utilisateurs (particuliers)
// ============================================================

// Charge les utilisateurs depuis users.csv et lie chaque
// particulier au capteur dont il est propriétaire.
void DataReader::loadUsers() {
    ifstream fichier(dataDir_ + "/users.csv");

    if (!fichier) {
        throw runtime_error("Impossible d'ouvrir : " + dataDir_ + "/users.csv");
    }

    string ligne;
    bool estEntete = true;

    while (getline(fichier, ligne)) {
        if (ligne.empty()) {
            continue;
        }

        if (estEntete) {
            estEntete = false;
            if (ligne.find("UserID") != string::npos) {
                continue;
            }
        }

        vector<string> colonnes = splitLine(ligne);

        if (colonnes.size() < 2) {
            continue;
        }

        const string& idUtilisateur = colonnes[0];
        const string& idCapteur     = colonnes[1];

        // On crée le Particulier seulement s'il n'existe pas encore dans la liste.
        bool dejaPresent = false;
        for (const shared_ptr<Utilisateur>& utilisateur : utilisateurs_) {
            if (utilisateur->getId() == idUtilisateur) {
                dejaPresent = true;
                break;
            }
        }

        if (!dejaPresent) {
            utilisateurs_.push_back(make_shared<Particulier>(
                idUtilisateur,
                "particulier",
                "Particulier " + idUtilisateur,
                idUtilisateur + "@user.fr",
                "Adresse inconnue",
                0,
                true,
                true
            ));
        }

        // On lie le capteur correspondant à ce propriétaire.
        for (Capteur& capteur : capteurs_) {
            if (capteur.getId() == idCapteur) {
                capteur.setProprietaireId(idUtilisateur);
                break;
            }
        }
    }

    // On ajoute une agence par défaut pour permettre la connexion de supervision.
    utilisateurs_.push_back(make_shared<Agence>(
        "agence",
        "agence",
        "Agence centrale",
        "agence@gouv.fr",
        "Surveillance ATMO",
        "France"
    ));
}

// ============================================================
// Accès aux capteurs
// ============================================================

// Retourne un pointeur vers le capteur dont l'identifiant correspond,
// ou nullptr s'il n'existe pas (version non-constante).
Capteur* DataReader::getCapteur(const string& idCapteur) {
    for (Capteur& capteur : capteurs_) {
        if (capteur.getId() == idCapteur) {
            return &capteur;
        }
    }
    return nullptr;
}

// Retourne un pointeur constant vers le capteur dont l'identifiant correspond,
// ou nullptr s'il n'existe pas (version constante).
const Capteur* DataReader::getCapteur(const string& idCapteur) const {
    for (const Capteur& capteur : capteurs_) {
        if (capteur.getId() == idCapteur) {
            return &capteur;
        }
    }
    return nullptr;
}

// Retourne tous les capteurs sous forme de pointeurs non-constants.
vector<Capteur*> DataReader::getTousCapteurs() {
    vector<Capteur*> resultat;
    resultat.reserve(capteurs_.size());

    for (Capteur& capteur : capteurs_) {
        resultat.push_back(&capteur);
    }

    return resultat;
}

// Retourne les capteurs situés dans un rayon de tolérance
// autour d'une position géographique donnée.
vector<Capteur*> DataReader::getTousCapteursLatLon(double lat, double lon, double tolKm) {
    vector<Capteur*> resultat;

    for (Capteur& capteur : capteurs_) {
        if (distanceKm(lat, lon, capteur.getLatitude(), capteur.getLongitude()) <= tolKm) {
            resultat.push_back(&capteur);
        }
    }

    return resultat;
}

// Retourne tous les capteurs à l'exception du capteur de référence identifié.
vector<Capteur*> DataReader::getAutresCapteurs(const string& idRef) {
    vector<Capteur*> resultat;

    for (Capteur& capteur : capteurs_) {
        if (capteur.getId() != idRef) {
            resultat.push_back(&capteur);
        }
    }

    return resultat;
}

// Retourne les capteurs situés dans un rayon donné autour d'un point géographique.
vector<Capteur*> DataReader::getCapteursDansRayon(double lat, double lon, double rayonKm) {
    vector<Capteur*> resultat;

    for (Capteur& capteur : capteurs_) {
        if (distanceKm(lat, lon, capteur.getLatitude(), capteur.getLongitude()) <= rayonKm) {
            resultat.push_back(&capteur);
        }
    }

    return resultat;
}

// Retourne les capteurs fiables situés dans un rayon donné.
// Les capteurs appartenant à un particulier non fiable sont exclus.
vector<Capteur*> DataReader::getCapteursValidesDansRayon(double lat, double lon, double rayonKm) {
    vector<Capteur*> resultat;

    for (Capteur& capteur : capteurs_) {
        // On ignore les capteurs marqués comme non fiables.
        if (!capteur.estFiable()) {
            continue;
        }

        // Si le capteur appartient à un particulier, on vérifie sa fiabilité.
        if (capteur.aProprietairePrive()) {
            const Particulier* proprietaire = nullptr;

            for (const shared_ptr<Utilisateur>& utilisateur : utilisateurs_) {
                if (utilisateur->getId() == capteur.getProprietaireId()
                    && utilisateur->getRole() == RoleUtilisateur::PARTICULIER)
                {
                    proprietaire = static_cast<const Particulier*>(utilisateur.get());
                    break;
                }
            }

            // Si le propriétaire existe mais n'est pas fiable, on exclut le capteur.
            if (proprietaire != nullptr && !proprietaire->estFiable()) {
                continue;
            }
        }

        if (distanceKm(lat, lon, capteur.getLatitude(), capteur.getLongitude()) <= rayonKm) {
            resultat.push_back(&capteur);
        }
    }

    return resultat;
}

// Retourne les capteurs voisins d'un capteur de référence
// situés dans le rayon spécifié (le capteur de référence lui-même est exclu).
vector<Capteur*> DataReader::getCapteursVoisins(const string& idCapteur, double rayonKm) {
    Capteur* reference = getCapteur(idCapteur);

    if (reference == nullptr) {
        return vector<Capteur*>();
    }

    vector<Capteur*> resultat;

    for (Capteur& capteur : capteurs_) {
        if (capteur.getId() == idCapteur) {
            continue;
        }

        if (distanceKm(reference->getLatitude(), reference->getLongitude(),
                       capteur.getLatitude(), capteur.getLongitude()) <= rayonKm)
        {
            resultat.push_back(&capteur);
        }
    }

    return resultat;
}

// Retourne uniquement les capteurs officiels (sans propriétaire privé)
// voisins du capteur de référence dans le rayon spécifié.
vector<Capteur*> DataReader::getCapteursOfficielsVoisins(const string& idCapteur, double rayonKm) {
    vector<Capteur*> voisins = getCapteursVoisins(idCapteur, rayonKm);
    vector<Capteur*> resultat;

    for (Capteur* capteur : voisins) {
        if (!capteur->aProprietairePrive()) {
            resultat.push_back(capteur);
        }
    }

    return resultat;
}

// ============================================================
// Accès aux mesures
// ============================================================

// Retourne toutes les mesures enregistrées pour un capteur donné.
vector<Mesure> DataReader::getMesuresCapteur(const string& idCapteur) const {
    vector<Mesure> resultat;

    for (const Mesure& mesure : mesures_) {
        if (mesure.getCapteurId() == idCapteur) {
            resultat.push_back(mesure);
        }
    }

    return resultat;
}

// Retourne les mesures d'un capteur comprises dans un intervalle de temps.
vector<Mesure> DataReader::getMesuresPeriode(const string& idCapteur,
                                             const DateTime& debut,
                                             const DateTime& fin) const {
    vector<Mesure> resultat;

    for (const Mesure& mesure : mesures_) {
        if (mesure.getCapteurId() != idCapteur) {
            continue;
        }
        if (mesure.getTimestamp() < debut) {
            continue;
        }
        if (mesure.getTimestamp() > fin) {
            continue;
        }
        resultat.push_back(mesure);
    }

    return resultat;
}

// Retourne les mesures d'un capteur proches d'un horodatage donné,
// dans une fenêtre de tolérance exprimée en secondes.
vector<Mesure> DataReader::getMesuresAuTimestamp(const string& idCapteur,
                                                 const DateTime& timestamp,
                                                 long toleranceSec) const {
    vector<Mesure> resultat;
    long long epochReference = static_cast<long long>(timestamp.toEpoch());

    for (const Mesure& mesure : mesures_) {
        if (mesure.getCapteurId() != idCapteur) {
            continue;
        }

        long long diff = static_cast<long long>(mesure.getTimestamp().toEpoch()) - epochReference;

        // On prend la valeur absolue de la différence.
        if (diff < 0) {
            diff = -diff;
        }

        if (diff <= toleranceSec) {
            resultat.push_back(mesure);
        }
    }

    return resultat;
}

// Retourne toutes les mesures des capteurs voisins d'un capteur de référence
// situés dans le rayon spécifié.
vector<Mesure> DataReader::getMesuresCapteursVoisins(const string& idCapteur, double rayonKm) const {
    const Capteur* reference = getCapteur(idCapteur);
    if (reference == nullptr) {
        return vector<Mesure>();
    }

    // Construire la liste des capteurs voisins avec leur distance
    vector<pair<const Capteur*, double>> voisinsDistances;

    for (const Capteur& capteur : capteurs_) {
        if (capteur.getId() == idCapteur) {
            continue;
        }
        double dist = distanceKm(reference->getLatitude(), reference->getLongitude(),capteur.getLatitude(),    capteur.getLongitude());
        if (dist <= rayonKm) {
            voisinsDistances.emplace_back(&capteur, dist);
        }
    }

    // Construire un set des IDs voisins pour une recherche en O(1)
    unordered_set<string> voisinIds;
    for (const auto& voisin : voisinsDistances) {
        voisinIds.insert(voisin.first->getId());
    }

    // Filtrer les mesures appartenant aux capteurs voisins
    vector<Mesure> resultat;
    for (const Mesure& mesure : mesures_) {
        if (voisinIds.count(mesure.getCapteurId()) > 0) {
            resultat.push_back(mesure);
        }
    }   

    return resultat;
}

// ============================================================
// Accès aux purificateurs
// ============================================================

// Retourne un pointeur vers le purificateur correspondant à l'identifiant donné,
// ou nullptr s'il n'existe pas.
Purificateur* DataReader::getPurificateur(const string& idPurificateur) {
    for (Purificateur& purificateur : purificateurs_) {
        if (purificateur.getId() == idPurificateur) {
            return &purificateur;
        }
    }
    return nullptr;
}

// ============================================================
// Mutations (modification de l'état des données)
// ============================================================

// Marque un capteur comme fiable ou non fiable.
void DataReader::marquerCapteur(const string& idCapteur, bool estFiable) {
    Capteur* capteur = getCapteur(idCapteur);
    if (capteur != nullptr) {
        capteur->setEstFiable(estFiable);
    }
}

// Marque toutes les mesures d'un capteur donné comme invalides.
void DataReader::marquerMesuresInvalides(const string& idCapteur) {
    for (Mesure& mesure : mesures_) {
        if (mesure.getCapteurId() == idCapteur) {
            mesure.setEstValide(false);
        }
    }
}

// Met à jour un capteur existant ou l'ajoute s'il n'existe pas encore.
void DataReader::sauvegarderCapteur(const Capteur& capteur) {
    Capteur* existant = getCapteur(capteur.getId());

    if (existant != nullptr) {
        // On écrase les données du capteur existant.
        *existant = capteur;
    } else {
        // Le capteur est nouveau : on l'ajoute à la collection.
        capteurs_.push_back(capteur);
    }
}

// ============================================================
// Ajouts d'entités
// ============================================================

// Ajoute un capteur à la collection.
void DataReader::addCapteur(const Capteur& capteur) {
    capteurs_.push_back(capteur);
}

// Ajoute un utilisateur à la collection (via pointeur partagé).
void DataReader::addUtilisateur(shared_ptr<Utilisateur> utilisateur) {
    utilisateurs_.push_back(move(utilisateur));
}

// Ajoute un purificateur à la collection.
void DataReader::addPurificateur(const Purificateur& purificateur) {
    purificateurs_.push_back(purificateur);
}

// Ajoute une mesure à la collection.
void DataReader::addMesure(const Mesure& mesure) {
    mesures_.push_back(mesure);
}

// ============================================================
// Accès aux utilisateurs
// ============================================================

// Retourne le pointeur partagé vers l'utilisateur correspondant à l'identifiant donné,
// ou nullptr s'il n'existe pas.
shared_ptr<Utilisateur> DataReader::getUtilisateur(const string& id) const {
    for (const shared_ptr<Utilisateur>& utilisateur : utilisateurs_) {
        if (utilisateur->getId() == id) {
            return utilisateur;
        }
    }
    return nullptr;
}

// Retourne un pointeur brut vers le Particulier correspondant à l'identifiant donné,
// ou nullptr s'il n'existe pas ou si l'utilisateur n'est pas un particulier.
Particulier* DataReader::getParticulier(const string& id) {
    for (shared_ptr<Utilisateur>& utilisateur : utilisateurs_) {
        if (utilisateur->getId() == id
            && utilisateur->getRole() == RoleUtilisateur::PARTICULIER)
        {
            return static_cast<Particulier*>(utilisateur.get());
        }
    }
    return nullptr;
}

} // namespace airwatcher