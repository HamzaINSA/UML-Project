// Tests pour les principaux services d'AirWatcher.
// Donnees CSV : annee 2019, capteurs Sensor0..Sensor99, users User0/User1,
// purificateurs Cleaner0/Cleaner1.

#include <iostream>
#include <string>

#include "../src/model/DateTime.h"
#include "../src/model/Particulier.h"
#include "../src/repository/DataReader.h"
#include "../src/service/AdministrationService.h"
#include "../src/service/AirQualityService.h"
#include "../src/service/EnvironmentalService.h"
#include "../src/service/PerformanceMonitor.h"

using namespace airwatcher;
using namespace std;

// On charge les CSV une seule fois pour aller plus vite.
static DataReader reader("data");
static PerformanceMonitor perf;


// --- Test 1 : analyser un capteur ---------------------------------------
void test_analyserCapteur() {
    cout << "\n--- Test analyserCapteur ---\n";
    AdministrationService admin(reader, perf);

    string id1 = "Sensor0";
    cout << "Parametres : idCapteur = " << id1 << "\n";
    Rapport r1 = admin.analyserCapteur(id1);
    cout << "Resultat   : " << Rapport::toString(r1.getEtat()) << "\n";

    string id2 = "SensorXXX";
    cout << "Parametres : idCapteur = " << id2 << "\n";
    Rapport r2 = admin.analyserCapteur(id2);
    cout << "Resultat   : " << Rapport::toString(r2.getEtat()) << "\n";
}


// --- Test 2 : classifier la fiabilite -----------------------------------
void test_classifierFiabilite() {
    cout << "\n--- Test classifierFiabilite ---\n";
    AdministrationService admin(reader, perf);

    string id1 = "Sensor0";
    cout << "Parametres : idCapteur = " << id1 << "\n";
    bool fiable = admin.classifierFiabilite(id1);
    cout << "Resultat   : " << (fiable ? "fiable" : "non fiable") << "\n";

    string id2 = "SensorXXX";
    cout << "Parametres : idCapteur = " << id2 << "\n";
    bool inconnu = admin.classifierFiabilite(id2);
    cout << "Resultat   : " << (inconnu ? "fiable" : "non fiable") << "\n";
}


// --- Test 3 : mesurer l'impact d'un purificateur ------------------------
void test_impactPurificateur() {
    cout << "\n--- Test mesurerImpactPurificateur ---\n";
    AirQualityService air(reader, perf);
    EnvironmentalService env(reader, perf, air);

    string id1 = "Cleaner0";
    cout << "Parametres : idPurificateur = " << id1 << "\n";
    ImpactPurificateur impact = env.mesurerImpactPurificateur(id1);
    if (impact.estSucces()) {
        cout << "Resultat   : delta = " << impact.getDelta()
             << ", rayon = " << impact.getRayonAction() << " km\n";
    } else {
        cout << "Resultat   : aucun impact mesurable\n";
    }

    string id2 = "CleanerXXX";
    cout << "Parametres : idPurificateur = " << id2 << "\n";
    ImpactPurificateur inconnu = env.mesurerImpactPurificateur(id2);
    cout << "Resultat   : "
         << (inconnu.estSucces() ? "succes" : "echec attendu") << "\n";
}


// --- Test 4 : moyenne AQI sur une zone ----------------------------------
void test_calculerMoyenneAQI() {
    cout << "\n--- Test calculerMoyenneAQI ---\n";
    AirQualityService air(reader, perf);
    AdministrationService admin(reader, perf);
    air.setAdministrationService(&admin);

    double lat = 45.0;
    double lon = 0.0;
    double rayon = 100.0;
    DateTime debut = DateTime::parse("2019-01-01 00:00:00");
    DateTime fin   = DateTime::parse("2019-12-31 23:59:59");

    cout << "Parametres : lat = " << lat << ", lon = " << lon
         << ", rayon = " << rayon << " km, periode = 2019\n";
    double indice = air.calculerMoyenneAQI(lat, lon, rayon, debut, fin);
    cout << "Resultat   : indice ATMO moyen = " << indice << "\n";
}


// --- Test 5 : comparer des capteurs -------------------------------------
void test_comparerCapteurs() {
    cout << "\n--- Test comparerCapteurs ---\n";
    AirQualityService air(reader, perf);

    string idRef = "Sensor0";
    DateTime debut = DateTime::parse("2019-01-01 00:00:00");
    DateTime fin   = DateTime::parse("2019-12-31 23:59:59");

    cout << "Parametres : idRef = " << idRef << ", periode = 2019\n";
    vector<CapteurScore> classement = air.comparerCapteurs(idRef, debut, fin);
    cout << "Resultat   : " << classement.size() << " capteurs compares\n";

    // On affiche le top 3 des plus similaires.
    int n = (classement.size() < 3) ? classement.size() : 3;
    for (int i = 0; i < n; ++i) {
        cout << "  #" << (i + 1) << " " << classement[i].capteur->getId()
             << " (score = " << classement[i].score << ")\n";
    }
}


// --- Test 6 : interpolation a une position quelconque -------------------
void test_interpolation() {
    cout << "\n--- Test interpolation (capteurs proches + IDW) ---\n";
    AirQualityService air(reader, perf);

    double lat = 44.2;
    double lon = -0.5;
    cout << "Parametres : lat = " << lat << ", lon = " << lon << "\n";

    auto proches = air.trouverCapteursProchesPourInterpolation(lat, lon);
    cout << "Resultat   : " << proches.size() << " capteurs proches retenus\n";
    double indice = air.interpolerPondereeParDistance(proches, lat, lon);
    cout << "             indice ATMO interpole = " << indice << "\n";
}


// --- Test 7 : attribution de points a un particulier --------------------
void test_attribuerPoints() {
    cout << "\n--- Test attribuerPoints ---\n";
    AdministrationService admin(reader, perf);

    string idUser = "User0";
    Particulier* user = reader.getParticulier(idUser);
    if (user == nullptr) {
        cout << "Resultat   : " << idUser << " introuvable\n";
        return;
    }

    int avant = user->getPoints();
    cout << "Parametres : idParticulier = " << idUser
         << ", points avant = " << avant << "\n";
    admin.attribuerPoints({idUser});
    cout << "Resultat   : points apres = " << user->getPoints() << "\n";
}


// --- main ---------------------------------------------------------------
int main() {
    cout << "=== Tests AirWatcher ===\n";
    reader.loadDataFromCSV();
    cout << "Donnees chargees : " << reader.getCapteurs().size()
         << " capteurs, " << reader.getMesures().size()
         << " mesures, " << reader.getPurificateurs().size()
         << " purificateurs\n";

    test_analyserCapteur();
    test_classifierFiabilite();
    test_impactPurificateur();
    test_calculerMoyenneAQI();
    test_comparerCapteurs();
    test_interpolation();
    test_attribuerPoints();

    cout << "\n=== Fin des tests ===\n";
    return 0;
}
