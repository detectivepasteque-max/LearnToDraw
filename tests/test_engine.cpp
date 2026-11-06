// Tests unitaires sans dépendance externe.
#include "td/Game.hpp"
#include "td/Pathfinding.hpp"
#include "td/Terminal.hpp"
#include "td/World.hpp"

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                                              \
    do {                                                                                         \
        ++g_checks;                                                                              \
        if (!(cond)) {                                                                           \
            ++g_failures;                                                                        \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                        \
        }                                                                                        \
    } while (0)

#define CHECK_THROWS(expr)                                                                       \
    do {                                                                                         \
        ++g_checks;                                                                              \
        bool thrown = false;                                                                     \
        try { (void)(expr); } catch (const std::exception&) { thrown = true; }                   \
        if (!thrown) { ++g_failures; std::printf("  FAIL %s:%d: no throw: %s\n", __FILE__, __LINE__, #expr); } \
    } while (0)

bool near(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) <= eps; }

td::Grid simpleGrid() {
    return td::Grid::fromAscii({
        "#######",
        "S====.#",
        "#...=.#",
        "#...=.#",
        "#...G.#",
        "#######",
    });
}

void run(td::World& w, float seconds, float dt = 1.f / 60.f) {
    for (float t = 0.f; t < seconds; t += dt) w.update(dt);
}

void testGrid() {
    std::puts("grid");
    td::Grid g = simpleGrid();
    CHECK(g.width() == 7 && g.height() == 6);
    CHECK(g.at(0, 1) == td::Tile::Spawn);
    CHECK(g.goal() == td::GridPos(4, 4));
    CHECK(g.spawns().size() == 1 && g.spawns()[0] == td::GridPos(0, 1));
    CHECK(g.isWalkable({2, 1}) && !g.isWalkable({2, 2}) && !g.isWalkable({-1, 0}));
    CHECK(g.isBuildable({2, 2}) && !g.isBuildable({0, 0}) && !g.isBuildable({4, 2}));
    CHECK(td::Grid::cellCenter({2, 3}) == td::Vec2(2.5f, 3.5f));
    CHECK(td::Grid::worldToCell({2.9f, 3.1f}) == td::GridPos(2, 3));

    CHECK_THROWS(td::Grid::fromAscii({}));
    CHECK_THROWS(td::Grid::fromAscii({"S=G", "##"}));         // non rectangulaire
    CHECK_THROWS(td::Grid::fromAscii({"S=?"}));               // symbole inconnu
    CHECK_THROWS(td::Grid::fromAscii({"==G"}));               // pas de spawn
    CHECK_THROWS(td::Grid::fromAscii({"S=GG"}));              // deux buts
}

void testPathfinding() {
    std::puts("pathfinding");
    td::Grid g = simpleGrid();
    auto path = td::findPath(g, g.spawns()[0], g.goal());
    CHECK(path.size() == 8);
    CHECK(path.front() == td::GridPos(0, 1) && path.back() == td::GridPos(4, 4));
    for (std::size_t i = 1; i < path.size(); ++i)
        CHECK(std::abs(path[i].x - path[i - 1].x) + std::abs(path[i].y - path[i - 1].y) == 1);

    auto wps = td::toWaypoints(path);
    CHECK(wps.size() == 3); // départ, virage, arrivée
    CHECK(wps[1] == td::Vec2(4.5f, 1.5f));

    CHECK(td::findPath(g, {0, 1}, {0, 1}).size() == 1);
    CHECK(td::findPath(g, {2, 2}, g.goal()).empty()); // départ non praticable

    td::Grid blocked = td::Grid::fromAscii({"S=#=G"});
    CHECK(td::findPath(blocked, {0, 0}, {4, 0}).empty());
    CHECK_THROWS(td::World(blocked));
}

td::World makeWorld(td::WorldConfig cfg = {}) {
    td::World w(simpleGrid(), cfg);
    td::EnemyType e;
    e.maxHp = 10.f; e.speed = 2.f; e.reward = 5; e.damage = 3;
    w.addEnemyType(e);
    td::TowerType t;
    t.cost = 50; t.range = 2.f; t.damage = 4.f; t.fireRate = 4.f; t.projectileSpeed = 100.f;
    w.addTowerType(t);
    return w;
}

void testBuilding() {
    std::puts("building");
    td::WorldConfig cfg;
    cfg.startingGold = 120;
    td::World w = makeWorld(cfg);

    CHECK(w.buildTower({1, 2}, 0) == td::BuildResult::Ok);
    CHECK(w.gold() == 70);
    CHECK(w.towerAt({1, 2}) != nullptr);
    CHECK(w.buildTower({1, 2}, 0) == td::BuildResult::Occupied);
    CHECK(w.buildTower({4, 2}, 0) == td::BuildResult::NotBuildable);
    CHECK(w.buildTower({0, 0}, 0) == td::BuildResult::NotBuildable);
    CHECK(w.buildTower({99, 0}, 0) == td::BuildResult::OutOfBounds);
    CHECK(w.buildTower({2, 2}, 7) == td::BuildResult::InvalidType);
    CHECK(w.buildTower({2, 2}, 0) == td::BuildResult::Ok);
    CHECK(w.buildTower({3, 2}, 0) == td::BuildResult::NotEnoughGold);
    CHECK(w.towers().size() == 2);

    CHECK(w.sellTower({2, 2}));
    CHECK(w.gold() == 45);
    CHECK(!w.sellTower({2, 2}));
    CHECK(w.setTargeting({1, 2}, td::Targeting::Weakest));
    CHECK(w.towerAt({1, 2})->targeting == td::Targeting::Weakest);
    CHECK(!w.setTargeting({3, 3}, td::Targeting::Weakest));
}

void testEnemyReachesGoal() {
    std::puts("enemy reaches goal");
    td::WorldConfig cfg;
    cfg.autoStartWaves = false;
    cfg.startingLives = 5;
    td::World w = makeWorld(cfg);
    td::WaveDef wave;
    wave.groups.push_back({0, 2, 1.f, 0.f});
    w.addWave(wave);

    int reached = 0, spawned = 0;
    w.events.onEnemyReachedGoal = [&](const td::Enemy&) { ++reached; };
    w.events.onEnemySpawned = [&](const td::Enemy&) { ++spawned; };

    CHECK(!w.waveInProgress());
    run(w, 5.f);
    CHECK(w.enemies().empty()); // rien ne se passe sans startNextWave
    CHECK(w.startNextWave());
    CHECK(w.waveInProgress() && w.currentWave() == 1);
    CHECK(!w.startNextWave()); // déjà en cours
    w.update(1.f / 60.f);
    CHECK(spawned == 1 && w.enemies().size() == 1);
    CHECK(w.enemies()[0].pos.x > 0.5f && w.enemies()[0].pos.x < 0.6f && near(w.enemies()[0].pos.y, 1.5f));

    run(w, 1.f);
    CHECK(spawned == 2);
    const td::Enemy& first = w.enemies()[0];
    CHECK(first.progress > 1.9f && first.progress < 2.1f); // 2 cases/s

    run(w, 5.f);
    CHECK(reached == 2);
    CHECK(w.lives() == 5 - 2 * 3 || w.lives() == 0);
    CHECK(w.isGameOver() && !w.isVictory());
}

void testTowerKillsAndVictory() {
    std::puts("tower kills, gold and victory");
    td::WorldConfig cfg;
    cfg.autoStartWaves = true;
    cfg.waveCountdown = 0.5f;
    cfg.startingGold = 100;
    td::World w = makeWorld(cfg);
    td::WaveDef wave;
    wave.groups.push_back({0, 3, 0.5f, 0.f});
    w.addWave(wave);
    w.addWave(wave);

    int killed = 0, fired = 0, cleared = 0;
    bool victory = false;
    w.events.onEnemyKilled = [&](const td::Enemy&) { ++killed; };
    w.events.onTowerFired = [&](const td::Tower&) { ++fired; };
    w.events.onWaveCleared = [&](int) { ++cleared; };
    w.events.onGameOver = [&](bool v) { victory = v; };

    CHECK(w.buildTower({2, 2}, 0) == td::BuildResult::Ok); // couvre le chemin horizontal
    CHECK(w.waveCountdown() > 0.f);
    run(w, 30.f);
    CHECK(killed == 6);
    CHECK(fired >= 6 * 3); // 10 pv / 4 dégâts = 3 tirs par ennemi
    CHECK(w.towerAt({2, 2})->kills == 6);
    CHECK(w.gold() == 50 + 6 * 5);
    CHECK(w.lives() == cfg.startingLives);
    CHECK(cleared == 2 && w.currentWave() == 2);
    CHECK(w.isGameOver() && w.isVictory() && victory);
    CHECK(w.buildTower({3, 3}, 0) == td::BuildResult::GameOver);
}

void testSplashAndSlow() {
    std::puts("splash and slow");
    td::WorldConfig cfg;
    cfg.autoStartWaves = false;
    cfg.startingGold = 1000;
    td::World w = makeWorld(cfg);
    td::TowerType frost;
    frost.cost = 10; frost.range = 3.f; frost.damage = 0.f; frost.fireRate = 10.f; frost.projectileSpeed = 100.f;
    frost.slowFactor = 0.5f; frost.slowDuration = 10.f;
    const int kFrost = w.addTowerType(frost);
    td::TowerType bomb;
    bomb.cost = 10; bomb.range = 3.f; bomb.damage = 100.f; bomb.fireRate = 0.01f; bomb.projectileSpeed = 100.f;
    bomb.splashRadius = 10.f;
    const int kBomb = w.addTowerType(bomb);

    td::WaveDef wave;
    wave.groups.push_back({0, 1, 1.f, 0.f});
    w.addWave(wave);
    CHECK(w.buildTower({1, 2}, kFrost) == td::BuildResult::Ok);
    w.startNextWave();
    run(w, 1.f);
    CHECK(w.enemies().size() == 1);
    CHECK(near(w.enemies()[0].slowFactor, 0.5f));
    CHECK(w.enemies()[0].progress < 1.5f); // ralenti : moins de 2 cases en 1 s

    // Deux ennemis, une bombe : les deux meurent d'un seul coup.
    td::World w2 = makeWorld(cfg);
    w2.addTowerType(frost);
    w2.addTowerType(bomb);
    td::WaveDef pair;
    pair.groups.push_back({0, 2, 0.2f, 0.f});
    w2.addWave(pair);
    int killed = 0;
    w2.events.onEnemyKilled = [&](const td::Enemy&) { ++killed; };
    w2.startNextWave();
    run(w2, 0.4f); // les deux ennemis sont apparus
    CHECK(w2.enemies().size() == 2);
    CHECK(w2.buildTower({3, 2}, kBomb) == td::BuildResult::Ok);
    run(w2, 1.f);
    CHECK(killed == 2);
    CHECK(w2.towerAt({3, 2})->kills == 2);
}

void testTargeting() {
    std::puts("targeting");
    auto firstShotTarget = [](td::Targeting mode) {
        td::WorldConfig cfg;
        cfg.autoStartWaves = false;
        td::World w = makeWorld(cfg);
        td::WaveDef wave;
        wave.groups.push_back({0, 2, 0.5f, 0.f});
        w.addWave(wave);
        w.startNextWave();
        run(w, 0.7f); // les deux ennemis sont en portée de (2,2)
        if (w.enemies().size() != 2) return std::pair<td::EntityId, td::EntityId>{0, 0};
        const td::EntityId leader = w.enemies().front().id;
        const td::EntityId trailer = w.enemies().back().id;
        w.buildTower({2, 2}, 0);
        w.setTargeting({2, 2}, mode);
        td::EntityId shot = td::kNoEntity;
        w.events.onTowerFired = [&](const td::Tower&) {
            if (shot == td::kNoEntity) shot = w.projectiles().back().target;
        };
        w.update(1.f / 60.f);
        return std::pair<td::EntityId, td::EntityId>{shot, mode == td::Targeting::First ? leader : trailer};
    };
    auto first = firstShotTarget(td::Targeting::First);
    CHECK(first.first != td::kNoEntity && first.first == first.second);
    auto last = firstShotTarget(td::Targeting::Last);
    CHECK(last.first != td::kNoEntity && last.first == last.second);
    CHECK(first.first != last.first);
    CHECK(td::nextTargeting(td::Targeting::Weakest) == td::Targeting::First);
    CHECK(std::string(td::targetingName(td::Targeting::Nearest)) == "Nearest");
}

void testGameLoop() {
    std::puts("game loop");
    struct NullRenderer : td::IRenderer {
        int frames = 0;
        void render(const td::World&, const td::ViewState&) override { ++frames; }
    };
    struct ScriptedInput : td::IInput {
        std::vector<td::Command> queue;
        td::Command poll() override {
            if (queue.empty()) return {};
            td::Command c = queue.front();
            queue.erase(queue.begin());
            return c;
        }
    };

    td::WorldConfig cfg;
    cfg.autoStartWaves = false;
    td::World w = makeWorld(cfg);
    td::WaveDef wave;
    wave.groups.push_back({0, 1, 1.f, 0.f});
    w.addWave(wave);

    NullRenderer r;
    ScriptedInput in;
    td::Game game(w, r, in);
    CHECK(game.view().cursor == td::GridPos(5, 1)); // première case constructible

    in.queue.push_back(td::TerminalInput::translate('s'));
    in.queue.push_back(td::TerminalInput::translate('1'));
    game.step(0.f);
    CHECK(game.view().cursor == td::GridPos(5, 2));
    CHECK(w.towerAt({5, 2}) != nullptr);
    CHECK(game.view().message.rfind("Built", 0) == 0);

    in.queue.push_back(td::TerminalInput::translate('p'));
    in.queue.push_back(td::TerminalInput::translate('n'));
    game.step(1.f);
    CHECK(game.view().paused);
    CHECK(w.waveInProgress() && w.enemies().empty()); // en pause : pas de simulation

    in.queue.push_back(td::TerminalInput::translate('p'));
    in.queue.push_back(td::TerminalInput::translate('f'));
    game.step(1.f);
    CHECK(!game.view().paused && near(game.view().speed, 2.f));
    CHECK(near(w.elapsed(), 0.5f, 0.02f)); // dt plafonné à 0.25 s, vitesse x2

    in.queue.push_back(td::TerminalInput::translate(27));
    game.step(0.f);
    CHECK(!game.running());
    CHECK(r.frames == 4);
}

void testTerminalFrame() {
    std::puts("terminal frame");
    td::World w = makeWorld();
    w.buildTower({1, 2}, 0);
    td::ViewState view;
    view.cursor = {1, 2};
    // frame() construit l'image sans rien écrire sur le terminal.
    td::TerminalRenderer renderer("Test");
    const std::string f = renderer.frame(w, view);
    CHECK(f.find("Gold: 50") != std::string::npos);
    CHECK(f.find("Lives: 20") != std::string::npos);
    CHECK(f.find("Cursor (1,2): tower") != std::string::npos);
}

} // namespace

int main() {
    testGrid();
    testPathfinding();
    testBuilding();
    testEnemyReachesGoal();
    testTowerKillsAndVictory();
    testSplashAndSlow();
    testTargeting();
    testGameLoop();
    testTerminalFrame();
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
