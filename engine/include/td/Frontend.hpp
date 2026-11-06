#pragma once
// Interfaces à implémenter pour brancher un affichage et des entrées.
// Le moteur fournit deux implémentations : terminal (ANSI) et SDL2 (optionnel).
#include "World.hpp"

#include <string>

namespace td {

enum class CommandType {
    None,
    Quit,
    MoveCursor,      ///< dx/dy
    SetCursor,       ///< x/y absolus (souris)
    Build,           ///< index = type de tour
    Sell,
    NextWave,
    TogglePause,
    CycleSpeed,
    CycleTargeting,
};

struct Command {
    CommandType type = CommandType::None;
    int dx = 0;
    int dy = 0;
    int x = 0;
    int y = 0;
    int index = 0;
};

/// Ce que le jeu affiche en plus du monde : curseur, pause, messages.
struct ViewState {
    GridPos cursor;
    bool paused = false;
    float speed = 1.f;
    std::string message;
    float messageTimer = 0.f;
};

class IInput {
public:
    virtual ~IInput() = default;
    /// Retourne la prochaine commande en attente, ou CommandType::None.
    virtual Command poll() = 0;
};

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void render(const World& world, const ViewState& view) = 0;
};

} // namespace td
