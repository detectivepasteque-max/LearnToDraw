#include "td/Grid.hpp"

#include <stdexcept>

namespace td {

Grid Grid::fromAscii(const std::vector<std::string>& rows) {
    if (rows.empty() || rows.front().empty())
        throw std::invalid_argument("Grid::fromAscii: empty map");

    Grid g;
    g.height_ = static_cast<int>(rows.size());
    g.width_ = static_cast<int>(rows.front().size());
    g.tiles_.reserve(static_cast<std::size_t>(g.width_ * g.height_));

    int goals = 0;
    for (int y = 0; y < g.height_; ++y) {
        const std::string& row = rows[static_cast<std::size_t>(y)];
        if (static_cast<int>(row.size()) != g.width_)
            throw std::invalid_argument("Grid::fromAscii: row " + std::to_string(y) + " has a different length");
        for (int x = 0; x < g.width_; ++x) {
            const char c = row[static_cast<std::size_t>(x)];
            switch (c) {
            case '#': case '.': case '=':
                break;
            case 'S':
                g.spawns_.push_back({x, y});
                break;
            case 'G':
                g.goal_ = {x, y};
                ++goals;
                break;
            default:
                throw std::invalid_argument(std::string("Grid::fromAscii: unknown symbol '") + c + "'");
            }
            g.tiles_.push_back(static_cast<Tile>(c));
        }
    }
    if (g.spawns_.empty()) throw std::invalid_argument("Grid::fromAscii: no spawn ('S')");
    if (goals != 1) throw std::invalid_argument("Grid::fromAscii: exactly one goal ('G') required");
    return g;
}

bool Grid::isWalkable(GridPos p) const {
    if (!inBounds(p)) return false;
    const Tile t = at(p);
    return t == Tile::Path || t == Tile::Spawn || t == Tile::Goal;
}

bool Grid::isBuildable(GridPos p) const {
    return inBounds(p) && at(p) == Tile::Buildable;
}

} // namespace td
