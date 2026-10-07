#include "environment/environment_scene.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace silic2;
static bool close(glm::vec3 a,glm::vec3 b){return glm::length(a-b)<.0001f;}
static void loaded(bool success,const Map& map){if(!success)std::cerr<<map.getLastError()<<'\n';assert(success);}
int main(){
    Map map;
    loaded(map.loadFromJson(SimpleJson::parse(R"JSON({
        "schemaVersion":2,"gameMode":"legacy","environmentPreset":"starry_crystal",
        "environment":{"transformationStart":10,"transformationDuration":6,"particleCount":120},
        "arena":{"floorBrushId":1},
        "geometry":{"objects":[
            {"id":1,"shape":"plane","position":[0,0,0],"size":[36,0,36],"type":"floor","material":"plain_floor"},
            {"id":6,"shape":"box","position":[0,2.1,0],"size":[4,4.2,4],"type":"wall","material":"arena_living_crystal"}
        ]},
        "entities":[{"type":"rift","slot":1,"name":"east","position":[15,0,0],"rotation":[0,90,0],"scale":[1,1,1]}],
        "lights":[
            {"type":"point","position":[0,5.2,0],"anchor":{"brushId":6,"offset":[0,3.1,0]}},
            {"type":"point","position":[15,2,0],"anchor":{"entityName":"east","offset":[0,2,0]}}
        ]
    })JSON")),map);
    auto scene=deriveEnvironmentScene(map);
    assert(scene.settings.preset==EnvironmentPreset::StarryCrystal);
    assert(map.getGameMode()==GameMode::Legacy);
    assert(scene.hasFloor && scene.minimumXZ==glm::vec2(-18) && scene.maximumXZ==glm::vec2(18));
    assert(!scene.rifts[0].active && scene.rifts[1].active && !scene.rifts[2].active && !scene.rifts[3].active);
    assert(environmentPhase(scene.settings,13)==.5f && scene.settings.particleCount==120);
    assert(close(map.getLights()[0].position,{0,5.2f,0}));
    auto object=map.getObjects()[1];object.position={5,3,4};object.size={6,6,2};
    loaded(map.updateObject(1,object),map);
    assert(close(map.getLights()[0].position,{5,6.1f,4}));
    const auto& solid=map.getBrushes()[1];glm::vec3 lo(1e9),hi(-1e9);
    for(auto v:solid.vertices){lo=glm::min(lo,v);hi=glm::max(hi,v);}
    assert(close(lo,{2,0,3}) && close(hi,{8,6,5}));
    bool obstacleMatches=false;
    for(const auto& obstacle:map.getArenaLayout().obstacles)
        if(obstacle.brushId==6)obstacleMatches=close(obstacle.min,lo)&&close(obstacle.max,hi);
    assert(obstacleMatches);
    auto rift=map.getEntities()[0];rift.position={11,0,5};rift.scale={2,1.5f,.5f};rift.rotation={0,45,0};
    loaded(map.updateEntity(0,rift),map);scene=deriveEnvironmentScene(map);
    assert(close(scene.rifts[1].position,rift.position));
    assert(close(glm::vec3(scene.rifts[1].localToWorld*glm::vec4(0,0,0,1)),rift.position));
    assert(close(map.getLights()[1].position,{11,2,5}));
    assert(close(glm::vec3(scene.rifts[1].worldToLocal*glm::vec4(rift.position,1)),{0,0,0}));
    auto floor=map.getObjects()[0];floor.position={3,0,-2};floor.size={40,0,44};
    loaded(map.updateObject(0,floor),map);scene=deriveEnvironmentScene(map);
    assert(scene.minimumXZ==glm::vec2(-17,-24) && scene.maximumXZ==glm::vec2(23,20));
    auto settings=map.getEnvironment();settings.preset=EnvironmentPreset::None;
    loaded(map.setEnvironment(settings),map);
    assert(deriveEnvironmentScene(map).settings.preset==EnvironmentPreset::None);
    Map sky;loaded(sky.loadFromJson(SimpleJson::parse(R"({"schemaVersion":2,"environmentPreset":"starry_crystal","gameMode":"legacy"})")),sky);
    assert(!deriveEnvironmentScene(sky).hasFloor);
    assert(buildEnvironmentGeometry(deriveEnvironmentScene(sky)).empty());
    Map legacy;loaded(legacy.loadFromFile("res/maps/test_room.json"),legacy);
    assert(deriveEnvironmentScene(legacy).settings.preset==EnvironmentPreset::None);
    std::cout<<"PASS: shared Map preset/floor/rifts/timing, moved/scaled obstacle and light anchors, floor resizing, sky-only/legacy-none\n";
}
