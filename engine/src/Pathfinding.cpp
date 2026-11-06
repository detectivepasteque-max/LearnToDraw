#include "td/Pathfinding.hpp"

#include <algorithm>
#include <queue>

namespace td {

std::vector<GridPos> findPath(const Grid& grid, GridPos from, GridPos to) {
    if (!grid.isWalkable(from) || !grid.isWalkable(to)) return {};

    const int w = grid.width();
    const int h = grid.height();
    auto index = [w](GridPos p) { return static_cast<std::size_t>(p.y * w + p.x); };

    std::vector<int> parent(static_cast<std::size_t>(w * h), -1);
    std::vector<bool> visited(static_cast<std::size_t>(w * h), false);
    std::queue<GridPos> open;
    open.push(from);
    visited[index(from)] = true;

    const GridPos dirs[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    bool found = (from == to);
    while (!open.empty() && !found) {
        const GridPos cur = open.front();
        open.pop();
        for (GridPos d : dirs) {
            const GridPos n{cur.x + d.x, cur.y + d.y};
            if (!grid.isWalkable(n) || visited[index(n)]) continue;
            visited[index(n)] = true;
            parent[index(n)] = static_cast<int>(index(cur));
            if (n == to) { found = true; break; }
            open.push(n);
        }
    }
    if (!found) return {};

    std::vector<GridPos> path;
    for (int i = static_cast<int>(index(to)); i != -1; i = parent[static_cast<std::size_t>(i)])
        path.push_back({i % w, i / w});
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<Vec2> toWaypoints(const std::vector<GridPos>& cells) {
    std::vector<Vec2> out;
    out.reserve(cells.size());
    for (std::size_t i = 0; i < cells.size(); ++i) {
        if (i > 0 && i + 1 < cells.size()) {
            const GridPos a = cells[i - 1], b = cells[i], c = cells[i + 1];
            const bool straight = (b.x - a.x == c.x - b.x) && (b.y - a.y == c.y - b.y);
            if (straight) continue;
        }
        out.push_back(Grid::cellCenter(cells[i]));
    }
    return out;
}

} // namespace td
