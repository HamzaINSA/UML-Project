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

namespace airwatcher {

ConsoleUI::ConsoleUI(DataReader& data,
                     AirQualityService& airQuality,
                     AdministrationService& admin,
                     EnvironmentalService& env)
    : data_(data), airQuality_(airQuality), admin_(admin), env_(env) {}

// ----- saisies utilitaires -----

std::string ConsoleUI::saisirLigne(const std::string& invite) {
    std::cout << invite;
    std::string s;
    std::getline(std::cin, s);
    return s;
}

int ConsoleUI::saisirEntier(const std::string& invite) {
    while (true) {
        std::string s = saisirLigne(invite);
        try { return std::stoi(s); } catch (...) {
            std::cout << "Entree invalide.\n";
        }
    }
}

double ConsoleUI::saisirDouble(const std::string& invite) {
    while (true) {
        std::string s = saisirLigne(invite);
        try { return std::stod(s); } catch (...) {
            std::cout << "Entree invalide.\n";
        }
    }
}

DateTime ConsoleUI::saisirDateTime(const std::string& invite) {
    while (true) {
        std::string s = saisirLigne(invite);
        try { return DateTime::parse(s); } catch (...) {
            std::cout << "Format attendu : YYYY-MM-DD HH:MM:SS\n";
        }
    }
}

// ----- saisies métier -----

std::string ConsoleUI::saisirIdCapteur() {
    return saisirLigne("ID du capteur : ");
}

ParametresZone ConsoleUI::saisirParametres() {
    ParametresZone p;
    p.latitude  = saisirDouble("Latitude : ");
    p.longitude = saisirDouble("Longitude : ");
    p.rayon     = saisirDouble("Rayon (km) : ");
    p.debut     = saisirDateTime("Date de debut (YYYY-MM-DD HH:MM:SS) : ");
    p.fin       = saisirDateTime("Date de fin   (YYYY-MM-DD HH:MM:SS) : ");
    return p;
}


ParametresComparaison ConsoleUI::choisirCapteurRef() {
    std::cout << "Liste des capteurs disponibles :\n";
    for (const auto& c : data_.getCapteurs()) {
        std::cout << "  - " << c.getId()
                  << " (" << c.getLatitude() << ", " << c.getLongitude() << ")\n";
    }
    ParametresComparaison p;
    p.idCapteur = saisirLigne("ID du capteur de reference : ");
    p.debut     = saisirDateTime("Date de debut (YYYY-MM-DD HH:MM:SS) : ");
    p.fin       = saisirDateTime("Date de fin   (YYYY-MM-DD HH:MM:SS) : ");
    return p;
}

std::string ConsoleUI::choisirPurificateur() {
    std::cout << "Liste des purificateurs :\n";
    for (const auto& p : data_.getPurificateurs()) {
        std::cout << "  - " << p.getId()
                  << " (" << p.getLatitude() << ", " << p.getLongitude() << ")"
                  << " du " << p.getDateDebut().toString()
                  << " au " << p.getDateFin().toString() << '\n';
    }
    return saisirLigne("ID du purificateur : ");
}

ParametresFiabilite ConsoleUI::choisirParticulierEtCapteur() {
    ParametresFiabilite p;
    // Affichage de la liste des particuliers
    std::cout << "Liste des particuliers :\n";
    for (const auto& u : data_.getUtilisateurs()) {
        if (u->getRole() != RoleUtilisateur::PARTICULIER) continue;
        const Particulier* part = static_cast<const Particulier*>(u.get());
        std::cout << "  - " << part->getId() << " (" << part->getNom() << ")\n";
    }
    p.idParticulier = saisirLigne("ID utilisateur (particulier) : ");
    Particulier* part = data_.getParticulier(p.idParticulier);
    if (!part) {
        std::cout << "Particulier non trouve.\n";
        return choisirParticulierEtCapteur();
    }
    // Affichage de la liste des capteurs du particulier choisi
    std::cout << "Capteurs de " << part->getNom() << " :\n";
    for (const auto& c : data_.getCapteurs()) {
        if (c.getProprietaireId() == part->getId()) {
            std::cout << "  - " << c.getId() << " (" << c.getLatitude() << ", " << c.getLongitude() << ")\n";
        }
    }
    p.idCapteur = saisirLigne("ID du capteur a evaluer : ");
    return p;
}

void ConsoleUI::afficherResultat(const std::string& resultat) {
    std::cout << "Resultat : " << resultat << '\n';
}

void ConsoleUI::afficherAlerte(const std::string& message) {
    std::cout << "[ALERTE] " << message << '\n';
}
// ----- authentification -----

void ConsoleUI::run() {
    while (true) {
        afficherMenuPrincipal();
        if (!authentification()) return;
        switch (utilisateurConnecte_->getRole()) {
            case RoleUtilisateur::AGENCE:      menuAgence();       break;
            case RoleUtilisateur::FOURNISSEUR: menuFournisseur();  break;
            case RoleUtilisateur::PARTICULIER: menuParticulier();  break;
        }
        utilisateurConnecte_.reset();
    }
}

void ConsoleUI::afficherMenuPrincipal() {
    std::cout << "\n";
    std::cout << "  1 -> Se connecter\n";
    std::cout << "  2 -> Creer un compte\n";
    std::cout << "  0 -> Quitter\n";
}

bool ConsoleUI::authentification() {
    while (true) {
        int choix = saisirEntier("Votre choix : ");
        if (choix == 0) return false;
        if (choix == 1) {
            if (seConnecter()) return true;
        } else if (choix == 2) {
            if (creerCompte()) return true;
        }
        afficherMenuPrincipal();
    }
}

bool ConsoleUI::seConnecter() {
    std::string id  = saisirLigne("Identifiant : ");
    std::string mdp = saisirLigne("Mot de passe : ");
    auto u = data_.getUtilisateur(id);
    if (!u || !u->verifierMdp(mdp)) {
        afficherAlerte("Identifiants invalides.");
        return false;
    }
    utilisateurConnecte_ = u;
    std::cout << "Bienvenue " << u->getNom() << " !\n";
    return true;
}

bool ConsoleUI::creerCompte() {
    std::cout << "Choisissez un role :\n"
              << "  1 -> Agence\n"
              << "  2 -> Particulier\n"
              << "  3 -> Fournisseur\n";
    int role = saisirEntier("Role : ");
    std::string id    = saisirLigne("Identifiant : ");
    if (data_.getUtilisateur(id)) {
        afficherAlerte("Cet identifiant est deja utilise.");
        return false;
    }
    std::string mdp   = saisirLigne("Mot de passe : ");
    std::string nom   = saisirLigne("Nom : ");
    std::string email = saisirLigne("Email : ");
    std::shared_ptr<Utilisateur> u;
    switch (role) {
        case 1: u = std::make_shared<Agence>(id, mdp, nom, email);       break;
        case 2: u = std::make_shared<Particulier>(id, mdp, nom, email);  break;
        case 3: u = std::make_shared<Fournisseur>(id, mdp, nom, email);  break;
        default: afficherAlerte("Role invalide."); return false;
    }
    data_.addUtilisateur(u);
    utilisateurConnecte_ = u;
    std::cout << "Compte cree. Bienvenue " << nom << " !\n";
    return true;
}

// ----- menus par role -----

void ConsoleUI::menuAgence() {
    while (true) {
        std::cout << "\n";
        std::cout << "  1 -> Statistiques\n";
        std::cout << "  2 -> Analyser un capteur\n";
        std::cout << "  3 -> Comparer des capteurs\n";
        std::cout << "  4 -> Qualite de l'air dans une zone donnée\n";
        std::cout << "  5 -> Impact des purificateurs\n";
        std::cout << "  6 -> Fiabilite d'un utilisateur\n";
        std::cout << "  7 -> Quitter\n";
        int c = saisirEntier("Votre choix : ");
        switch (c) {
            case 1: statistiques();         break;
            case 2: analyserCapteur();      break;
            case 3: comparerCapteurs();     break;
            case 4: qualiteAirPosition();   break;
            case 5: impactPurificateurs();  break;
            case 6: fiabiliteUtilisateur(); break;
            case 7: return;
            default: afficherAlerte("Choix invalide.");
        }
    }
}

void ConsoleUI::menuUtilisateurLimite() {
    while (true) {
        std::cout << "\n";
        std::cout << "  1 -> Statistiques\n";
        std::cout << "  2 -> Comparer des capteurs\n";
        std::cout << "  3 -> Impact des purificateurs\n";
        std::cout << "  4 -> Quitter\n";
        int c = saisirEntier("Votre choix : ");
        switch (c) {
            case 1: statistiques();        break;
            case 2: comparerCapteurs();    break;
            case 3: impactPurificateurs(); break;
            case 4: return;
            default: afficherAlerte("Choix invalide.");
        }
    }
}

void ConsoleUI::menuFournisseur() { menuUtilisateurLimite(); }
void ConsoleUI::menuParticulier() { menuUtilisateurLimite(); }

// ----- cas d'utilisation -----

void ConsoleUI::analyserCapteur() {
    //affichage de la liste des capteurs pour aider à choisir un ID valide
    std::cout << "Liste des capteurs disponibles :\n";
    for (const auto& c : data_.getCapteurs()) {
        std::cout << "  - " << c.getId()
                  << " (" << c.getLatitude() << ", " << c.getLongitude() << ")\n";
    }
    
    std::string id = saisirIdCapteur();
    Rapport r = admin_.analyserCapteur(id);
    std::cout << "Etat : " << Rapport::toString(r.getEtat()) << '\n';
    if (r.getEtat() == EtatCapteur::DEFAILLANT) {
        afficherAlerte("Capteur defaillant");
        for (const auto& a : r.getAnomalies()) std::cout << "  - " << a << '\n';
    } else if (r.getEtat() == EtatCapteur::FONCTIONNEL) {
        std::cout << "Capteur fonctionnel\n";
    } else {
        afficherAlerte("Capteur introuvable");
    }
}

void ConsoleUI::comparerCapteurs() {
    auto p = choisirCapteurRef();
    auto classement = airQuality_.comparerCapteurs(p.idCapteur, p.debut, p.fin);
    if (classement.empty()) {
        afficherAlerte("Aucun capteur comparable.");
        return;
    }
    std::cout << "Capteurs classes par similarite (score croissant) :\n";
    int rang = 1;
    for (const auto& cs : classement) {
        std::cout << "  " << rang++ << ". " << cs.capteur->getId()
                  << " | score = " << cs.score << '\n';
    }
}

void ConsoleUI::qualiteAirPosition() {
    auto p = saisirParametres();
    double indice = airQuality_.estimerQualiteZone(p.latitude, p.longitude, p.rayon, p.debut, p.fin);
    std::ostringstream os;
    os << "Indice ATMO moyen = " << indice;
    afficherResultat(os.str());
}

void ConsoleUI::impactPurificateurs() {
    std::string id = choisirPurificateur();
    if (!data_.getPurificateur(id)) {
        afficherAlerte("Purificateur introuvable.");
        return;
    }
    auto impact = env_.mesurerImpactPurificateur(id);
    if (!impact.estSucces()) {
        afficherAlerte("Aucun impact mesuré pour ce purificateur car il n'y pas de capteur dans le rayon d'action : 100 km");
        return;
    }
    std::ostringstream os;
    os << "Delta indice ATMO avant et pendant utilisation du purificateur = " << impact.getDelta()
       << ", rayon d'action = " << impact.getRayonAction() << " km";
    afficherResultat(os.str());
}

void ConsoleUI::fiabiliteUtilisateur() {
    std::cout << "  1 -> Calculer fiabilite pour un capteur d'un utilisateur\n";
    std::cout << "  2 -> Consulter fiabilite d'un utilisateur suivant les derniers calculs qui ont ete effectues\n";
    int c = saisirEntier("Votre choix : ");
    if (c == 1) {
        auto p = choisirParticulierEtCapteur();
        if (!data_.getParticulier(p.idParticulier)) {
            afficherAlerte("Utilisateur introuvable.");
            return;
        }
        Capteur* cap = data_.getCapteur(p.idCapteur);
        if (!cap) {
            afficherAlerte("Capteur introuvable.");
            return;
        }
        if (cap->getProprietaireId() != p.idParticulier) {
            afficherAlerte("Ce capteur n'appartient pas a cet utilisateur.");
            return;
        }
        bool fiable = admin_.classifierFiabilite(p.idCapteur);
        if (fiable) {
            afficherResultat("Capteur fiable - donnees conservees");
        } else {
            afficherAlerte("Capteur non fiable - donnees exclues, points suspendus");
        }
    } else if (c == 2) {
        // afficher liste des particuliers pour aider à choisir un ID valide
        std::cout << "Liste des particuliers :\n";
        for (const auto& u : data_.getUtilisateurs()) {
            if (u->getRole() != RoleUtilisateur::PARTICULIER) continue;
            const Particulier* part = static_cast<const Particulier*>(u.get());
            std::cout << "  - " << part->getId() << " (" <<
                part->getNom() << ", points=" << part->getPoints() << ")\n";
        }
        std::string id = saisirLigne("ID utilisateur : ");
        Particulier* p = data_.getParticulier(id);
        if (!p) { afficherAlerte("Utilisateur introuvable."); return; }
        if (p->estFiable()) {
            afficherResultat("Cet utilisateur est fiable.");
        } else {
            afficherResultat("Cet utilisateur n'est pas fiable car il a au moins un capteur non fiable, il est retire de l'application.");
        }
    } else {
        afficherAlerte("Choix invalide.");
    }
}

void ConsoleUI::statistiques() {
    std::cout << "Statistiques disponibles :\n"
              << "  1 -> Qualite de l'air sur une zone (moyenne ATMO)\n"
              << "  2 -> Nombre de capteurs / mesures charges\n";
    int c = saisirEntier("Votre choix : ");
    if (c == 1) {
        auto p = saisirParametres();
        double indice = airQuality_.calculerMoyenneAQI(p.latitude, p.longitude, p.rayon, p.debut, p.fin);
        std::ostringstream os;
        os << "Indice ATMO moyen = " << indice;
        afficherResultat(os.str());
    } else if (c == 2) {
        std::cout << "Capteurs charges : " << data_.getCapteurs().size() << '\n';
        std::cout << "Mesures chargees : " << data_.getMesures().size() << '\n';
        std::cout << "Purificateurs    : " << data_.getPurificateurs().size() << '\n';
    } else {
        afficherAlerte("Choix invalide.");
    }
}

} // namespace airwatcher
