#include "game/encounter_director.h"
#include "game/seed_service.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace silic2 {

const char* toString(SpawnDelayReason reason) {
    switch (reason) {
        case SpawnDelayReason::None: return "none";
        case SpawnDelayReason::UnsafePlayerDistance: return "unsafe_player_distance";
        case SpawnDelayReason::ActiveCap: return "active_cap";
        case SpawnDelayReason::LocationBusy: return "location_busy";
        case SpawnDelayReason::Deadline: return "deadline";
        case SpawnDelayReason::Count: break;
    }
    return "unknown";
}

EncounterDirector::EncounterDirector(ArenaDefinition definition) : arena(std::move(definition)) {
    reset(42);
}

void EncounterDirector::reset(uint64_t seed) {
    elapsed = 0.0;
    counters = {};
    requests.clear();
    SeedService random(seed);
    constexpr std::array<int, 8> times{{0, 10, 22, 34, 46, 58, 70, 82}};
    constexpr std::array<int, 8> chasers{{2, 3, 3, 4, 4, 5, 5, 6}};
    uint32_t id = 1;
    auto add = [&](EnemyType type, int time) {
        Request request;
        request.id = id++;
        request.type = type;
        request.scheduled = request.nextAttempt = time;
        request.candidateOrder = {{0, 1, 2, 3}};
        const int count = type == EnemyType::Chaser ? 4 : 3;
        for (int i = count - 1; i > 0; --i) {
            const int swapIndex = random.nextInt("arena.spawn_order", 0, i);
            std::swap(request.candidateOrder[i], request.candidateOrder[swapIndex]);
        }
        requests.push_back(request);
    };
    for (size_t wave = 0; wave < times.size(); ++wave) {
        for (int i = 0; i < chasers[wave]; ++i) add(EnemyType::Chaser, times[wave]);
        if (wave == 2 || wave == 4 || wave == 6) add(EnemyType::Sentry, times[wave]);
    }
}

std::vector<DirectorEvent> EncounterDirector::update(double dt, const DirectorInput& input) {
    std::vector<DirectorEvent> events;
    if (!(dt > 0.0) || !std::isfinite(dt)) return events;
    const double target = elapsed + dt;
    DirectorInput current = input;
    auto emit = [&](const Request& request, DirectorEvent::Kind kind, double time,
                    SpawnDelayReason reason = SpawnDelayReason::None) {
        DirectorEvent event;
        event.kind = kind;
        event.type = request.type;
        event.requestId = request.id;
        event.combatSeconds = time;
        event.scheduledSeconds = request.scheduled;
        event.delaySeconds = std::max(0.0, time - request.scheduled -
            (request.type == EnemyType::Chaser ? WarningDuration : 0.0));
        event.locationIndex = request.location;
        if (request.location >= 0) event.position = request.type == EnemyType::Chaser ?
            arena.rifts[request.location] : arena.sentryPositions[request.location];
        event.reason = reason;
        events.push_back(event);
    };
    auto defer = [&](Request& request, double time, SpawnDelayReason reason) {
        if (request.warningStart >= 0.0) {
            emit(request, DirectorEvent::Kind::WarningCancelled, time, reason);
            request.warningStart = -1.0;
        }
        if (!request.delayed) {
            request.delayed = true;
            ++counters.delayedRequests;
        }
        ++counters.retryCount;
        ++counters.retriesByReason[static_cast<size_t>(reason)];
        request.lastBlocker = reason;
        emit(request, DirectorEvent::Kind::Deferred, time, reason);
        request.location = -1;
        request.nextAttempt = time + RetryInterval;
    };

    // Event-time processing avoids losing warnings or waves in a long frame.
    // Stable request ordering resolves simultaneous attempts consistently.
    for (;;) {
        Request* selected = nullptr;
        for (auto& request : requests) {
            if (!request.finished && request.nextAttempt < GenerationDeadline &&
                request.nextAttempt <= target + 1e-9 &&
                (!selected || request.nextAttempt < selected->nextAttempt - 1e-9)) selected = &request;
        }
        if (!selected) break;
        auto& request = *selected;
        const double time = request.nextAttempt;
        const bool chaser = request.type == EnemyType::Chaser;
        if ((chaser ? current.liveChasers : current.liveSentries) >= (chaser ? ChaserCap : SentryCap)) {
            defer(request, time, SpawnDelayReason::ActiveCap);
            continue;
        }
        auto safe = [&](int index) {
            const auto delta = arena.rifts[index] - input.playerPosition;
            return arena.spatialWorld ? glm::dot(delta,delta)>=25.0f : delta.x * delta.x + delta.z * delta.z >= 25.0f;
        };
        if (chaser && request.warningStart >= 0.0) {
            if (!safe(request.location)) {
                defer(request, time, SpawnDelayReason::UnsafePlayerDistance);
                continue;
            }
        } else {
            bool anySafe = !chaser;
            for (int candidate = 0; candidate < (chaser ? 4 : 3); ++candidate) {
                const int index = request.candidateOrder[candidate];
                if (chaser && !safe(index)) continue;
                anySafe = true;
                bool busy = !chaser && current.occupiedSentrySlots[index];
                if (chaser) {
                    for (const auto& other : requests) {
                        if (!other.finished && other.warningStart >= 0.0 && other.location == index) busy = true;
                    }
                }
                if (!busy) { request.location = index; break; }
            }
            if (request.location < 0) {
                defer(request, time, anySafe ? SpawnDelayReason::LocationBusy : SpawnDelayReason::UnsafePlayerDistance);
                continue;
            }
            if (chaser) {
                request.warningStart = time;
                request.nextAttempt = time + WarningDuration;
                request.lastBlocker = SpawnDelayReason::None;
                emit(request, DirectorEvent::Kind::WarningStarted, time);
                continue;
            }
        }
        emit(request, DirectorEvent::Kind::Spawn, time);
        counters.totalSpawnDelaySeconds += events.back().delaySeconds;
        if (chaser) { ++current.liveChasers; ++counters.spawnedChasers; }
        else {
            ++current.liveSentries;
            ++counters.spawnedSentries;
            current.occupiedSentrySlots[request.location] = true;
        }
        request.finished = true;
        request.warningStart = -1.0;
    }
    if (target >= GenerationDeadline - 1e-9) {
        for (auto& request : requests) {
            if (request.finished) continue;
            const auto reason = request.warningStart >= 0.0 || request.lastBlocker == SpawnDelayReason::None ?
                SpawnDelayReason::Deadline : request.lastBlocker;
            if (request.warningStart >= 0.0) emit(request, DirectorEvent::Kind::WarningCancelled, GenerationDeadline, reason);
            emit(request, DirectorEvent::Kind::Cancelled, GenerationDeadline, reason);
            ++counters.cancelledRequests;
            ++counters.cancellationsByReason[static_cast<size_t>(reason)];
            request.finished = true;
            request.warningStart = -1.0;
        }
    }
    elapsed = target >= GenerationDeadline - 1e-9 && target < GenerationDeadline ? GenerationDeadline : target;
    return events;
}

size_t EncounterDirector::pendingCount() const {
    return static_cast<size_t>(std::count_if(requests.begin(), requests.end(),
        [](const Request& request) { return !request.finished; }));
}

bool EncounterDirector::complete(size_t livingEnemies) const {
    return generationFinished() && pendingCount() == 0 && livingEnemies == 0;
}

std::array<float, 4> EncounterDirector::warningProgress() const {
    std::array<float, 4> result{{-1, -1, -1, -1}};
    for (const auto& request : requests) {
        if (!request.finished && request.warningStart >= 0.0 && request.location >= 0) {
            result[request.location] = static_cast<float>(std::clamp((elapsed - request.warningStart) / WarningDuration, 0.0, 1.0));
        }
    }
    return result;
}

} // namespace silic2
