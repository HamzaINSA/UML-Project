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

using namespace std;

namespace airwatcher {

class DataReader {
public:
    explicit DataReader(string dataDir = "data");

    // Chargement initial de tous les CSV.
    void loadDataFromCSV();

    // ----- Accès capteurs -----
    Capteur*               getCapteur(const string& idCapteur);
    const Capteur*         getCapteur(const string& idCapteur) const;
    vector<Capteur*>  getTousCapteurs();
    vector<Capteur*>  getTousCapteursLatLon(double lat, double lon, double tolKm = 5.0);
    vector<Capteur*>  getAutresCapteurs(const string& idRef);
    vector<Capteur*>  getCapteursDansRayon(double lat, double lon, double rayonKm);
    vector<Capteur*>  getCapteursValidesDansRayon(double lat, double lon, double rayonKm);
    vector<Capteur*>  getCapteursVoisins(const string& idCapteur, double rayonKm = 10.0);
    vector<Capteur*>  getCapteursOfficielsVoisins(const string& idCapteur, double rayonKm);

    // ----- Accès mesures -----
    vector<Mesure>    getMesuresCapteur(const string& idCapteur) const;
    vector<Mesure>    getMesuresPeriode(const string& idCapteur,
                                             const DateTime& debut, const DateTime& fin) const;
    vector<Mesure>    getMesuresAuTimestamp(const string& idCapteur,
                                                 const DateTime& timestamp,
                                                 long toleranceSec = 3600) const;
    vector<Mesure>    getMesuresCapteursVoisins(const string& idCapteur,
                                                     double rayonKm = 10.0) const;

    // ----- Purificateurs -----
    Purificateur*          getPurificateur(const string& idPurificateur);
    const vector<Purificateur>& getPurificateurs() const { return purificateurs_; }

    // ----- Mutations capteurs/mesures -----
    void marquerCapteur(const string& idCapteur, bool estFiable);
    void marquerMesuresInvalides(const string& idCapteur);
    void sauvegarderCapteur(const Capteur& capteur);

    // ----- Ajouts -----
    void addCapteur(const Capteur& c);
    void addUtilisateur(shared_ptr<Utilisateur> u);
    void addPurificateur(const Purificateur& p);
    void addMesure(const Mesure& m);

    // ----- Accès collections -----
    const vector<Capteur>&        getCapteurs() const   { return capteurs_; }
    const vector<Mesure>&         getMesures() const    { return mesures_; }
    const vector<AttributMesure>& getAttributs() const  { return attributs_; }
    const vector<shared_ptr<Utilisateur>>& getUtilisateurs() const { return utilisateurs_; }

    // Recherche utilisateurs.
    shared_ptr<Utilisateur> getUtilisateur(const string& id) const;
    Particulier*                 getParticulier(const string& id);

    // Aide : distance Haversine entre deux points (en kilomètres).
    static double distanceKm(double lat1, double lon1, double lat2, double lon2);

private:
    string dataDir_;

    vector<Capteur>        capteurs_;
    vector<Mesure>         mesures_;
    vector<AttributMesure> attributs_;
    vector<Purificateur>   purificateurs_;
    vector<shared_ptr<Utilisateur>> utilisateurs_;

    void loadAttributes();
    void loadSensors();
    void loadMeasurements();
    void loadCleaners();
    void loadProviders();
    void loadUsers();

    static vector<string> splitLine(const string& line, char sep = ';');
};

} // namespace airwatcher

#endif
