#include "AdministrationService.h"

#include <array>
#include <cmath>

#include "../model/Mesure.h"
#include "../model/Particulier.h"
#include "../repository/DataReader.h"
#include "PerformanceMonitor.h"


using namespace std;

namespace airwatcher {

namespace {
    // Liste des quatre attributs de qualité de l'air surveillés.
    const array<string, 4> kAttributs = {"O3", "SO2", "NO2", "PM10"};

    // Rayon de voisinage en kilomètres pour la comparaison entre capteurs.
    const double kRayonVoisinage = 10.0;

    // Facteur multiplicateur de l'écart-type pour détecter une anomalie de voisinage.
    const double kK = 2.0;

    // Nombre maximal d'anomalies tolérées avant de déclarer un capteur non fiable.
    const int kSeuilTolerance = 2;

    // Seuil en dessous duquel l'écart-type est considéré anormalement faible (variance constante).
    const double kVarianceMin = 1e-3;

    // Retourne le seuil d'écart absolu acceptable entre un capteur et ses voisins
    // pour un attribut donné. Au-delà de ce seuil, une anomalie est signalée.
    double seuilEcart(const string& attribut) {
        if (attribut == "O3")   return 40.0;
        if (attribut == "SO2")  return 30.0;
        if (attribut == "NO2")  return 30.0;
        if (attribut == "PM10") return 20.0;
        // Valeur par défaut pour tout attribut inconnu.
        return 50.0;
    }
}  // namespace anonyme

// ============================================================
// Constructeur
// ============================================================

// Initialise le service avec le dépôt de données et le moniteur de performance.
AdministrationService::AdministrationService(DataReader& data, PerformanceMonitor& perf)
    : data_(data), perf_(perf) {}

// ============================================================
// Fonctions statistiques internes
// ============================================================

// Calcule la moyenne des valeurs valides pour un attribut donné
// parmi un ensemble de mesures. Retourne 0 si aucune mesure valide.
double AdministrationService::moyenne(const vector<Mesure>& mesures, const string& attribut) {
    double somme = 0.0;
    int nombre   = 0;

    for (const Mesure& mesure : mesures) {
        if (mesure.getAttributId() == attribut && mesure.estValide()) {
            somme  = somme + mesure.getValeur();
            nombre = nombre + 1;
        }
    }

    return nombre == 0 ? 0.0 : somme / nombre;
}

// Calcule l'écart-type des valeurs valides pour un attribut donné
// parmi un ensemble de mesures. Retourne 0 si aucune mesure valide.
double AdministrationService::ecartType(const vector<Mesure>& mesures, const string& attribut) {
    double moyenneAttribut = moyenne(mesures, attribut);

    // Somme des carrés des écarts à la moyenne pour les mesures valides de cet attribut.
    double sommeCarres = 0.0;
    // Nombre de mesures valides pour cet attribut.
    int nombre = 0;

    for (const Mesure& mesure : mesures) {
        if (mesure.getAttributId() == attribut && mesure.estValide()) {
            double ecart  = mesure.getValeur() - moyenneAttribut;
            sommeCarres   = sommeCarres + ecart * ecart;
            nombre        = nombre + 1;
        }
    }

    // On retourne la racine carrée de la moyenne des carrés des écarts (écart-type).
    return nombre == 0 ? 0.0 : sqrt(sommeCarres / nombre);
}

// Détecte si les mesures d'un attribut sont anormalement constantes,
// c'est-à-dire si l'écart-type est inférieur au seuil de variance minimale.
// Retourne false si le nombre de mesures valides est insuffisant (moins de 3).
bool AdministrationService::varianceConstante(const vector<Mesure>& mesures, const string& attribut) {
    int nombre = 0;

    for (const Mesure& mesure : mesures) {
        if (mesure.getAttributId() == attribut && mesure.estValide()) {
            nombre = nombre + 1;
        }
    }

    // On ne peut pas conclure à une variance constante avec moins de 3 mesures.
    if (nombre < 3) {
        return false;
    }

    // Si l'écart-type est inférieur au seuil, les valeurs sont suspectes (constantes).
    return ecartType(mesures, attribut) < kVarianceMin;
}

// Vérifie si une valeur est hors des bornes physiques acceptables
// pour un attribut donné. Retourne true si la valeur est aberrante.
bool AdministrationService::horsBornes(const string& attribut, double valeur) {
    // Une valeur négative est toujours hors bornes pour une concentration.
    if (valeur < 0) {
        return true;
    }

    if (attribut == "O3")   return valeur > 600.0;
    if (attribut == "SO2")  return valeur > 1000.0;
    if (attribut == "NO2")  return valeur > 800.0;
    if (attribut == "PM10") return valeur > 500.0;

    // Pour tout attribut inconnu, on ne signale pas d'anomalie de borne.
    return false;
}

// ============================================================
// Analyse d'un capteur
// ============================================================

// Analyse un capteur identifié et retourne un rapport indiquant
// son état (FONCTIONNEL, DEFAILLANT ou INTROUVABLE) avec la liste
// des anomalies détectées le cas échéant.
Rapport AdministrationService::analyserCapteur(const string& idCapteur) {
    perf_.demarrer("analyserCapteur");

    // récupérer le capteur à analyser
    Capteur* capteur = data_.getCapteur(idCapteur);

    // si échec 
    if (capteur == nullptr) {
        perf_.arreter("analyserCapteur");
        return Rapport(EtatCapteur::INTROUVABLE);
    }

    // récupérer les mesures du capteur et de ses voisins
    vector<Mesure> mesures     = data_.getMesuresCapteur(idCapteur);
    vector<Mesure> mesuresVois = data_.getMesuresCapteursVoisins(idCapteur);
    bool aVoisins              = !mesuresVois.empty();

    // liste des anomalies détectées pour ce capteur
    vector<string> anomalies;

    // pour chaque attributs (O3, SO2, NO2, PM10)
    for (const string& attribut : kAttributs) {
        // calcul de la moyenne de l'attribut pour le capteur analysé
        double moyenneCapteur = moyenne(mesures, attribut);

        // verifier que la moyenne n'est pas hors bornes physiques acceptables pour cet attribut
        if (horsBornes(attribut, moyenneCapteur)) {
            // si hors bornes, on ajoute une anomalie à la liste
            anomalies.push_back("Valeur hors bornes : " + attribut);
        }

        // si des capteurs voisins existent, on compare les moyennes.
        if (aVoisins) {
            // calcul de l'ecarit absolu entre la moyenne du capteur et la moyenne de ses voisins pour cet attribut
            double ecart = fabs(moyenneCapteur - moyenne(mesuresVois, attribut));

            // un écart trop important par rapport aux voisins est suspect
            if (ecart > seuilEcart(attribut)) {
                // si écart anormal, on ajoute une anomalie à la liste
                anomalies.push_back("Ecart anormal vs voisins : " + attribut);
            }
        }

        // nn vérifie que les valeurs ne sont pas anormalement constantes.
        if (varianceConstante(mesures, attribut)) {
            // si variance anormalement faible, on ajoute une anomalie à la liste
            // car le capteur est bloqué sur une valeur constante, ce qui est suspect pour un capteur de qualité de l'air.
            anomalies.push_back("Valeurs constantes suspectes : " + attribut);
        }
    }

    // construire le rapport final : si aucune anomalie, le capteur est fonctionnel, sinon il est défaillant avec la liste des anomalies.
    Rapport rapport;

    if (anomalies.empty()) {
        rapport = Rapport(EtatCapteur::FONCTIONNEL);
    } else {
        rapport = Rapport(EtatCapteur::DEFAILLANT, move(anomalies)); // move() permet de transférer la liste des anomalies sans copie inutile
    }

    perf_.arreter("analyserCapteur");
    return rapport;
}

// ============================================================
// Classification de la fiabilité d'un capteur
// ============================================================

// Évalue la fiabilité d'un capteur en comptant ses anomalies
// et met à jour l'état du capteur et de son propriétaire en conséquence.
// Retourne true si le capteur est fiable, false sinon.
bool AdministrationService::classifierFiabilite(const string& idCapteur) {
    perf_.demarrer("classifierFiabilite");

    Capteur* capteur = data_.getCapteur(idCapteur);

    if (capteur == nullptr) {
        perf_.arreter("classifierFiabilite");
        return false;
    }

    // On récupère le particulier propriétaire du capteur s'il existe.
    Particulier* particulier = nullptr;
    if (capteur->aProprietairePrive()) {
        particulier = data_.getParticulier(capteur->getProprietaireId());
    }

    vector<Mesure> mesuresPrive   = data_.getMesuresCapteur(idCapteur);
    vector<Mesure> mesuresVoisins = data_.getMesuresCapteursVoisins(idCapteur);

    int nbAnomalies = 0;

    // On compte les anomalies pour chacun des quatre attributs surveillés.
    for (const string& attribut : kAttributs) {
        double moyennePrive     = moyenne(mesuresPrive, attribut);
        double moyenneVoisinage = moyenne(mesuresVoisins, attribut);
        double ecartTypeVoisins = ecartType(mesuresVoisins, attribut);

        // Anomalie si la moyenne du capteur s'éloigne trop de celle de ses voisins.
        if (ecartTypeVoisins > 0.0
            && fabs(moyennePrive - moyenneVoisinage) > kK * ecartTypeVoisins)
        {
            nbAnomalies = nbAnomalies + 1;
        }

        // Anomalie si les valeurs du capteur sont anormalement constantes.
        if (varianceConstante(mesuresPrive, attribut)) {
            nbAnomalies = nbAnomalies + 1;
        }

        // Anomalie si la moyenne est hors des bornes physiques acceptables.
        if (horsBornes(attribut, moyennePrive)) {
            nbAnomalies = nbAnomalies + 1;
        }
    }

    bool estFiable;

    // Si le nombre d'anomalies ne dépasse pas le seuil de tolérance,
    // le capteur est considéré fiable.
    if (nbAnomalies <= kSeuilTolerance) {
        capteur->setEstFiable(true);

        if (particulier != nullptr) {
            particulier->setEstFiable(true);
        }

        data_.marquerCapteur(idCapteur, true);
        data_.sauvegarderCapteur(*capteur);
        estFiable = true;

    } else {
        // Le capteur est non fiable : son propriétaire est également marqué non fiable.
        capteur->setEstFiable(false);

        if (particulier != nullptr) {
            particulier->setEstFiable(false);
        }

        data_.marquerCapteur(idCapteur, false);
        data_.marquerMesuresInvalides(idCapteur);
        data_.sauvegarderCapteur(*capteur);

        // On désactive l'attribution de points pour le propriétaire non fiable.
        if (particulier != nullptr) {
            desactiverAttributionPoints(particulier->getId());
        }

        estFiable = false;
    }

    perf_.arreter("classifierFiabilite");
    return estFiable;
}

// ============================================================
// Gestion des points des particuliers
// ============================================================

// Attribue un point à chaque particulier fiable et actif
// parmi la liste d'identifiants fournie.
void AdministrationService::attribuerPoints(const vector<string>& idsParticuliers) {
    for (const string& identifiant : idsParticuliers) {
        Particulier* particulier = data_.getParticulier(identifiant);

        if (particulier == nullptr) {
            continue;
        }

        // On n'attribue des points qu'aux particuliers fiables dont l'attribution est active.
        if (!particulier->estFiable()) {
            continue;
        }

        if (!particulier->attributionPointsActive()) {
            continue;
        }

        particulier->ajouterPoints(1);
    }
}

// Désactive l'attribution de points pour un particulier identifié,
// typiquement après qu'un de ses capteurs a été jugé non fiable.
void AdministrationService::desactiverAttributionPoints(const string& idParticulier) {
    Particulier* particulier = data_.getParticulier(idParticulier);

    if (particulier != nullptr) {
        particulier->setAttributionPointsActive(false);
    }
}

} // namespace airwatcher