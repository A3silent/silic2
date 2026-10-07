#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace silic2 {

enum class HudPhase { Ready, Playing, Paused, Won, Dead };
enum class HudAudioStatus { Available, Muted, Unavailable };

struct HudEnemy {
    glm::vec2 worldXZ{0.0f};
    // Normalized top-left screen coordinates, projected by the gameplay adapter.
    glm::vec2 healthAnchor{0.5f};
    float hp = 0.0f;
    float maxHp = 1.0f;
    float secondsSinceDamage = 1000.0f;
    bool alive = true;
    bool inView = false;
    bool unobstructed = false;
    bool sentry = false;
};

struct HudTerrain { glm::vec2 minXZ{0.0f}, maxXZ{0.0f}; };
struct HudRift { glm::vec2 worldXZ{0.0f}; bool warning = false; };

// Value-only snapshot. Gameplay owns all time, visibility, statistics and input.
// Build this from the same measurement snapshot used by JSONL logging.
struct HudViewModel {
    HudPhase phase = HudPhase::Ready;
    float hp = 100.0f, maxHp = 100.0f;
    float combatSeconds = 0.0f;
    bool cleanup = false;
    bool arenaSurvival = true;
    bool hasTransformation = true;
    float transformationStart = 45.0f, transformationDuration = 4.0f;
    int livingEnemies = 0;
    float resultLockRemaining = 0.0f;
    int kills = 0, shotsFired = 0, shotsHit = 0;
    float actualHpLoss = 0.0f;
    std::string fatalSource = "UNKNOWN";
    uint64_t seed = 42;
    HudAudioStatus audioStatus = HudAudioStatus::Available;
    // Ages use simulation time, stop on pause and reset on restart. No event = 1000.
    float hitAge = 1000.0f, killAge = 1000.0f, damageAge = 1000.0f;
    // Radians: 0 = forward/top, +pi/2 = right, pi = behind/bottom.
    float damageDirectionRadians = 0.0f;
    glm::vec2 playerXZ{0.0f}, playerForwardXZ{0.0f, -1.0f};
    glm::vec2 mapMinXZ{-18.0f}, mapMaxXZ{18.0f};
    std::vector<HudTerrain> terrain;
    std::vector<HudRift> rifts;
    std::vector<HudEnemy> enemies;
};

std::string hudPhaseLabel(const HudViewModel& model);
std::string hudObjectiveLabel(const HudViewModel& model);
std::string hudReadyObjective(const HudViewModel& model);

bool hudEnemyVisible(const HudEnemy& enemy);
bool hudEnemyBarVisible(const HudEnemy& enemy);
std::string hudClock(float seconds);
std::string hudAccuracy(int hits, int shots);
std::string hudHpLoss(float amount);
std::string hudFatalSource(const std::string& source);

} // namespace silic2
