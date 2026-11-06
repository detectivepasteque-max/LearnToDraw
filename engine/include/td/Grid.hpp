#pragma once
// Carte en tuiles : le terrain sur lequel se déroule la partie.
#include "Math.hpp"

#include <string>
#include <vector>

namespace td {

/// Type de tuile. La valeur est le caractère utilisé dans les cartes ASCII.
enum class Tile : char {
    Blocked   = '#', ///< Décor infranchissable, on ne peut rien y construire.
    Buildable = '.', ///< Terrain libre pour poser une tour.
    Path      = '=', ///< Chemin emprunté par les ennemis.
    Spawn     = 'S', ///< Point d'apparition des ennemis (fait partie du chemin).
    Goal      = 'G', ///< Base à défendre (fait partie du chemin).
};

class Grid {
public:
    Grid() = default;

    /// Construit une grille à partir de lignes ASCII (voir Tile pour les symboles).
    /// Lève std::invalid_argument si la carte est vide, non rectangulaire,
    /// contient un symbole inconnu, n'a pas de spawn ou n'a pas exactement un but.
    static Grid fromAscii(const std::vector<std::string>& rows);

    int width() const { return width_; }
    int height() const { return height_; }

    bool inBounds(GridPos p) const {
        return p.x >= 0 && p.y >= 0 && p.x < width_ && p.y < height_;
    }
    Tile at(GridPos p) const { return tiles_[static_cast<std::size_t>(p.y * width_ + p.x)]; }
    Tile at(int x, int y) const { return at(GridPos{x, y}); }

    /// Vrai si les ennemis peuvent marcher sur cette case.
    bool isWalkable(GridPos p) const;
    /// Vrai si une tour peut être posée sur cette case (hors occupation).
    bool isBuildable(GridPos p) const;

    const std::vector<GridPos>& spawns() const { return spawns_; }
    GridPos goal() const { return goal_; }

    /// Centre d'une case en coordonnées monde (une case = 1 unité).
    static Vec2 cellCenter(GridPos p) {
        return {static_cast<float>(p.x) + 0.5f, static_cast<float>(p.y) + 0.5f};
    }
    /// Case contenant une position monde.
    static GridPos worldToCell(Vec2 v) {
        return {static_cast<int>(std::floor(v.x)), static_cast<int>(std::floor(v.y))};
    }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<Tile> tiles_;
    std::vector<GridPos> spawns_;
    GridPos goal_;
};

} // namespace td
