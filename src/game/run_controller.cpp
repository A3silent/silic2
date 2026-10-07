#include "game/run_controller.h"

namespace silic2 {

RunController::RunController(uint64_t initialSeed) : seedService(initialSeed) {
    runState.seed = initialSeed;
}

bool RunController::startRun(const std::string& roomId) {
    if (runState.active || roomId.empty()) {
        return false;
    }

    runState.seed = seedService.getRunSeed();
    runState.runIndex = ++nextRunIndex;
    runState.act = 1;
    runState.floor = 1;
    runState.currency = 0;
    runState.active = true;

    if (!runStats.beginRun(runState.seed, runState.runIndex) ||
        !runStats.beginRoom(roomId)) {
        runState.active = false;
        return false;
    }
    return true;
}

bool RunController::restartRun(const std::string& roomId) {
    if (runStats.isRunActive()) {
        runStats.endRun(RunEndReason::Restarted);
    }
    runState.active = false;
    seedService.reset(runState.seed);
    return startRun(roomId);
}

bool RunController::startNewRun(uint64_t seed, const std::string& roomId) {
    if (runStats.isRunActive()) {
        runStats.endRun(RunEndReason::Restarted);
    }
    runState.active = false;
    seedService.reset(seed);
    return startRun(roomId);
}

bool RunController::endRoom(RoomResult result) {
    return runStats.endRoom(result);
}

bool RunController::endRun(RunEndReason reason) {
    const bool ended = runStats.endRun(reason);
    if (ended) {
        runState.active = false;
    }
    return ended;
}

void RunController::tick(float dt, bool simulationActive) {
    runStats.tick(dt, simulationActive);
}

} // namespace silic2
