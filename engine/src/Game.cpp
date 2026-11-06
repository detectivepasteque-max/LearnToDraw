#include "td/Game.hpp"

#include <algorithm>
#include <chrono>
#include <thread>

namespace td {

Game::Game(World& world, IRenderer& renderer, IInput& input, GameConfig config)
    : world_(world), renderer_(renderer), input_(input), config_(config) {
    // Curseur initial : première case constructible trouvée.
    const Grid& g = world_.grid();
    for (int y = 0; y < g.height(); ++y)
        for (int x = 0; x < g.width(); ++x)
            if (g.isBuildable({x, y})) { view_.cursor = {x, y}; return; }
}

void Game::say(std::string message) {
    view_.message = std::move(message);
    view_.messageTimer = config_.messageDuration;
}

void Game::handle(const Command& cmd) {
    const Grid& g = world_.grid();
    switch (cmd.type) {
    case CommandType::None:
        break;
    case CommandType::Quit:
        running_ = false;
        break;
    case CommandType::MoveCursor:
        view_.cursor.x = clamp(view_.cursor.x + cmd.dx, 0, g.width() - 1);
        view_.cursor.y = clamp(view_.cursor.y + cmd.dy, 0, g.height() - 1);
        break;
    case CommandType::SetCursor:
        if (g.inBounds({cmd.x, cmd.y})) view_.cursor = {cmd.x, cmd.y};
        break;
    case CommandType::Build: {
        const BuildResult r = world_.buildTower(view_.cursor, cmd.index);
        if (r == BuildResult::Ok) say("Built " + world_.towerTypes()[static_cast<std::size_t>(cmd.index)].name);
        else say(buildResultName(r));
        break;
    }
    case CommandType::Sell:
        say(world_.sellTower(view_.cursor) ? "Tower sold" : "Nothing to sell here");
        break;
    case CommandType::NextWave:
        if (world_.startNextWave()) say("Wave " + std::to_string(world_.currentWave()) + " incoming!");
        else say(world_.waveInProgress() ? "Wave already in progress" : "No more waves");
        break;
    case CommandType::TogglePause:
        view_.paused = !view_.paused;
        break;
    case CommandType::CycleSpeed:
        view_.speed = view_.speed >= 4.f ? 1.f : view_.speed * 2.f;
        break;
    case CommandType::CycleTargeting:
        if (const Tower* t = world_.towerAt(view_.cursor)) {
            const Targeting next = nextTargeting(t->targeting);
            world_.setTargeting(view_.cursor, next);
            say(std::string("Targeting: ") + targetingName(next));
        } else {
            say("No tower here");
        }
        break;
    }
}

void Game::step(float realDt) {
    for (Command c = input_.poll(); c.type != CommandType::None; c = input_.poll())
        handle(c);

    realDt = std::min(realDt, config_.maxFrameTime);
    if (view_.messageTimer > 0.f) {
        view_.messageTimer -= realDt;
        if (view_.messageTimer <= 0.f) view_.message.clear();
    }
    if (!view_.paused) {
        accumulator_ += realDt * view_.speed;
        while (accumulator_ >= config_.fixedDt) {
            world_.update(config_.fixedDt);
            accumulator_ -= config_.fixedDt;
        }
    }
    renderer_.render(world_, view_);
}

void Game::run() {
    using clock = std::chrono::steady_clock;
    const auto frame = std::chrono::duration<double>(1.0 / std::max(1, config_.targetFps));
    auto last = clock::now();
    while (running_) {
        const auto now = clock::now();
        const float dt = std::chrono::duration<float>(now - last).count();
        last = now;
        step(dt);
        const auto spent = clock::now() - now;
        if (spent < frame) std::this_thread::sleep_for(frame - spent);
    }
}

} // namespace td
