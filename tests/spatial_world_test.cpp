#include "engine/map.h"
#include "engine/camera.h"
#include "player/player.h"
#include "enemy/enemy.h"
#include "enemy/enemy_geometry.h"
#include "enemy/enemy_projectile_system.h"
#include "game/arena_definition.h"
#include "test_support.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <filesystem>
using namespace silic2;
using silic2::test::require;
namespace {
Map rampFixture() {
    auto json=SimpleJson::parse(R"({"schemaVersion":3,"gameMode":"legacy","environmentPreset":"none","spatial":{"mode":"triangles","navigation":{"nodes":[{"id":1,"position":[0,0,14]},{"id":2,"position":[0,2,6]},{"id":3,"position":[0,4,0]},{"id":4,"position":[0,4,-10]},{"id":5,"position":[0,0,-10]},{"id":6,"position":[6,0,-10]},{"id":7,"position":[6,0,14]}],"links":[{"from":1,"to":2},{"from":2,"to":3},{"from":3,"to":4},{"from":5,"to":6},{"from":6,"to":7},{"from":7,"to":1}]}},"geometry":{"objects":[{"id":1,"shape":"plane","position":[0,0,0],"size":[40,0,40],"type":"floor"},{"id":2,"shape":"ramp","position":[0,2,6],"size":[6,4,12]},{"id":3,"shape":"box","position":[0,3.7,-6],"size":[6,0.6,12]}]}})");
    Map map;require(map.loadFromJson(json),"ramp fixture: "+map.getLastError());return map;
}
void walk(Player& player,Camera& camera,const Map& map,glm::vec3 destination,int maxFrames=3000) {
    for(int frame=0;frame<maxFrames;++frame){auto delta=destination-player.getPosition();delta.y=0;if(glm::length(delta)<.12f)break;camera.setDirection(delta);PlayerInput keys;keys.forward=true;player.processInput(keys,&camera,.01f);player.update(.01f,&map);}
    auto p=player.getPosition();require(glm::length(glm::vec2(p.x-destination.x,p.z-destination.z))<.13f && std::abs(p.y-destination.y)<.2f,"actual W reaches "+std::to_string(destination.z)+" at "+std::to_string(p.x)+","+std::to_string(p.y)+","+std::to_string(p.z));
}
}
void runSpatialWorldTests() {
    auto map=rampFixture();auto world=map.getSpatialWorld();require(bool(world),"explicit spatial world");
    for(auto& diagnostic:world->diagnostics())std::cerr<<"Fixture diagnostic: "<<diagnostic<<'\n';
    require(world->navigationConnected(),"ramp and bridge graph supported/connected");
    require(world->support({0,4,-8},.1f,.1f).height==4,"upper bridge support");
    require(world->support({0,0,-8},.1f,.1f).height==0,"lower bridge independent support");
    require(!world->canTraverse({0,0,-8},{0,4,-8}),"no automatic vertical link across same XZ");
    require(!world->isWalkable({0,0,3}),"closed ramp interior not walkable just because floor underneath");
    require(world->raycast({0,1,-8},{0,7,-8}).hit,"upward ray meets actual bridge deck");
    require(!world->raycast({0,1,-10},{0,1,-2}).hit,"bridge underpass ray remains open");
    Player player;Camera camera;player.respawn({0,0,14});
    walk(player,camera,map,{0,2,6});walk(player,camera,map,{0,4,-10});walk(player,camera,map,{0,0,14});
    player.respawn({0,4.015f,-4});camera.setDirection({0,0,1});PlayerInput keys;keys.forward=true;keys.sprint=true;player.processInput(keys,&camera,.01f);player.update(.01f,&map);keys.sprint=false;
    for(int i=0;i<30;++i){player.processInput(keys,&camera,.01f);player.update(.01f,&map);}keys.crouch=true;player.processInput(keys,&camera,.01f);player.update(.01f,&map);
    require(player.getState()==PlayerState::SLIDING,"slide enters through ordinary sprint/crouch keys");
    for(int i=0;i<80;++i){player.processInput(keys,&camera,.01f);player.update(.01f,&map);}
    require(player.getPosition().z>4 && player.getPosition().y<3.5f,"slide follows descending ramp without floating or teleporting");
    // Same-XZ upper target requires the lower detour, then ramp, then bridge.
    auto definition=std::make_shared<ArenaDefinition>();definition->spatialWorld=world;
    Enemy enemy({0,.015f,-10});enemy.setArenaDefinition(definition);
    glm::vec3 target{0,4.015f,-10};
    require(!enemy.isTouchingPlayer(target),"contact cannot damage through bridge levels");
    for(int i=0;i<6000 && glm::length(enemy.getPosition()-target)>.8f;++i){enemy.update(.01f,target,&map);}
    require(glm::length(enemy.getPosition()-target)<.8f,"enemy traverses L detour from exact node and ramp to upper level");
    Enemy jumpingChase({0,.015f,-10});jumpingChase.setArenaDefinition(definition);
    for(int i=0;i<6000 && glm::length(jumpingChase.getPosition()-target)>.8f;++i)
        jumpingChase.update(.01f,target+glm::vec3(0,1.8f,0),&map);
    require(glm::length(jumpingChase.getPosition()-target)<.8f,"airborne upper target projects to upper support and enemy completes detour");
    const auto temporary=std::filesystem::temp_directory_path()/"silic2-spatial-quad-test.obj";
    auto document=map.toJson();document["geometry"]["models"]=SimpleJson::parse(R"([{"id":99,"asset":"silic2-spatial-quad-test.obj","collisionAsset":"silic2-spatial-quad-test.obj","position":[0,0,0],"rotation":[0,0,0],"scale":[1,1,1]}])");
    const auto original=map.toJson().toString();
    for(const auto* vertices:{"v 0 0 0\nv 2 0 0\nv 1 0 .5\nv 2 0 2\n", "v 0 0 0\nv 2 0 0\nv 2 0 2\nv 0 1 2\n"}) {
        {std::ofstream obj(temporary);obj<<vertices<<"f 1 2 3 4\n";}
        require(!map.loadFromJson(document,temporary.parent_path().string()),"concave or twisted quad rejected");
        require(map.getLastError().find("quad")!=std::string::npos,"quad failure explains malformed face");
        require(map.toJson().toString()==original,"bad OBJ load preserves prior scene transactionally");
    }
    {std::ofstream obj(temporary);obj<<"v 0 0 0\nv 2 0 0\nv 2 0 2\nv 0 0 2\nf 1 2 3 4\n";}
    require(map.loadFromJson(document,temporary.parent_path().string()),"planar convex quad accepted");
    std::filesystem::remove(temporary);
    auto first=world->nextWaypoint({0,.015f,-10},target);require(glm::length(first-glm::vec3(0,.015f,-10))>.5f,"node skin offset cannot return own foot forever");
    std::cout<<"Spatial fixture passed: real slope up/down/crest/slide, separate bridge levels, solid interior, ray opening, enemy detour and height contact\n";
}
