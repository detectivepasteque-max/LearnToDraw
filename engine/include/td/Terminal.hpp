#pragma once
// Frontend texte : rendu ANSI dans le terminal et lecture clavier non bloquante.
#include "Frontend.hpp"

#include <string>

namespace td {

class TerminalRenderer : public IRenderer {
public:
    explicit TerminalRenderer(std::string title = "Tower Defense");
    ~TerminalRenderer() override;
    void render(const World& world, const ViewState& view) override;

    /// Produit l'image sans l'écrire (utile pour les tests ou un autre flux).
    std::string frame(const World& world, const ViewState& view) const;

private:
    std::string title_;
    bool active_ = false; ///< Vrai dès que le terminal a été mis en mode plein écran.
};

/// Touches : flèches ou ZQSD/WASD, 1-9 construire, x vendre, n vague suivante,
/// p pause, f vitesse, t ciblage, échap pour quitter (q = gauche en ZQSD).
class TerminalInput : public IInput {
public:
    TerminalInput();
    ~TerminalInput() override;
    Command poll() override;

    /// Traduit une touche en commande (exposé pour les tests).
    static Command translate(int key);

private:
    int readKey();
    struct State;
    State* state_;
};

} // namespace td
