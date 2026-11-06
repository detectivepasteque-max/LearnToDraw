#pragma once
// Recherche de chemin sur la grille (parcours en largeur, 4 voisins).
#include "Grid.hpp"

#include <vector>

namespace td {

/// Plus court chemin de `from` à `to` en passant uniquement par des cases
/// praticables. Retourne la liste des cases, `from` incluse et `to` incluse.
/// Retourne un vecteur vide s'il n'existe aucun chemin.
std::vector<GridPos> findPath(const Grid& grid, GridPos from, GridPos to);

/// Convertit un chemin de cases en points de passage (centres de cases).
/// Les points intermédiaires alignés sont supprimés : seuls les virages restent.
std::vector<Vec2> toWaypoints(const std::vector<GridPos>& cells);

} // namespace td
