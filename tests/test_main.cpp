// ============================================================================
// AirWatcher - Suite de tests unitaires et d'integration.
//
// Couvre les 6 scenarios du dossier LIVRABLES/Pseudo-code :
//   Scenario 1 : AdministrationService::analyserCapteur
//   Scenario 2 : AirQualityService::calculerMoyenneAQI
//   Scenario 3 : AirQualityService::comparerCapteurs
//   Scenario 4 : AirQualityService::interpolerPondereeParDistance
//                (+ trouverCapteursProchesPourInterpolation)
//   Scenario 5 : EnvironmentalService::mesurerImpactPurificateur
//   Scenario 6 : AdministrationService::classifierFiabilite
//
// + utilitaires (DateTime, distanceKm), acces DataReader,
//   attribution de points, PerformanceMonitor, et plusieurs cas extremes
//   destines a demontrer la robustesse du code (ID inconnus, periodes
//   hors donnees, zones vides, mesures invalides, capteur aberrant injecte).
//
// NOTE : les donnees CSV couvrent uniquement l'annee 2019, tous les tests
// utilisent donc des dates entre 2019-01-01 et 2019-12-31.
// ============================================================================

#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../src/model/Capteur.h"
#include "../src/model/DateTime.h"
#include "../src/model/ImpactPurificateur.h"
#include "../src/model/Mesure.h"
#include "../src/model/Particulier.h"
#include "../src/model/Rapport.h"
#include "../src/repository/DataReader.h"
#include "../src/service/AdministrationService.h"
#include "../src/service/AirQualityService.h"
#include "../src/service/EnvironmentalService.h"
#include "../src/service/PerformanceMonitor.h"

using namespace airwatcher;
using namespace std;

// ----------------------------------------------------------------------------
// Compteur d'assertions et macros d'assertion.
// ----------------------------------------------------------------------------
static int g_assertions = 0;

#define EXPECT(cond)                                                           \
    do {                                                                       \
        ++g_assertions;                                                        \
        if (!(cond)) {                                                         \
            cerr << "  [FAIL] " << __FILE__ << ":" << __LINE__                 \
                 << " : " #cond << '\n';                                       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

#define EXPECT_NEAR(a, b, tol)                                                 \
    do {                                                                       \
        ++g_assertions;                                                        \
        double _va = (a);                                                      \
        double _vb = (b);                                                      \
        if (fabs(_va - _vb) > (tol)) {                                         \
            cerr << "  [FAIL] " << __FILE__ << ":" << __LINE__                 \
                 << " : |" #a " - " #b "| > " << (tol)                         \
                 << "  (a=" << _va << ", b=" << _vb << ")\n";                  \
            return 1;                                                          \
        }                                                                     \
    } while (0)

// ----------------------------------------------------------------------------
// Constantes de periode (les donnees CSV couvrent uniquement 2019).
// ----------------------------------------------------------------------------
static const DateTime kDebut2019    = DateTime::parse("2019-01-01 00:00:00");
static const DateTime kFin2019      = DateTime::parse("2019-12-31 23:59:59");
static const DateTime kDebutJanvier = DateTime::parse("2019-01-01 00:00:00");
static const DateTime kFinJanvier   = DateTime::parse("2019-01-31 23:59:59");

// ============================================================================
// SECTION 0 : DateTime et fonctions geographiques utilitaires.
// ============================================================================

static int test_dateTime_parse() {
    cout << "[TEST] DateTime::parse - format ISO et constructeur equivalent\n";
    DateTime a = DateTime::parse("2019-06-15 12:30:45");
    DateTime b(2019, 6, 15, 12, 30, 45);
    EXPECT(a == b);
    EXPECT(a.toEpoch() == b.toEpoch());
    return 0;
}

static int test_dateTime_compare() {
    cout << "[TEST] DateTime - operateurs d'ordre (min < date < max)\n";
    DateTime mini = DateTime::min();
    DateTime mid  = DateTime::parse("2019-06-15 00:00:00");
    DateTime maxi = DateTime::max();
    EXPECT(mini < mid);
    EXPECT(mid  < maxi);
    EXPECT(mini < maxi);
    EXPECT(mid  >= mid);
    EXPECT(mid  <= mid);
    EXPECT(!(mid != mid));
    return 0;
}

static int test_distanceKm_basique() {
    cout << "[TEST] DataReader::distanceKm - paire de points proches (~0.7 km)\n";
    double d = DataReader::distanceKm(45.7640, 4.8357, 45.7700, 4.8400);
    EXPECT(d > 0.5 && d < 1.0);
    return 0;
}

static int test_distanceKm_meme_point() {
    cout << "[TEST] DataReader::distanceKm - meme point => distance nulle\n";
    double d = DataReader::distanceKm(45.0, 3.0, 45.0, 3.0);
    EXPECT_NEAR(d, 0.0, 1e-9);
    return 0;
}

static int test_distanceKm_un_degre() {
    cout << "[TEST] DataReader::distanceKm - 1 degre longitude a l'equateur (~111 km)\n";
    double d = DataReader::distanceKm(0.0, 0.0, 0.0, 1.0);
    EXPECT(d > 100.0 && d < 120.0);
    return 0;
}

// ============================================================================
// SECTION 1 : DataReader - chargement et acces.
// ============================================================================

static int test_chargement() {
    cout << "[TEST] DataReader::loadDataFromCSV - tailles attendues\n";
    DataReader r("data");
    r.loadDataFromCSV();
    // Sensor0 .. Sensor99
    EXPECT(r.getCapteurs().size() == 100);
    // 100 capteurs * 4 attributs * 365 jours
    EXPECT(r.getMesures().size() == 146000);
    // Cleaner0 et Cleaner1
    EXPECT(r.getPurificateurs().size() == 2);
    // O3, SO2, NO2, PM10
    EXPECT(r.getAttributs().size() == 4);
    // 2 Particuliers (User0, User1) + 1 Agence + 2 Fournisseurs
    EXPECT(r.getUtilisateurs().size() == 5);
    return 0;
}

static int test_getCapteur_connu_inconnu() {
    cout << "[TEST] DataReader::getCapteur - ID connu, inconnu et vide\n";
    DataReader r("data");
    r.loadDataFromCSV();
    Capteur* s0 = r.getCapteur("Sensor0");
    EXPECT(s0 != nullptr);
    EXPECT(s0->getId() == "Sensor0");
    EXPECT_NEAR(s0->getLatitude(),  44.0, 1e-6);
    EXPECT_NEAR(s0->getLongitude(), -1.0, 1e-6);
    // Cas extreme : ID inconnu et ID vide doivent renvoyer nullptr (pas planter).
    EXPECT(r.getCapteur("SensorXXX") == nullptr);
    EXPECT(r.getCapteur("") == nullptr);
    return 0;
}

static int test_getParticulier_connu_inconnu() {
    cout << "[TEST] DataReader::getParticulier - User0/User1 oui ; agence/Provider/inconnu non\n";
    DataReader r("data");
    r.loadDataFromCSV();
    EXPECT(r.getParticulier("User0") != nullptr);
    EXPECT(r.getParticulier("User1") != nullptr);
    // L'agence existe en tant qu'Utilisateur mais n'est pas un Particulier.
    EXPECT(r.getParticulier("agence")    == nullptr);
    // Un fournisseur n'est pas un Particulier.
    EXPECT(r.getParticulier("Provider0") == nullptr);
    // ID inconnu : nullptr (pas d'exception).
    EXPECT(r.getParticulier("UserXXX")   == nullptr);
    EXPECT(r.getParticulier("")          == nullptr);
    return 0;
}

static int test_proprietaire_lie() {
    cout << "[TEST] DataReader - liens proprietaire <-> capteur (User0->Sensor70, User1->Sensor36)\n";
    DataReader r("data");
    r.loadDataFromCSV();
    Capteur* s70 = r.getCapteur("Sensor70");
    Capteur* s36 = r.getCapteur("Sensor36");
    Capteur* s0  = r.getCapteur("Sensor0");
    EXPECT(s70 != nullptr && s36 != nullptr && s0 != nullptr);
    EXPECT(s70->aProprietairePrive());
    EXPECT(s36->aProprietairePrive());
    EXPECT(s70->getProprietaireId() == "User0");
    EXPECT(s36->getProprietaireId() == "User1");
    // Les capteurs officiels n'ont pas de proprietaire prive.
    EXPECT(!s0->aProprietairePrive());
    return 0;
}

static int test_capteurs_dans_rayon() {
    cout << "[TEST] DataReader::getCapteursDansRayon - zone peuplee et zone vide\n";
    DataReader r("data");
    r.loadDataFromCSV();
    // Autour de Sensor0 (44, -1) : au moins Sensor0 lui-meme.
    vector<Capteur*> proches = r.getCapteursDansRayon(44.0, -1.0, 1.0);
    EXPECT(!proches.empty());
    bool trouveS0 = false;
    for (Capteur* c : proches) {
        if (c->getId() == "Sensor0") { trouveS0 = true; break; }
    }
    EXPECT(trouveS0);
    // Cas extreme : zone tres loin de toute mesure (ocean Atlantique).
    vector<Capteur*> vide = r.getCapteursDansRayon(0.0, 0.0, 1.0);
    EXPECT(vide.empty());
    return 0;
}

static int test_capteurs_valides_filtre_non_fiable() {
    cout << "[TEST] DataReader::getCapteursValidesDansRayon - filtrage des capteurs/proprietaires non fiables\n";
    DataReader r("data");
    r.loadDataFromCSV();
    // Avant : Sensor36 (45.2, 3.2) est present dans la zone.
    vector<Capteur*> avant = r.getCapteursValidesDansRayon(45.2, 3.2, 1.0);
    bool trouveAvant = false;
    for (Capteur* c : avant) {
        if (c->getId() == "Sensor36") { trouveAvant = true; break; }
    }
    EXPECT(trouveAvant);

    // On marque User1 (proprietaire de Sensor36) comme non fiable.
    Particulier* u1 = r.getParticulier("User1");
    EXPECT(u1 != nullptr);
    u1->setEstFiable(false);

    // Apres : Sensor36 doit etre exclu de la zone valide.
    vector<Capteur*> apres = r.getCapteursValidesDansRayon(45.2, 3.2, 1.0);
    for (Capteur* c : apres) {
        EXPECT(c->getId() != "Sensor36");
    }
    return 0;
}

static int test_getMesuresPeriode() {
    cout << "[TEST] DataReader::getMesuresPeriode - filtrage temporel (janvier = 31*4 mesures)\n";
    DataReader r("data");
    r.loadDataFromCSV();
    vector<Mesure> annee   = r.getMesuresPeriode("Sensor0", kDebut2019, kFin2019);
    vector<Mesure> janvier = r.getMesuresPeriode("Sensor0", kDebutJanvier, kFinJanvier);
    EXPECT(annee.size() > janvier.size());
    EXPECT(janvier.size() == 31 * 4);
    // Toutes les mesures retournees appartiennent bien a Sensor0.
    for (const Mesure& m : janvier) {
        EXPECT(m.getCapteurId() == "Sensor0");
    }
    return 0;
}

static int test_getMesuresPeriode_hors_2019() {
    cout << "[TEST] DataReader::getMesuresPeriode - aucune donnee hors 2019 (avant + apres)\n";
    DataReader r("data");
    r.loadDataFromCSV();
    vector<Mesure> avant2019 = r.getMesuresPeriode("Sensor0",
        DateTime::parse("2018-01-01 00:00:00"),
        DateTime::parse("2018-12-31 23:59:59"));
    EXPECT(avant2019.empty());
    vector<Mesure> apres2019 = r.getMesuresPeriode("Sensor0",
        DateTime::parse("2020-01-01 00:00:00"),
        DateTime::parse("2020-12-31 23:59:59"));
    EXPECT(apres2019.empty());
    return 0;
}

static int test_getMesuresPeriode_capteur_inconnu() {
    cout << "[TEST] DataReader::getMesuresPeriode - capteur inconnu => liste vide\n";
    DataReader r("data");
    r.loadDataFromCSV();
    vector<Mesure> m = r.getMesuresPeriode("SensorXXX", kDebut2019, kFin2019);
    EXPECT(m.empty());
    return 0;
}

// ============================================================================
// SECTION 2 : AirQualityService - calculs ATMO directs.
// ============================================================================

static int test_atmo_vecteur_vide() {
    cout << "[TEST] AirQualityService::calculerIndiceATMO - vecteur vide => 0\n";
    DataReader r("data");
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    vector<Mesure> aucune;
    EXPECT_NEAR(air.calculerIndiceATMO(aucune), 0.0, 1e-9);
    return 0;
}

static int test_atmo_valeurs_basses() {
    cout << "[TEST] AirQualityService::calculerIndiceATMO - 4 valeurs basses => sous-indice 2 (NO2 baseline)\n";
    DataReader r("data");
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    DateTime t = DateTime::parse("2019-06-01 12:00:00");
    vector<Mesure> mesures;
    mesures.emplace_back(t, "X", "O3",   10.0, true);  // sous-indice 1 (<=30)
    mesures.emplace_back(t, "X", "NO2",  40.0, true);  // sous-indice 2 (<=55)
    mesures.emplace_back(t, "X", "SO2",  30.0, true);  // sous-indice 1 (<=40)
    mesures.emplace_back(t, "X", "PM10",  5.0, true);  // sous-indice 1 (<=7)
    EXPECT_NEAR(air.calculerIndiceATMO(mesures), 2.0, 1e-9);
    return 0;
}

static int test_atmo_max_grand_polluant() {
    cout << "[TEST] AirQualityService::calculerIndiceATMO - retient le maximum (PM10=100 => 10)\n";
    DataReader r("data");
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    DateTime t = DateTime::parse("2019-06-01 12:00:00");
    vector<Mesure> mesures;
    mesures.emplace_back(t, "X", "O3",    10.0, true);
    mesures.emplace_back(t, "X", "PM10", 100.0, true);  // >79 => sous-indice 10
    EXPECT_NEAR(air.calculerIndiceATMO(mesures), 10.0, 1e-9);
    return 0;
}

static int test_atmo_ignore_invalides() {
    cout << "[TEST] AirQualityService::calculerIndiceATMO - mesures invalides ignorees\n";
    DataReader r("data");
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    DateTime t = DateTime::parse("2019-06-01 12:00:00");
    vector<Mesure> mesures;
    mesures.emplace_back(t, "X", "O3",   10.0,  true);   // seule mesure valide
    mesures.emplace_back(t, "X", "PM10", 9999.0, false); // ignoree
    // Seul O3 contribue => sous-indice 1.
    EXPECT_NEAR(air.calculerIndiceATMO(mesures), 1.0, 1e-9);
    return 0;
}

static int test_score_meme_capteur() {
    cout << "[TEST] AirQualityService::calculerScoreSimilarite - meme set => score 0\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    vector<Mesure> m = r.getMesuresPeriode("Sensor0", kDebut2019, kFin2019);
    double score = air.calculerScoreSimilarite(m, m);
    EXPECT_NEAR(score, 0.0, 1e-9);
    return 0;
}

// ============================================================================
// SECTION 3 : Scenario 2 - AirQualityService::calculerMoyenneAQI.
// ============================================================================

static int test_calculerMoyenneAQI() {
    cout << "[TEST] Scenario 2 - calculerMoyenneAQI sur zone reelle (45.0, 0.0, 100km)\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    AdministrationService admin(r, perf);
    air.setAdministrationService(&admin);

    double indice = air.calculerMoyenneAQI(45.0, 0.0, 100.0, kDebut2019, kFin2019);
    EXPECT(indice > 0.0);
    EXPECT(indice <= 10.0);
    return 0;
}

static int test_calculerMoyenneAQI_zone_vide() {
    cout << "[TEST] Scenario 2 - aucune zone (0,0 + rayon 0.1km) => indice 0\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    AdministrationService admin(r, perf);
    air.setAdministrationService(&admin);
    EXPECT_NEAR(air.calculerMoyenneAQI(0.0, 0.0, 0.1, kDebut2019, kFin2019), 0.0, 1e-9);
    return 0;
}

static int test_calculerMoyenneAQI_periode_hors_donnees() {
    cout << "[TEST] Scenario 2 - periode 2018 (avant les donnees) => indice 0\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    AdministrationService admin(r, perf);
    air.setAdministrationService(&admin);
    double indice = air.calculerMoyenneAQI(44.0, -1.0, 10.0,
        DateTime::parse("2018-01-01 00:00:00"),
        DateTime::parse("2018-12-31 00:00:00"));
    EXPECT_NEAR(indice, 0.0, 1e-9);
    return 0;
}

static int test_calculerMoyenneAQI_sans_admin() {
    cout << "[TEST] Scenario 2 - sans AdministrationService branche (pas de crash)\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    // Pas de setAdministrationService -> admin_ reste nullptr.
    double indice = air.calculerMoyenneAQI(45.0, 0.0, 100.0, kDebut2019, kFin2019);
    EXPECT(indice >= 0.0);
    return 0;
}

// ============================================================================
// SECTION 4 : Scenario 3 - AirQualityService::comparerCapteurs.
// ============================================================================

static int test_comparerCapteurs() {
    cout << "[TEST] Scenario 3 - comparerCapteurs(Sensor0) : tri ascendant + Sensor0 exclu\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    vector<CapteurScore> classement = air.comparerCapteurs("Sensor0", kDebut2019, kFin2019);
    EXPECT(!classement.empty());
    // Tri ascendant.
    for (size_t i = 1; i < classement.size(); ++i) {
        EXPECT(classement[i-1].score <= classement[i].score);
    }
    // Le capteur de reference ne doit pas figurer dans le classement.
    for (const CapteurScore& cs : classement) {
        EXPECT(cs.capteur->getId() != "Sensor0");
    }
    // Scores positifs ou nuls.
    for (const CapteurScore& cs : classement) {
        EXPECT(cs.score >= 0.0);
    }
    return 0;
}

static int test_comparerCapteurs_ref_inconnue() {
    cout << "[TEST] Scenario 3 - capteur de reference inconnu : pas de crash, tri respecte\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    vector<CapteurScore> classement = air.comparerCapteurs("SensorXXX", kDebut2019, kFin2019);
    for (size_t i = 1; i < classement.size(); ++i) {
        EXPECT(classement[i-1].score <= classement[i].score);
    }
    return 0;
}

// ============================================================================
// SECTION 5 : Scenario 4 - interpolation spatiale ponderee par distance.
// ============================================================================

static int test_interpolation_point_arbitraire() {
    cout << "[TEST] Scenario 4 - interpolation sur point quelconque (44.2, -0.5)\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    auto proches = air.trouverCapteursProchesPourInterpolation(44.2, -0.5);
    EXPECT(!proches.empty());
    EXPECT(proches.size() <= 5);  // kNbVoisinsInterpolation = 5
    double indice = air.interpolerPondereeParDistance(proches, 44.2, -0.5);
    EXPECT(indice > 0.0);
    EXPECT(indice <= 10.0);
    return 0;
}

static int test_interpolation_point_exact() {
    cout << "[TEST] Scenario 4 - point exact d'un capteur => indice du capteur\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    // Sensor0 est exactement a (44.0, -1.0).
    auto proches = air.trouverCapteursProchesPourInterpolation(44.0, -1.0);
    EXPECT(!proches.empty());
    double indiceInterp = air.interpolerPondereeParDistance(proches, 44.0, -1.0);
    // Reference : indice ATMO de Sensor0 directement.
    vector<Mesure> mesS0 = r.getMesuresCapteur("Sensor0");
    double indiceDirect = air.calculerIndiceATMO(mesS0);
    EXPECT_NEAR(indiceInterp, indiceDirect, 1e-9);
    return 0;
}

static int test_interpolation_loin_de_tout() {
    cout << "[TEST] Scenario 4 - point a >50km de tout capteur => liste vide\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    // Coordonnees au large de l'Atlantique (loin du grid lat 44-47.6).
    auto proches = air.trouverCapteursProchesPourInterpolation(0.0, -30.0);
    EXPECT(proches.empty());
    double indice = air.interpolerPondereeParDistance(proches, 0.0, -30.0);
    EXPECT_NEAR(indice, 0.0, 1e-9);
    return 0;
}

// ============================================================================
// SECTION 6 : Scenario 1 - AdministrationService::analyserCapteur.
// ============================================================================

static int test_analyserCapteur_introuvable() {
    cout << "[TEST] Scenario 1 - capteur inconnu => etat INTROUVABLE\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AdministrationService admin(r, perf);
    Rapport rap = admin.analyserCapteur("SensorXXX");
    EXPECT(rap.getEtat() == EtatCapteur::INTROUVABLE);
    EXPECT(rap.getAnomalies().empty());
    return 0;
}

static int test_analyserCapteur_normal() {
    cout << "[TEST] Scenario 1 - Sensor0 (donnees du CSV) doit etre FONCTIONNEL\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AdministrationService admin(r, perf);
    Rapport rap = admin.analyserCapteur("Sensor0");
    EXPECT(rap.getEtat() == EtatCapteur::FONCTIONNEL);
    EXPECT(rap.getAnomalies().empty());
    return 0;
}

static int test_analyserCapteur_defaillant_synthetique() {
    cout << "[TEST] Scenario 1 - capteur synthetique aberrant => DEFAILLANT + anomalies\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AdministrationService admin(r, perf);

    // Capteur synthetique a 1.4km de Sensor0 avec valeurs hors bornes physiques.
    r.addCapteur(Capteur("TEST_HS", 44.01, -1.01, true, ""));
    for (int i = 0; i < 5; ++i) {
        DateTime ti(2019, 6, i + 1, 12, 0, 0);
        r.addMesure(Mesure(ti, "TEST_HS", "O3",   9999.0, true));
        r.addMesure(Mesure(ti, "TEST_HS", "PM10", 9999.0, true));
    }

    Rapport rap = admin.analyserCapteur("TEST_HS");
    EXPECT(rap.getEtat() == EtatCapteur::DEFAILLANT);
    EXPECT(!rap.getAnomalies().empty());
    return 0;
}

// ============================================================================
// SECTION 7 : Scenario 6 - AdministrationService::classifierFiabilite.
// ============================================================================

static int test_classifierFiabilite_inconnu() {
    cout << "[TEST] Scenario 6 - capteur inconnu => false (pas d'exception)\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AdministrationService admin(r, perf);
    EXPECT(admin.classifierFiabilite("SensorXXX") == false);
    return 0;
}

static int test_classifierFiabilite_normal() {
    cout << "[TEST] Scenario 6 - Sensor0 (donnees coherentes) reste fiable\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AdministrationService admin(r, perf);
    bool fiable = admin.classifierFiabilite("Sensor0");
    EXPECT(fiable == true);
    EXPECT(r.getCapteur("Sensor0")->estFiable() == true);
    return 0;
}

static int test_classifierFiabilite_aberrant() {
    cout << "[TEST] Scenario 6 - capteur synthetique aberrant => non fiable + cascade\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AdministrationService admin(r, perf);

    // Particulier proprietaire du capteur aberrant.
    auto particulier = make_shared<Particulier>(
        "USER_BAD", "mdp", "Bad", "bad@user.fr", "n/a", 0, true, true);
    r.addUtilisateur(particulier);

    // Capteur synthetique pres de Sensor0 avec valeurs grossierement hors bornes
    // pour les 4 attributs (cumul d'anomalies > kSeuilTolerance).
    r.addCapteur(Capteur("TEST_BAD", 44.01, -1.01, true, "USER_BAD"));
    for (int i = 0; i < 10; ++i) {
        DateTime ti(2019, 6, i + 1, 12, 0, 0);
        r.addMesure(Mesure(ti, "TEST_BAD", "O3",   9999.0, true));
        r.addMesure(Mesure(ti, "TEST_BAD", "NO2",  9999.0, true));
        r.addMesure(Mesure(ti, "TEST_BAD", "SO2",  9999.0, true));
        r.addMesure(Mesure(ti, "TEST_BAD", "PM10", 9999.0, true));
    }

    bool fiable = admin.classifierFiabilite("TEST_BAD");
    EXPECT(fiable == false);

    Capteur* cap = r.getCapteur("TEST_BAD");
    EXPECT(cap != nullptr);
    EXPECT(cap->estFiable() == false);

    // Le proprietaire passe non fiable et perd l'attribution de points.
    Particulier* p = r.getParticulier("USER_BAD");
    EXPECT(p != nullptr);
    EXPECT(p->estFiable() == false);
    EXPECT(p->attributionPointsActive() == false);

    // Toutes les mesures du capteur doivent etre marquees invalides.
    bool toutesInvalides = true;
    for (const Mesure& m : r.getMesures()) {
        if (m.getCapteurId() == "TEST_BAD" && m.estValide()) {
            toutesInvalides = false;
            break;
        }
    }
    EXPECT(toutesInvalides);
    return 0;
}

// ============================================================================
// SECTION 8 : Attribution de points (AdministrationService::attribuerPoints).
// ============================================================================

static int test_attribution_via_calculerMoyenneAQI() {
    cout << "[TEST] Attribution - User1 (proprietaire de Sensor36) gagne +1 point\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    AdministrationService admin(r, perf);
    air.setAdministrationService(&admin);

    Particulier* u1 = r.getParticulier("User1");
    EXPECT(u1 != nullptr);
    int avant = u1->getPoints();

    // Sensor36 est exactement a (45.2, 3.2). Rayon 1km => seul Sensor36.
    air.calculerMoyenneAQI(45.2, 3.2, 1.0, kDebut2019, kFin2019);
    EXPECT(u1->getPoints() == avant + 1);
    return 0;
}

static int test_attribution_non_fiable_refusee() {
    cout << "[TEST] Attribution - particulier non fiable : aucun point ajoute\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AdministrationService admin(r, perf);

    Particulier* u0 = r.getParticulier("User0");
    EXPECT(u0 != nullptr);
    u0->setEstFiable(false);
    int avant = u0->getPoints();
    admin.attribuerPoints({"User0"});
    EXPECT(u0->getPoints() == avant);
    return 0;
}

static int test_attribution_desactivee() {
    cout << "[TEST] Attribution - flag attributionPointsActive=false : aucun point\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AdministrationService admin(r, perf);

    Particulier* u0 = r.getParticulier("User0");
    EXPECT(u0 != nullptr);
    u0->setAttributionPointsActive(false);
    int avant = u0->getPoints();
    admin.attribuerPoints({"User0"});
    EXPECT(u0->getPoints() == avant);
    return 0;
}

static int test_attribution_ids_inconnus() {
    cout << "[TEST] Attribution - IDs inconnus / non-Particulier : silencieusement ignores\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AdministrationService admin(r, perf);
    // Ne doit ni crasher, ni affecter d'autres particuliers.
    Particulier* u0 = r.getParticulier("User0");
    Particulier* u1 = r.getParticulier("User1");
    int pts0Avant = u0->getPoints();
    int pts1Avant = u1->getPoints();
    admin.attribuerPoints({"UserXXX", "agence", "Provider0", ""});
    EXPECT(u0->getPoints() == pts0Avant);
    EXPECT(u1->getPoints() == pts1Avant);
    return 0;
}

// ============================================================================
// SECTION 9 : Scenario 5 - EnvironmentalService::mesurerImpactPurificateur.
// ============================================================================

static int test_impact_purificateur_inconnu() {
    cout << "[TEST] Scenario 5 - purificateur inconnu => ImpactPurificateur::echec()\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    AdministrationService admin(r, perf);
    air.setAdministrationService(&admin);
    EnvironmentalService env(r, perf, air);

    ImpactPurificateur impact = env.mesurerImpactPurificateur("CleanerXXX");
    EXPECT(impact.estSucces() == false);
    return 0;
}

static int test_impact_purificateur_existant() {
    cout << "[TEST] Scenario 5 - Cleaner0 reel : appel ne plante pas + resultat coherent\n";
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    AdministrationService admin(r, perf);
    air.setAdministrationService(&admin);
    EnvironmentalService env(r, perf, air);

    // Selon la nature aleatoire des donnees, l'amelioration peut etre detectee
    // ou non : on verifie surtout la coherence interne du resultat.
    ImpactPurificateur impact = env.mesurerImpactPurificateur("Cleaner0");
    if (impact.estSucces()) {
        EXPECT(impact.getRayonAction() > 0.0);
    } else {
        EXPECT_NEAR(impact.getRayonAction(), 0.0, 1e-9);
    }
    return 0;
}

// ============================================================================
// SECTION 10 : PerformanceMonitor.
// ============================================================================

static int test_performanceMonitor() {
    cout << "[TEST] PerformanceMonitor::demarrer/arreter - duree non negative et memorisee\n";
    PerformanceMonitor perf;
    perf.demarrer("test_op");
    long duree = perf.arreter("test_op");
    EXPECT(duree >= 0);
    EXPECT(perf.getDerniereDuree("test_op") == duree);
    return 0;
}

// ============================================================================
// main : execute tous les tests, affiche le bilan global.
// ============================================================================

int main() {
    cout << "============================================================\n";
    cout << " AirWatcher - Suite de tests (scenarios 1-6 + cas extremes)\n";
    cout << "============================================================\n";

    int fails = 0;

    // Section 0 : utilitaires.
    fails += test_dateTime_parse();
    fails += test_dateTime_compare();
    fails += test_distanceKm_basique();
    fails += test_distanceKm_meme_point();
    fails += test_distanceKm_un_degre();

    // Section 1 : DataReader.
    fails += test_chargement();
    fails += test_getCapteur_connu_inconnu();
    fails += test_getParticulier_connu_inconnu();
    fails += test_proprietaire_lie();
    fails += test_capteurs_dans_rayon();
    fails += test_capteurs_valides_filtre_non_fiable();
    fails += test_getMesuresPeriode();
    fails += test_getMesuresPeriode_hors_2019();
    fails += test_getMesuresPeriode_capteur_inconnu();

    // Section 2 : calculs ATMO directs.
    fails += test_atmo_vecteur_vide();
    fails += test_atmo_valeurs_basses();
    fails += test_atmo_max_grand_polluant();
    fails += test_atmo_ignore_invalides();
    fails += test_score_meme_capteur();

    // Section 3 : scenario 2.
    fails += test_calculerMoyenneAQI();
    fails += test_calculerMoyenneAQI_zone_vide();
    fails += test_calculerMoyenneAQI_periode_hors_donnees();
    fails += test_calculerMoyenneAQI_sans_admin();

    // Section 4 : scenario 3.
    fails += test_comparerCapteurs();
    fails += test_comparerCapteurs_ref_inconnue();

    // Section 5 : scenario 4 (interpolation).
    fails += test_interpolation_point_arbitraire();
    fails += test_interpolation_point_exact();
    fails += test_interpolation_loin_de_tout();

    // Section 6 : scenario 1.
    fails += test_analyserCapteur_introuvable();
    fails += test_analyserCapteur_normal();
    fails += test_analyserCapteur_defaillant_synthetique();

    // Section 7 : scenario 6.
    fails += test_classifierFiabilite_inconnu();
    fails += test_classifierFiabilite_normal();
    fails += test_classifierFiabilite_aberrant();

    // Section 8 : attribution de points.
    fails += test_attribution_via_calculerMoyenneAQI();
    fails += test_attribution_non_fiable_refusee();
    fails += test_attribution_desactivee();
    fails += test_attribution_ids_inconnus();

    // Section 9 : scenario 5.
    fails += test_impact_purificateur_inconnu();
    fails += test_impact_purificateur_existant();

    // Section 10 : monitoring.
    fails += test_performanceMonitor();

    cout << "------------------------------------------------------------\n";
    if (fails == 0) {
        cout << " Tous les tests sont OK (" << g_assertions << " assertions verifiees).\n";
        cout << "============================================================\n";
        return 0;
    }
    cerr << " " << fails << " test(s) en echec.\n";
    cerr << "============================================================\n";
    return 1;
}
