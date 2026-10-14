#pragma once
#include "engine/map.h"
#include <cstddef>
#include <string>
#include <vector>
namespace silic2 {
enum class ExplorationPhase { Approach, Encounter, Observe, Return, Complete };
struct ExplorationInput {
    glm::vec3 feet{0}, eye{0}, forward{0,0,-1};
    size_t livingEnemies=0;
};
struct ExplorationSnapshot {
    ExplorationPhase phase=ExplorationPhase::Approach;
    std::string zoneName="LIGHT BAFFLE";
    float observationProgress=0;
    bool calibrated=false, observing=false;
};
struct ExplorationEvents {
    std::vector<ExplorationSpawn> spawns;
    bool calibrated=false, completed=false;
};
// Owns a single finite journey. It cannot mutate the world or complete through rendering.
// Call update only during PLAYING; inactive frames preserve every timer and latch.
class ExplorationController {
public:
    explicit ExplorationController(const Map& map);
    void reset();
    ExplorationEvents update(float dt,bool active,const ExplorationInput& input);
    const ExplorationSnapshot& snapshot() const { return state; }
    std::string objective() const;
    bool sightlineClear(glm::vec3 eye) const;
private:
    ExplorationSettings settings;
    std::vector<RegionBounds> solids;
    ExplorationSnapshot state;
    float held=0;
    bool exitOccupied=false;
};
}
