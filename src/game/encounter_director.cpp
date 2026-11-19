#include "game/encounter_director.h"
#include "game/seed_service.h"
#include "enemy/crystal_combat.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <utility>

namespace silic2 {

const char* toString(SpawnDelayReason reason) {
    switch (reason) {
        case SpawnDelayReason::None: return "none";
        case SpawnDelayReason::UnsafePlayerDistance: return "unsafe_player_distance";
        case SpawnDelayReason::ActiveCap: return "active_cap";
        case SpawnDelayReason::LocationBusy: return "location_busy";
        case SpawnDelayReason::Deadline: return "deadline";
        case SpawnDelayReason::StageChanged: return "stage_changed";
        case SpawnDelayReason::Count: break;
    }
    return "unknown";
}

namespace {
struct Wave {
    int second;
    int chasers, sentries, hounds, rays, censers;
};
// The opening preserves one readable introduction to each move. From 45s on,
// the pulse table supplies ordinary enemies as well as the later heavy waves.
constexpr Wave crystalOpening[] = {
    {0,2,0,0,0,0}, {12,0,0,1,0,0}, {25,3,1,0,0,0}, {36,0,0,0,1,0},
    {50,0,0,1,0,0}, {62,0,0,0,0,1}, {76,0,0,1,1,0}
};
struct Pressure {
    double interval;
    EnemyCounts caps;
    EnemyCounts perPulse;
    EnemyCounts every; // e.g. 3 means every third pulse.
};
constexpr Pressure stages[] = {
    //                 Chaser, Sentry, Hound, Ray, Censer (element zero is Unknown).
    {45,   {0,10,3,2,2,1},   {},             {}},
    {7.5,  {0,10,3,2,2,1},   {0,5,1,0,0,0}, {0,1,4,0,0,0}},
    {5,    {0, 6,3,6,5,3},    {0,2,1,1,1,1}, {0,2,4,1,1,2}},
    {3,    {0, 2,3,10,10,6},  {0,1,1,1,2,1}, {0,5,5,1,1,1}},
    {1.8,  {0, 0,3,15,15,10}, {0,0,1,1,3,2}, {0,0,6,1,1,1}}
};
unsigned stageAt(double seconds) {
    return static_cast<unsigned>(std::min(4.0,std::floor((seconds+1.e-9)/EncounterDirector::PressureInterval)));
}
bool newCreature(EnemyType type) {
    return type == EnemyType::ForkHound || type == EnemyType::VeilRay || type == EnemyType::HangingCenser;
}

AABB spawnBounds(EnemyType type, glm::vec3 feet) {
    // All creatures materialize looking along -Z. Width therefore runs along X.
    glm::vec3 size;
    switch (type) {
        case EnemyType::VeilRay: size = crystalCombat::RaySize; break;
        case EnemyType::HangingCenser: size = crystalCombat::CenserSize; break;
        case EnemyType::ForkHound: size = crystalCombat::HoundSize; break;
        case EnemyType::Sentry: size = {1.2f,1.f,1.2f}; break;
        default: size = {.6f,1.8f,.6f}; break;
    }
    return AABB::fromCenterHalfSize(feet + glm::vec3(0,size.y*.5f,0), size*.5f);
}

// Separating-axis triangle/box test for the vertical takeoff corridor. The
// bounds of a sloping wall alone can cover empty space beside a rift.
bool intersectsSurface(const AABB& box, const SpatialTriangle& triangle) {
    if (glm::any(glm::greaterThan(glm::min(triangle.a,glm::min(triangle.b,triangle.c)),box.max)) ||
        glm::any(glm::lessThan(glm::max(triangle.a,glm::max(triangle.b,triangle.c)),box.min))) return false;
    const auto center=box.getCenter(), half=box.getSize()*.5f;
    const auto a=triangle.a-center, b=triangle.b-center, c=triangle.c-center;
    auto separated=[&](glm::vec3 axis) {
        const float radius=glm::dot(glm::abs(axis),half);
        const float x=glm::dot(a,axis), y=glm::dot(b,axis), z=glm::dot(c,axis);
        return std::min({x,y,z})>radius || std::max({x,y,z}) < -radius;
    };
    if (separated(triangle.normal)) return false;
    for (auto edge : {b-a,c-b,a-c}) {
        if (separated({0,edge.z,-edge.y}) || separated({-edge.z,0,edge.x}) ||
            separated({edge.y,-edge.x,0})) return false;
    }
    return true;
}

bool hasTakeoffSpace(const ArenaDefinition& arena, glm::vec3 feet, EnemyType type) {
    AABB corridor=spawnBounds(type,feet);
    // The actor may touch its supporting floor. Test its full width above the
    // floor all the way to cruising height without inflating its vertical size.
    corridor.min.y+=.01f;
    const float altitude=type==EnemyType::VeilRay ? crystalCombat::RayAltitude : crystalCombat::CenserAltitude;
    corridor.max.y+=std::max(0.f,arena.floorY+altitude-feet.y);
    const auto center=(arena.minXZ+arena.maxXZ)*.5f;
    const auto extent=glm::min((arena.maxXZ-arena.minXZ)*.5f,glm::vec2(crystalCombat::CrystalFlightRadius));
    for (float x : {corridor.min.x,corridor.max.x}) {
        for (float z : {corridor.min.z,corridor.max.z}) {
            auto corner=(glm::vec2(x,z)-center)/extent;
            if (glm::dot(corner,corner)>=1.f) return false;
        }
    }
    if (arena.spatialWorld) {
        const auto& surfaces=arena.spatialWorld->triangles();
        return std::none_of(surfaces.begin(),surfaces.end(),
                            [&](const auto& t){return intersectsSurface(corridor,t);});
    }
    return std::none_of(arena.obstacles.begin(),arena.obstacles.end(),[&](const auto& o){
        return corridor.intersects({o.minimum,o.maximum});
    });
}
}

EncounterDirector::EncounterDirector(ArenaDefinition definition) : arena(std::move(definition)) {
    if (arena.encounterProfile==EncounterProfile::CrystalSurvival) {
        for (auto feet : arena.rifts)
            takeoffClear.push_back({hasTakeoffSpace(arena,feet,EnemyType::VeilRay),
                                    hasTakeoffSpace(arena,feet,EnemyType::HangingCenser)});
    }
    reset(42);
}

double EncounterDirector::generationDeadline() const {
    return isEndless() ? std::numeric_limits<double>::infinity() : GenerationDeadline;
}
unsigned EncounterDirector::pressureStage() const {
    return isEndless() ? stageAt(elapsed) : 0;
}
size_t EncounterDirector::crystalCap(EnemyType type,unsigned stage) {
    const auto& caps=stages[std::min(4u,stage)].caps;
    return size_t(type)<caps.size() ? caps[size_t(type)] : 0;
}
size_t EncounterDirector::crystalMaxAlive(EnemyType type) {
    if (size_t(type)>=EnemyCounts{}.size()) return 0;
    size_t maximum=0;
    for (const auto& stage:stages)
        maximum=std::max(maximum,stage.caps[size_t(type)]);
    return maximum;
}
size_t EncounterDirector::cap(EnemyType type,double time) const {
    if(isEndless()) return crystalCap(type,stageAt(time));
    return type==EnemyType::Chaser ? ChaserCap : SentryCap;
}

bool EncounterDirector::legalRift(EnemyType type, int slot, glm::vec3 player) const {
    const auto start = arena.rifts[slot];
    const auto delta = start - player;
    if (!newCreature(type)) {
        return (arena.spatialWorld ? glm::dot(delta,delta) : delta.x*delta.x + delta.z*delta.z) >= 25.f;
    }
    if (delta.x*delta.x + delta.z*delta.z < 18.f*18.f) return false;
    // Crystal has one contiguous flat floor. Test the actual solid route,
    // including the central spire, at body height above that floor.
    const glm::vec3 lift(0,1.f,0);
    return arena.spatialWorld ? !arena.spatialWorld->raycast(start + lift, player + lift, .65f).hit :
                                arena.hasClearPath(start, player, .65f);
}

void EncounterDirector::enqueue(EnemyType type,double time) {
    Request request;
    request.id=nextId++;
    request.type=type;
    request.scheduled=request.nextAttempt=time;
    const bool allRifts=newCreature(type) || (isEndless() && time>=PressureInterval);
    const int count=type==EnemyType::Sentry ? int(arena.sentryPositions.size()) :
                    allRifts ? int(arena.rifts.size()) : std::min(4,int(arena.rifts.size()));
    request.candidateOrder.resize(count);
    std::iota(request.candidateOrder.begin(),request.candidateOrder.end(),0);
    for(int i=count-1;i>0;--i) {
        int slot=random.nextInt("arena.spawn_order",0,i);
        std::swap(request.candidateOrder[i],request.candidateOrder[slot]);
    }
    requests.push_back(std::move(request));
}

void EncounterDirector::nextCrystalPulse(const EnemyCounts& live) {
    const auto stage=stageAt(nextPulse);
    const auto& tuning=stages[stage];
    EnemyCounts reserved=live;
    for(const auto& request:requests)
        if(!request.finished && request.scheduled<=nextPulse) ++reserved[size_t(request.type)];
    // Check live actors and older outstanding warnings before asking for more.
    // Enemy kills free capacity on the next pulse; blocked rifts never build an infinite backlog.
    for(size_t i=1;i<reserved.size();++i) {
        if(!tuning.every[i] || pulseInStage%tuning.every[i]) continue;
        for(size_t j=0;j<tuning.perPulse[i] && reserved[i]<tuning.caps[i];++j,++reserved[i])
            enqueue(static_cast<EnemyType>(i),nextPulse);
    }
    ++pulseInStage;
    const double period=std::floor((nextPulse+1.e-9)/PressureInterval);
    const double boundary=(period+1)*PressureInterval;
    // Long survivors still see faster reinforcement after 3:45, 4:30, etc.
    // Actor budgets and the rift's full warning time remain hard limits.
    const double acceleration=std::max(.38,std::pow(.78,std::max(0.,period-4.)));
    nextPulse+=tuning.interval*acceleration;
    if(nextPulse>=boundary-1.e-8) { nextPulse=boundary; pulseInStage=0; }
}

void EncounterDirector::reset(uint64_t seed) {
    elapsed=0;
    counters={};
    requests.clear();
    random.reset(seed);
    nextId=1;
    nextPulse=PressureInterval;
    pulseInStage=0;
    constexpr std::array<int,8> times{{0,10,22,34,46,58,70,82}};
    constexpr std::array<int,8> chasers{{2,3,3,4,4,5,5,6}};
    if (arena.encounterProfile == EncounterProfile::CrystalSurvival) {
        for (const Wave& w : crystalOpening) {
            for (int i=0; i<w.chasers; ++i) enqueue(EnemyType::Chaser,w.second);
            for (int i=0; i<w.sentries; ++i) enqueue(EnemyType::Sentry,w.second);
            for (int i=0; i<w.hounds; ++i) enqueue(EnemyType::ForkHound,w.second);
            for (int i=0; i<w.rays; ++i) enqueue(EnemyType::VeilRay,w.second);
            for (int i=0; i<w.censers; ++i) enqueue(EnemyType::HangingCenser,w.second);
        }
        return;
    }
    for (size_t wave = 0; wave < times.size(); ++wave) {
        for (int i = 0; i < chasers[wave]; ++i) enqueue(EnemyType::Chaser, times[wave]);
        if (wave == 2 || wave == 4 || wave == 6) enqueue(EnemyType::Sentry, times[wave]);
    }
}

std::vector<DirectorEvent> EncounterDirector::update(double dt, const DirectorInput& input) {
    std::vector<DirectorEvent> events;
    if (!(dt > 0.0) || !std::isfinite(dt)) return events;
    const double target = elapsed + dt;
    if ((!isEndless() && requests.empty()) || elapsed >= generationDeadline()) { elapsed = target; return events; }
    DirectorInput current = input;
    auto& counts = current.liveByType;
    counts[size_t(EnemyType::Chaser)] = std::max(counts[size_t(EnemyType::Chaser)], current.liveChasers);
    counts[size_t(EnemyType::Sentry)] = std::max(counts[size_t(EnemyType::Sentry)], current.liveSentries);
    if (arena.encounterProfile == EncounterProfile::Classic90)
        current.occupiedRifts.clear();
    current.occupiedRifts.resize(arena.rifts.size(), false);
    auto locationBusy=[&](EnemyType type,int index) {
        if (current.occupiedRifts[index]) return true;
        if (arena.encounterProfile!=EncounterProfile::CrystalSurvival) return false;
        if (type==EnemyType::VeilRay && !takeoffClear[index][0]) return true;
        if (type==EnemyType::HangingCenser && !takeoffClear[index][1]) return true;
        const auto body=spawnBounds(type,arena.rifts[index]);
        return std::any_of(current.liveBounds.begin(),current.liveBounds.end(),
                           [&](const AABB& other){return body.intersects(other);});
    };
    const double deadline = generationDeadline();
    auto emit = [&](const Request& request, DirectorEvent::Kind kind, double time,
                    SpawnDelayReason reason = SpawnDelayReason::None) {
        DirectorEvent event;
        event.kind = kind;
        event.type = request.type;
        event.requestId = request.id;
        event.combatSeconds = time;
        event.scheduledSeconds = request.scheduled;
        event.delaySeconds = std::max(0.0, time - request.scheduled -
            (request.type == EnemyType::Sentry ? 0.0 : WarningDuration));
        event.locationIndex = request.location;
        if (request.location >= 0) event.position = request.type != EnemyType::Sentry ?
            arena.rifts[request.location] : arena.sentryPositions[request.location];
        event.reason = reason;
        events.push_back(event);
    };
    auto cancel = [&](Request& request,double time,SpawnDelayReason reason) {
        if (request.warningStart >= 0.0) emit(request,DirectorEvent::Kind::WarningCancelled,time,reason);
        emit(request,DirectorEvent::Kind::Cancelled,time,reason);
        ++counters.cancelledRequests;
        ++counters.cancellationsByReason[static_cast<size_t>(reason)];
        request.finished=true;
        request.warningStart=-1.0;
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
            if (!request.finished && request.nextAttempt < deadline &&
                request.nextAttempt <= target + 1e-9 &&
                (!selected || request.nextAttempt < selected->nextAttempt - 1e-9)) selected = &request;
        }
        if (isEndless() && nextPulse<=target+1.e-9 &&
            (!selected || nextPulse<=selected->nextAttempt+1.e-9)) {
            // A shrinking quota belongs to the new roster. Release even lit rifts
            // from the previous stage before reserving its replacements.
            const unsigned nextStage=stageAt(nextPulse);
            if (nextStage>0 && crystalCap(EnemyType::Chaser,nextStage)<crystalCap(EnemyType::Chaser,nextStage-1)) {
                for (auto& request:requests) {
                    if (!request.finished && request.type==EnemyType::Chaser && stageAt(request.scheduled)<nextStage)
                        cancel(request,nextPulse,SpawnDelayReason::StageChanged);
                }
            }
            nextCrystalPulse(counts);
            continue;
        }
        if (!selected) break;
        auto& request = *selected;
        const double time = request.nextAttempt;
        const bool usesRift = request.type != EnemyType::Sentry;
        if (counts[size_t(request.type)] >= cap(request.type,time)) {
            defer(request, time, SpawnDelayReason::ActiveCap);
            continue;
        }
        if (request.warningStart >= 0.0) {
            if (!legalRift(request.type, request.location, input.playerPosition)) {
                defer(request, time, SpawnDelayReason::UnsafePlayerDistance);
                continue;
            }
            if (locationBusy(request.type,request.location)) {
                defer(request, time, SpawnDelayReason::LocationBusy);
                continue;
            }
        } else {
            bool anySafe = !usesRift;
            int fallback = -1;
            float best=std::numeric_limits<float>::infinity();
            const bool pressureRift=isEndless() && usesRift && time>=PressureInterval;
            // Some mouths target the current position, others intercept progress along the lane.
            const auto focus=input.playerPosition+input.playerVelocity*(.7f*float(request.id%4));
            for (int index : request.candidateOrder) {
                if (usesRift && !legalRift(request.type, index, input.playerPosition)) continue;
                anySafe = true;
                if (!usesRift && current.occupiedSentrySlots[index]) continue;
                if (usesRift) {
                    if (locationBusy(request.type,index)) continue;
                    const auto reserved = std::any_of(requests.begin(), requests.end(), [&](const Request& other) {
                        return !other.finished && other.warningStart >= 0.0 && other.location == index;
                    });
                    if (reserved) continue;
                    if (pressureRift) {
                        const float range=glm::distance(arena.rifts[index],input.playerPosition);
                        // Keep the large crowd in the fight; distant legal rifts still serve an
                        // edited arena with no reachable close candidate.
                        const float score=glm::distance(arena.rifts[index],focus)+(range>58.f ? 26.f : 0.f);
                        if(score<best) {best=score;fallback=index;}
                        continue;
                    }
                    if (newCreature(request.type) && glm::distance(arena.rifts[index], input.playerPosition) > 48.f) {
                        if (fallback < 0) fallback = index;
                        continue;
                    }
                }
                request.location = index;
                break;
            }
            if (request.location < 0) request.location = fallback;
            if (request.location < 0) {
                defer(request, time, anySafe ? SpawnDelayReason::LocationBusy : SpawnDelayReason::UnsafePlayerDistance);
                continue;
            }
            if (usesRift) {
                request.warningStart = time;
                request.nextAttempt = time + WarningDuration;
                request.lastBlocker = SpawnDelayReason::None;
                emit(request, DirectorEvent::Kind::WarningStarted, time);
                continue;
            }
        }
        emit(request, DirectorEvent::Kind::Spawn, time);
        if (arena.encounterProfile==EncounterProfile::CrystalSurvival)
            current.liveBounds.push_back(spawnBounds(request.type,events.back().position));
        counters.totalSpawnDelaySeconds += events.back().delaySeconds;
        ++counts[size_t(request.type)];
        ++counters.spawnedByType[size_t(request.type)];
        if (request.type == EnemyType::Chaser) ++counters.spawnedChasers;
        if (request.type == EnemyType::Sentry) {
            ++counters.spawnedSentries;
            current.occupiedSentrySlots[request.location] = true;
        }
        request.finished = true;
        request.warningStart = -1.0;
    }
    if (target >= deadline - 1e-9) {
        for (auto& request : requests) {
            if (request.finished) continue;
            const auto reason = request.warningStart >= 0.0 || request.lastBlocker == SpawnDelayReason::None ?
                SpawnDelayReason::Deadline : request.lastBlocker;
            cancel(request,deadline,reason);
        }
    }
    if(isEndless())
        requests.erase(std::remove_if(requests.begin(),requests.end(),[](const auto& r){return r.finished;}),requests.end());
    elapsed = target >= deadline - 1e-9 && target < deadline ? deadline : target;
    return events;
}

size_t EncounterDirector::pendingCount() const {
    return static_cast<size_t>(std::count_if(requests.begin(), requests.end(),
        [](const Request& request) { return !request.finished; }));
}

bool EncounterDirector::complete(size_t livingEnemies) const {
    return generationFinished() && pendingCount() == 0 && livingEnemies == 0;
}

std::vector<float> EncounterDirector::warningProgress() const {
    std::vector<float> result(arena.rifts.size(), -1.f);
    for (const auto& request : requests) {
        if (!request.finished && request.warningStart >= 0.0 && request.location >= 0) {
            result[request.location] = static_cast<float>(std::clamp((elapsed - request.warningStart) / WarningDuration, 0.0, 1.0));
        }
    }
    return result;
}

} // namespace silic2
