#pragma once

#include "game/gameplay_types.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace silic2 {

struct RunStatsSnapshot {
    bool syntheticInput = false;
    std::string acceptanceMode;
    std::string contentVersion = "two-minute-sample-v1";
    uint64_t seed = 0;
    uint32_t runIndex = 0;
    float runTime = 0.0f;
    float roomTime = 0.0f;
    float distanceTravelled = 0.0f;
    float slidingTime = 0.0f;
    int kills = 0;
    int shotsFired = 0;
    int shotsHit = 0;
    int hostileShots = 0;
    int hostileHits = 0;
    int spawnDelays = 0;
    int spawnCancellations = 0;
    int damageEvents = 0;
    int blockedDamageEvents = 0;
    float damageTaken = 0.0f;
    int roomsCleared = 0;
    EnemyType lastKilledEnemy = EnemyType::Unknown;
    DamageSource lastDamageSource = DamageSource::Unknown;
    DamageSource fatalSource = DamageSource::Unknown;
    RunEndReason endReason = RunEndReason::Quit;
    std::array<int, static_cast<size_t>(DamageSource::Count)> damageEventsBySource{};
    std::array<float, static_cast<size_t>(DamageSource::Count)> damageBySource{};
    std::array<int, static_cast<size_t>(EnemyType::Count)> killsByType{};
    std::map<std::string, int> spawnDelaysByReason;
    std::map<std::string, int> spawnCancellationsByReason;
};

class RunStats {
public:
    bool beginRun(uint64_t seed, uint32_t runIndex);
    bool beginRoom(const std::string& roomId);
    bool endRoom(RoomResult result);
    bool endRun(RunEndReason reason);

    void tick(float dt, bool simulationActive);
    void setExecutionContext(bool synthetic, const std::string& mode) { data.syntheticInput=synthetic; data.acceptanceMode=mode; }
    void setContentVersion(const std::string& version) { data.contentVersion=version; }
    void recordKill(EnemyType type);
    void recordDamage(float actualAmount, DamageSource source, bool startsNewEvent = true);
    void recordShot();
    void recordHit();
    void recordHostileShot();
    void recordHostileHit();
    void recordSpawnDelay(const std::string& reason = "unknown");
    void recordSpawnCancellation(const std::string& reason = "unknown");
    // Call only when actual damage kills the player; injury never implies death.
    void recordFatalSource(DamageSource source);
    void recordMovement(float distance, MovementState state, float dt);

    bool isRunActive() const { return runActive; }
    bool isRoomActive() const { return roomActive; }
    const std::string& getRoomId() const { return roomId; }
    const RunStatsSnapshot& snapshot() const { return data; }

    std::string formatDebugSummary(size_t liveEnemies, float playerHp, float fps) const;
    std::string formatRunResult() const;

private:
    RunStatsSnapshot data;
    std::string roomId;
    bool runActive = false;
    bool roomActive = false;
};

} // namespace silic2
