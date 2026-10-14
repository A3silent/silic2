#include "game/exploration_controller.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace silic2 {
ExplorationController::ExplorationController(const Map& map):settings(map.getExploration()) {
    if(map.getGameMode()!=GameMode::Exploration) throw std::invalid_argument("exploration controller requires exploration map");
    for(const auto& brush:map.getBrushes()) if(!brush.vertices.empty()) {
        RegionBounds b; b.minimum=b.maximum=brush.vertices.front();
        for(auto v:brush.vertices) { b.minimum=glm::min(b.minimum,v); b.maximum=glm::max(b.maximum,v); }
        solids.push_back(b);
    }
    reset();
}
void ExplorationController::reset() { state={}; held=0; exitOccupied=false; }
bool ExplorationController::sightlineClear(glm::vec3 eye) const {
    const glm::vec3 delta=settings.target-eye;
    for(const auto& b:solids) {
        float enter=0,leave=1; bool intersects=true;
        for(int axis=0;axis<3;++axis) {
            if(std::abs(delta[axis])<1e-6f) {
                if(eye[axis]<b.minimum[axis] || eye[axis]>b.maximum[axis]) { intersects=false; break; }
            } else {
                float a=(b.minimum[axis]-eye[axis])/delta[axis],z=(b.maximum[axis]-eye[axis])/delta[axis];
                if(a>z) std::swap(a,z);
                enter=std::max(enter,a); leave=std::min(leave,z);
                if(enter>leave) { intersects=false; break; }
            }
        }
        if(intersects && enter<.999f && leave>0) return false;
    }
    return true;
}
ExplorationEvents ExplorationController::update(float dt,bool active,const ExplorationInput& in) {
    ExplorationEvents events;
    if(!active || !std::isfinite(dt) || dt<=0) return events;
    for(int i=0;i<3;++i) if(!std::isfinite(in.feet[i]) || !std::isfinite(in.eye[i]) || !std::isfinite(in.forward[i])) return events;
    for(const auto& zone:settings.zones) if(zone.contains(in.feet)) { state.zoneName=zone.name; break; }
    const bool inExit=settings.exit.contains(in.feet);
    state.observing=false;
    switch(state.phase) {
    case ExplorationPhase::Approach:
        for(const auto& zone:settings.zones) if(zone.id==settings.encounterZone && zone.contains(in.feet)) {
            state.phase=ExplorationPhase::Encounter; events.spawns=settings.spawns; break;
        }
        break;
    case ExplorationPhase::Encounter:
        if(in.livingEnemies==0) state.phase=ExplorationPhase::Observe;
        break;
    case ExplorationPhase::Observe: {
        auto direction=settings.target-in.eye;
        const float distance=glm::length(direction),magnitude=glm::length(in.forward);
        const bool aligned=distance>.001f && magnitude>.001f && glm::dot(direction/distance,in.forward/magnitude)>=std::cos(glm::radians(settings.angleDegrees));
        state.observing=settings.observation.contains(in.feet) && aligned && sightlineClear(in.eye);
        // A lost alignment restarts the short continuous observation.
        held=state.observing?held+std::min(dt,.1f):0;
        state.observationProgress=std::min(1.f,held/settings.holdSeconds);
        if(held>=settings.holdSeconds) { state.phase=ExplorationPhase::Return; state.calibrated=true; events.calibrated=true; }
        break;
    }
    case ExplorationPhase::Return:
        if(inExit && !exitOccupied) { state.phase=ExplorationPhase::Complete; events.completed=true; }
        break;
    case ExplorationPhase::Complete: break;
    }
    exitOccupied=inExit;
    return events;
}
std::string ExplorationController::objective() const {
    switch(state.phase) {
    case ExplorationPhase::Approach:return "FOLLOW THE LIGHT TO THE COURT";
    case ExplorationPhase::Encounter:return "CLEAR THE COURT";
    case ExplorationPhase::Observe:return "STAND IN THE RING. ALIGN THE THREE ARCS";
    case ExplorationPhase::Return:return "THE STAR RETURNS. FOLLOW THE EXIT LIGHT";
    case ExplorationPhase::Complete:return "OBSERVATION COMPLETE";
    }
    return {};
}
}
