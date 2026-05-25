#include "AirQualityService.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <set>

#include "../model/Particulier.h"
#include "../repository/DataReader.h"
#include "AdministrationService.h"
#include "PerformanceMonitor.h"

using namespace std;

namespace airwatcher {

namespace {
    // Nombre maximal de capteurs voisins utilisés pour l'interpolation (voir code ci-dessous).
    const int kNbVoisinsInterpolation = 5;

    // Liste des quatre attributs de qualité de l'air surveillés.
    const array<string, 4> kAttributs = {"O3", "SO2", "NO2", "PM10"};
}  // namespace anonyme

// ============================================================
// Constructeur
// ============================================================

// Initialise le service avec le dépôt de données et le moniteur de performance.
AirQualityService::AirQualityService(DataReader& data, PerformanceMonitor& perf)
    : data_(data), perf_(perf) {}

// ============================================================
// Fonctions statistiques internes
// ============================================================

// Calcule la moyenne des valeurs valides pour un attribut donné
// parmi un ensemble de mesures. Retourne 0 si aucune mesure valide.
double AirQualityService::moyenneAttribut(const vector<Mesure>& mesures,
                                          const string& attributId)
{
    double somme = 0.0;
    int nombre   = 0;

    for (const Mesure& mesure : mesures) {
        if (mesure.getAttributId() == attributId && mesure.estValide()) {
            somme  = somme + mesure.getValeur();
            nombre = nombre + 1;
        }
    }

    return nombre == 0 ? 0.0 : somme / nombre;
}

// ============================================================
// Calcul des indices ATMO
// ============================================================

// Retourne le sous-indice ATMO (de 1 à 10) correspondant à une valeur
// de concentration pour un attribut donné, selon les seuils français (µg/m³).
// Retourne 0 si l'attribut est inconnu.
int AirQualityService::sousIndiceATMO(const string& attribut, double valeur) {
    if (attribut == "O3") {
        // Seuils pour l'ozone (O3).
        const double seuils[10] = { 30, 55, 80, 105, 130, 150, 180, 210, 240,numeric_limits<double>::infinity()};
        for (int i = 0; i < 10; i = i + 1) {
            if (valeur <= seuils[i]) {
                return i + 1;
            }
        }
    } else if (attribut == "NO2") {
        // Seuils pour le dioxyde d'azote (NO2).
        const double seuils[10] = {0, 55, 85, 110, 135, 165, 200, 275, 400,numeric_limits<double>::infinity()};
        for (int i = 0; i < 10; i = i + 1) {
            if (valeur <= seuils[i]) {
                return i + 1;
            }
        }
    } else if (attribut == "SO2") {
        // Seuils pour le dioxyde de soufre (SO2).
        const double seuils[10] = {40, 80, 120, 160, 200, 250, 300, 400, 500,numeric_limits<double>::infinity()};
        for (int i = 0; i < 10; i = i + 1) {
            if (valeur <= seuils[i]) {
                return i + 1;
            }
        }
    } else if (attribut == "PM10") {
        // Seuils pour les particules fines (PM10).
        const double seuils[10] = {7, 14, 20, 27, 34, 41, 49, 64, 79,numeric_limits<double>::infinity()};
        for (int i = 0; i < 10; i = i + 1) {
            if (valeur <= seuils[i]) {
                return i + 1;
            }
        }
    }

    // Attribut inconnu : indice non défini.
    return 0;
}

// Calcule l'indice ATMO global à partir d'un ensemble de mesures.
// L'indice ATMO correspond au maximum des sous-indices de chaque attribut.
double AirQualityService::calculerIndiceATMO(const vector<Mesure>& mesures) const {
    int indiceMaximal = 0;

    for (const string& attribut : kAttributs) {
        double moyenneAttr = moyenneAttribut(mesures, attribut);

        // On ignore les attributs sans mesure valide.
        if (moyenneAttr <= 0.0) {
            continue;
        }

        int sousIndice = sousIndiceATMO(attribut, moyenneAttr);

        if (sousIndice > indiceMaximal) {
            indiceMaximal = sousIndice;
        }
    }

    return static_cast<double>(indiceMaximal);
}

// Calcule le score de similarité entre un capteur de référence et un autre capteur.
// Le score est la distance euclidienne entre les moyennes de chaque attribut.
// Un score faible indique une forte similarité.
double AirQualityService::calculerScoreSimilarite(const vector<Mesure>& mesuresRef,
                                                  const vector<Mesure>& mesures) const
{
    double sommeCarres = 0.0;

    for (const string& attribut : kAttributs) {
        double moyRef    = moyenneAttribut(mesuresRef, attribut);
        double moyCapteur = moyenneAttribut(mesures, attribut);
        double ecart      = moyRef - moyCapteur;
        sommeCarres       = sommeCarres + ecart * ecart;
    }

    return sqrt(sommeCarres);
}

// ============================================================
// Calcul de la qualité de l'air sur une zone
// ============================================================

// Calcule l'indice ATMO moyen pour une zone géographique et une période données.
// Attribue des points aux particuliers dont les capteurs ont contribué au calcul.
double AirQualityService::calculerMoyenneAQI(double lat, double lon, double rayon,const DateTime& debut, const DateTime& fin)
{
    perf_.demarrer("calculerMoyenneAQI");

    // on récupere les capteurs valides dans la zone définie par le centre (lat, lon) et le rayon.
    vector<Capteur*> capteursZone = data_.getCapteursValidesDansRayon(lat, lon, rayon);
    vector<Mesure> toutesMesures;
    set<string> particuliersImpliques;

    // pour chaque capteur de la zone
    for (Capteur* capteur : capteursZone) {
        // récuperer les mesures du capteur pour la période définie par debut et fin
        vector<Mesure> mesuresPeriode = data_.getMesuresPeriode(capteur->getId(), debut, fin);
        vector<Mesure> mesuresFiltrees;

        // parmit toutes les mesures du capteur, on ne garde que celles qui sont valides (certain vienne de capteurs défaillant ou avec propriétaire douteux)
        for (Mesure& mesure : mesuresPeriode) {
            if (mesure.estValide()) {mesuresFiltrees.push_back(mesure);}
        }

        // si aucune mesure valide n'est disponible pour ce capteur on l'ignore.
        if (mesuresFiltrees.empty()) {
            continue;
        }

        // on ajoute les mesures valides à l'ensemble global de la zone
        toutesMesures.insert(toutesMesures.end(), mesuresFiltrees.begin(),mesuresFiltrees.end());

        // on mémorise les particuliers propriétaires de capteurs ayant contribué
        if (capteur->aProprietairePrive()) { 
            particuliersImpliques.insert(capteur->getProprietaireId());
        }
    }

    // on calcule l'indice ATMO moyen pour la zone à partir de toutes les mesures valides collectées.
    double indiceATMO = calculerIndiceATMO(toutesMesures);

    // Si un service d'administration est disponible et des particuliers ont contribué,
    // on leur attribue des points de récompense.
    if (admin_ != nullptr && !particuliersImpliques.empty()) {
        // récupérer les identifiants des particuliers impliqués
        vector<string> idsParticuliers(particuliersImpliques.begin(), particuliersImpliques.end());
        // leurs attribuer des points
        admin_->attribuerPoints(idsParticuliers);
    }

    perf_.arreter("calculerMoyenneAQI");
    return indiceATMO;
}

// Calcule l'indice ATMO moyen pour une zone pendant une période donnée.
// Délègue directement à calculerMoyenneAQI.
double AirQualityService::calculerMoyenneZonePendant(double lat, double lon, double rayon,const DateTime& debut, const DateTime& fin)
{
    return calculerMoyenneAQI(lat, lon, rayon, debut, fin);
}

// Calcule l'indice ATMO moyen pour une zone depuis le début des données
// jusqu'à une date limite. Délègue à calculerMoyenneAQI avec DateTime::min() comme début.
double AirQualityService::calculerMoyenneZoneJusquaDateDebut(double lat, double lon, double rayon,const DateTime& dateLimite)
{
    return calculerMoyenneAQI(lat, lon, rayon, DateTime::min(), dateLimite);
}

// ============================================================
// Comparaison de capteurs
// ============================================================

// Compare tous les capteurs fiables au capteur de référence sur une période donnée
// et retourne un classement par score de similarité croissant.
vector<CapteurScore> AirQualityService::comparerCapteurs(const string& idRef,const DateTime& debut,const DateTime& fin)
{
    perf_.demarrer("comparerCapteurs");

    // récupère les mesures du capteur de référence pour la période donnée
    vector<Mesure> mesuresRef       = data_.getMesuresPeriode(idRef, debut, fin);
    // récupère les autres capteurs disponibles pour comparaison
    vector<Capteur*> autresCapteurs = data_.getAutresCapteurs(idRef);
    vector<CapteurScore> classement;

    // pour chaque capteur à comparer
    for (Capteur* capteur : autresCapteurs) {
        // on ignore les capteurs non fiables.
        if (!capteur->estFiable()) {
            continue;
        }

        // on récupère les mesures du capteur pour la même période que le capteur de référence
        vector<Mesure> mesuresCapteur = data_.getMesuresPeriode(capteur->getId(), debut, fin);

        // on vérifie qu'il existe au moins une mesure valide pour ce capteur
        bool aMesureValide = false;
        for (const Mesure& mesure : mesuresCapteur) {
            if (mesure.estValide()) {
                aMesureValide = true;
                break;
            }
        }

        // si aucune mesure valide n'est disponible pour ce capteur, on l'ignore.
        if (!aMesureValide) {
            continue;
        }

        // on calcule le score de similarité entre le capteur de référence et ce capteur
        double score = calculerScoreSimilarite(mesuresRef, mesuresCapteur);
        // on ajoute le capteur et son score au classement
        CapteurScore entree;
        entree.capteur = capteur;
        entree.score   = score;
        classement.push_back(entree);
    }

    // une fois le classement initial construit, on le trie par score de similarité croissant
    trierParScoreCroissant(classement);

    perf_.arreter("comparerCapteurs");
    return classement;
}

// Trie un classement de capteurs par score de similarité croissant
// (les capteurs les plus similaires apparaissent en premier).
void AirQualityService::trierParScoreCroissant(vector<CapteurScore>& classement) const {
    sort(classement.begin(), classement.end(),
         [](const CapteurScore& a, const CapteurScore& b) {
             return a.score < b.score;
         });
}

// ============================================================
// Interpolation spatiale
// ============================================================

// Trouve les capteurs proches d'un point géographique pour l'interpolation.
// Trouve les capteurs proches d'un point géographique pour l'interpolation.
vector<pair<Capteur*, vector<Mesure>>> AirQualityService::trouverCapteursProchesPourInterpolation(double lat, double lon) const {

    // rayon de recherche initial pour récupérer les capteurs candidats
    const double kRayonMaxInterpolation = 50.0; // km

    // on récupère directement les capteurs dans un rayon de 50 km autour du point.
    vector<Capteur*> candidats = data_.getTousCapteursLatLon(lat, lon, kRayonMaxInterpolation);

    // on construit la liste des capteurs fiables avec leur distance au point demandé.
    vector<pair<Capteur*, double>> capteursDistances;

    for (Capteur* capteur : candidats) {
        if (!capteur->estFiable()) {
            continue;
        }

        double distance = DataReader::distanceKm(lat, lon, capteur->getLatitude(), capteur->getLongitude());
        capteursDistances.push_back(make_pair(capteur, distance));
    }

    // on trie les capteurs par distance croissante pour prioriser les plus proches
    sort(capteursDistances.begin(), capteursDistances.end(),
         [](const pair<Capteur*, double>& a, const pair<Capteur*, double>& b) {
             return a.second < b.second;
         });

    // on sélectionne au maximum kNbVoisinsInterpolation capteurs parmi les candidats triés
    vector<pair<Capteur*, vector<Mesure>>> capteursProches;
    int nombreSelectionnes = 0;

    for (const pair<Capteur*, double>& entree : capteursDistances) {
        if (nombreSelectionnes >= kNbVoisinsInterpolation) {
            // on a atteint le nombre maximal de capteurs voulu.
            break;
        }

        // on récupère les mesures du capteur pour les utiliser dans l'interpolation
        Capteur* capteur =entree.first;
        vector<Mesure> mesuresCapteur = data_.getMesuresCapteur(capteur->getId());

        // on ajoute le capteur et ses mesures à la liste des capteurs proches pour l'interpolation
        capteursProches.push_back(make_pair(capteur, mesuresCapteur));
        nombreSelectionnes = nombreSelectionnes + 1;
    }

    return capteursProches;
}


// Estime l'indice ATMO en un point géographique par interpolation pondérée
// par l'inverse du carré de la distance aux capteurs les plus proches.
// Si un capteur est exactement au point demandé, son indice est retourné directement.
double AirQualityService::interpolerPondereeParDistance(const vector<pair<Capteur*, vector<Mesure>>>& capteursProches,double lat, double lon) const
{
    double sommeNumerateur   = 0.0;
    double sommeDenominateur = 0.0;

    // pour chaque capteur proche et ses mesures associées
    for (const pair<Capteur*, vector<Mesure>>& entree : capteursProches) {
        // récupérer le capteur et ses mesures associées
        Capteur* capteur            = entree.first;
        const vector<Mesure>& mesures = entree.second;

        // calculer la distance entre le capteur et le point demandé
        double distance = DataReader::distanceKm(lat, lon, capteur->getLatitude(), capteur->getLongitude());

        // si le capteur est exactement au point demandé, on retourne son indice directement.
        if (distance < 1e-6) {
            return calculerIndiceATMO(mesures);
        }

        // Le poids est inversement proportionnel au carré de la distance
        // Cela donne plus d'importance aux capteurs proches
        double poids  = 1.0 / (distance * distance);
        double indice = calculerIndiceATMO(mesures);

        // On ignore les capteurs sans indice valide.
        if (indice <= 0.0) {
            continue;
        }

        // on ajoute la contribution de ce capteur à la somme pondérée
        sommeNumerateur   = sommeNumerateur + poids * indice;
        // on ajoute le poids à la somme des poids pour la normalisation
        sommeDenominateur = sommeDenominateur + poids;
    }

    // on renvoit la moyenne pondérée des indices des capteurs proches, ou 0 si aucun capteur valide n'est disponible
    return sommeDenominateur == 0.0 ? 0.0 : sommeNumerateur / sommeDenominateur;
}

// ============================================================
// Estimation de la qualité d'une zone
// ============================================================

// Estime la qualité de l'air pour une zone géographique et une période données.
// Délègue à calculerMoyenneAQI.
double AirQualityService::estimerQualiteZone(double lat, double lon, double rayon,const DateTime& debut, const DateTime& fin)
{
    // 
    return calculerMoyenneAQI(lat, lon, rayon, debut, fin);
}

}  // namespace airwatcher