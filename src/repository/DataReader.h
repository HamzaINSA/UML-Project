#ifndef AIRWATCHER_DATA_READER_H
#define AIRWATCHER_DATA_READER_H

#include <memory>
#include <string>
#include <vector>

#include "../model/Agence.h"
#include "../model/AttributMesure.h"
#include "../model/Capteur.h"
#include "../model/DateTime.h"
#include "../model/Fournisseur.h"
#include "../model/Mesure.h"
#include "../model/Particulier.h"
#include "../model/Purificateur.h"
#include "../model/Utilisateur.h"

namespace airwatcher {

class DataReader {
public:
    explicit DataReader(std::string dataDir = "data");

    // Chargement initial de tous les CSV.
    void loadDataFromCSV();

    // ----- Accès capteurs -----
    Capteur*               getCapteur(const std::string& idCapteur);
    const Capteur*         getCapteur(const std::string& idCapteur) const;
    std::vector<Capteur*>  getTousCapteurs();
    std::vector<Capteur*>  getTousCapteursLatLon(double lat, double lon, double tolKm = 5.0);
    std::vector<Capteur*>  getAutresCapteurs(const std::string& idRef);
    std::vector<Capteur*>  getCapteursDansRayon(double lat, double lon, double rayonKm);
    std::vector<Capteur*>  getCapteursValidesDansRayon(double lat, double lon, double rayonKm);
    std::vector<Capteur*>  getCapteursVoisins(const std::string& idCapteur, double rayonKm = 10.0);
    std::vector<Capteur*>  getCapteursOfficielsVoisins(const std::string& idCapteur, double rayonKm);

    // ----- Accès mesures -----
    std::vector<Mesure>    getMesuresCapteur(const std::string& idCapteur) const;
    std::vector<Mesure>    getMesuresPeriode(const std::string& idCapteur,
                                             const DateTime& debut, const DateTime& fin) const;
    std::vector<Mesure>    getMesuresAuTimestamp(const std::string& idCapteur,
                                                 const DateTime& timestamp,
                                                 long toleranceSec = 3600) const;
    std::vector<Mesure>    getMesuresCapteursVoisins(const std::string& idCapteur,
                                                     double rayonKm = 10.0) const;

    // ----- Purificateurs -----
    Purificateur*          getPurificateur(const std::string& idPurificateur);
    const std::vector<Purificateur>& getPurificateurs() const { return purificateurs_; }

    // ----- Mutations capteurs/mesures -----
    void marquerCapteur(const std::string& idCapteur, bool estFiable);
    void marquerMesuresInvalides(const std::string& idCapteur);
    void sauvegarderCapteur(const Capteur& capteur);

    // ----- Ajouts -----
    void addCapteur(const Capteur& c);
    void addUtilisateur(std::shared_ptr<Utilisateur> u);
    void addPurificateur(const Purificateur& p);
    void addMesure(const Mesure& m);

    // ----- Accès collections -----
    const std::vector<Capteur>&        getCapteurs() const   { return capteurs_; }
    const std::vector<Mesure>&         getMesures() const    { return mesures_; }
    const std::vector<AttributMesure>& getAttributs() const  { return attributs_; }
    const std::vector<std::shared_ptr<Utilisateur>>& getUtilisateurs() const { return utilisateurs_; }

    // Recherche utilisateurs.
    std::shared_ptr<Utilisateur> getUtilisateur(const std::string& id) const;
    Particulier*                 getParticulier(const std::string& id);

    // Aide : distance Haversine entre deux points (en kilomètres).
    static double distanceKm(double lat1, double lon1, double lat2, double lon2);

private:
    std::string dataDir_;

    std::vector<Capteur>        capteurs_;
    std::vector<Mesure>         mesures_;
    std::vector<AttributMesure> attributs_;
    std::vector<Purificateur>   purificateurs_;
    std::vector<std::shared_ptr<Utilisateur>> utilisateurs_;

    void loadAttributes();
    void loadSensors();
    void loadMeasurements();
    void loadCleaners();
    void loadProviders();
    void loadUsers();

    static std::vector<std::string> splitLine(const std::string& line, char sep = ';');
};

} // namespace airwatcher

#endif
