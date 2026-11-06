#include "td/Terminal.hpp"

#include <cstdio>
#include <sstream>
#include <iomanip>

#if defined(_WIN32)
#  define NOMINMAX
#  include <conio.h>
#  include <windows.h>
#else
#  include <termios.h>
#  include <unistd.h>
#endif

namespace td {

namespace {

constexpr int kKeyUp = 1000, kKeyDown = 1001, kKeyLeft = 1002, kKeyRight = 1003, kKeyEsc = 27;

std::string fg(Color c) {
    return "\x1b[38;2;" + std::to_string(c.r) + ";" + std::to_string(c.g) + ";" + std::to_string(c.b) + "m";
}
std::string bg(Color c) {
    return "\x1b[48;2;" + std::to_string(c.r) + ";" + std::to_string(c.g) + ";" + std::to_string(c.b) + "m";
}
constexpr const char* kReset = "\x1b[0m";

Color tileColor(Tile t) {
    switch (t) {
    case Tile::Blocked: return {60, 60, 70};
    case Tile::Buildable: return {40, 90, 40};
    case Tile::Path: return {150, 120, 80};
    case Tile::Spawn: return {170, 60, 60};
    case Tile::Goal: return {60, 110, 190};
    }
    return {0, 0, 0};
}

} // namespace

// --- Rendu -----------------------------------------------------------------

TerminalRenderer::TerminalRenderer(std::string title) : title_(std::move(title)) {}

TerminalRenderer::~TerminalRenderer() {
    if (!active_) return;
    std::fputs("\x1b[0m\x1b[?25h\n", stdout);
    std::fflush(stdout);
}

std::string TerminalRenderer::frame(const World& world, const ViewState& view) const {
    const Grid& g = world.grid();
    std::ostringstream out;
    out << std::fixed << std::setprecision(1);

    // Ligne d'état
    out << "\x1b[H" << kReset << " " << title_ << "   Gold: " << world.gold() << "   Lives: " << world.lives()
        << "   Wave: " << world.currentWave() << "/" << world.waveCount();
    if (world.isGameOver()) out << "   " << (world.isVictory() ? "*** VICTORY ***" : "*** DEFEAT ***");
    else if (world.waveInProgress()) out << "   Enemies: " << world.enemies().size();
    else if (world.currentWave() < world.waveCount()) {
        if (world.config().autoStartWaves) out << "   Next wave in " << world.waveCountdown() << "s";
        else out << "   Press [n] for next wave";
    }
    out << "   Speed x" << static_cast<int>(view.speed);
    if (view.paused) out << "   [PAUSED]";
    out << "\x1b[K\n\n";

    // Carte : chaque case occupe deux colonnes pour un rendu à peu près carré.
    std::vector<std::string> cellText(static_cast<std::size_t>(g.width() * g.height()));
    std::vector<Color> cellFg(cellText.size(), Color{255, 255, 255});
    auto idx = [&g](GridPos p) { return static_cast<std::size_t>(p.y * g.width() + p.x); };

    for (int y = 0; y < g.height(); ++y)
        for (int x = 0; x < g.width(); ++x) {
            const Tile t = g.at(x, y);
            cellText[idx({x, y})] = (t == Tile::Spawn) ? "S " : (t == Tile::Goal) ? "G " : "  ";
        }
    for (const Tower& t : world.towers()) {
        const TowerType& def = world.towerTypes()[static_cast<std::size_t>(t.type)];
        cellText[idx(t.cell)] = std::string(1, def.glyph) + " ";
        cellFg[idx(t.cell)] = def.color;
    }
    for (const Projectile& p : world.projectiles()) {
        const GridPos c = Grid::worldToCell(p.pos);
        if (g.inBounds(c)) { cellText[idx(c)] = "* "; cellFg[idx(c)] = {255, 240, 120}; }
    }
    for (const Enemy& e : world.enemies()) {
        const GridPos c = Grid::worldToCell(e.pos);
        if (!g.inBounds(c)) continue;
        const EnemyType& def = world.enemyTypes()[static_cast<std::size_t>(e.type)];
        cellText[idx(c)] = std::string(1, def.glyph) + (e.slowTimer > 0.f ? "~" : " ");
        cellFg[idx(c)] = def.color;
    }

    for (int y = 0; y < g.height(); ++y) {
        out << " ";
        for (int x = 0; x < g.width(); ++x) {
            const GridPos p{x, y};
            const bool cursor = (p == view.cursor);
            out << bg(tileColor(g.at(p))) << fg(cellFg[idx(p)]);
            if (cursor) out << "\x1b[7m"; // vidéo inverse
            out << cellText[idx(p)] << kReset;
        }
        out << "\x1b[K\n";
    }

    // Informations sur la case sous le curseur
    out << "\n Cursor (" << view.cursor.x << "," << view.cursor.y << "): ";
    if (const Tower* t = world.towerAt(view.cursor)) {
        const TowerType& def = world.towerTypes()[static_cast<std::size_t>(t->type)];
        out << def.name << " | range " << def.range << " | dmg " << def.damage << " | kills " << t->kills
            << " | targeting " << targetingName(t->targeting) << " | sell for "
            << static_cast<int>(static_cast<float>(def.cost) * world.config().sellRefund) << "g";
    } else if (g.isBuildable(view.cursor)) {
        out << "free, buildable";
    } else {
        out << "not buildable";
    }
    out << "\x1b[K\n";

    // Boutique
    out << " ";
    for (std::size_t i = 0; i < world.towerTypes().size() && i < 9; ++i) {
        const TowerType& def = world.towerTypes()[i];
        const bool affordable = world.gold() >= def.cost;
        out << (affordable ? "" : "\x1b[2m") << "[" << (i + 1) << "] " << fg(def.color) << def.glyph << kReset
            << (affordable ? "" : "\x1b[2m") << " " << def.name << " " << def.cost << "g" << kReset << "   ";
    }
    out << "\x1b[K\n";
    out << " Keys: arrows/WASD move | 1-9 build | x sell | t targeting | n next wave | p pause | f speed | Esc quit\x1b[K\n";
    out << " " << (view.message.empty() ? "" : "> " + view.message) << "\x1b[K\n";
    out << "\x1b[J"; // efface ce qui reste sous la zone dessinée
    return out.str();
}

void TerminalRenderer::render(const World& world, const ViewState& view) {
    if (!active_) {
        active_ = true;
#if defined(_WIN32)
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (GetConsoleMode(h, &mode)) SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
        std::fputs("\x1b[?25l\x1b[2J", stdout); // cache le curseur, efface l'écran
    }
    const std::string f = frame(world, view);
    std::fwrite(f.data(), 1, f.size(), stdout);
    std::fflush(stdout);
}

// --- Entrées ---------------------------------------------------------------

struct TerminalInput::State {
#if !defined(_WIN32)
    termios saved{};
    bool rawMode = false;
#endif
};

TerminalInput::TerminalInput() : state_(new State) {
#if !defined(_WIN32)
    if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &state_->saved) == 0) {
        termios raw = state_->saved;
        raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) state_->rawMode = true;
    }
#endif
}

TerminalInput::~TerminalInput() {
#if !defined(_WIN32)
    if (state_->rawMode) tcsetattr(STDIN_FILENO, TCSANOW, &state_->saved);
#endif
    delete state_;
}

int TerminalInput::readKey() {
#if defined(_WIN32)
    if (!_kbhit()) return -1;
    int c = _getch();
    if (c == 0 || c == 224) {
        switch (_getch()) {
        case 72: return kKeyUp;
        case 80: return kKeyDown;
        case 75: return kKeyLeft;
        case 77: return kKeyRight;
        default: return -1;
        }
    }
    return c;
#else
    unsigned char c = 0;
    if (read(STDIN_FILENO, &c, 1) != 1) return -1;
    if (c != kKeyEsc) return c;
    unsigned char seq[2] = {0, 0};
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return kKeyEsc;
    if (seq[0] != '[' || read(STDIN_FILENO, &seq[1], 1) != 1) return kKeyEsc;
    switch (seq[1]) {
    case 'A': return kKeyUp;
    case 'B': return kKeyDown;
    case 'D': return kKeyLeft;
    case 'C': return kKeyRight;
    default: return -1;
    }
#endif
}

Command TerminalInput::translate(int key) {
    Command c;
    switch (key) {
    case kKeyUp: case 'w': case 'W': case 'z': case 'Z': c.type = CommandType::MoveCursor; c.dy = -1; break;
    case kKeyDown: case 's': case 'S': c.type = CommandType::MoveCursor; c.dy = 1; break;
    case kKeyLeft: case 'a': case 'A': case 'q': case 'Q': c.type = CommandType::MoveCursor; c.dx = -1; break;
    case kKeyRight: case 'd': case 'D': c.type = CommandType::MoveCursor; c.dx = 1; break;
    case 'x': case 'X': c.type = CommandType::Sell; break;
    case 'n': case 'N': c.type = CommandType::NextWave; break;
    case 'p': case 'P': case ' ': c.type = CommandType::TogglePause; break;
    case 'f': case 'F': c.type = CommandType::CycleSpeed; break;
    case 't': case 'T': c.type = CommandType::CycleTargeting; break;
    case kKeyEsc: case 3 /*Ctrl-C*/: c.type = CommandType::Quit; break;
    default:
        if (key >= '1' && key <= '9') { c.type = CommandType::Build; c.index = key - '1'; }
        break;
    }
    return c;
}

Command TerminalInput::poll() {
    const int key = readKey();
    return key < 0 ? Command{} : translate(key);
}

} // namespace td
