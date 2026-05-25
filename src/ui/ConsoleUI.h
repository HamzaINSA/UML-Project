#ifndef AIRWATCHER_CONSOLE_UI_H
#define AIRWATCHER_CONSOLE_UI_H

#include <memory>
#include <string>

#include "../model/Parametres.h"
#include "../model/Utilisateur.h"

using namespace std;

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
    string saisirIdCapteur();
    ParametresZone        saisirParametres();
    ParametresPosition    saisirParametresPosition();
    ParametresComparaison choisirCapteurRef();
    string           choisirPurificateur();
    ParametresFiabilite   choisirParticulierEtCapteur();

    void afficherResultat(const string& resultat);
    void afficherAlerte(const string& message);
    // Cas d'utilisation déclenchés depuis les menus
    void analyserCapteur();
    void comparerCapteurs();
    void qualiteAirPosition();
    void qualiteAirZone();
    void impactPurificateurs();
    void fiabiliteUtilisateur();
    void statistiques();

private:
    DataReader&            data_;
    AirQualityService&     airQuality_;
    AdministrationService& admin_;
    EnvironmentalService&  env_;
    shared_ptr<Utilisateur> utilisateurConnecte_;

    void menuUtilisateurLimite();

    bool authentification();
    bool seConnecter();
    bool creerCompte();

    DateTime    saisirDateTime(const string& invite);
    double      saisirDouble(const string& invite);
    int         saisirEntier(const string& invite);
    string saisirLigne(const string& invite);
};

} // namespace airwatcher

#endif
