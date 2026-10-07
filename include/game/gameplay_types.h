#pragma once

#include <cstdint>

namespace silic2 {

enum class EnemyType : uint8_t {
    Unknown,
    Chaser,
    Sentry,
    Count
};

enum class DamageSource : uint8_t {
    EnemyContact,
    EnemyProjectile,
    Environment,
    SelfDamage,
    Unknown,
    Count
};

enum class RoomResult : uint8_t {
    Cleared,
    PlayerDied,
    Abandoned
};

enum class RunEndReason : uint8_t {
    Completed,
    PlayerDied,
    Restarted,
    Quit
};

enum class MovementState : uint8_t {
    Other,
    Sliding
};

const char* toString(EnemyType type);
const char* toString(DamageSource source);
const char* toString(RoomResult result);
const char* toString(RunEndReason reason);

} // namespace silic2
