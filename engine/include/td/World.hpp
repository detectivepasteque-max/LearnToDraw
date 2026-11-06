#pragma once
// L'état complet d'une partie et sa simulation. Aucune dépendance au rendu.
#include "Entities.hpp"
#include "Grid.hpp"

#include <functional>
#include <vector>

namespace td {

struct WorldConfig {
    int startingGold = 100;
    int startingLives = 20;
    float waveCountdown = 10.f;  ///< Secondes entre deux vagues (si autoStartWaves).
    bool autoStartWaves = true;  ///< Sinon il faut appeler startNextWave().
    float sellRefund = 0.5f;     ///< Fraction du coût remboursée à la vente.
};

/// Points d'accroche pour réagir aux événements (son, effets, statistiques...).
struct WorldEvents {
    std::function<void(const Enemy&)> onEnemySpawned;
    std::function<void(const Enemy&)> onEnemyKilled;
    std::function<void(const Enemy&)> onEnemyReachedGoal;
    std::function<void(const Tower&)> onTowerFired;
    std::function<void(int waveNumber)> onWaveStarted;   ///< Numéro à partir de 1.
    std::function<void(int waveNumber)> onWaveCleared;
    std::function<void(bool victory)> onGameOver;
};

enum class BuildResult { Ok, OutOfBounds, NotBuildable, Occupied, NotEnoughGold, InvalidType, GameOver };
const char* buildResultName(BuildResult r);

class World {
public:
    explicit World(Grid grid, WorldConfig config = {});

    // --- Configuration (à faire avant de jouer) ---------------------------
    int addEnemyType(EnemyType type);
    int addTowerType(TowerType type);
    void addWave(WaveDef wave);

    // --- Commandes du joueur ---------------------------------------------
    BuildResult buildTower(GridPos cell, int towerType);
    bool sellTower(GridPos cell);
    bool setTargeting(GridPos cell, Targeting targeting);
    /// Lance la vague suivante. Échoue si une vague est en cours, s'il n'en
    /// reste plus, ou si la partie est finie.
    bool startNextWave();

    // --- Simulation -------------------------------------------------------
    /// Avance la simulation de `dt` secondes. Utiliser un pas fixe (ex. 1/60).
    void update(float dt);

    // --- Lecture de l'état -----------------------------------------------
    const Grid& grid() const { return grid_; }
    const WorldConfig& config() const { return config_; }
    const std::vector<EnemyType>& enemyTypes() const { return enemyTypes_; }
    const std::vector<TowerType>& towerTypes() const { return towerTypes_; }
    const std::vector<WaveDef>& waves() const { return waves_; }
    const std::vector<std::vector<Vec2>>& paths() const { return paths_; }

    const std::vector<Enemy>& enemies() const { return enemies_; }
    const std::vector<Tower>& towers() const { return towers_; }
    const std::vector<Projectile>& projectiles() const { return projectiles_; }
    const Tower* towerAt(GridPos cell) const;
    const Enemy* findEnemy(EntityId id) const;

    int gold() const { return gold_; }
    int lives() const { return lives_; }
    /// Numéro de la dernière vague lancée (0 avant la première).
    int currentWave() const { return wavesStarted_; }
    int waveCount() const { return static_cast<int>(waves_.size()); }
    bool waveInProgress() const { return waveInProgress_; }
    /// Secondes avant la prochaine vague automatique (0 si non applicable).
    float waveCountdown() const { return waveInProgress_ ? 0.f : countdown_; }
    bool isGameOver() const { return gameOver_; }
    bool isVictory() const { return gameOver_ && victory_; }
    float elapsed() const { return elapsed_; }

    WorldEvents events;

private:
    struct PendingSpawn { float time; int enemyType; };

    void updateWave(float dt);
    void updateEnemies(float dt);
    void updateTowers(float dt);
    void updateProjectiles(float dt);
    void removeDead();
    void checkEndConditions();

    void spawnEnemy(int enemyType);
    Enemy* findEnemyMutable(EntityId id);
    const Enemy* pickTarget(const Tower& tower) const;
    void fire(Tower& tower, const Enemy& target);
    void impact(const Projectile& p, Enemy* directTarget);
    void hit(Enemy& enemy, const Projectile& p);

    Grid grid_;
    WorldConfig config_;
    std::vector<EnemyType> enemyTypes_;
    std::vector<TowerType> towerTypes_;
    std::vector<WaveDef> waves_;
    std::vector<std::vector<Vec2>> paths_;  ///< Un chemin par point de spawn.

    std::vector<Enemy> enemies_;
    std::vector<Tower> towers_;
    std::vector<Projectile> projectiles_;

    int gold_ = 0;
    int lives_ = 0;
    int wavesStarted_ = 0;
    bool waveInProgress_ = false;
    float countdown_ = 0.f;
    float waveClock_ = 0.f;
    std::vector<PendingSpawn> pending_;
    std::size_t nextPending_ = 0;
    std::size_t spawnCursor_ = 0;
    bool gameOver_ = false;
    bool victory_ = false;
    float elapsed_ = 0.f;
    EntityId nextId_ = 1;
};

} // namespace td
