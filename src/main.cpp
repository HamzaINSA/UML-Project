#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "repository/DataReader.h"
#include "service/AdministrationService.h"
#include "service/AirQualityService.h"
#include "service/EnvironmentalService.h"
#include "service/PerformanceMonitor.h"
#include "ui/ConsoleUI.h"

int main(int argc, char** argv) {
    using namespace airwatcher;

    std::string dataDir = "data";
    if (argc > 1) dataDir = argv[1];

    DataReader data(dataDir);
    try {
        data.loadDataFromCSV();
    } catch (const std::exception& e) {
        std::cerr << "Erreur de chargement : " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    PerformanceMonitor    perf;
    AirQualityService     airQuality(data, perf);
    AdministrationService admin(data, perf);
    EnvironmentalService  env(data, perf, airQuality);
    airQuality.setAdministrationService(&admin);

    std::cout << "=== AirWatcher ===\n";
    std::cout << "Donnees chargees depuis : " << dataDir << '\n';

    ConsoleUI ui(data, airQuality, admin, env);
    ui.run();

    std::cout << "Au revoir.\n";
    return EXIT_SUCCESS;
}
