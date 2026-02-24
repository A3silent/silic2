#pragma once

namespace silic2 {

enum class GameState {
    PLAYING,       // normal combat
    ROOM_CLEARED,  // all enemies dead — hook for augment select (Phase 5.3)
    PLAYER_DEAD,   // player died — hook for death screen (Phase 5 later)
    PAUSED         // ESC toggle
};

struct RunState {
    int act      = 1;
    int floor    = 1;
    int currency = 0;
    // Phase 5.3: std::vector<AugmentType> augments;
};

} // namespace silic2
