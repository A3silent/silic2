#include "engine/map.h"
#include "game/arena_definition.h"
#include "game/exploration_controller.h"
#include "test_support.h"
#include "player/player.h"
#include "engine/camera.h"
#include <iostream>
using namespace silic2;
using silic2::test::require;
namespace {
Map sample() { Map map; require(map.loadFromFile("tests/fixtures/crystal_observatory.json"),"exploration load: "+map.getLastError()); return map; }
ExplorationInput observer(const Map& map) {
    const auto& s=map.getExploration(); ExplorationInput in; in.feet=s.observerPosition; in.eye=in.feet+glm::vec3(0,s.observerEyeHeight,0); in.forward=glm::normalize(s.target-in.eye); return in;
}
void clearEncounter(ExplorationController& controller,const Map& map) {
    ExplorationInput in; in.feet=map.getExploration().spawns[0].position;
    auto events=controller.update(.01f,true,in);
    require(events.spawns.size()==map.getExploration().spawns.size(),"court entry generates finite batch");
    in.livingEnemies=events.spawns.size();
    require(controller.update(.01f,true,in).spawns.empty(),"entry cannot duplicate encounter");
    in.feet=map.getExploration().observerPosition;
    controller.update(.1f,true,in);
    require(controller.snapshot().phase==ExplorationPhase::Encounter,"leaving court cannot skip combat");
    in.livingEnemies=0; controller.update(.01f,true,in);
    require(controller.snapshot().phase==ExplorationPhase::Observe,"cleared court unlocks quiet observation");
}
}
void runExplorationTests() {
    Map map=sample(); const auto& e=map.getExploration();
    require(map.getGameMode()==GameMode::Exploration,"explicit exploration mode");
    Map copy; auto json=map.toJson(); json["exploration"]["futureMetadata"]=SimpleJson("retained");
    require(copy.loadFromJson(json),"exploration typed parse");
    const auto saved=copy.toJson().toString(); require(copy.loadFromJson(SimpleJson::parse(saved)) && copy.toJson().toString()==saved,"complete exploration metadata roundtrip");
    auto reject=[&](SimpleJson bad,const char* message) { require(!copy.loadFromJson(bad),message); require(copy.toJson().toString()==saved,"failed exploration load is atomic"); };
    auto bad=json; bad.erase("exploration"); reject(bad,"missing exploration rejected");
    bad=json; bad["exploration"]["encounterZone"]=SimpleJson("missing"); reject(bad,"missing zone ref rejected");
    bad=json; bad["exploration"]["zones"][1]["id"]=bad["exploration"]["zones"][0]["id"]; reject(bad,"duplicate zones rejected");
    bad=json; bad["exploration"]["holdSeconds"]=SimpleJson(0.0); reject(bad,"zero hold rejected");
    bad=json; bad["exploration"]["angleDegrees"]=SimpleJson(180.0); reject(bad,"unbounded aim tolerance rejected");
    bad=json; bad["exploration"]["fragmentDepths"][1]=SimpleJson(.2); reject(bad,"reversed fragments rejected");
    bad=json; bad["exploration"]["spawns"][0]["position"][0]=SimpleJson(99.0); reject(bad,"outside spawn rejected");
    bad=json; bad["exploration"]["spawns"][0]["position"]=SimpleJson::parse("[-18,0,-16]"); reject(bad,"spawn inside cover rejected");
    bad=json; bad["exploration"]["observation"]["max"]=bad["exploration"]["observation"]["min"]; reject(bad,"empty region rejected");
    auto nav=ArenaDefinition::fromMap(map); require(nav.hasConnectedWaypoints(.34f),"all floor navigation connected");
    for(const auto& spawn:e.spawns) {
        require(nav.isWalkable(spawn.position,.34f),"encounter spawn navigable");
        glm::vec3 pos=spawn.position,target=e.guidePoints[2];
        for(int n=0;n<1000 && glm::length(pos-target)>.1f;++n) { auto next=nav.nextWaypoint(pos,target,.34f); auto delta=next-pos; float length=glm::length(delta); if(length>.001f)pos+=delta/length*std::min(length,.2f); }
        require(glm::length(pos-target)<.1f,"spawn reaches actual player route around authored cover");
    }
    for(size_t i=1;i<e.guidePoints.size();++i) require(nav.hasClearPath(e.guidePoints[i-1],e.guidePoints[i],.34f),"authored guide segment "+std::to_string(i)+" is physically navigable");
    // Follow every guide segment using the same input/physics/collision path as GLFW.
    Player walker; walker.respawn(map.getArenaLayout().playerStart); Camera camera;
    for(size_t i=1;i<e.guidePoints.size();++i) {
        for(int frame=0;frame<2000;++frame) {
            auto delta=e.guidePoints[i]-walker.getPosition(); delta.y=0;
            if(glm::length(delta)<.18f) break;
            camera.setDirection(delta); PlayerInput keys; keys.forward=true;
            walker.processInput(keys,&camera,.01f); walker.update(.01f,&map);
        }
        auto delta=e.guidePoints[i]-walker.getPosition(); delta.y=0;
        require(glm::length(delta)<.18f,"actual WASD collision route reaches node "+std::to_string(i)+" at "+std::to_string(walker.getPosition().x)+","+std::to_string(walker.getPosition().y)+","+std::to_string(walker.getPosition().z));
    }
    ExplorationController controller(map); auto in=observer(map);
    for(int i=0;i<100;++i)controller.update(.05f,true,in);
    require(!controller.snapshot().calibrated,"looking early cannot bypass encounter");
    in.feet=(e.exit.minimum+e.exit.maximum)*.5f; controller.update(.05f,true,in);
    require(controller.snapshot().phase==ExplorationPhase::Approach,"exit early cannot finish");
    clearEncounter(controller,map); in=observer(map);
    require(controller.sightlineClear(in.eye),"authored observation has unobstructed focal point");
    in.forward=-in.forward; for(int i=0;i<60;++i)controller.update(.05f,true,in);
    require(controller.snapshot().observationProgress==0,"wrong facing cannot observe");
    in=observer(map); in.feet.x-=3; for(int i=0;i<60;++i)controller.update(.05f,true,in);
    require(controller.snapshot().observationProgress==0,"remote aiming cannot observe");
    in=observer(map); for(int i=0;i<20;++i)controller.update(.05f,true,in);
    float progress=controller.snapshot().observationProgress; require(progress>0 && progress<1,"partial real observation");
    controller.update(10,false,in); require(controller.snapshot().observationProgress==progress,"pause freezes observation");
    in.forward=-in.forward; controller.update(.05f,true,in); require(controller.snapshot().observationProgress==0,"broken attention resets continuous hold");
    in=observer(map); int calibrations=0; for(int i=0;i<80;++i) calibrations+=controller.update(.05f,true,in).calibrated;
    require(calibrations==1 && controller.snapshot().calibrated && controller.snapshot().phase==ExplorationPhase::Return,"calibration emits exactly once and never ends journey");
    in.feet={(e.exit.minimum.x+e.exit.maximum.x)*.5f,0,(e.exit.minimum.z+e.exit.maximum.z)*.5f};
    require(controller.update(.05f,true,in).completed,"walking into exit after calibration completes");
    require(!controller.update(.05f,true,in).completed,"completion emitted once");
    controller.reset(); require(controller.snapshot().phase==ExplorationPhase::Approach && !controller.snapshot().calibrated && controller.snapshot().observationProgress==0,"restart clears all journey state");
    clearEncounter(controller,map);
    Map blocked=map; GeometryObject wall; wall.id=900;wall.position={9,5.3f,3};wall.size={1,6,6};wall.surfaceType=SurfaceType::WALL;
    require(blocked.addObject(wall),"add high sightline obstruction without blocking floor route");
    ExplorationController occluded(blocked); clearEncounter(occluded,blocked); in=observer(blocked);
    require(!occluded.sightlineClear(in.eye),"actual brush obstruction found");
    for(int i=0;i<80;++i)occluded.update(.05f,true,in);
    require(!occluded.snapshot().calibrated && occluded.snapshot().observationProgress==0,"cannot calibrate through geometry");
    std::cout<<"Exploration tests passed: schema, transactional roundtrip, navigation, encounter, actual observation, occlusion, pause, exit and restart\n";
}
