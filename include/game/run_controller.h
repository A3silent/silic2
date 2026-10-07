#pragma once

#include "engine/run_state.h"
#include "game/run_stats.h"
#include "game/seed_service.h"

#include <cstdint>
#include <string>

namespace silic2 {

class RunController {
public:
    explicit RunController(uint64_t initialSeed);

    bool startRun(const std::string& roomId);
    bool restartRun(const std::string& roomId);
    bool startNewRun(uint64_t seed, const std::string& roomId);
    bool endRoom(RoomResult result);
    bool endRun(RunEndReason reason);
    void tick(float dt, bool simulationActive);

    const RunState& state() const { return runState; }
    RunState& state() { return runState; }
    const RunStats& stats() const { return runStats; }
    RunStats& stats() { return runStats; }
    SeedService& seeds() { return seedService; }
    const SeedService& seeds() const { return seedService; }

private:
    RunState runState;
    RunStats runStats;
    SeedService seedService;
    uint32_t nextRunIndex = 0;
};

} // namespace silic2
