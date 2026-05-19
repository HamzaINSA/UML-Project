#ifndef AIRWATCHER_CONSOLE_UI_H
#define AIRWATCHER_CONSOLE_UI_H

#include <memory>
#include <string>

#include "../model/Parametres.h"
#include "../model/Utilisateur.h"

namespace airwatcher {

class DataReader;
class PerformanceMonitor;
class AirQualityService;
class AdministrationService;
class EnvironmentalService;

class ConsoleUI {
public:
    ConsoleUI(DataReader& data,
              AirQualityService& airQuality,
              AdministrationService& admin,
              EnvironmentalService& env);

    void run();

    // Menus principaux
    void afficherMenuPrincipal();
    void menuAgence();
    void menuFournisseur();
    void menuParticulier();

    // Saisies (signatures du diagramme de classe)
    std::string saisirIdCapteur();
    ParametresZone        saisirParametres();
    ParametresComparaison choisirCapteurRef();
    std::string           choisirPurificateur();
    ParametresFiabilite   choisirParticulierEtCapteur();

    void afficherResultat(const std::string& resultat);
    void afficherAlerte(const std::string& message);
    // Cas d'utilisation déclenchés depuis les menus
    void analyserCapteur();
    void comparerCapteurs();
    void qualiteAirPosition();
    void impactPurificateurs();
    void fiabiliteUtilisateur();
    void statistiques();

private:
    DataReader&            data_;
    AirQualityService&     airQuality_;
    AdministrationService& admin_;
    EnvironmentalService&  env_;
    std::shared_ptr<Utilisateur> utilisateurConnecte_;

    void menuUtilisateurLimite();

    bool authentification();
    bool seConnecter();
    bool creerCompte();

    DateTime    saisirDateTime(const std::string& invite);
    double      saisirDouble(const std::string& invite);
    int         saisirEntier(const std::string& invite);
    std::string saisirLigne(const std::string& invite);
};

} // namespace airwatcher

#endif
