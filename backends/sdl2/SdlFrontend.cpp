#include "SdlFrontend.hpp"

#include <SDL.h>

#include <cctype>
#include <cstring>
#include <stdexcept>

namespace td {

namespace {

// Police 3x5 minuscule, suffisante pour l'interface.
struct Glyph { char c; const char* rows[5]; };
const Glyph kFont[] = {
    {'0', {"###","#.#","#.#","#.#","###"}}, {'1', {".#.","##.",".#.",".#.","###"}},
    {'2', {"###","..#","###","#..","###"}}, {'3', {"###","..#","###","..#","###"}},
    {'4', {"#.#","#.#","###","..#","..#"}}, {'5', {"###","#..","###","..#","###"}},
    {'6', {"###","#..","###","#.#","###"}}, {'7', {"###","..#","..#","..#","..#"}},
    {'8', {"###","#.#","###","#.#","###"}}, {'9', {"###","#.#","###","..#","###"}},
    {'A', {".#.","#.#","###","#.#","#.#"}}, {'B', {"##.","#.#","##.","#.#","##."}},
    {'C', {"###","#..","#..","#..","###"}}, {'D', {"##.","#.#","#.#","#.#","##."}},
    {'E', {"###","#..","##.","#..","###"}}, {'F', {"###","#..","##.","#..","#.."}},
    {'G', {"###","#..","#.#","#.#","###"}}, {'H', {"#.#","#.#","###","#.#","#.#"}},
    {'I', {"###",".#.",".#.",".#.","###"}}, {'J', {"..#","..#","..#","#.#","###"}},
    {'K', {"#.#","#.#","##.","#.#","#.#"}}, {'L', {"#..","#..","#..","#..","###"}},
    {'M', {"#.#","###","###","#.#","#.#"}}, {'N', {"##.","#.#","#.#","#.#","#.#"}},
    {'O', {"###","#.#","#.#","#.#","###"}}, {'P', {"###","#.#","###","#..","#.."}},
    {'Q', {"###","#.#","#.#","###","..#"}}, {'R', {"##.","#.#","##.","#.#","#.#"}},
    {'S', {"###","#..","###","..#","###"}}, {'T', {"###",".#.",".#.",".#.",".#."}},
    {'U', {"#.#","#.#","#.#","#.#","###"}}, {'V', {"#.#","#.#","#.#","#.#",".#."}},
    {'W', {"#.#","#.#","###","###","#.#"}}, {'X', {"#.#","#.#",".#.","#.#","#.#"}},
    {'Y', {"#.#","#.#",".#.",".#.",".#."}}, {'Z', {"###","..#",".#.","#..","###"}},
    {':', {"...",".#.","...",".#.","..."}}, {'/', {"..#","..#",".#.","#..","#.."}},
    {'-', {"...","...","###","...","..."}}, {'.', {"...","...","...","...",".#."}},
    {'!', {".#.",".#.",".#.","...",".#."}}, {'*', {"#.#",".#.","###",".#.","#.#"}},
    {'(', {".#.","#..","#..","#..",".#."}}, {')', {".#.","..#","..#","..#",".#."}},
    {'[', {"##.","#..","#..","#..","##."}}, {']', {".##","..#","..#","..#",".##"}},
    {'>', {"#..",".#.","..#",".#.","#.."}}, {',', {"...","...","...",".#.","#.."}},
    {'|', {".#.",".#.",".#.",".#.",".#."}}, {'\'', {".#.",".#.","...","...","..."}},
    {'?', {"###","..#",".##","...",".#."}}, {'+', {"...",".#.","###",".#.","..."}},
};

const Glyph* findGlyph(char c) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    for (const Glyph& g : kFont)
        if (g.c == c) return &g;
    return nullptr;
}

Color tileColor(Tile t) {
    switch (t) {
    case Tile::Blocked: return {52, 54, 64};
    case Tile::Buildable: return {72, 120, 68};
    case Tile::Path: return {176, 150, 104};
    case Tile::Spawn: return {190, 70, 70};
    case Tile::Goal: return {70, 120, 200};
    }
    return {0, 0, 0};
}

std::string fmt1(float v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.1f", static_cast<double>(v));
    return buf;
}

} // namespace

// --- SdlFrontend -------------------------------------------------------------

SdlFrontend::SdlFrontend(std::string title, int cellSize) : cell_(cellSize < 8 ? 8 : cellSize) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
                               SDL_WINDOW_SHOWN);
    if (!window_) {
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) renderer_ = SDL_CreateRenderer(window_, -1, 0);
    if (!renderer_) {
        SDL_DestroyWindow(window_);
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
    }
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
}

SdlFrontend::~SdlFrontend() {
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}

void SdlFrontend::resizeFor(const Grid& grid) {
    if (grid.width() == gridW_ && grid.height() == gridH_) return;
    gridW_ = grid.width();
    gridH_ = grid.height();
    SDL_SetWindowSize(window_, gridW_ * cell_, gridH_ * cell_ + hudHeight_);
    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void SdlFrontend::fillRect(int x, int y, int w, int h, Color c, int alpha) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, static_cast<Uint8>(alpha));
    const SDL_Rect r{x, y, w, h};
    SDL_RenderFillRect(renderer_, &r);
}

void SdlFrontend::fillCircle(int cx, int cy, int r, Color c) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, 255);
    for (int dy = -r; dy <= r; ++dy) {
        const int dx = static_cast<int>(std::sqrt(static_cast<double>(r * r - dy * dy)));
        SDL_RenderDrawLine(renderer_, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void SdlFrontend::strokeCircle(int cx, int cy, int r, Color c) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, 160);
    const int steps = 64;
    int px = cx + r, py = cy;
    for (int i = 1; i <= steps; ++i) {
        const double a = 2.0 * 3.14159265358979 * i / steps;
        const int nx = cx + static_cast<int>(r * std::cos(a));
        const int ny = cy + static_cast<int>(r * std::sin(a));
        SDL_RenderDrawLine(renderer_, px, py, nx, ny);
        px = nx; py = ny;
    }
}

int SdlFrontend::textWidth(const std::string& text, int scale) const {
    return static_cast<int>(text.size()) * 4 * scale;
}

void SdlFrontend::drawText(int x, int y, const std::string& text, Color c, int scale) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, 255);
    for (char ch : text) {
        if (const Glyph* g = findGlyph(ch)) {
            for (int row = 0; row < 5; ++row)
                for (int col = 0; col < 3; ++col)
                    if (g->rows[row][col] == '#') {
                        const SDL_Rect px{x + col * scale, y + row * scale, scale, scale};
                        SDL_RenderFillRect(renderer_, &px);
                    }
        }
        x += 4 * scale;
    }
}

void SdlFrontend::render(const World& world, const ViewState& view) {
    const Grid& g = world.grid();
    resizeFor(g);
    const int cs = cell_;
    auto toPx = [cs](Vec2 v) { return SDL_Point{static_cast<int>(v.x * static_cast<float>(cs)), static_cast<int>(v.y * static_cast<float>(cs))}; };

    SDL_SetRenderDrawColor(renderer_, 24, 24, 30, 255);
    SDL_RenderClear(renderer_);

    // Tuiles
    for (int y = 0; y < g.height(); ++y)
        for (int x = 0; x < g.width(); ++x) {
            fillRect(x * cs, y * cs, cs, cs, tileColor(g.at(x, y)));
            fillRect(x * cs, y * cs, cs, 1, Color{0, 0, 0}, 40);
            fillRect(x * cs, y * cs, 1, cs, Color{0, 0, 0}, 40);
        }
    drawText(g.spawns().empty() ? 0 : g.spawns()[0].x * cs + cs / 2 - 3, g.spawns().empty() ? 0 : g.spawns()[0].y * cs + cs / 2 - 5, "S", Color{255, 255, 255});
    drawText(g.goal().x * cs + cs / 2 - 3, g.goal().y * cs + cs / 2 - 5, "G", Color{255, 255, 255});

    // Tours
    for (const Tower& t : world.towers()) {
        const TowerType& def = world.towerTypes()[static_cast<std::size_t>(t.type)];
        const int inset = cs / 6;
        fillRect(t.cell.x * cs + inset, t.cell.y * cs + inset, cs - 2 * inset, cs - 2 * inset, def.color);
        const int scale = cs >= 32 ? 3 : 2;
        drawText(t.cell.x * cs + cs / 2 - scale * 3 / 2, t.cell.y * cs + cs / 2 - scale * 5 / 2, std::string(1, def.glyph),
                 Color{20, 20, 20}, scale);
    }

    // Portée de la tour sous le curseur
    if (const Tower* t = world.towerAt(view.cursor)) {
        const TowerType& def = world.towerTypes()[static_cast<std::size_t>(t->type)];
        const SDL_Point c = toPx(Grid::cellCenter(t->cell));
        strokeCircle(c.x, c.y, static_cast<int>(def.range * static_cast<float>(cs)), Color{255, 255, 255});
    }

    // Projectiles
    for (const Projectile& p : world.projectiles()) {
        const SDL_Point c = toPx(p.pos);
        const int r = p.splashRadius > 0.f ? cs / 8 : cs / 12;
        fillCircle(c.x, c.y, r < 2 ? 2 : r, Color{255, 240, 120});
    }

    // Ennemis + barres de vie
    for (const Enemy& e : world.enemies()) {
        const EnemyType& def = world.enemyTypes()[static_cast<std::size_t>(e.type)];
        const SDL_Point c = toPx(e.pos);
        const int r = cs * 3 / 10;
        fillCircle(c.x, c.y, r, e.slowTimer > 0.f ? Color{150, 200, 255} : def.color);
        const int barW = cs * 3 / 4;
        const float ratio = def.maxHp > 0.f ? clamp(e.hp / def.maxHp, 0.f, 1.f) : 0.f;
        fillRect(c.x - barW / 2, c.y - r - 6, barW, 4, Color{40, 40, 40});
        fillRect(c.x - barW / 2, c.y - r - 6, static_cast<int>(static_cast<float>(barW) * ratio), 4,
                 ratio > 0.5f ? Color{90, 220, 90} : Color{230, 90, 60});
    }

    // Curseur
    {
        SDL_SetRenderDrawColor(renderer_, 255, 230, 80, 255);
        const SDL_Rect r{view.cursor.x * cs, view.cursor.y * cs, cs, cs};
        SDL_RenderDrawRect(renderer_, &r);
        const SDL_Rect r2{view.cursor.x * cs + 1, view.cursor.y * cs + 1, cs - 2, cs - 2};
        SDL_RenderDrawRect(renderer_, &r2);
    }

    // HUD
    const int hudY = g.height() * cs;
    fillRect(0, hudY, g.width() * cs, hudHeight_, Color{30, 30, 38});
    const Color white{235, 235, 235};
    const Color dim{140, 140, 150};

    std::string status = "GOLD " + std::to_string(world.gold()) + "   LIVES " + std::to_string(world.lives()) +
                         "   WAVE " + std::to_string(world.currentWave()) + "/" + std::to_string(world.waveCount());
    if (world.isGameOver()) status += world.isVictory() ? "   VICTORY!" : "   DEFEAT";
    else if (world.waveInProgress()) status += "   ENEMIES " + std::to_string(world.enemies().size());
    else if (world.currentWave() < world.waveCount())
        status += world.config().autoStartWaves ? "   NEXT WAVE IN " + fmt1(world.waveCountdown()) : "   PRESS N FOR NEXT WAVE";
    status += "   X" + std::to_string(static_cast<int>(view.speed));
    if (view.paused) status += "   PAUSED";
    drawText(8, hudY + 8, status, white);

    int x = 8;
    for (std::size_t i = 0; i < world.towerTypes().size() && i < 9; ++i) {
        const TowerType& def = world.towerTypes()[i];
        const std::string label = "[" + std::to_string(i + 1) + "] " + def.name + " " + std::to_string(def.cost) + "G";
        fillRect(x, hudY + 26, 10, 10, def.color);
        drawText(x + 14, hudY + 26, label, world.gold() >= def.cost ? white : dim);
        x += textWidth(label) + 30;
    }

    std::string info;
    if (const Tower* t = world.towerAt(view.cursor)) {
        const TowerType& def = world.towerTypes()[static_cast<std::size_t>(t->type)];
        info = def.name + "  RANGE " + fmt1(def.range) + "  DMG " + fmt1(def.damage) + "  KILLS " + std::to_string(t->kills) +
               "  TARGET " + targetingName(t->targeting) + "  SELL " +
               std::to_string(static_cast<int>(static_cast<float>(def.cost) * world.config().sellRefund)) + "G";
    } else if (g.isBuildable(view.cursor)) {
        info = "FREE CELL - CLICK OR 1-9 TO BUILD";
    } else {
        info = "CANNOT BUILD HERE";
    }
    drawText(8, hudY + 44, info, dim);
    drawText(8, hudY + 60, "ARROWS/WASD MOVE  1-9 BUILD  X SELL  T TARGET  N WAVE  P PAUSE  F SPEED  ESC QUIT", dim, 1);
    if (!view.message.empty()) drawText(8, hudY + 74, "> " + view.message, Color{255, 220, 120});

    SDL_RenderPresent(renderer_);
}

// --- SdlInput ----------------------------------------------------------------

Command SdlInput::poll() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        Command c;
        switch (ev.type) {
        case SDL_QUIT:
            c.type = CommandType::Quit;
            return c;
        case SDL_MOUSEMOTION: {
            const GridPos p = frontend_.pixelToCell(ev.motion.x, ev.motion.y);
            c.type = CommandType::SetCursor; c.x = p.x; c.y = p.y;
            return c;
        }
        case SDL_MOUSEBUTTONDOWN:
            if (ev.button.button == SDL_BUTTON_LEFT) { c.type = CommandType::Build; c.index = selectedTower_; return c; }
            if (ev.button.button == SDL_BUTTON_RIGHT) { c.type = CommandType::Sell; return c; }
            break;
        case SDL_KEYDOWN: {
            if (ev.key.repeat) break;
            const SDL_Keycode k = ev.key.keysym.sym;
            switch (k) {
            case SDLK_ESCAPE: c.type = CommandType::Quit; return c;
            case SDLK_UP: case SDLK_w: case SDLK_z: c.type = CommandType::MoveCursor; c.dy = -1; return c;
            case SDLK_DOWN: case SDLK_s: c.type = CommandType::MoveCursor; c.dy = 1; return c;
            case SDLK_LEFT: case SDLK_a: case SDLK_q: c.type = CommandType::MoveCursor; c.dx = -1; return c;
            case SDLK_RIGHT: case SDLK_d: c.type = CommandType::MoveCursor; c.dx = 1; return c;
            case SDLK_x: c.type = CommandType::Sell; return c;
            case SDLK_n: c.type = CommandType::NextWave; return c;
            case SDLK_p: case SDLK_SPACE: c.type = CommandType::TogglePause; return c;
            case SDLK_f: c.type = CommandType::CycleSpeed; return c;
            case SDLK_t: c.type = CommandType::CycleTargeting; return c;
            default:
                if (k >= SDLK_1 && k <= SDLK_9) {
                    selectedTower_ = static_cast<int>(k - SDLK_1);
                    c.type = CommandType::Build; c.index = selectedTower_;
                    return c;
                }
                break;
            }
            break;
        }
        default:
            break;
        }
    }
    return {};
}

} // namespace td
