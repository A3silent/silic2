#include "game/arena_definition.h"
#include "engine/map.h"
#include "enemy/enemy.h"
#include "test_support.h"

#include <algorithm>
#include <memory>

namespace {
using namespace silic2;
using silic2::test::require;

Map sampleMap() {
    Map map;
    require(map.loadFromFile("tests/fixtures/starry_crystal_arena_v2.json"), "load actual arena: " + map.getLastError());
    require(map.getGameMode() == GameMode::ArenaSurvival, "sample explicitly declares arena mode");
    return map;
}

void translateBrush(Map& map, uint32_t id, glm::vec3 delta) {
    for (size_t i=0; i<map.getObjects().size(); ++i) {
        auto object=map.getObjects()[i];
        if (object.id!=id) continue;
        object.position+=delta;
        require(map.updateObject(i,object), "move authored primitive: " + map.getLastError());
        return;
    }
    for (size_t i=0; i<map.getLegacyBrushes().size(); ++i) {
        auto brush=map.getLegacyBrushes()[i];
        if (brush.id!=id) continue;
        for (auto& vertex:brush.vertices) vertex+=delta;
        require(map.updateBrush(i,brush), "move authored brush: " + map.getLastError());
        return;
    }
    require(false,"obstacle resolves to a single editable authored geometry source");
}

uint32_t centralId(const Map& map) {
    for (const auto& obstacle:map.getArenaLayout().obstacles)
        if (obstacle.min.x<0 && obstacle.max.x>0 && obstacle.min.z<0 && obstacle.max.z>0)
            return obstacle.brushId;
    require(false,"sample contains central obstacle");
    return 0;
}

void verifyRoutes(const ArenaDefinition& arena) {
    require(arena.hasConnectedWaypoints(.34f),"geometry-derived graph is connected for Chasers");
    for (const auto& start:arena.rifts) {
        auto current=start;
        for (int step=0; step<100 && glm::length(current-arena.playerStart)>.001f; ++step) {
            const auto next=arena.nextWaypoint(current,arena.playerStart,.34f);
            require(arena.hasClearPath(current,next,.34f),"selected route respects compiled geometry");
            require(glm::length(next-current)>.001f,"connected map route must advance");
            current=next;
        }
        require(glm::length(current-arena.playerStart)<.001f,"every authored rift reaches authored player start");
    }
}

void testEditedGeometry() {
    auto map=sampleMap();
    const auto original=ArenaDefinition::fromMap(map);
    verifyRoutes(original);
    require(!original.hasClearPath({-6,0,0},{6,0,0},.34f),"original central brush blocks the straight route");
    const auto id=centralId(map);
    translateBrush(map,id,{0,0,5});
    auto moved=std::make_shared<ArenaDefinition>(ArenaDefinition::fromMap(map));
    require(moved->hasClearPath({-6,0,0},{6,0,0},.34f),"old obstacle location becomes traversable after authoring edit");
    require(!moved->hasClearPath({-6,0,5},{6,0,5},.34f),"new obstacle location blocks the route");
    verifyRoutes(*moved);
    require(moved->rifts==map.getArenaLayout().rifts && moved->playerStart==map.getArenaLayout().playerStart,
            "navigation uses authored spawn anchors");
    for (const auto& obstacle:map.getArenaLayout().obstacles) {
        const auto it=std::find_if(map.getBrushes().begin(),map.getBrushes().end(),[&](const Brush& b){return b.id==obstacle.brushId;});
        require(it!=map.getBrushes().end(),"navigation obstacle resolves to collision brush");
        auto low=it->vertices.front(), high=low;
        for (auto v:it->vertices) {low=glm::min(low,v);high=glm::max(high,v);}
        require(low==obstacle.min && high==obstacle.max,"collision and navigation use identical bounds");
    }
    Enemy enemy({-6,.05f,0},6);
    enemy.setArenaDefinition(moved);
    for(int frame=0; frame<300; ++frame) enemy.update(1.f/60,{6,0,0},&map);
    require(glm::length(glm::vec2(enemy.getPosition().x-6,enemy.getPosition().z))<.1f,
            "actual Chaser crosses former obstacle location without a hardcoded arena route");
    // Reuse a Map and force both graphs' edge caches to be populated.
    require(map.loadFromFile("tests/fixtures/starry_crystal_arena_v2.json"),"reload original sample");
    auto reloaded=ArenaDefinition::fromMap(map);
    verifyRoutes(reloaded);
    require(!reloaded.hasClearPath({-6,0,0},{6,0,0},.34f),"reload restores original navigation geometry");
    require(moved->hasClearPath({-6,0,0},{6,0,0},.34f),"independent graph caches do not bleed across map versions");
}

// Change a complete authored document in one transaction: rectangular X extent,
// translated X/Z origin, raised floor. This exercises actual JSON compilation.
void transformPosition(SimpleJson& value) {
    value[0]=SimpleJson(value[0].getNumber()*1.5+100);
    value[1]=SimpleJson(value[1].getNumber()+7);
    value[2]=SimpleJson(value[2].getNumber()+40);
}
void testTranslatedRectangularArena() {
    auto map=sampleMap();
    const auto original=ArenaDefinition::fromMap(map);
    auto document=map.toJson();
    auto& geometry=document["geometry"];
    if (geometry.hasKey("brushes")) {
        auto& brushes=geometry["brushes"];
        for(size_t i=0;i<brushes.size();++i) {
            auto& vertices=brushes[i]["vertices"];
            for(size_t j=0;j<vertices.size();j+=3) {
                vertices[j]=SimpleJson(vertices[j].getNumber()*1.5+100);
                vertices[j+1]=SimpleJson(vertices[j+1].getNumber()+7);
                vertices[j+2]=SimpleJson(vertices[j+2].getNumber()+40);
            }
        }
    }
    if (geometry.hasKey("objects")) {
        auto& objects=geometry["objects"];
        for(size_t i=0;i<objects.size();++i) {
            transformPosition(objects[i]["position"]);
            auto& size=objects[i]["size"];
            size[0]=SimpleJson(size[0].getNumber()*1.5);
        }
    }
    auto& entities=document["entities"];
    for(size_t i=0;i<entities.size();++i) transformPosition(entities[i]["position"]);
    require(map.loadFromJson(document),"translated rectangular arena transaction: "+map.getLastError());
    auto definition=std::make_shared<ArenaDefinition>(ArenaDefinition::fromMap(map));
    require(std::abs(definition->floorY-7)<.001f,"navigation floor follows map elevation");
    require(std::abs(definition->minXZ.x-(original.minXZ.x*1.5f+100))<.001f &&
            std::abs(definition->maxXZ.y-(original.maxXZ.y+40))<.001f,"navigation rectangular bounds follow authored floor");
    require(!definition->isWalkable(original.playerStart),"old origin is not part of the translated arena");
    verifyRoutes(*definition);
    for(auto rift:definition->rifts) {
        Enemy enemy(rift+glm::vec3(0,.05f,0),6);
        enemy.setArenaDefinition(definition);
        for(int frame=0;frame<1500;++frame) enemy.update(1.f/60,definition->playerStart,&map);
        const auto position=enemy.getPosition();
        require(glm::length(glm::vec2(position.x-definition->playerStart.x,position.z-definition->playerStart.z))<.1f,
                "actual Chaser pursues across translated non-square map");
        require(std::abs(position.y-(definition->floorY+.05f))<.1f,"Chaser remains on raised collision floor");
    }
    require(map.loadFromJson(document),"repeated translated map load");
    const auto repeat=ArenaDefinition::fromMap(map);
    require(repeat.obstacles.size()==definition->obstacles.size(),"repeated load cannot accumulate nav blockers");
    require(repeat.nextWaypoint(repeat.rifts[0],repeat.playerStart,.34f)==
            definition->nextWaypoint(definition->rifts[0],definition->playerStart,.34f),"same authored map produces deterministic route ordering");
}
}

void runMapNavigationTests() {
    testEditedGeometry();
    testTranslatedRectangularArena();
}
