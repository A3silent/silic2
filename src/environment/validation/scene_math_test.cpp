#include "environment/environment_scene.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using namespace silic2;
static bool close(float a,float b) { return std::abs(a-b)<.0001f; }
static bool close(glm::vec3 a,glm::vec3 b) { return glm::length(a-b)<.0001f; }

int main() {
    EnvironmentScene scene;
    scene.settings.preset=EnvironmentPreset::StarryCrystal;
    scene.settings.transformationStart=12;
    scene.settings.transformationDuration=8;
    assert(environmentPhase(scene.settings,11)==0);
    assert(environmentPhase(scene.settings,12)==0);
    assert(close(environmentPhase(scene.settings,16),.5f));
    assert(environmentPhase(scene.settings,20)==1);
    assert(environmentPhase(scene.settings,100)==1);
    assert(environmentPhase(scene.settings,std::numeric_limits<float>::infinity())==0);
    scene.settings.preset=EnvironmentPreset::None;
    assert(environmentPhase(scene.settings,20)==0);
    assert(buildEnvironmentGeometry(scene).empty()); // Sky-only has no invented floor/rifts.

    scene.settings.preset=EnvironmentPreset::HollowOrbit;
    scene.hasFloor=true;
    scene.minimumXZ={-18,-18};scene.maximumXZ={18,18};scene.floorY=0;
    const auto original=buildEnvironmentGeometry(scene);
    assert(!original.empty() && original.size()/7<=8000); // Bounded static backdrop.
    for(size_t i=0;i<original.size();i+=7){
        const float x=original[i],z=original[i+2];
        assert(x < -18 || x > 18 || z < -18 || z > 18); // No route/camera obstruction.
    }
    for(auto extent : {glm::vec2(.5f,100.f),glm::vec2(60.f,5.f),glm::vec2(100.f,100.f)}) {
        EnvironmentScene resized=scene;resized.minimumXZ=-extent;resized.maximumXZ=extent;
        const auto mesh=buildEnvironmentGeometry(resized);
        for(size_t i=0;i<mesh.size();i+=7)
            assert(mesh[i] < -extent.x || mesh[i] > extent.x || mesh[i+2] < -extent.y || mesh[i+2] > extent.y);
    }
    scene.minimumXZ+=glm::vec2(8,-5);scene.maximumXZ+=glm::vec2(8,-5);scene.floorY=3;
    const auto moved=buildEnvironmentGeometry(scene);
    assert(moved.size()==original.size());
    for(size_t i=0;i<original.size();i+=7) {
        assert(close(glm::vec3(moved[i],moved[i+1],moved[i+2]),
                     glm::vec3(original[i]+8,original[i+1]+3,original[i+2]-5)));
    }
    auto& rift=scene.rifts[2]; // Slot 2 at a non-cardinal position, rotated/scaled.
    rift.active=true;rift.position={10,3,-6};rift.scale={2,3,.5f};
    rift.localToWorld=environmentRiftTransform(rift.position,{0,90,0},rift.scale);
    rift.worldToLocal=glm::inverse(rift.localToWorld);
    assert(close(glm::vec3(rift.worldToLocal*glm::vec4(rift.position,1)),glm::vec3(0)));
    assert(close(glm::vec3(rift.localToWorld*glm::vec4(0,1,0,1)),glm::vec3(10,6,-6)));
    const auto withRift=buildEnvironmentGeometry(scene);
    glm::vec3 minimum(1e9),maximum(-1e9);
    size_t riftVertexCount=0;
    for(size_t i=0;i<withRift.size();i+=7) if(withRift[i+6]<0) {
        assert(withRift[i+6]==-3); // Slot identity survives position/orientation edits.
        const glm::vec3 p(withRift[i],withRift[i+1],withRift[i+2]);
        minimum=glm::min(minimum,p);maximum=glm::max(maximum,p);++riftVertexCount;
        const glm::vec3 local(rift.worldToLocal*glm::vec4(p,1));
        assert(close(local.z,0));
    }
    assert(riftVertexCount==6*4*3);
    assert(minimum.y>=3 && maximum.y<=10.5f);
    assert(minimum.z>=-8.2f && maximum.z<=-3.8f); // Local aperture remains bounded.
    assert(close(minimum.x,10) && close(maximum.x,10));
    const auto frozen=buildEnvironmentGeometry(scene);
    assert(withRift==frozen);
    std::cout<<"PASS: configurable transformation, no phantom geometry, translated floor backdrop, transformed rift/light alignment, stable slot IDs, deterministic geometry\n";
}
