#include "td/World.hpp"
#include "td/Pathfinding.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace td {

World::World(Grid grid, WorldConfig config)
    : grid_(std::move(grid)), config_(config),
      gold_(config.startingGold), lives_(config.startingLives), countdown_(config.waveCountdown) {
    for (GridPos spawn : grid_.spawns()) {
        std::vector<GridPos> cells = findPath(grid_, spawn, grid_.goal());
        if (cells.empty())
            throw std::invalid_argument("World: no path from spawn (" + std::to_string(spawn.x) + "," +
                                        std::to_string(spawn.y) + ") to goal");
        paths_.push_back(toWaypoints(cells));
    }
}

// --- Configuration ---------------------------------------------------------

int World::addEnemyType(EnemyType type) {
    enemyTypes_.push_back(std::move(type));
    return static_cast<int>(enemyTypes_.size()) - 1;
}

int World::addTowerType(TowerType type) {
    towerTypes_.push_back(std::move(type));
    return static_cast<int>(towerTypes_.size()) - 1;
}

void World::addWave(WaveDef wave) {
    for (const SpawnGroup& g : wave.groups)
        if (g.enemyType < 0 || g.enemyType >= static_cast<int>(enemyTypes_.size()))
            throw std::invalid_argument("World::addWave: unknown enemy type");
    waves_.push_back(std::move(wave));
}

// --- Commandes -------------------------------------------------------------

BuildResult World::buildTower(GridPos cell, int towerType) {
    if (gameOver_) return BuildResult::GameOver;
    if (towerType < 0 || towerType >= static_cast<int>(towerTypes_.size())) return BuildResult::InvalidType;
    if (!grid_.inBounds(cell)) return BuildResult::OutOfBounds;
    if (!grid_.isBuildable(cell)) return BuildResult::NotBuildable;
    if (towerAt(cell)) return BuildResult::Occupied;
    const TowerType& def = towerTypes_[static_cast<std::size_t>(towerType)];
    if (gold_ < def.cost) return BuildResult::NotEnoughGold;

    gold_ -= def.cost;
    Tower t;
    t.id = nextId_++;
    t.type = towerType;
    t.cell = cell;
    towers_.push_back(t);
    return BuildResult::Ok;
}

bool World::sellTower(GridPos cell) {
    if (gameOver_) return false;
    auto it = std::find_if(towers_.begin(), towers_.end(), [cell](const Tower& t) { return t.cell == cell; });
    if (it == towers_.end()) return false;
    gold_ += static_cast<int>(static_cast<float>(towerTypes_[static_cast<std::size_t>(it->type)].cost) * config_.sellRefund);
    towers_.erase(it);
    return true;
}

bool World::setTargeting(GridPos cell, Targeting targeting) {
    for (Tower& t : towers_)
        if (t.cell == cell) { t.targeting = targeting; return true; }
    return false;
}

bool World::startNextWave() {
    if (gameOver_ || waveInProgress_ || wavesStarted_ >= waveCount()) return false;

    const WaveDef& wave = waves_[static_cast<std::size_t>(wavesStarted_)];
    pending_.clear();
    for (const SpawnGroup& g : wave.groups)
        for (int i = 0; i < g.count; ++i)
            pending_.push_back({g.delay + g.interval * static_cast<float>(i), g.enemyType});
    std::stable_sort(pending_.begin(), pending_.end(),
                     [](const PendingSpawn& a, const PendingSpawn& b) { return a.time < b.time; });
    nextPending_ = 0;
    waveClock_ = 0.f;
    waveInProgress_ = true;
    ++wavesStarted_;
    if (events.onWaveStarted) events.onWaveStarted(wavesStarted_);
    return true;
}

// --- Simulation ------------------------------------------------------------

void World::update(float dt) {
    if (gameOver_ || dt <= 0.f) return;
    elapsed_ += dt;
    updateWave(dt);
    updateEnemies(dt);
    updateTowers(dt);
    updateProjectiles(dt);
    removeDead();
    checkEndConditions();
}

void World::updateWave(float dt) {
    if (waveInProgress_) {
        waveClock_ += dt;
        while (nextPending_ < pending_.size() && pending_[nextPending_].time <= waveClock_) {
            spawnEnemy(pending_[nextPending_].enemyType);
            ++nextPending_;
        }
        const bool allSpawned = nextPending_ >= pending_.size();
        const bool noneAlive = std::none_of(enemies_.begin(), enemies_.end(), [](const Enemy& e) { return e.alive; });
        if (allSpawned && noneAlive) {
            waveInProgress_ = false;
            countdown_ = config_.waveCountdown;
            if (events.onWaveCleared) events.onWaveCleared(wavesStarted_);
        }
    } else if (config_.autoStartWaves && wavesStarted_ < waveCount()) {
        countdown_ -= dt;
        if (countdown_ <= 0.f) startNextWave();
    }
}

void World::spawnEnemy(int enemyType) {
    Enemy e;
    e.id = nextId_++;
    e.type = enemyType;
    e.hp = enemyTypes_[static_cast<std::size_t>(enemyType)].maxHp;
    e.pathIndex = static_cast<int>(spawnCursor_ % paths_.size());
    spawnCursor_ = (spawnCursor_ + 1) % paths_.size();
    e.pos = paths_[static_cast<std::size_t>(e.pathIndex)].front();
    e.nextWaypoint = 1;
    enemies_.push_back(e);
    if (events.onEnemySpawned) events.onEnemySpawned(enemies_.back());
}

void World::updateEnemies(float dt) {
    for (Enemy& e : enemies_) {
        if (!e.alive) continue;
        if (e.slowTimer > 0.f) {
            e.slowTimer -= dt;
            if (e.slowTimer <= 0.f) { e.slowTimer = 0.f; e.slowFactor = 1.f; }
        }
        const EnemyType& type = enemyTypes_[static_cast<std::size_t>(e.type)];
        const std::vector<Vec2>& path = paths_[static_cast<std::size_t>(e.pathIndex)];
        float budget = type.speed * e.slowFactor * dt;
        while (budget > 0.f && e.nextWaypoint < path.size()) {
            const Vec2 to = path[e.nextWaypoint] - e.pos;
            const float d = to.length();
            if (d <= budget) {
                e.pos = path[e.nextWaypoint];
                e.progress += d;
                budget -= d;
                ++e.nextWaypoint;
            } else {
                e.pos += to * (budget / d);
                e.progress += budget;
                budget = 0.f;
            }
        }
        if (e.nextWaypoint >= path.size()) {
            e.alive = false;
            e.reachedGoal = true;
            lives_ -= type.damage;
            if (events.onEnemyReachedGoal) events.onEnemyReachedGoal(e);
        }
    }
}

const Enemy* World::pickTarget(const Tower& tower) const {
    const TowerType& def = towerTypes_[static_cast<std::size_t>(tower.type)];
    const Vec2 origin = Grid::cellCenter(tower.cell);
    const float rangeSq = def.range * def.range;

    const Enemy* best = nullptr;
    float bestScore = 0.f;
    for (const Enemy& e : enemies_) {
        if (!e.alive) continue;
        const float dSq = distanceSq(origin, e.pos);
        if (dSq > rangeSq) continue;
        float score = 0.f;
        switch (tower.targeting) {
        case Targeting::First: score = e.progress; break;
        case Targeting::Last: score = -e.progress; break;
        case Targeting::Nearest: score = -dSq; break;
        case Targeting::Strongest: score = e.hp; break;
        case Targeting::Weakest: score = -e.hp; break;
        }
        if (!best || score > bestScore) { best = &e; bestScore = score; }
    }
    return best;
}

void World::fire(Tower& tower, const Enemy& target) {
    const TowerType& def = towerTypes_[static_cast<std::size_t>(tower.type)];
    Projectile p;
    p.id = nextId_++;
    p.owner = tower.id;
    p.target = target.id;
    p.pos = Grid::cellCenter(tower.cell);
    p.lastTargetPos = target.pos;
    p.speed = def.projectileSpeed;
    p.damage = def.damage;
    p.splashRadius = def.splashRadius;
    p.slowFactor = def.slowFactor;
    p.slowDuration = def.slowDuration;
    projectiles_.push_back(p);
    tower.cooldown = def.fireRate > 0.f ? 1.f / def.fireRate : std::numeric_limits<float>::infinity();
    if (events.onTowerFired) events.onTowerFired(tower);
}

void World::updateTowers(float dt) {
    for (Tower& t : towers_) {
        t.cooldown -= dt;
        if (t.cooldown > 0.f) continue;
        t.cooldown = 0.f; // prêt : tire dès qu'une cible entre en portée
        if (const Enemy* target = pickTarget(t)) fire(t, *target);
    }
}

void World::updateProjectiles(float dt) {
    // On parcourt par index : impact() ne modifie pas projectiles_, mais restons prudents.
    for (std::size_t i = 0; i < projectiles_.size(); ++i) {
        Projectile& p = projectiles_[i];
        if (p.done) continue;
        Enemy* target = findEnemyMutable(p.target);
        if (target && !target->alive) target = nullptr;
        if (target) p.lastTargetPos = target->pos;

        const Vec2 to = p.lastTargetPos - p.pos;
        const float d = to.length();
        const float step = p.speed * dt;
        if (d <= step) {
            p.pos = p.lastTargetPos;
            p.done = true;
            impact(p, target);
        } else {
            p.pos += to * (step / d);
        }
    }
}

void World::impact(const Projectile& p, Enemy* directTarget) {
    if (p.splashRadius > 0.f) {
        const float rSq = p.splashRadius * p.splashRadius;
        for (Enemy& e : enemies_)
            if (e.alive && distanceSq(e.pos, p.pos) <= rSq) hit(e, p);
    } else if (directTarget) {
        hit(*directTarget, p);
    }
}

void World::hit(Enemy& enemy, const Projectile& p) {
    enemy.hp -= p.damage;
    if (p.slowFactor < 1.f && p.slowDuration > 0.f) {
        enemy.slowFactor = std::min(enemy.slowFactor, p.slowFactor);
        enemy.slowTimer = std::max(enemy.slowTimer, p.slowDuration);
    }
    if (enemy.hp <= 0.f && enemy.alive) {
        enemy.alive = false;
        gold_ += enemyTypes_[static_cast<std::size_t>(enemy.type)].reward;
        for (Tower& t : towers_)
            if (t.id == p.owner) { ++t.kills; break; }
        if (events.onEnemyKilled) events.onEnemyKilled(enemy);
    }
}

void World::removeDead() {
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& e) { return !e.alive; }),
                   enemies_.end());
    projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(), [](const Projectile& p) { return p.done; }),
                       projectiles_.end());
}

void World::checkEndConditions() {
    if (lives_ <= 0) {
        lives_ = 0;
        gameOver_ = true;
        victory_ = false;
        if (events.onGameOver) events.onGameOver(false);
        return;
    }
    if (!waveInProgress_ && wavesStarted_ >= waveCount() && waveCount() > 0 && enemies_.empty()) {
        gameOver_ = true;
        victory_ = true;
        if (events.onGameOver) events.onGameOver(true);
    }
}

// --- Requêtes --------------------------------------------------------------

const Tower* World::towerAt(GridPos cell) const {
    for (const Tower& t : towers_)
        if (t.cell == cell) return &t;
    return nullptr;
}

const Enemy* World::findEnemy(EntityId id) const {
    for (const Enemy& e : enemies_)
        if (e.id == id) return &e;
    return nullptr;
}

Enemy* World::findEnemyMutable(EntityId id) {
    for (Enemy& e : enemies_)
        if (e.id == id) return &e;
    return nullptr;
}

} // namespace td
