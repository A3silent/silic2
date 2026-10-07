#include "game/run_stats.h"
#include "engine/simple_json.h"
#include "test_support.h"

#include <limits>

using silic2::DamageSource;
using silic2::EnemyType;
using silic2::MovementState;
using silic2::RoomResult;
using silic2::RunEndReason;
using silic2::RunStats;
using silic2::test::require;
using silic2::test::requireNear;

void runRunStatsTests() {
    RunStats stats;
    require(stats.beginRun(42, 1), "could not begin run");
    require(!stats.beginRun(43, 2), "second active run was accepted");
    require(stats.beginRoom("test_room"), "could not begin room");
    require(!stats.beginRoom("other_room"), "second active room was accepted");

    stats.tick(1.5f, true);
    stats.tick(10.0f, false);
    requireNear(stats.snapshot().runTime, 1.5f, 0.0001f,
                "paused time changed run time");
    requireNear(stats.snapshot().roomTime, 1.5f, 0.0001f,
                "paused time changed room time");

    stats.recordMovement(3.25f, MovementState::Sliding, 0.5f);
    requireNear(stats.snapshot().distanceTravelled, 3.25f, 0.0001f,
                "movement distance was not recorded");
    requireNear(stats.snapshot().slidingTime, 0.5f, 0.0001f,
                "sliding time was not recorded");

    stats.recordDamage(12.0f, DamageSource::EnemyContact);
    stats.recordDamage(0.0f, DamageSource::EnemyContact);
    stats.recordDamage(3.0f, DamageSource::EnemyContact, false);
    requireNear(stats.snapshot().damageTaken, 15.0f, 0.0001f,
                "actual HP loss was not recorded");
    require(stats.snapshot().damageEvents == 2, "damage event count is wrong");
    require(stats.snapshot().blockedDamageEvents == 1, "blocked damage was not counted");

    stats.recordShot();
    stats.recordHit();
    stats.recordKill(EnemyType::Chaser);
    require(stats.snapshot().shotsFired == 1, "shot was not counted");
    require(stats.snapshot().shotsHit == 1, "hit was not counted");
    require(stats.snapshot().kills == 1, "kill was not counted");

    require(stats.endRoom(RoomResult::Cleared), "could not end room");
    require(!stats.endRoom(RoomResult::Cleared), "room ended twice");
    require(stats.snapshot().roomsCleared == 1, "cleared room was not counted");
    require(stats.endRun(RunEndReason::Completed), "could not end run");
    require(!stats.endRun(RunEndReason::Completed), "run ended twice");

    require(stats.beginRun(77, 2), "could not restart after result");
    stats.recordDamage(7.0f, DamageSource::EnemyProjectile);
    stats.recordDamage(0.0f, DamageSource::EnemyContact);
    stats.recordDamage(-5.0f, DamageSource::Environment);
    stats.recordDamage(std::numeric_limits<float>::quiet_NaN(), DamageSource::SelfDamage);
    requireNear(stats.snapshot().damageTaken, 7.0f, 0.0001f,
                "blocked or invalid damage changed actual HP loss");
    require(stats.snapshot().lastDamageSource == DamageSource::EnemyProjectile,
            "blocked damage replaced last actual injury source");
    require(stats.snapshot().fatalSource == DamageSource::Unknown,
            "nonfatal injury invented a fatal source");
    require(stats.snapshot().damageEvents == 3, "invalid damage created an event");
    require(stats.snapshot().blockedDamageEvents == 2, "blocked event total is wrong");
    stats.recordKill(EnemyType::Chaser);
    stats.recordKill(EnemyType::Sentry);
    stats.recordKill(EnemyType::Sentry);
    require(stats.snapshot().killsByType[static_cast<size_t>(EnemyType::Chaser)] == 1,
            "Chaser kill count is wrong");
    require(stats.snapshot().killsByType[static_cast<size_t>(EnemyType::Sentry)] == 2,
            "Sentry kill count is wrong");
    stats.recordHostileShot();
    stats.recordHostileShot();
    stats.recordHostileHit();
    stats.recordShot();
    stats.recordShot();
    stats.recordHit();
    stats.recordSpawnDelay("unsafe");
    stats.recordSpawnDelay("cap");
    stats.recordSpawnDelay("unsafe");
    stats.recordSpawnCancellation("deadline");
    stats.recordFatalSource(DamageSource::EnemyProjectile);
    require(stats.endRun(RunEndReason::PlayerDied), "could not end death run");
    const std::string result = stats.formatRunResult();
    const auto json = silic2::SimpleJson::parse(result);
    require(json.isObject(), "run result is not a JSON object");
    require(json.getNumber("seed") == 77 && json.getNumber("run_index") == 2,
            "JSON lost run identity");
    require(json.getString("content_version") == stats.snapshot().contentVersion,
            "JSON lost content version");
    require(json.getString("reason") == "player_died" &&
            json.getString("fatal_source") == "enemy_projectile",
            "JSON lost explicit death source");
    require(json.getNumber("damage_taken") == 7 && json.getNumber("blocked_damage_events") == 2,
            "JSON actual damage statistics are wrong");
    require(json["kills_by_type"].getNumber("chaser") == 1 &&
            json["kills_by_type"].getNumber("sentry") == 2,
            "JSON per-type kill counts are wrong");
    require(json.getNumber("hostile_shots") == 2 && json.getNumber("hostile_hits") == 1,
            "JSON hostile projectile counts are wrong");
    require(json.getNumber("accuracy") == 0.5, "JSON player accuracy is wrong");
    require(json.getNumber("spawn_delays") == 3 &&
            json["spawn_delays_by_reason"].getNumber("unsafe") == 2 &&
            json["spawn_delays_by_reason"].getNumber("cap") == 1 &&
            json.getNumber("spawn_cancellations") == 1 &&
            json["spawn_cancellations_by_reason"].getNumber("deadline") == 1,
            "JSON spawn reason counts are wrong");
    require(result.find('\n') == std::string::npos, "result contains a JSONL-breaking newline");

    stats.recordDamage(99.0f, DamageSource::Environment);
    stats.recordKill(EnemyType::Chaser);
    stats.recordHostileShot();
    stats.recordHostileHit();
    stats.recordSpawnDelay("unsafe");
    stats.recordSpawnCancellation("deadline");
    stats.recordFatalSource(DamageSource::Environment);
    stats.tick(10.0f, true);
    require(!stats.endRun(RunEndReason::Completed), "death result could be overwritten");
    require(stats.formatRunResult() == result, "completed run statistics changed");

    require(stats.beginRun(77, 3), "could not restart after death");
    const auto& reset = stats.snapshot();
    require(reset.damageTaken == 0 && reset.kills == 0 && reset.hostileShots == 0 &&
            reset.hostileHits == 0 && reset.spawnDelays == 0 && reset.spawnCancellations == 0 &&
            reset.spawnDelaysByReason.empty() && reset.spawnCancellationsByReason.empty() &&
            reset.lastDamageSource == DamageSource::Unknown && reset.fatalSource == DamageSource::Unknown,
            "restart leaked previous run statistics");
    for (int count : reset.killsByType) require(count == 0, "restart leaked a per-type kill");
    stats.recordSpawnCancellation("quote\" slash\\ newline\n");
    const auto escaped = stats.formatRunResult();
    require(escaped.find("quote\\\" slash\\\\ newline\\u000a") != std::string::npos &&
            escaped.find('\n') == std::string::npos,
            "reason text was not escaped for JSONL");
    stats.recordDamage(2.0f, DamageSource::EnemyContact);
    stats.endRun(RunEndReason::PlayerDied);
    require(stats.snapshot().fatalSource == DamageSource::Unknown,
            "ending a death run inferred its fatal source from old injury");
}
