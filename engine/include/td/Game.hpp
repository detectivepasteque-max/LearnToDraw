#pragma once
// Boucle de jeu à pas fixe reliant World, IRenderer et IInput.
#include "Frontend.hpp"

namespace td {

struct GameConfig {
    float fixedDt = 1.f / 60.f;   ///< Pas de simulation.
    float maxFrameTime = 0.25f;   ///< Évite la spirale de la mort après un gel.
    int targetFps = 60;           ///< Cadence de rendu visée par run().
    float messageDuration = 2.5f; ///< Durée d'affichage d'un message.
};

class Game {
public:
    Game(World& world, IRenderer& renderer, IInput& input, GameConfig config = {});

    /// Boucle bloquante jusqu'à la commande Quit.
    void run();
    /// Une itération : entrées, simulation, rendu. Pour intégrer sa propre boucle.
    void step(float realDt);

    bool running() const { return running_; }
    void quit() { running_ = false; }

    World& world() { return world_; }
    ViewState& view() { return view_; }
    const ViewState& view() const { return view_; }

    void handle(const Command& cmd);
    void say(std::string message);

private:
    World& world_;
    IRenderer& renderer_;
    IInput& input_;
    GameConfig config_;
    ViewState view_;
    float accumulator_ = 0.f;
    bool running_ = true;
};

} // namespace td
