#include "game/run_stats.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace silic2 {

namespace {
std::string jsonString(const std::string& value) {
    std::ostringstream output;
    output << '"';
    for (unsigned char character : value) {
        if (character == '"' || character == '\\') {
            output << '\\' << character;
        } else if (character < 0x20) {
            output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                   << static_cast<unsigned int>(character) << std::dec;
        } else {
            output << character;
        }
    }
    output << '"';
    return output.str();
}

void writeReasons(std::ostream& output, const std::map<std::string, int>& reasons) {
    output << '{';
    bool first = true;
    for (const auto& reason : reasons) {
        if (!first) output << ',';
        output << jsonString(reason.first) << ':' << reason.second;
        first = false;
    }
    output << '}';
}
} // namespace

const char* toString(EnemyType type) {
    switch (type) {
        case EnemyType::Chaser: return "chaser";
        case EnemyType::Sentry: return "sentry";
        case EnemyType::Unknown: return "unknown";
        case EnemyType::Count: break;
    }
    return "unknown";
}

const char* toString(DamageSource source) {
    switch (source) {
        case DamageSource::EnemyContact: return "enemy_contact";
        case DamageSource::EnemyProjectile: return "enemy_projectile";
        case DamageSource::Environment: return "environment";
        case DamageSource::SelfDamage: return "self_damage";
        case DamageSource::Unknown: return "unknown";
        case DamageSource::Count: break;
    }
    return "unknown";
}

const char* toString(RoomResult result) {
    switch (result) {
        case RoomResult::Cleared: return "cleared";
        case RoomResult::PlayerDied: return "player_died";
        case RoomResult::Abandoned: return "abandoned";
    }
    return "abandoned";
}

const char* toString(RunEndReason reason) {
    switch (reason) {
        case RunEndReason::Completed: return "completed";
        case RunEndReason::PlayerDied: return "player_died";
        case RunEndReason::Restarted: return "restarted";
        case RunEndReason::Quit: return "quit";
    }
    return "quit";
}

bool RunStats::beginRun(uint64_t seed, uint32_t runIndex) {
    if (runActive) {
        return false;
    }

    data = RunStatsSnapshot{};
    data.seed = seed;
    data.runIndex = runIndex;
    roomId.clear();
    runActive = true;
    roomActive = false;
    return true;
}

bool RunStats::beginRoom(const std::string& nextRoomId) {
    if (!runActive || roomActive || nextRoomId.empty()) {
        return false;
    }

    roomId = nextRoomId;
    data.roomTime = 0.0f;
    roomActive = true;
    return true;
}

bool RunStats::endRoom(RoomResult result) {
    if (!runActive || !roomActive) {
        return false;
    }

    if (result == RoomResult::Cleared) {
        ++data.roomsCleared;
    }
    roomActive = false;
    return true;
}

bool RunStats::endRun(RunEndReason reason) {
    if (!runActive) {
        return false;
    }

    data.endReason = reason;
    roomActive = false;
    runActive = false;
    return true;
}

void RunStats::tick(float dt, bool simulationActive) {
    if (!runActive || !simulationActive || !std::isfinite(dt) || dt <= 0.0f) {
        return;
    }

    data.runTime += dt;
    if (roomActive) {
        data.roomTime += dt;
    }
}

void RunStats::recordKill(EnemyType type) {
    if (!runActive) return;
    if (static_cast<size_t>(type) >= data.killsByType.size()) type = EnemyType::Unknown;
    ++data.kills;
    ++data.killsByType[static_cast<size_t>(type)];
    data.lastKilledEnemy = type;
}

void RunStats::recordDamage(float actualAmount, DamageSource source, bool startsNewEvent) {
    if (!runActive || !std::isfinite(actualAmount)) return;

    const float amount = std::max(0.0f, actualAmount);
    size_t index = static_cast<size_t>(source);
    if (index >= data.damageEventsBySource.size()) {
        source = DamageSource::Unknown;
        index = static_cast<size_t>(source);
    }

    if (startsNewEvent) {
        ++data.damageEvents;
        ++data.damageEventsBySource[index];
        if (amount <= 0.0f) {
            ++data.blockedDamageEvents;
        }
    }
    data.damageTaken += amount;
    data.damageBySource[index] += amount;
    if (amount > 0.0f) data.lastDamageSource = source;
}

void RunStats::recordShot() {
    if (runActive) ++data.shotsFired;
}

void RunStats::recordHit() {
    if (runActive) ++data.shotsHit;
}

void RunStats::recordHostileShot() {
    if (runActive) ++data.hostileShots;
}

void RunStats::recordHostileHit() {
    if (runActive) ++data.hostileHits;
}

void RunStats::recordSpawnDelay(const std::string& reason) {
    if (!runActive) return;
    ++data.spawnDelays;
    ++data.spawnDelaysByReason[reason.empty() ? "unknown" : reason];
}

void RunStats::recordSpawnCancellation(const std::string& reason) {
    if (!runActive) return;
    ++data.spawnCancellations;
    ++data.spawnCancellationsByReason[reason.empty() ? "unknown" : reason];
}

void RunStats::recordFatalSource(DamageSource source) {
    if (!runActive) return;
    data.fatalSource = static_cast<size_t>(source) < static_cast<size_t>(DamageSource::Count)
        ? source : DamageSource::Unknown;
}

void RunStats::recordMovement(float distance, MovementState state, float dt) {
    if (!runActive) return;
    if (std::isfinite(distance) && distance > 0.0f) {
        data.distanceTravelled += distance;
    }
    if (state == MovementState::Sliding && std::isfinite(dt) && dt > 0.0f) {
        data.slidingTime += dt;
    }
}

std::string RunStats::formatDebugSummary(size_t liveEnemies, float playerHp, float fps) const {
    std::ostringstream output;
    output << std::fixed << std::setprecision(1)
           << "FPS " << fps
           << " | seed " << data.seed
           << " | run " << data.runTime << "s"
           << " | enemies " << liveEnemies
           << " | HP " << playerHp
           << " | shots " << data.shotsFired << "/" << data.shotsHit;
    return output.str();
}

std::string RunStats::formatRunResult() const {
    const float accuracy = data.shotsFired > 0
        ? static_cast<float>(data.shotsHit) / static_cast<float>(data.shotsFired)
        : 0.0f;

    std::ostringstream output;
    output << std::fixed << std::setprecision(3)
           << "{\"event\":\"run_end\""
           << ",\"content_version\":" << jsonString(data.contentVersion)
           << ",\"synthetic_input\":" << (data.syntheticInput ? "true" : "false")
           << ",\"acceptance_mode\":" << jsonString(data.acceptanceMode)
           << ",\"seed\":" << data.seed
           << ",\"run_index\":" << data.runIndex
           << ",\"reason\":\"" << toString(data.endReason) << "\""
           << ",\"run_time\":" << data.runTime
           << ",\"room_time\":" << data.roomTime
           << ",\"kills\":" << data.kills
           << ",\"shots_fired\":" << data.shotsFired
           << ",\"shots_hit\":" << data.shotsHit
           << ",\"hostile_shots\":" << data.hostileShots
           << ",\"hostile_hits\":" << data.hostileHits
           << ",\"accuracy\":" << accuracy
           << ",\"damage_events\":" << data.damageEvents
           << ",\"blocked_damage_events\":" << data.blockedDamageEvents
           << ",\"damage_taken\":" << data.damageTaken
           << ",\"distance\":" << data.distanceTravelled
           << ",\"sliding_time\":" << data.slidingTime
           << ",\"rooms_cleared\":" << data.roomsCleared
           << ",\"last_damage_source\":\"" << toString(data.lastDamageSource) << "\""
           << ",\"fatal_source\":\"" << toString(data.fatalSource) << "\""
           << ",\"kills_by_type\":{";
    for (size_t i = 0; i < data.killsByType.size(); ++i) {
        if (i) output << ',';
        output << '"' << toString(static_cast<EnemyType>(i)) << "\":" << data.killsByType[i];
    }
    output << "},\"damage_by_source\":{";
    for (size_t i = 0; i < data.damageBySource.size(); ++i) {
        if (i) output << ',';
        output << '"' << toString(static_cast<DamageSource>(i)) << "\":" << data.damageBySource[i];
    }
    output << "},\"spawn_delays\":" << data.spawnDelays
           << ",\"spawn_cancellations\":" << data.spawnCancellations
           << ",\"spawn_delays_by_reason\":";
    writeReasons(output, data.spawnDelaysByReason);
    output << ",\"spawn_cancellations_by_reason\":";
    writeReasons(output, data.spawnCancellationsByReason);
    output << '}';
    return output.str();
}

} // namespace silic2
