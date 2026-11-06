#pragma once
// Données de conception : types d'ennemis, de tours et vagues.
// Le moteur ne connaît aucune unité concrète ; c'est le jeu qui les déclare.
#include "Math.hpp"

#include <string>
#include <vector>

namespace td {

struct EnemyType {
    std::string name = "enemy";
    float maxHp = 10.f;
    float speed = 1.f;      ///< Cases par seconde.
    int reward = 1;         ///< Or gagné quand il meurt.
    int damage = 1;         ///< Vies perdues s'il atteint la base.
    char glyph = 'e';       ///< Symbole pour les rendus texte.
    Color color{220, 60, 60};
};

struct TowerType {
    std::string name = "tower";
    int cost = 50;
    float range = 2.5f;          ///< Portée en cases, depuis le centre de la tour.
    float damage = 5.f;
    float fireRate = 1.f;        ///< Tirs par seconde.
    float projectileSpeed = 8.f; ///< Cases par seconde.
    float splashRadius = 0.f;    ///< 0 = dégâts sur une seule cible.
    float slowFactor = 1.f;      ///< Multiplicateur de vitesse appliqué (0.5 = moitié). 1 = pas de ralentissement.
    float slowDuration = 0.f;    ///< Durée du ralentissement en secondes.
    char glyph = 'T';
    Color color{80, 160, 255};
};

/// Un groupe d'ennemis identiques dans une vague.
struct SpawnGroup {
    int enemyType = 0;     ///< Index retourné par World::addEnemyType.
    int count = 1;
    float interval = 1.f;  ///< Secondes entre deux apparitions du groupe.
    float delay = 0.f;     ///< Secondes avant la première apparition (depuis le début de la vague).
};

struct WaveDef {
    std::vector<SpawnGroup> groups;
};

/// Stratégie de choix de cible d'une tour.
enum class Targeting {
    First,    ///< L'ennemi le plus avancé sur le chemin.
    Last,     ///< L'ennemi le moins avancé.
    Nearest,  ///< Le plus proche de la tour.
    Strongest,///< Le plus de points de vie restants.
    Weakest,  ///< Le moins de points de vie restants.
};

const char* targetingName(Targeting t);
Targeting nextTargeting(Targeting t);

} // namespace td
