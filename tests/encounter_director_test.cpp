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
    compact.rifts = {{0,0,0}, {1,0,0}, {0,0,1}, {-1,0,0}};
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
void testCrystalEncounters() {
    Map map;
    require(map.loadFromFile("res/maps/starry_crystal_arena.json"), "load three-enemy Crystal map");
    const ArenaDefinition arena = ArenaDefinition::fromMap(map);
    require(arena.rifts.size() == 12 && arena.encounterProfile == EncounterProfile::CrystalSurvival,
            "current Crystal enables the longer encounter and its authored slots");
    for (size_t i=4; i<12; ++i) {
        const float angle = glm::radians(22.5f + 45.f * float(i-4));
        const glm::vec3 expected(78.f*std::cos(angle), 0, 78.f*std::sin(angle));
        require(glm::distance(arena.rifts[i], expected) < .001f && arena.isWalkable(arena.rifts[i], .7f),
                "outer rift on the authored, traversable 78m circle");
    }
    auto simulate=[&](double step) {
        EncounterDirector director(arena);
        DirectorInput input;
        input.playerPosition={90,0,0};
        input.playerVelocity={0,0,8};
        std::array<EnemyCounts,6> byStage{};
        std::vector<DirectorEvent> spawns;
        while(director.combatSeconds()<270.) {
            auto batch=director.update(std::min(step,270.-director.combatSeconds()),input);
            for(const auto& e:batch) {
                if(e.kind!=DirectorEvent::Kind::Spawn) continue;
                ++byStage[std::min(size_t(e.combatSeconds/45),size_t(5))][size_t(e.type)];
                spawns.push_back(e);
                if(e.type==EnemyType::Sentry) continue;
                require(e.combatSeconds+1.e-7>=e.scheduledSeconds+1.2,"full rift warning at every density");
                if(e.type==EnemyType::Chaser) continue;
                const float distance=glm::distance(input.playerPosition,e.position);
                require(distance>=18,"heavy creature materializes outside safety radius");
                if(e.scheduledSeconds<=62) require(distance<=48 && e.locationIndex>=4,"nearby outer introductions");
            }
        }
        const size_t chaser=size_t(EnemyType::Chaser),hound=size_t(EnemyType::ForkHound);
        require(byStage[1][chaser]>byStage[0][chaser] && byStage[1][chaser]>byStage[2][chaser] &&
                byStage[2][chaser]>byStage[3][chaser] && byStage[3][chaser]>0,
                "common arrivals peak at 45s and taper at 90s and 135s");
        require(byStage[4][chaser]==0 && byStage[5][chaser]==0,"no chasers materialize from 180s onward");
        for(size_t stage=2;stage<byStage.size();++stage) {
            const auto& counts=byStage[stage];
            const size_t rays=counts[size_t(EnemyType::VeilRay)], censers=counts[size_t(EnemyType::HangingCenser)];
            require(counts[hound]>0 && rays>0 && censers>0 &&
                    counts[hound]+rays+censers>counts[chaser]+counts[size_t(EnemyType::Sentry)],
                    "midgame and overtime reinforcements are predominantly heavy enemies");
            require(rays+censers>counts[hound],"flying reinforcements outnumber hounds from 90s onward");
        }
        require(byStage[0][hound]==1 && byStage[1][hound]==2,"opening hound introductions keep their cadence");
        require(director.isEndless() && director.pressureStage()==4 && !director.generationFinished() &&
                !director.complete(0) && !director.complete(20),"survivors keep fighting past 210s even if the arena momentarily empties");
        return spawns;
    };
    const auto spawns=simulate(1./30);
    equalEvents(spawns,simulate(1./144));

    // No kills and no moving bodies crossing a rift. Update real counts and
    // sentry slots just as App does, then release capacity deep into overtime.
    EncounterDirector full(arena);
    DirectorInput living;
    living.playerPosition={90,0,0};
    unsigned stagesSeen=0;
    auto keepAlive=[&] {
        for (auto& e:full.update(.1,living)) if(e.kind==DirectorEvent::Kind::Spawn) {
            const size_t count=++living.liveByType[size_t(e.type)];
            require(count<=EncounterDirector::crystalCap(e.type,full.pressureStage()),
                    "a new arrival fits the current stage budget");
            if(e.type==EnemyType::Chaser) require(count<=10,"at most ten live chasers");
            if(e.type==EnemyType::Sentry) living.occupiedSentrySlots[e.locationIndex]=true;
        }
        stagesSeen|=1u<<full.pressureStage();
    };
    for(int i=0;i<3000;++i) keepAlive();
    require(stagesSeen==31 && full.pendingCount()<12,"endless capped pressure keeps a bounded backlog");
    require(living.liveByType[size_t(EnemyType::Chaser)]==10,"early chasers can survive through overtime");
    constexpr EnemyType heavies[]={EnemyType::ForkHound,EnemyType::VeilRay,EnemyType::HangingCenser};
    for(auto type:heavies) {
        require(living.liveByType[size_t(type)]==EncounterDirector::crystalCap(type,4),
                "the final heavy budget is attainable");
        --living.liveByType[size_t(type)];
    }
    living.liveByType[size_t(EnemyType::Chaser)]=0;
    for(int i=0;i<60;++i) keepAlive();
    require(living.liveByType[size_t(EnemyType::Chaser)]==0,"late kills leave basic slots empty");
    for(auto type:heavies)
        require(living.liveByType[size_t(type)]==EncounterDirector::crystalCap(type,4),
                "released elite slots are replenished in overtime");
    EncounterDirector late(arena);
    DirectorInput in;
    in.playerPosition={90,0,0};
    in.liveByType[size_t(EnemyType::ForkHound)]=2;
    auto first=late.update(12.05,in);
    require(first.back().type==EnemyType::ForkHound && first.back().reason==SpawnDelayReason::ActiveCap,
            "cannot exceed two live hounds");
    in.liveByType[size_t(EnemyType::ForkHound)]=0;
    auto resumed=late.update(.2,in);
    require(resumed.size()==1 && resumed[0].kind==DirectorEvent::Kind::WarningStarted,
            "freed hound slot starts full warning");
    in.occupiedRifts.resize(12,false);
    in.occupiedRifts[resumed[0].locationIndex]=true;
    auto cancelled=late.update(1.2,in);
    require(cancelled.front().kind==DirectorEvent::Kind::WarningCancelled &&
            cancelled.front().position==resumed[0].position && cancelled.front().reason==SpawnDelayReason::LocationBusy,
            "occupied rift cancels the matching world-space warning at materialization");
    late.reset(42);
    for (float v : late.warningProgress()) require(v<0,"same-seed restart clears peripheral warnings");
}

void testChaserWarningAtStageBoundary() {
    Map map;
    require(map.loadFromFile("res/maps/starry_crystal_arena.json"),"load Crystal phase transition fixture");
    const auto arena=ArenaDefinition::fromMap(map);
    // Keep the rifts blocked until an old request can start a warning that would
    // finish after the stage change. All other enemy types are already at cap.
    for(double boundary:{90.,135.,180.}) {
        EncounterDirector director(arena);
        DirectorInput input;
        input.playerPosition={90,0,0};
        input.liveByType.fill(100);
        input.liveByType[size_t(EnemyType::Chaser)]=0;
        input.occupiedRifts.assign(arena.rifts.size(),true);
        director.update(boundary-1.,input);
        input.occupiedRifts[4]=false;
        const auto before=director.update(.5,input);
        const auto started=std::find_if(before.begin(),before.end(),[](const auto& e) {
            return e.type==EnemyType::Chaser && e.kind==DirectorEvent::Kind::WarningStarted;
        });
        require(started!=before.end(),"late unblock starts an old-stage chaser warning");
        const auto crossing=director.update(1.,input);
        const auto cancelled=std::find_if(crossing.begin(),crossing.end(),[&](const auto& e) {
            return e.requestId==started->requestId && e.kind==DirectorEvent::Kind::WarningCancelled;
        });
        require(cancelled!=crossing.end() && std::abs(cancelled->combatSeconds-boundary)<1.e-8 &&
                cancelled->position==started->position,"next stage extinguishes the corresponding rift warning");
        const auto later=director.update(3.,input);
        for(const auto* events:{&crossing,&later}) {
            for(const auto& e:*events)
                require(e.kind!=DirectorEvent::Kind::Spawn || e.type!=EnemyType::Chaser ||
                        e.scheduledSeconds>=boundary,"deferred chasers from the old budget never materialize");
        }
    }
}

void testFlyingSpawnClearance() {
    Map original;
    require(original.loadFromFile("res/maps/starry_crystal_arena.json"), "load Crystal spawn geometry");
    constexpr int slot=4;
    const auto feet=original.getArenaLayout().rifts[slot];
    auto selectedInput=[&](EnemyType type,int rift) {
        DirectorInput in;
        in.playerPosition=original.getArenaLayout().rifts[rift]+glm::vec3(0,0,23);
        in.occupiedRifts.assign(original.getArenaLayout().rifts.size(),true);
        in.occupiedRifts[rift]=false;
        in.liveByType.fill(100);
        in.liveByType[size_t(type)]=0;
        return in;
    };
    auto eventFor=[](const std::vector<DirectorEvent>& events,EnemyType type,DirectorEvent::Kind kind) {
        return std::any_of(events.begin(),events.end(),[&](const auto& e){return e.type==type && e.kind==kind;});
    };
    auto offers=[&](const Map& map,EnemyType type) {
        EncounterDirector director(ArenaDefinition::fromMap(map));
        const auto events=director.update(type==EnemyType::VeilRay?36.01:62.01,selectedInput(type,slot));
        return eventFor(events,type,DirectorEvent::Kind::WarningStarted);
    };
    auto edited=[&](glm::vec3 offset,glm::vec3 size) {
        Map map=original;
        GeometryObject block;
        block.id=99999;
        block.name="Edited rift clearance obstacle";
        block.position=feet+offset;
        block.size=size;
        require(map.addObject(block),"editor accepts obstacle outside generic marker clearance");
        return map;
    };
    EncounterDirector authored(ArenaDefinition::fromMap(original));
    for (size_t i=0;i<original.getArenaLayout().rifts.size();++i) {
        for (auto type : {EnemyType::VeilRay,EnemyType::HangingCenser}) {
            authored.reset(42);
            const auto events=authored.update(type==EnemyType::VeilRay?37.21:63.21,selectedInput(type,int(i)));
            require(eventFor(events,type,DirectorEvent::Kind::Spawn),
                    "both resized bodies materialize at every authored Crystal rift");
        }
    }
    const auto wingObstruction=edited({4.1f,3,0},{.25f,.5f,.4f});
    require(!offers(wingObstruction,EnemyType::VeilRay) && offers(wingObstruction,EnemyType::HangingCenser),
            "a side obstruction meets the rising ray wing, outside the censer shell");
    const auto ceiling=edited({0,13.8f,0},{3,.3f,3});
    require(!offers(ceiling,EnemyType::HangingCenser) && offers(ceiling,EnemyType::VeilRay),
            "takeoff checks the top of the ascending censer beneath an edited overhang");

    EncounterDirector late(ArenaDefinition::fromMap(original));
    auto live=selectedInput(EnemyType::HangingCenser,slot);
    require(eventFor(late.update(62.01,live),EnemyType::HangingCenser,DirectorEvent::Kind::WarningStarted),
            "clear censer rift starts its warning");
    live.liveBounds.emplace_back(feet+glm::vec3(2.4f,7,-.2f),feet+glm::vec3(3.f,8,.2f));
    const auto blocked=late.update(1.2,live);
    require(eventFor(blocked,EnemyType::HangingCenser,DirectorEvent::Kind::WarningCancelled) &&
            !eventFor(blocked,EnemyType::HangingCenser,DirectorEvent::Kind::Spawn),
            "occupant intersecting the upper shell cancels materialization after warning");
    live.liveBounds.clear();
    require(eventFor(late.update(.25,live),EnemyType::HangingCenser,DirectorEvent::Kind::WarningStarted),
            "vacated censer rift starts a fresh warning");

    EncounterDirector ground(ArenaDefinition::fromMap(original));
    auto under=selectedInput(EnemyType::Chaser,0);
    const auto inner=original.getArenaLayout().rifts[0];
    under.liveBounds.emplace_back(inner+glm::vec3(-3,2,-2),inner+glm::vec3(3,10,2));
    require(eventFor(ground.update(1.21,under),EnemyType::Chaser,DirectorEvent::Kind::Spawn),
            "a ground enemy can spawn beneath a body that has fully lifted out of its volume");
}
} // namespace

void runEncounterDirectorTests() {
    testCrystalEncounters();
    testChaserWarningAtStageBoundary();
    testFlyingSpawnClearance();
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
