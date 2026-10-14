#include "environment/environment_scene.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace silic2;
int main(){
    Map crystal,horror;
    assert(crystal.loadFromFile("res/maps/starry_crystal_arena.json"));
    assert(horror.loadFromFile("res/maps/hollow_orbit_arena.json"));
    assert(crystal.getEnvironmentPreset()==EnvironmentPreset::StarryCrystal);
    assert(horror.getEnvironmentPreset()==EnvironmentPreset::HollowOrbit);
    const auto c=deriveEnvironmentScene(crystal),h=deriveEnvironmentScene(horror);
    assert(c.minimumXZ==h.minimumXZ && c.maximumXZ==h.maximumXZ && c.floorY==h.floorY);
    for(size_t i=0;i<4;++i){
        assert(c.rifts[i].active && h.rifts[i].active);
        assert(c.rifts[i].localToWorld==h.rifts[i].localToWorld);
    }
    const auto cm=buildEnvironmentGeometry(c),hm=buildEnvironmentGeometry(h);
    assert(cm.size()/7==3804 && hm.size()/7==5832 && cm!=hm);
    for(auto mesh:{cm,hm})for(auto value:mesh)assert(std::isfinite(value));
    for(auto scene:{c,h}){
        assert(environmentPhase(scene.settings,0)==0);
        assert(environmentPhase(scene.settings,47)==.5f);
        assert(environmentPhase(scene.settings,49)==1);
        scene.settings.transformationStart=10;scene.settings.transformationDuration=8;
        assert(environmentPhase(scene.settings,14)==.5f);
        assert(buildEnvironmentGeometry(scene)==buildEnvironmentGeometry(scene));
        scene.settings.preset=EnvironmentPreset::None;
        assert(buildEnvironmentGeometry(scene).empty());
    }
    // Every edited map remains the single source for either presentation.
    for(auto preset:{EnvironmentPreset::StarryCrystal,EnvironmentPreset::HollowOrbit}){
        Map edited=crystal;auto settings=edited.getEnvironment();settings.preset=preset;
        assert(edited.setEnvironment(settings));
        auto source=edited.toJson();source["gameMode"]=SimpleJson("legacy");
        assert(edited.loadFromJson(source));
        for(size_t i=0;i<edited.getEntities().size();++i){
            auto rift=edited.getEntities()[i];if(rift.type!=EntityType::RIFT || rift.slot!=1)continue;
            rift.position={12,0,6};rift.rotation={0,35,0};rift.scale={2,1.5f,.8f};
            assert(edited.updateEntity(i,rift));break;
        }
        const auto scene=deriveEnvironmentScene(edited);
        assert(scene.settings.preset==preset);
        assert(scene.rifts[1].position==glm::vec3(12,0,6));
        assert(glm::length(glm::vec3(scene.rifts[1].localToWorld*glm::vec4(0,1,0,1))-glm::vec3(12,1.5f,6))<.0001f);
        assert(!buildEnvironmentGeometry(scene).empty());
    }
    std::cout<<"PASS dual presets: geometry budgets, authoritative map/rift transforms, phase timing, deterministic generation and None\n";
}
