#pragma once
// Instances vivantes pendant une partie.
#include "Definitions.hpp"

#include <cstddef>
#include <cstdint>

namespace td {

using EntityId = std::uint32_t;
constexpr EntityId kNoEntity = 0;

struct Enemy {
    EntityId id = kNoEntity;
    int type = 0;
    Vec2 pos;
    float hp = 0.f;
    int pathIndex = 0;             ///< Quel chemin (un par spawn) il suit.
    std::size_t nextWaypoint = 0;  ///< Prochain point de passage à atteindre.
    float progress = 0.f;          ///< Distance parcourue depuis le spawn.
    float slowFactor = 1.f;
    float slowTimer = 0.f;
    bool alive = true;
    bool reachedGoal = false;
};

struct Tower {
    EntityId id = kNoEntity;
    int type = 0;
    GridPos cell;
    float cooldown = 0.f;
    Targeting targeting = Targeting::First;
    int kills = 0;
};

struct Projectile {
    EntityId id = kNoEntity;
    EntityId owner = kNoEntity;   ///< Tour qui a tiré.
    EntityId target = kNoEntity;  ///< Ennemi visé.
    Vec2 pos;
    Vec2 lastTargetPos;           ///< Dernière position connue de la cible.
    float speed = 1.f;
    float damage = 0.f;
    float splashRadius = 0.f;
    float slowFactor = 1.f;
    float slowDuration = 0.f;
    bool done = false;
};

} // namespace td
