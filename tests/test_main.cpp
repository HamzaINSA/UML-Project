// Tests unitaires et d'integration legers (assertions simples).
#include <cassert>
#include <cmath>
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

#define EXPECT(cond)                                                  \
    do {                                                              \
        if (!(cond)) {                                                \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__       \
                      << " : " #cond << '\n';                         \
            return 1;                                                 \
        }                                                             \
    } while (0)

static int test_distanceKm() {
    double d = DataReader::distanceKm(45.7640, 4.8357, 45.7700, 4.8400);
    // ~0.7 km
    EXPECT(d > 0.5 && d < 1.0);
    return 0;
}

static int test_chargement() {
    DataReader r("data");
    r.loadDataFromCSV();
    EXPECT(r.getCapteurs().size() == 8);
    EXPECT(r.getMesures().size() > 100);
    EXPECT(r.getPurificateurs().size() == 3);
    return 0;
}

static int test_calculerMoyenneAQI() {
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    AdministrationService admin(r, perf);
    air.setAdministrationService(&admin);

    DateTime debut = DateTime::parse("2024-05-01 00:00:00");
    DateTime fin   = DateTime::parse("2024-09-30 00:00:00");
    double indice = air.calculerMoyenneAQI(45.7700, 4.8400, 5.0, debut, fin);
    EXPECT(indice > 0.0);
    EXPECT(indice <= 10.0);
    return 0;
}

static int test_attributionPoints() {
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    AdministrationService admin(r, perf);
    air.setAdministrationService(&admin);

    Particulier* u1 = r.getParticulier("U001");
    EXPECT(u1 != nullptr);
    int avant = u1->getPoints();

    DateTime debut = DateTime::parse("2024-05-01 00:00:00");
    DateTime fin   = DateTime::parse("2024-09-30 00:00:00");
    air.calculerMoyenneAQI(45.7650, 4.8360, 1.0, debut, fin);

    EXPECT(u1->getPoints() == avant + 1);
    return 0;
}

static int test_classifierFiabilite_aberrant() {
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AdministrationService admin(r, perf);

    // S007 a des valeurs tres aberrantes -> doit etre marque non fiable.
    bool fiable = admin.classifierFiabilite("S007");
    EXPECT(fiable == false);
    EXPECT(r.getCapteur("S007")->estFiable() == false);
    Particulier* u2 = r.getParticulier("U002");
    EXPECT(u2 != nullptr);
    EXPECT(u2->estFiable() == false);
    EXPECT(u2->attributionPointsActive() == false);
    return 0;
}

static int test_comparerCapteurs() {
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);

    DateTime debut = DateTime::parse("2024-05-01 00:00:00");
    DateTime fin   = DateTime::parse("2024-09-30 00:00:00");
    auto classement = air.comparerCapteurs("S001", debut, fin);
    EXPECT(!classement.empty());
    // Score croissant
    for (size_t i = 1; i < classement.size(); ++i) {
        EXPECT(classement[i-1].score <= classement[i].score);
    }
    return 0;
}

static int test_estimerQualitePosition() {
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    AdministrationService admin(r, perf);
    air.setAdministrationService(&admin);

    // Position interpolée
    double indice = air.estimerQualitePosition(45.78, 4.85,
                        DateTime::parse("2024-06-15 12:00:00"));
    EXPECT(indice > 0.0);
    return 0;
}

static int test_impactPurificateur() {
    DataReader r("data");
    r.loadDataFromCSV();
    PerformanceMonitor perf;
    AirQualityService air(r, perf);
    AdministrationService admin(r, perf);
    air.setAdministrationService(&admin);
    EnvironmentalService env(r, perf, air);

    auto impact = env.mesurerImpactPurificateur("C001");
    EXPECT(impact.estSucces());
    return 0;
}

int main() {
    int fails = 0;
    fails += test_distanceKm();
    fails += test_chargement();
    fails += test_calculerMoyenneAQI();
    fails += test_attributionPoints();
    fails += test_classifierFiabilite_aberrant();
    fails += test_comparerCapteurs();
    fails += test_estimerQualitePosition();
    fails += test_impactPurificateur();

    if (fails == 0) {
        std::cout << "Tous les tests sont OK.\n";
        return 0;
    }
    std::cerr << fails << " test(s) en echec.\n";
    return 1;
}
