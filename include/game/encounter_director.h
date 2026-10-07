#pragma once

#include "game/arena_definition.h"
#include "game/gameplay_types.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace silic2 {

enum class SpawnDelayReason : uint8_t {
    None, UnsafePlayerDistance, ActiveCap, LocationBusy, Deadline, Count
};
const char* toString(SpawnDelayReason reason);

struct DirectorInput {
    glm::vec3 playerPosition{0.0f, 0.0f, 13.0f};
    size_t liveChasers = 0;
    size_t liveSentries = 0;
    std::array<bool, 3> occupiedSentrySlots{};
};

struct DirectorEvent {
    enum class Kind { WarningStarted, WarningCancelled, Spawn, Deferred, Cancelled };
    Kind kind = Kind::Deferred;
    EnemyType type = EnemyType::Chaser;
    uint32_t requestId = 0; // Stable, nonzero audio source ID within a run.
    double combatSeconds = 0.0;
    double scheduledSeconds = 0.0;
    double delaySeconds = 0.0; // Beyond earliest possible materialization.
    glm::vec3 position{0.0f};
    int locationIndex = -1;
    SpawnDelayReason reason = SpawnDelayReason::None;
};

struct DirectorMetrics {
    size_t spawnedChasers = 0;
    size_t spawnedSentries = 0;
    size_t delayedRequests = 0;
    size_t retryCount = 0;
    size_t cancelledRequests = 0;
    double totalSpawnDelaySeconds = 0.0;
    std::array<size_t, static_cast<size_t>(SpawnDelayReason::Count)> retriesByReason{};
    // At deadline, the last blocker is retained; an active warning uses Deadline.
    std::array<size_t, static_cast<size_t>(SpawnDelayReason::Count)> cancellationsByReason{};
};

class EncounterDirector {
public:
    static constexpr double GenerationDeadline = 90.0;
    static constexpr double WarningDuration = 1.2;
    static constexpr double RetryInterval = 0.25;
    static constexpr size_t ChaserCap = 18;
    static constexpr size_t SentryCap = 3;

    explicit EncounterDirector(ArenaDefinition arena = ArenaDefinition::starryCrystal());
    void reset(uint64_t seed);
    // Call only while PLAYING. dt <= 0 is a no-op, including at time zero.
    // Consume all Spawn events before the next call and supply fresh live counts.
    std::vector<DirectorEvent> update(double dt, const DirectorInput& input);
    double combatSeconds() const { return elapsed; }
    size_t pendingCount() const;
    bool generationFinished() const { return elapsed >= GenerationDeadline; }
    bool complete(size_t livingEnemies) const;
    std::array<float, 4> warningProgress() const; // -1 inactive; [0,1] active
    const DirectorMetrics& metrics() const { return counters; }
    const ArenaDefinition& arenaDefinition() const { return arena; }

private:
    struct Request {
        uint32_t id = 0;
        EnemyType type = EnemyType::Chaser;
        double scheduled = 0.0;
        double nextAttempt = 0.0;
        double warningStart = -1.0;
        std::array<int, 4> candidateOrder{};
        int location = -1;
        bool finished = false;
        bool delayed = false;
        SpawnDelayReason lastBlocker = SpawnDelayReason::None;
    };
    ArenaDefinition arena;
    std::vector<Request> requests;
    DirectorMetrics counters;
    double elapsed = 0.0;
};

} // namespace silic2
