#include "ConsoleUI.h"

#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "../model/Agence.h"
#include "../model/Capteur.h"
#include "../model/Fournisseur.h"
#include "../model/Particulier.h"
#include "../repository/DataReader.h"
#include "../service/AdministrationService.h"
#include "../service/AirQualityService.h"
#include "../service/EnvironmentalService.h"


using namespace std;

namespace airwatcher {

// ============================================================
// Constructeur
// ============================================================

// Initialise l'interface console avec les références vers les services
// et le dépôt de données.
ConsoleUI::ConsoleUI(DataReader& data,
                     AirQualityService& airQuality,
                     AdministrationService& admin,
                     EnvironmentalService& env)
    : data_(data), airQuality_(airQuality), admin_(admin), env_(env) {} // data_(data) => initialisation de l'attribut data_ avec le paramètre data du constructeur

// ============================================================
// Saisies utilitaires
// ============================================================

// Affiche une invite et retourne la ligne saisie par l'utilisateur.
string ConsoleUI::saisirLigne(const string& invite) {
    cout << invite;
    string saisie;
    getline(cin, saisie);
    return saisie;
}

// Demande à l'utilisateur de saisir un entier.
// Redemande tant que la saisie est invalide.
int ConsoleUI::saisirEntier(const string& invite) {
    while (true) {
        string saisie = saisirLigne(invite);
        try {
            return stoi(saisie); // converti string en int
        } catch (...) {
            cout << "Entree invalide.\n";
        }
    }
}

// Demande à l'utilisateur de saisir un nombre décimal.
// Redemande tant que la saisie est invalide.
double ConsoleUI::saisirDouble(const string& invite) {
    while (true) {
        string saisie = saisirLigne(invite);
        try {
            return stod(saisie); // stod converit string en double,
        } catch (...) {
            cout << "Entree invalide.\n";
        }
    }
}

// Demande à l'utilisateur de saisir une date et heure au format ISO.
// Redemande tant que le format est incorrect.
DateTime ConsoleUI::saisirDateTime(const string& invite) {
    while (true) {
        string saisie = saisirLigne(invite);
        try {
            return DateTime::parse(saisie);
        } catch (...) {
            cout << "Format attendu : YYYY-MM-DD HH:MM:SS\n";
        }
    }
}

// ============================================================
// Saisies métier
// ============================================================

// Demande à l'utilisateur de saisir l'identifiant d'un capteur.
string ConsoleUI::saisirIdCapteur() {
    return saisirLigne("ID du capteur : ");
}

// Demande à l'utilisateur de saisir les paramètres d'une zone géographique
// avec une période temporelle.
ParametresZone ConsoleUI::saisirParametres() {
    ParametresZone parametres;
    parametres.latitude  = saisirDouble("Latitude : ");
    parametres.longitude = saisirDouble("Longitude : ");
    parametres.rayon     = saisirDouble("Rayon (km) : ");
    parametres.debut     = saisirDateTime("Date de debut (YYYY-MM-DD HH:MM:SS) : ");
    parametres.fin       = saisirDateTime("Date de fin   (YYYY-MM-DD HH:MM:SS) : ");
    return parametres;
}

// Demande à l'utilisateur de saisir les paramètres d'une position géographique
// avec une période temporelle.
ParametresPosition ConsoleUI::saisirParametresPosition() {
    ParametresPosition parametres;
    parametres.latitude  = saisirDouble("Latitude : ");
    parametres.longitude = saisirDouble("Longitude : ");
    parametres.debut     = saisirDateTime("Date de debut (YYYY-MM-DD HH:MM:SS) : ");
    parametres.fin       = saisirDateTime("Date de fin   (YYYY-MM-DD HH:MM:SS) : ");
    return parametres;
}

// Affiche la liste des capteurs disponibles et demande à l'utilisateur
// de choisir un capteur de référence avec une période temporelle.
ParametresComparaison ConsoleUI::choisirCapteurRef() {
    cout << "Liste des capteurs disponibles :\n";

    for (const Capteur& capteur : data_.getCapteurs()) {
        cout << "  - " << capteur.getId()
             << " (" << capteur.getLatitude() << ", " << capteur.getLongitude() << ")\n";
    }

    ParametresComparaison parametres;
    parametres.idCapteur = saisirLigne("ID du capteur de reference : ");
    parametres.debut     = saisirDateTime("Date de debut (YYYY-MM-DD HH:MM:SS) : ");
    parametres.fin       = saisirDateTime("Date de fin   (YYYY-MM-DD HH:MM:SS) : ");
    return parametres;
}

// Affiche la liste des purificateurs disponibles et demande à l'utilisateur
// d'en choisir un par son identifiant.
string ConsoleUI::choisirPurificateur() {
    cout << "Liste des purificateurs :\n";

    for (const Purificateur& purificateur : data_.getPurificateurs()) {
        cout << "  - " << purificateur.getId()
             << " (" << purificateur.getLatitude() << ", " << purificateur.getLongitude() << ")"
             << " du " << purificateur.getDateDebut().toString()
             << " au " << purificateur.getDateFin().toString() << '\n';
    }

    return saisirLigne("ID du purificateur : ");
}

// Affiche la liste des particuliers, demande à l'utilisateur d'en choisir un,
// puis affiche ses capteurs et demande d'en choisir un.
// Se rappelle récursivement si l'identifiant du particulier est invalide.
ParametresFiabilite ConsoleUI::choisirParticulierEtCapteur() {
    ParametresFiabilite parametres;

    // Affichage de la liste des particuliers.
    cout << "Liste des particuliers :\n";

    for (const shared_ptr<Utilisateur>& utilisateur : data_.getUtilisateurs()) {
        if (utilisateur->getRole() != RoleUtilisateur::PARTICULIER) {
            continue;
        }

        const Particulier* particulier = static_cast<const Particulier*>(utilisateur.get());
        cout << "  - " << particulier->getId() << " (" << particulier->getNom() << ")\n";
    }

    parametres.idParticulier = saisirLigne("ID utilisateur (particulier) : ");
    Particulier* particulier = data_.getParticulier(parametres.idParticulier);

    if (particulier == nullptr) {
        cout << "Particulier non trouve.\n";
        // On redemande à l'utilisateur de choisir un particulier valide.
        return choisirParticulierEtCapteur();
    }

    // Affichage des capteurs appartenant au particulier choisi.
    cout << "Capteurs de " << particulier->getNom() << " :\n";

    for (const Capteur& capteur : data_.getCapteurs()) {
        if (capteur.getProprietaireId() == particulier->getId()) {
            cout << "  - " << capteur.getId()
                 << " (" << capteur.getLatitude() << ", " << capteur.getLongitude() << ")\n";
        }
    }

    parametres.idCapteur = saisirLigne("ID du capteur a evaluer : ");
    return parametres;
}

// Affiche un résultat de traitement à l'écran.
void ConsoleUI::afficherResultat(const string& resultat) {
    cout << "Resultat : " << resultat << '\n';
}

// Affiche un message d'alerte à l'écran.
void ConsoleUI::afficherAlerte(const string& message) {
    cout << "[ALERTE] " << message << '\n';
}

// ============================================================
// Authentification et boucle principale
// ============================================================

// Boucle principale de l'application : affiche le menu d'accueil,
// gère l'authentification et redirige vers le menu du rôle connecté.
void ConsoleUI::run() {
    while (true) {
        afficherMenuPrincipal();

        if (!authentification()) {
            // L'utilisateur a choisi de quitter l'application.
            return;
        }

        switch (utilisateurConnecte_->getRole()) {
            case RoleUtilisateur::AGENCE:
                menuAgence();
                break;
            case RoleUtilisateur::FOURNISSEUR:
                menuFournisseur();
                break;
            case RoleUtilisateur::PARTICULIER:
                menuParticulier();
                break;
        }

        // On déconnecte l'utilisateur courant avant de revenir au menu principal.
        utilisateurConnecte_.reset();
    }
}

// Affiche le menu principal d'accueil (connexion, création de compte, quitter).
void ConsoleUI::afficherMenuPrincipal() {
    cout << "\n";
    cout << "  1 -> Se connecter\n";
    cout << "  2 -> Creer un compte\n";
    cout << "  0 -> Quitter\n";
}

// Gère la boucle d'authentification jusqu'à connexion réussie ou sortie.
// Retourne true si un utilisateur est connecté, false si l'utilisateur quitte.
bool ConsoleUI::authentification() {
    while (true) {
        int choix = saisirEntier("Votre choix : ");

        if (choix == 0) {
            return false;
        }

        if (choix == 1) {
            if (seConnecter()) {
                return true;
            }
        } else if (choix == 2) {
            if (creerCompte()) {
                return true;
            }
        }

        afficherMenuPrincipal();
    }
}

// Demande les identifiants, vérifie le mot de passe et connecte l'utilisateur.
// Retourne true en cas de succès, false sinon.
bool ConsoleUI::seConnecter() {
    string identifiant    = saisirLigne("Identifiant : ");
    string motDePasse     = saisirLigne("Mot de passe : ");
    shared_ptr<Utilisateur> utilisateur = data_.getUtilisateur(identifiant);

    if (utilisateur == nullptr || !utilisateur->verifierMdp(motDePasse)) {
        afficherAlerte("Identifiants invalides.");
        return false;
    }

    utilisateurConnecte_ = utilisateur;
    cout << "Bienvenue " << utilisateur->getNom() << " !\n";
    return true;
}

// Crée un nouveau compte utilisateur selon le rôle choisi.
// Retourne true si le compte a été créé et l'utilisateur connecté, false sinon.
bool ConsoleUI::creerCompte() {
    cout << "Choisissez un role :\n"
         << "  1 -> Agence\n"
         << "  2 -> Particulier\n"
         << "  3 -> Fournisseur\n";

    int role = saisirEntier("Role : ");

    string identifiant = saisirLigne("Identifiant : ");

    // On vérifie que l'identifiant n'est pas déjà utilisé.
    if (data_.getUtilisateur(identifiant) != nullptr) {
        afficherAlerte("Cet identifiant est deja utilise.");
        return false;
    }

    string motDePasse = saisirLigne("Mot de passe : ");
    string nom        = saisirLigne("Nom : ");
    string email      = saisirLigne("Email : ");

    shared_ptr<Utilisateur> nouvelUtilisateur;

    switch (role) {
        case 1:
            nouvelUtilisateur = make_shared<Agence>(identifiant, motDePasse, nom, email); // make_shared crée un objet de type Agence et retourne un shared_ptr qui le gère, on utilise le constructeur de la classe Agence pour initialiser les attributs avec les valeurs saisies
            break;
        case 2:
            nouvelUtilisateur = make_shared<Particulier>(identifiant, motDePasse, nom, email);
            break;
        case 3:
            nouvelUtilisateur = make_shared<Fournisseur>(identifiant, motDePasse, nom, email);
            break;
        default:
            afficherAlerte("Role invalide.");
            return false;
    }

    data_.addUtilisateur(nouvelUtilisateur);
    utilisateurConnecte_ = nouvelUtilisateur;
    cout << "Compte cree. Bienvenue " << nom << " !\n";
    return true;
}

// ============================================================
// Menus par rôle
// ============================================================

// Menu réservé aux utilisateurs de type Agence, avec accès complet aux fonctionnalités.
void ConsoleUI::menuAgence() {
    while (true) {
        cout << "\n";
        cout << "  1 -> Statistiques\n";
        cout << "  2 -> Analyser un capteur\n";
        cout << "  3 -> Comparer des capteurs\n";
        cout << "  4 -> Qualite de l'air à une position données\n";
        cout << "  5 -> Qualite de l'air dans une zone donnee\n";
        cout << "  6 -> Impact des purificateurs\n";
        cout << "  7 -> Fiabilite d'un utilisateur\n";
        cout << "  8 -> Quitter\n";

        int choix = saisirEntier("Votre choix : ");

        switch (choix) {
            case 1: statistiques();         break;
            case 2: analyserCapteur();      break;
            case 3: comparerCapteurs();     break;
            case 4: qualiteAirPosition();   break;
            case 5: qualiteAirZone();       break;
            case 6: impactPurificateurs();  break;
            case 7: fiabiliteUtilisateur(); break;
            case 8: return;
            default: afficherAlerte("Choix invalide.");
        }
    }
}

// Menu à fonctionnalités restreintes, partagé par les fournisseurs et les particuliers.
void ConsoleUI::menuUtilisateurLimite() {
    while (true) {
        cout << "\n";
        cout << "  1 -> Statistiques\n";
        cout << "  2 -> Comparer des capteurs\n";
        cout << "  3 -> Impact des purificateurs\n";
        cout << "  4 -> Quitter\n";

        int choix = saisirEntier("Votre choix : ");

        switch (choix) {
            case 1: statistiques();        break;
            case 2: comparerCapteurs();    break;
            case 3: impactPurificateurs(); break;
            case 4: return;
            default: afficherAlerte("Choix invalide.");
        }
    }
}

// Le menu fournisseur est identique au menu utilisateur limité.
void ConsoleUI::menuFournisseur() {
    menuUtilisateurLimite();
}

// Le menu particulier est identique au menu utilisateur limité.
void ConsoleUI::menuParticulier() {
    menuUtilisateurLimite();
}

// ============================================================
// Cas d'utilisation
// ============================================================

// Affiche la liste des capteurs, demande un identifiant et
// affiche le rapport d'analyse (état, anomalies éventuelles).
void ConsoleUI::analyserCapteur() {
    // Affichage de la liste des capteurs pour aider à choisir un identifiant valide.
    cout << "Liste des capteurs disponibles :\n";

    for (const Capteur& capteur : data_.getCapteurs()) {
        cout << "  - " << capteur.getId()
             << " (" << capteur.getLatitude() << ", " << capteur.getLongitude() << ")\n";
    }

    string identifiant = saisirIdCapteur();
    Rapport rapport    = admin_.analyserCapteur(identifiant);

    cout << "Etat : " << Rapport::toString(rapport.getEtat()) << '\n';

    if (rapport.getEtat() == EtatCapteur::DEFAILLANT) {
        afficherAlerte("Capteur defaillant");
        for (const string& anomalie : rapport.getAnomalies()) {
            cout << "  - " << anomalie << '\n';
        }
    } else if (rapport.getEtat() == EtatCapteur::FONCTIONNEL) {
        cout << "Capteur fonctionnel\n";
    } else {
        afficherAlerte("Capteur introuvable");
    }
}

// Affiche le classement des capteurs par similarité avec un capteur de référence
// sur une période donnée.
void ConsoleUI::comparerCapteurs() {
    ParametresComparaison parametres = choisirCapteurRef();
    vector<CapteurScore> classement  = airQuality_.comparerCapteurs(
        parametres.idCapteur, parametres.debut, parametres.fin);

    if (classement.empty()) {
        afficherAlerte("Aucun capteur comparable.");
        return;
    }

    cout << "Capteurs classes par similarite (score croissant) :\n";
    int rang = 1;

    for (const CapteurScore& entree : classement) {
        cout << "  " << rang << ". " << entree.capteur->getId()
             << " | score = " << entree.score << '\n';
        rang = rang + 1;
    }
}

// Estime la qualité de l'air à une position donnée en interpolant les mesures
// des capteurs proches et affiche l'indice ATMO moyen.
void ConsoleUI::qualiteAirPosition() {

    ParametresPosition parametres = saisirParametresPosition();

    // déterminer les capteurs proches du point demandé et leurs mesures associées
    vector<pair<Capteur*, vector<Mesure>>> capteursProches = airQuality_.trouverCapteursProchesPourInterpolation(parametres.latitude, parametres.longitude);

    // estimer l'indice ATMO au point demandé par interpolation pondérée par la distance aux capteurs proches
    double indice = airQuality_.interpolerPondereeParDistance(capteursProches,parametres.latitude, parametres.longitude);

    ostringstream flux;
    flux << "Indice ATMO moyen = " << indice;
    afficherResultat(flux.str()); // str convertit en string
}

// Estime la qualité de l'air pour une zone géographique et une période données,
// puis affiche l'indice ATMO moyen.
void ConsoleUI::qualiteAirZone() {
    ParametresZone parametres = saisirParametres();
    double indice = airQuality_.estimerQualiteZone(
        parametres.latitude, parametres.longitude,
        parametres.rayon, parametres.debut, parametres.fin);

    ostringstream flux;
    flux << "Indice ATMO moyen = " << indice;
    afficherResultat(flux.str());
}

// Mesure l'impact d'un purificateur sélectionné sur la qualité de l'air
// et affiche le delta d'indice ATMO.
void ConsoleUI::impactPurificateurs() {
    string identifiant = choisirPurificateur();

    if (data_.getPurificateur(identifiant) == nullptr) {
        afficherAlerte("Purificateur introuvable.");
        return;
    }

    auto impact = env_.mesurerImpactPurificateur(identifiant);

    if (!impact.estSucces()) {
        afficherAlerte(
            "Aucun impact mesure pour ce purificateur car il n'y a pas de capteur dans le rayon d'action : 100 km");
        return;
    }

    ostringstream flux;
    flux << "Delta indice ATMO avant et pendant utilisation du purificateur = " << impact.getDelta()
         << ", rayon d'action = " << impact.getRayonAction() << " km";
    afficherResultat(flux.str());
}

// Permet à l'agence de calculer ou de consulter la fiabilité d'un particulier
// et de son capteur.
void ConsoleUI::fiabiliteUtilisateur() {
    cout << "  1 -> Calculer fiabilite pour un capteur d'un utilisateur\n";
    cout << "  2 -> Consulter fiabilite d'un utilisateur suivant les derniers calculs effectues\n";

    int choix = saisirEntier("Votre choix : ");

    if (choix == 1) {
        // Calcul de la fiabilité d'un capteur appartenant à un particulier.
        ParametresFiabilite parametres = choisirParticulierEtCapteur();

        if (data_.getParticulier(parametres.idParticulier) == nullptr) {
            afficherAlerte("Utilisateur introuvable.");
            return;
        }

        Capteur* capteur = data_.getCapteur(parametres.idCapteur);

        if (capteur == nullptr) {
            afficherAlerte("Capteur introuvable.");
            return;
        }

        if (capteur->getProprietaireId() != parametres.idParticulier) {
            afficherAlerte("Ce capteur n'appartient pas a cet utilisateur.");
            return;
        }

        bool estFiable = admin_.classifierFiabilite(parametres.idCapteur);

        if (estFiable) {
            afficherResultat("Capteur fiable - donnees conservees");
        } else {
            afficherAlerte("Capteur non fiable - donnees exclues, points suspendus");
        }

    } else if (choix == 2) {
        // Consultation de la fiabilité d'un particulier selon les derniers calculs.
        cout << "Liste des particuliers :\n";

        for (const shared_ptr<Utilisateur>& utilisateur : data_.getUtilisateurs()) {
            if (utilisateur->getRole() != RoleUtilisateur::PARTICULIER) {
                continue;
            }

            const Particulier* particulier = static_cast<const Particulier*>(utilisateur.get());
            cout << "  - " << particulier->getId()
                 << " (" << particulier->getNom()
                 << ", points=" << particulier->getPoints() << ")\n";
        }

        string identifiant       = saisirLigne("ID utilisateur : ");
        Particulier* particulier = data_.getParticulier(identifiant);

        if (particulier == nullptr) {
            afficherAlerte("Utilisateur introuvable.");
            return;
        }

        if (particulier->estFiable()) {
            afficherResultat("Cet utilisateur est fiable.");
        } else {
            afficherResultat(
                "Cet utilisateur n'est pas fiable car il a au moins un capteur non fiable, "
                "il est retire de l'application.");
        }

    } else {
        afficherAlerte("Choix invalide.");
    }
}

// Affiche des statistiques générales : qualité de l'air sur une zone
// ou nombre d'entités chargées en mémoire.
void ConsoleUI::statistiques() {
    cout << "Statistiques disponibles :\n"
         << "  1 -> Qualite de l'air sur une zone (moyenne ATMO)\n"
         << "  2 -> Nombre de capteurs / mesures charges\n";

    int choix = saisirEntier("Votre choix : ");

    if (choix == 1) {
        ParametresZone parametres = saisirParametres();
        double indice = airQuality_.calculerMoyenneAQI(
            parametres.latitude, parametres.longitude,
            parametres.rayon, parametres.debut, parametres.fin);

        ostringstream flux;
        flux << "Indice ATMO moyen = " << indice;
        afficherResultat(flux.str());

    } else if (choix == 2) {
        // Affichage du nombre d'entités chargées depuis les fichiers CSV.
        cout << "Capteurs charges : " << data_.getCapteurs().size() << '\n';
        cout << "Mesures chargees : " << data_.getMesures().size() << '\n';
        cout << "Purificateurs    : " << data_.getPurificateurs().size() << '\n';

    } else {
        afficherAlerte("Choix invalide.");
    }
}

} // namespace airwatcher