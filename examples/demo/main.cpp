// Exemple complet : définit une carte, des ennemis, des tours et des vagues,
// puis lance la partie avec le frontend terminal (ou SDL2 avec --sdl).
#include "td/Game.hpp"
#include "td/Terminal.hpp"
#if TD_HAS_SDL2
#  include "SdlFrontend.hpp"
#endif

#include <cstring>
#include <iostream>
#include <memory>

namespace {

td::World makeWorld() {
    // Symboles : # décor, . constructible, = chemin, S spawn, G base.
    const td::Grid grid = td::Grid::fromAscii({
        "####################",
        "S=====.............#",
        "#....=.............#",
        "#....=....#####....#",
        "#....======........#",
        "#.........=........#",
        "#.###.....=..####..#",
        "#.#.......=====....#",
        "#.#...........=....#",
        "#.###.........=....#",
        "#.............===G.#",
        "####################",
    });

    td::WorldConfig cfg;
    cfg.startingGold = 150;
    cfg.startingLives = 20;
    cfg.waveCountdown = 12.f;
    td::World world(grid, cfg);

    // Ennemis
    td::EnemyType grunt;
    grunt.name = "Grunt"; grunt.maxHp = 30; grunt.speed = 1.6f; grunt.reward = 6; grunt.damage = 1;
    grunt.glyph = 'g'; grunt.color = {240, 90, 90};
    td::EnemyType runner;
    runner.name = "Runner"; runner.maxHp = 18; runner.speed = 3.2f; runner.reward = 8; runner.damage = 1;
    runner.glyph = 'r'; runner.color = {255, 200, 80};
    td::EnemyType brute;
    brute.name = "Brute"; brute.maxHp = 220; brute.speed = 0.9f; brute.reward = 40; brute.damage = 5;
    brute.glyph = 'B'; brute.color = {200, 80, 220};
    const int kGrunt = world.addEnemyType(grunt);
    const int kRunner = world.addEnemyType(runner);
    const int kBrute = world.addEnemyType(brute);

    // Tours
    td::TowerType arrow;
    arrow.name = "Arrow"; arrow.cost = 50; arrow.range = 2.6f; arrow.damage = 8; arrow.fireRate = 2.f;
    arrow.projectileSpeed = 9.f; arrow.glyph = 'A'; arrow.color = {120, 220, 255};
    td::TowerType cannon;
    cannon.name = "Cannon"; cannon.cost = 110; cannon.range = 3.2f; cannon.damage = 28; cannon.fireRate = 0.6f;
    cannon.projectileSpeed = 5.f; cannon.splashRadius = 1.0f; cannon.glyph = 'C'; cannon.color = {255, 150, 60};
    td::TowerType frost;
    frost.name = "Frost"; frost.cost = 80; frost.range = 2.4f; frost.damage = 3; frost.fireRate = 1.2f;
    frost.projectileSpeed = 7.f; frost.slowFactor = 0.5f; frost.slowDuration = 2.f; frost.glyph = 'F';
    frost.color = {170, 230, 255};
    world.addTowerType(arrow);
    world.addTowerType(cannon);
    world.addTowerType(frost);

    // Vagues
    for (int i = 0; i < 10; ++i) {
        td::WaveDef wave;
        wave.groups.push_back({kGrunt, 6 + i * 2, 0.8f, 0.f});
        if (i >= 2) wave.groups.push_back({kRunner, 3 + i, 0.5f, 4.f});
        if (i >= 4) wave.groups.push_back({kBrute, 1 + (i - 4) / 2, 3.f, 8.f});
        world.addWave(wave);
    }
    return world;
}

} // namespace

int main(int argc, char** argv) {
    bool useSdl = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--sdl") == 0) useSdl = true;
        else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::cout << "Usage: td_demo [--sdl]\n  --sdl   use the SDL2 window instead of the terminal\n";
            return 0;
        }
    }

    try {
        td::World world = makeWorld();
        std::unique_ptr<td::IRenderer> renderer;
        std::unique_ptr<td::IInput> input;
        if (useSdl) {
#if TD_HAS_SDL2
            auto sdl = std::make_unique<td::SdlFrontend>("Tower Defense demo", 40);
            input = std::make_unique<td::SdlInput>(*sdl);
            renderer = std::move(sdl);
#else
            std::cerr << "This build has no SDL2 support; falling back to the terminal.\n";
#endif
        }
        if (!renderer) {
            renderer = std::make_unique<td::TerminalRenderer>("Tower Defense demo");
            input = std::make_unique<td::TerminalInput>();
        }

        td::GameConfig cfg;
        cfg.targetFps = useSdl ? 60 : 30;
        td::Game game(world, *renderer, *input, cfg);
        world.events.onGameOver = [&game](bool victory) {
            game.say(victory ? "You win! Press Esc to quit." : "Your base has fallen. Press Esc to quit.");
        };
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
