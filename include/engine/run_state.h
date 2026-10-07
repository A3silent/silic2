#pragma once

#include <cstdint>

namespace silic2 {

enum class GameState {
    READY,
    PLAYING,       // normal combat
    ROOM_CLEARED,  // all enemies dead — hook for augment select (Phase 5.3)
    PLAYER_DEAD,   // player died — hook for death screen (Phase 5 later)
    PAUSED         // ESC toggle
};

struct RunState {
    uint64_t seed   = 0;
    uint32_t runIndex = 0;
    int act      = 1;
    int floor    = 1;
    int currency = 0;
    bool active  = false;
};

} // namespace silic2
