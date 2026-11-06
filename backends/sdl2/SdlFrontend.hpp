#pragma once
// Frontend graphique SDL2 : fenêtre, rendu 2D et clavier/souris.
#include "td/Frontend.hpp"

#include <string>

struct SDL_Window;
struct SDL_Renderer;

namespace td {

class SdlFrontend : public IRenderer {
public:
    /// Ouvre une fenêtre. `cellSize` est la taille d'une case en pixels.
    /// Lève std::runtime_error si SDL ne peut pas s'initialiser.
    explicit SdlFrontend(std::string title = "Tower Defense", int cellSize = 40);
    ~SdlFrontend() override;
    SdlFrontend(const SdlFrontend&) = delete;
    SdlFrontend& operator=(const SdlFrontend&) = delete;

    void render(const World& world, const ViewState& view) override;

    int cellSize() const { return cell_; }
    /// Convertit un pixel de la fenêtre en case (peut être hors grille).
    GridPos pixelToCell(int px, int py) const { return {px / cell_, py / cell_}; }

private:
    void resizeFor(const Grid& grid);
    void fillRect(int x, int y, int w, int h, Color c, int alpha = 255);
    void fillCircle(int cx, int cy, int r, Color c);
    void strokeCircle(int cx, int cy, int r, Color c);
    void drawText(int x, int y, const std::string& text, Color c, int scale = 2);
    int textWidth(const std::string& text, int scale = 2) const;

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    int cell_;
    int hudHeight_ = 96;
    int gridW_ = 0;
    int gridH_ = 0;
};

/// Clavier : mêmes touches que le terminal. Souris : déplacer = curseur,
/// clic gauche = construire la dernière tour choisie (touches 1-9), clic droit = vendre.
class SdlInput : public IInput {
public:
    explicit SdlInput(const SdlFrontend& frontend) : frontend_(frontend) {}
    Command poll() override;

private:
    const SdlFrontend& frontend_;
    int selectedTower_ = 0;
};

} // namespace td
