#include "game/encounter_director.h"
#include "test_support.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
using namespace silic2;
using silic2::test::require;

std::vector<DirectorEvent> runTimeline(const std::vector<double>& steps, uint64_t seed = 42) {
    EncounterDirector director;
    director.reset(seed);
    DirectorInput input;
    input.playerPosition = {0, 0, 8};
    std::vector<DirectorEvent> result;
    size_t index = 0;
    while (director.combatSeconds() < 91.0) {
        const double dt = std::min(steps[index++ % steps.size()], 91.0 - director.combatSeconds());
        auto events = director.update(dt, input);
        for (const auto& event : events) {
            if (event.kind == DirectorEvent::Kind::Spawn) {
                if (event.type == EnemyType::Chaser) ++input.liveChasers;
                else { ++input.liveSentries; input.occupiedSentrySlots[event.locationIndex] = true; }
            }
        }
        result.insert(result.end(), events.begin(), events.end());
        require(input.liveChasers <= EncounterDirector::ChaserCap, "Chaser cap must hold across calls");
        require(input.liveSentries <= EncounterDirector::SentryCap, "Sentry cap must hold across calls");
    }
    require(director.generationFinished(), "90s must end generation");
    require(director.pendingCount() == 0, "deadline must drain pending requests");
    require(director.metrics().spawnedChasers == 18, "surviving enemies should fill Chaser cap");
    require(director.metrics().spawnedSentries == 3, "three Sentry schedule entries");
    require(director.metrics().cancelledRequests == 14, "remaining 14 Chasers cancelled at cap");
    require(director.metrics().cancellationsByReason[static_cast<size_t>(SpawnDelayReason::ActiveCap)] == 14,
            "deadline cancellation retains last cap blocker");
    require(!director.complete(input.liveChasers + input.liveSentries), "cleanup requires no living enemies");
    require(director.complete(0), "cleared arena after deadline completes");
    return result;
}

void equalEvents(const std::vector<DirectorEvent>& a, const std::vector<DirectorEvent>& b) {
    require(a.size() == b.size(), "deterministic event count");
    for (size_t i = 0; i < a.size(); ++i) {
        require(a[i].kind == b[i].kind && a[i].requestId == b[i].requestId &&
                a[i].type == b[i].type && a[i].locationIndex == b[i].locationIndex &&
                a[i].reason == b[i].reason && glm::length(a[i].position - b[i].position) < 1e-6f &&
                std::abs(a[i].combatSeconds - b[i].combatSeconds) < 1e-8 &&
                std::abs(a[i].delaySeconds - b[i].delaySeconds) < 1e-8,
                "deterministic event identity, ordering, timing and location");
    }
}

void testNavigation() {
    const auto arena = ArenaDefinition::starryCrystal();
    require(arena.hasConnectedWaypoints(), "arena graph connected for default radius");
    require(arena.hasConnectedWaypoints(0.3f), "arena graph connected for Chaser radius");
    require(arena.isWalkable(arena.playerStart), "player start has clearance");
    for (const auto& rift : arena.rifts) require(arena.isWalkable(rift), "rift has clearance");
    for (const auto& sentry : arena.sentryPositions) require(arena.isWalkable(sentry), "Sentry has clearance");
    require(!arena.hasClearPath({0,0,8}, {0,0,-8}), "central crystal blocks direct pursuit");
    require(!arena.isWalkable({17.9f,0,0}), "boundary includes radius");
    require(!arena.isWalkable({8.1f,0,0}), "side cover blocks movement");
    for (const auto& start : arena.rifts) {
        for (const auto& destination : arena.waypoints) {
            auto current = start;
            for (int step = 0; step < 12 && glm::length(current - destination) > 1e-4f; ++step) {
                const auto next = arena.nextWaypoint(current, destination);
                require(arena.hasClearPath(current, next), "every selected path segment must be collision free");
                require(glm::length(next - current) > 1e-4f, "connected target cannot stall at a waypoint");
                current = next;
            }
            require(glm::length(current - destination) < 1e-4f, "all rifts reach all graph nodes");
        }
    }
    const auto left = arena.nextWaypoint({0,0,8}, {0,0,-8});
    require(glm::length(left - arena.nextWaypoint({0,0,8}, {0,0,-8})) < 1e-6f,
            "symmetric path ties are stable");
}

void testNavigationClearanceRecovery() {
    const auto arena = ArenaDefinition::starryCrystal();
    constexpr float navigationRadius = .34f;
    constexpr float bodyRadius = .30f;
    for (const glm::vec3 target : {glm::vec3{2.32f,0,0}, glm::vec3{17.68f,0,0}}) {
        auto current = glm::vec3{0,0,-10};
        require(arena.isWalkable(target, bodyRadius), "edge target must be physically legal");
        require(!arena.isWalkable(target, navigationRadius), "repro target is inside navigation margin");
        for (int step = 0; step < 12 && glm::length(current - target) > .1f; ++step) {
            const auto next = arena.nextWaypoint(current, target, navigationRadius);
            require(glm::length(next-current) > .01f, "legal edge target must not stop pursuit");
            require(arena.hasClearPath(current, next, navigationRadius), "projected target route stays outside inflated obstacles");
            current = next;
        }
        require(glm::length(current-target) < .1f, "pursuit reaches safe edge beside target");
    }
    for (const glm::vec3 start : {glm::vec3{2.32f,0,0}, glm::vec3{2.335f,0,0},
                                 glm::vec3{17.68f,0,0}, glm::vec3{17.665f,0,0}}) {
        const glm::vec3 target{0,0,-10};
        auto current = start;
        require(arena.isWalkable(start, bodyRadius), "recovery start must be physically legal");
        const auto recovery = arena.nextWaypoint(start, target, navigationRadius);
        require(glm::length(recovery-start) > .01f, "recovery crosses movement deadzone");
        require(arena.hasClearPath(start,recovery,bodyRadius), "recovery never crosses real collision geometry");
        require(arena.isWalkable(recovery,navigationRadius), "recovery exits navigation margin");
        current = recovery;
        for (int step = 0; step < 12 && glm::length(current-target) > .01f; ++step) {
            const auto next = arena.nextWaypoint(current,target,navigationRadius);
            require(arena.hasClearPath(current,next,navigationRadius), "recovered actor follows clear route");
            current = next;
        }
        require(glm::length(current-target) < .01f, "recovered actor reaches target");
    }
    require(arena.nextWaypoint({0,0,0}, {0,0,-10}, navigationRadius) == glm::vec3(0),
            "solid-interior origin cannot escape through geometry");
}

void testElevatedCoverTargets() {
    const auto arena = ArenaDefinition::starryCrystal();
    constexpr float radius = .34f;
    for (float coverX : {-9.0f, 9.0f}) {
        for (float height : {1.4f, 1.5f}) {
            const glm::vec3 top{coverX,height,0};
            require(!arena.isWalkable(top,radius), "top target must not make ground obstacle walkable");
            for (const auto& start : arena.rifts) {
                auto current = start;
                auto follow = [&](const glm::vec3& target, int frames) {
                    for (int i = 0; i < frames; ++i) {
                        const auto next = arena.nextWaypoint(current,target,radius);
                        auto delta = next-current;
                        delta.y = 0;
                        const float distance = glm::length(delta);
                        if (distance > 1e-5f) delta *= std::min(distance,3.0f/60.0f)/distance;
                        const auto moved = current+delta;
                        require(arena.hasClearPath(current,moved,radius), "per-frame elevated pursuit cannot cross cover");
                        current = moved;
                    }
                };
                require(glm::length(arena.nextWaypoint(start,top,radius)-start) > .01f,
                        "cover top must not stop a Chaser at a distant rift");
                follow(top,1200);
                require(glm::length(glm::vec2(current.x-coverX,current.z)) < 2.5f,
                        "every rift reaches a safe neighboring ground edge");
                const glm::vec3 landed{coverX,0,6};
                follow(landed,600);
                require(glm::length(current-landed) < .01f,
                        "Chaser resumes normal pursuit when player returns to ground");
            }
            require(arena.nextWaypoint({coverX,0,0},{0,0,-10},radius) == glm::vec3(coverX,0,0),
                    "elevated target handling cannot free a solid-interior origin");
            require(arena.nextWaypoint({coverX,0,-10},{coverX,1.0f,0},radius) == glm::vec3(coverX,0,-10),
                    "target inside the cover body is not a top target");
        }
    }
}

void testWarningsAndSafety() {
    EncounterDirector director;
    DirectorInput input;
    auto started = director.update(0.1, input);
    require(started.size() == 2, "time zero starts two warnings");
    require(started[0].kind == DirectorEvent::Kind::WarningStarted, "warn before spawn");
    require(started[0].locationIndex != 2 && started[1].locationIndex != 2,
            "initial nearby south rift must be excluded");
    require(started[0].locationIndex != started[1].locationIndex, "one concurrent warning per rift");
    const auto frozenWarnings = director.warningProgress();
    require(director.update(0, input).empty(), "pause emits no events");
    require(director.combatSeconds() == 0.1 && director.warningProgress() == frozenWarnings,
            "pause freezes clock and warnings");
    input.playerPosition = started[0].position;
    const auto atMaterialization = director.update(1.1, input);
    bool cancelledUnsafe = false;
    size_t actualSpawns = 0;
    for (const auto& event : atMaterialization) {
        if (event.kind == DirectorEvent::Kind::WarningCancelled && event.requestId == started[0].requestId)
            cancelledUnsafe = event.reason == SpawnDelayReason::UnsafePlayerDistance;
        if (event.kind == DirectorEvent::Kind::Spawn) {
            ++actualSpawns;
            require(glm::length(event.position - input.playerPosition) >= 5, "recheck before materialization");
        }
    }
    require(cancelledUnsafe && actualSpawns == 1, "entering a warned rift cancels only unsafe warning");
    input.liveChasers = actualSpawns;
    const auto retry = director.update(0.25, input);
    require(std::any_of(retry.begin(), retry.end(), [&](const DirectorEvent& event) {
        return event.requestId == started[0].requestId && event.kind == DirectorEvent::Kind::WarningStarted &&
               std::abs(event.combatSeconds - 1.45) < 1e-8 && event.locationIndex != started[0].locationIndex;
    }), "unsafe request retries after .25s at another legal rift");

    director.reset(42);
    input = {};
    director.update(0.1, input);
    input.liveChasers = 18;
    const auto capped = director.update(1.1, input);
    require(std::count_if(capped.begin(), capped.end(), [](const DirectorEvent& event) {
        return event.kind == DirectorEvent::Kind::WarningCancelled && event.reason == SpawnDelayReason::ActiveCap;
    }) == 2, "cap rechecked at warning completion");
    require(director.metrics().spawnedChasers == 0, "new cap prevents both materializations");
    director.reset(42);
    input = {};
    const auto resetEvents = director.update(.1, input);
    equalEvents(started, resetEvents);
    require(director.metrics().retryCount == 0 && director.metrics().cancelledRequests == 0,
            "restart clears all metrics");
}

void testDeadlineWarningAndSchedule() {
    EncounterDirector director;
    DirectorInput input;
    input.liveChasers = 18;
    input.liveSentries = 3;
    director.update(89, input);
    input.liveChasers = 0;
    auto warnings = director.update(.25, input);
    require(std::any_of(warnings.begin(), warnings.end(), [](const DirectorEvent& event) {
        return event.kind == DirectorEvent::Kind::WarningStarted;
    }), "released capacity may start warning before deadline");
    const auto deadline = director.update(.75, input);
    require(std::none_of(deadline.begin(), deadline.end(), [](const DirectorEvent& event) {
        return event.kind == DirectorEvent::Kind::Spawn;
    }), "warning due after deadline never materializes");
    require(director.pendingCount() == 0, "deadline cancels all queued generation");
    for (float warning : director.warningProgress()) require(warning == -1, "deadline clears all warnings");
    require(director.metrics().cancellationsByReason[static_cast<size_t>(SpawnDelayReason::Deadline)] > 0,
            "unfinished warnings carry deadline reason");
    require(director.update(10, input).empty(), "no generation after deadline");

    director.reset(42);
    input = {};
    // Clear each frame to verify total schedule in unconstrained play.
    for (int i = 0; i < 900; ++i) director.update(.1, input);
    require(director.metrics().spawnedChasers == 32 && director.metrics().spawnedSentries == 3,
            "full schedule contains 32 Chasers and 3 Sentries");
    require(director.complete(0), "unconstrained cleared schedule wins exactly at deadline");
}

void testBlockedLocations() {
    auto compact = ArenaDefinition::starryCrystal();
    compact.rifts = {{{0,0,0}, {1,0,0}, {0,0,1}, {-1,0,0}}};
    EncounterDirector unsafe(compact);
    DirectorInput input;
    input.playerPosition = {0,0,0};
    const auto blocked = unsafe.update(.1, input);
    require(blocked.size() == 2 && blocked[0].kind == DirectorEvent::Kind::Deferred &&
            blocked[0].reason == SpawnDelayReason::UnsafePlayerDistance,
            "all unsafe candidates defer without a warning");
    input.playerPosition = {0,0,10};
    const auto cleared = unsafe.update(.15, input);
    require(cleared.size() == 2 && cleared[0].kind == DirectorEvent::Kind::WarningStarted &&
            std::abs(cleared[0].combatSeconds - .25) < 1e-8,
            "unsafe candidates reconsidered on exact retry tick");

    EncounterDirector sentry;
    input = {};
    input.liveChasers = 18;
    input.occupiedSentrySlots = {{true, true, true}};
    const auto occupied = sentry.update(22.05, input);
    require(std::any_of(occupied.begin(), occupied.end(), [](const DirectorEvent& event) {
        return event.type == EnemyType::Sentry && event.reason == SpawnDelayReason::LocationBusy;
    }), "occupied Sentry slots cannot be reused even when total count is stale");
    input.occupiedSentrySlots[1] = false;
    const auto available = sentry.update(.2, input);
    require(std::any_of(available.begin(), available.end(), [](const DirectorEvent& event) {
        return event.kind == DirectorEvent::Kind::Spawn && event.type == EnemyType::Sentry &&
               event.locationIndex == 1 && std::abs(event.combatSeconds - 22.25) < 1e-8;
    }), "freed Sentry slot materializes on next retry");
}
} // namespace

void runEncounterDirectorTests() {
    testNavigation();
    testNavigationClearanceRecovery();
    testElevatedCoverTargets();
    testWarningsAndSafety();
    testDeadlineWarningAndSchedule();
    testBlockedLocations();
    const auto reference = runTimeline({1.0 / 60.0});
    equalEvents(reference, runTimeline({1.0 / 30.0}));
    equalEvents(reference, runTimeline({1.0 / 144.0}));
    equalEvents(reference, runTimeline({.007, .13, .016, .41, .033, .9}));
    equalEvents(reference, runTimeline({91.0}));
    equalEvents(reference, runTimeline({1.0 / 60.0}));
    const auto otherSeed = runTimeline({1.0 / 60.0}, 12345);
    bool different = reference.size() != otherSeed.size();
    for (size_t i = 0; !different && i < reference.size(); ++i)
        different = reference[i].locationIndex != otherSeed[i].locationIndex;
    require(different, "different seed changes candidate ordering");
}
