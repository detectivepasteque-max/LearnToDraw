#include "td/Definitions.hpp"
#include "td/World.hpp"

namespace td {

const char* targetingName(Targeting t) {
    switch (t) {
    case Targeting::First: return "First";
    case Targeting::Last: return "Last";
    case Targeting::Nearest: return "Nearest";
    case Targeting::Strongest: return "Strongest";
    case Targeting::Weakest: return "Weakest";
    }
    return "?";
}

Targeting nextTargeting(Targeting t) {
    switch (t) {
    case Targeting::First: return Targeting::Last;
    case Targeting::Last: return Targeting::Nearest;
    case Targeting::Nearest: return Targeting::Strongest;
    case Targeting::Strongest: return Targeting::Weakest;
    case Targeting::Weakest: return Targeting::First;
    }
    return Targeting::First;
}

const char* buildResultName(BuildResult r) {
    switch (r) {
    case BuildResult::Ok: return "Built";
    case BuildResult::OutOfBounds: return "Out of bounds";
    case BuildResult::NotBuildable: return "Cannot build here";
    case BuildResult::Occupied: return "Cell already occupied";
    case BuildResult::NotEnoughGold: return "Not enough gold";
    case BuildResult::InvalidType: return "Unknown tower type";
    case BuildResult::GameOver: return "Game is over";
    }
    return "?";
}

} // namespace td
