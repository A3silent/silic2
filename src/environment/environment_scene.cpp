#include "environment/environment_scene.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace silic2 {

static std::vector<float> buildHollowOrbitGeometry(const EnvironmentScene& scene) {
    constexpr float pi = 3.14159265359f;
    // All megastructure vertices remain outside the authored floor rectangle.
    // Nothing is added to Map brushes/collision; slot apertures stay hollow.
    std::vector<float> data;
    auto triangle = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, float shade) {
        const glm::vec3 n = glm::normalize(glm::cross(b-a, c-a));
        for (auto p : {a,b,c}) data.insert(data.end(), {p.x,p.y,p.z,n.x,n.y,n.z,shade});
    };
    auto beam = [&](glm::vec3 a,glm::vec3 b,float width,float depth,float shade) {
        const glm::vec3 direction=glm::normalize(b-a);
        const glm::vec3 reference=std::abs(direction.y)>.9f?glm::vec3(1,0,0):glm::vec3(0,1,0);
        const glm::vec3 right=glm::normalize(glm::cross(reference,direction))*width;
        const glm::vec3 up=glm::normalize(glm::cross(direction,right))*depth;
        const std::array<glm::vec3,8> v{{a-right-up,a+right-up,a+right+up,a-right+up,
                                      b-right-up,b+right-up,b+right+up,b-right+up}};
        const int faces[]={0,3,2,0,2,1,4,5,6,4,6,7,0,1,5,0,5,4,
                           1,2,6,1,6,5,2,3,7,2,7,6,3,0,4,3,4,7};
        for(int i=0;i<36;i+=3)triangle(v[faces[i]],v[faces[i+1]],v[faces[i+2]],shade);
    };
    const glm::vec2 centerXZ=(scene.minimumXZ+scene.maximumXZ)*.5f;
    const glm::vec2 halfExtent=(scene.maximumXZ-scene.minimumXZ)*.5f;
    for(int i=0;scene.hasFloor && i<14;++i){
        const float angle=(i+.17f)*2.f*pi/14.f;
        const glm::vec3 radial(std::cos(angle),0,std::sin(angle));
        const float xEdge=halfExtent.x/std::max(std::abs(radial.x),.0001f);
        const float zEdge=halfExtent.y/std::max(std::abs(radial.z),.0001f);
        const float edge=std::min(xEdge,zEdge);
        const glm::vec3 outward=xEdge<zEdge?glm::vec3(radial.x>0?1.f:-1.f,0,0):
                                             glm::vec3(0,0,radial.z>0?1.f:-1.f);
        const glm::vec3 tangent(-outward.z,0,outward.x);
        const glm::vec3 base=glm::vec3(centerXZ.x,scene.floorY-2.f,centerXZ.y)+radial*edge+outward*9.f;
        const float height=17.f+7.f*(.5f+.5f*std::sin(i*3.71f));
        const float variation=float(i%5)/5.f;
        glm::vec3 previous=base;
        for(int segment=1;segment<=5;++segment){
            const float t=segment/5.f;
            // Inward lean never reaches the playable rectangle (9-unit setback).
            const glm::vec3 next=base+glm::vec3(0,height*t,0)-outward*(3.f*t*t)+tangent*(.6f*std::sin(t*4.f+i));
            beam(previous,next,.46f-.17f*t,.32f,variation);
            const glm::vec3 spur=next+tangent*(segment%2?2.4f:-2.0f)+glm::vec3(0,-1.4f,0);
            if(segment>=2)beam(next,spur,.16f,.12f,1.f+variation);
            previous=next;
        }
        // Broken upper rails and hanging calcareous braces reveal their second
        // material in the timed transformation without moving any vertices.
        beam(previous-tangent*2.7f,previous+tangent*3.3f,.27f,.23f,variation);
        const glm::vec3 joint=base+glm::vec3(0,height*.61f,0)-outward*1.5f;
        beam(joint+tangent*2.f,joint+tangent*3.f+glm::vec3(0,-4.f,0),.14f,.11f,2.f+variation);
    }
    for(int i=0;i<4;++i){
        if(!scene.rifts[i].active)continue;
        const auto& transform=scene.rifts[i].localToWorld;
        const glm::vec3 center(0,1.25f,0);
        const std::array<glm::vec3,8> rim{{{-.82f,.10f,0},{-1.05f,.82f,0},{-.88f,1.86f,0},
            {-.35f,2.45f,0},{.30f,2.18f,0},{.83f,1.72f,0},{.67f,.72f,0},{.18f,.12f,0}}};
        for(int j=0;j<8;++j){
            if(j==2 || j==6)continue; // Incomplete slit, no opaque membrane.
            const glm::vec3 u=rim[j],v=rim[(j+1)%8];
            const glm::vec3 p(transform*glm::vec4(u,1)),q(transform*glm::vec4(v,1));
            const glm::vec3 r(transform*glm::vec4(center+(v-center)*.91f,1));
            const glm::vec3 t(transform*glm::vec4(center+(u-center)*.91f,1));
            const float id=-1.f-i;
            triangle(p,q,r,id);triangle(p,r,t,id);triangle(p,r,q,id);triangle(p,t,r,id);
        }
    }
    return data;
}

static std::vector<float> buildStarryCrystalGeometry(const EnvironmentScene& scene) {
    constexpr float pi = 3.14159265359f;
    // Decorative silhouettes follow the authored floor's center and extent.
    // Construction is independent from encounter/weapon random state.
    std::vector<float> data;
    auto triangle = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, float shade) {
        const glm::vec3 n = glm::normalize(glm::cross(b-a, c-a));
        for (auto p : {a,b,c}) data.insert(data.end(), {p.x,p.y,p.z,n.x,n.y,n.z,shade});
    };
    const glm::vec2 centerXZ = (scene.minimumXZ + scene.maximumXZ) * .5f;
    const glm::vec2 halfExtent = (scene.maximumXZ - scene.minimumXZ) * .5f;
    for (int i = 0; scene.hasFloor && i < 76; ++i) {
        const float a = i * 2.f * pi / 76.f;
        // Recover the sample's original ring from its floor half extent plus
        // the preset's setback; translations/resizing need no extra coordinates.
        const float setback = 8.f + 3.f * std::sin(i * 5.17f);
        const float cosA = std::cos(a), sinA = std::sin(a);
        const float ellipse = std::sqrt(1.f / (cosA*cosA/(halfExtent.x*halfExtent.x) +
                                                       sinA*sinA/(halfExtent.y*halfExtent.y)));
        const float radial = ellipse + setback;
        const glm::vec3 base(centerXZ.x + cosA*radial, scene.floorY - 1.f, centerXZ.y + sinA*radial);
        const float h = 5.f + 8.f * (.5f + .5f * std::sin(i*7.31f));
        const float radius = .6f + .8f*(.5f+.5f*std::cos(i*4.71f));
        const glm::vec3 tip = base + glm::vec3(.35f*std::sin(a),h,.35f*std::cos(a));
        for (int j = 0; j < 5; ++j) {
            const float p = j*2.f*pi/5.f, q = (j+1)*2.f*pi/5.f;
            glm::vec3 u = base + glm::vec3(radius*std::cos(p),0,radius*std::sin(p));
            glm::vec3 v = base + glm::vec3(radius*std::cos(q),0,radius*std::sin(q));
            glm::vec3 ut = u + glm::vec3(0,h*.65f,0), vt = v + glm::vec3(0,h*.65f,0);
            triangle(u,ut,vt, float(i%7)/7.f); triangle(u,vt,v,float(i%7)/7.f);
            triangle(ut,tip,vt,float(i%7)/7.f);
        }
    }
    // Open energy apertures, not solid geometry: actors can pass through the
    // hollow rift. Thin angular ribbons frame the spawn without hiding its body.
    for (int i = 0; i < 4; ++i) {
        if (!scene.rifts[i].active) continue;
        const glm::mat4& transform = scene.rifts[i].localToWorld;
        const glm::vec3 center(0,1.25f,0);
        const glm::vec3 right(1,0,0);
        for (int j = 0; j < 8; ++j) {
            const float a = j*2.f*pi/8.f, b = (j+1)*2.f*pi/8.f;
            const glm::vec3 u = right*std::cos(a)*1.05f + glm::vec3(0,std::sin(a)*1.2f,0);
            const glm::vec3 v = right*std::cos(b)*1.05f + glm::vec3(0,std::sin(b)*1.2f,0);
            const glm::vec3 p = glm::vec3(transform * glm::vec4(center+u,1));
            const glm::vec3 q = glm::vec3(transform * glm::vec4(center+v,1));
            const glm::vec3 r = glm::vec3(transform * glm::vec4(center+v*.955f,1));
            const glm::vec3 t = glm::vec3(transform * glm::vec4(center+u*.955f,1));
            const float id = -1.f-i;
            triangle(p,q,r,id); triangle(p,r,t,id);
            triangle(p,r,q,id); triangle(p,t,r,id);
        }
    }
    return data;
}

static std::vector<float> buildObservatoryGeometry(const EnvironmentScene& scene) {
    constexpr float pi=3.14159265359f;
    std::vector<float> data;
    auto tri=[&](glm::vec3 a,glm::vec3 b,glm::vec3 c,float tag) {
        const auto n=glm::normalize(glm::cross(b-a,c-a));
        for(auto p:{a,b,c})data.insert(data.end(),{p.x,p.y,p.z,n.x,n.y,n.z,tag});
        for(auto p:{c,b,a})data.insert(data.end(),{p.x,p.y,p.z,-n.x,-n.y,-n.z,tag});
    };
    auto quad=[&](glm::vec3 a,glm::vec3 b,glm::vec3 c,glm::vec3 d,float tag){tri(a,b,c,tag);tri(a,c,d,tag);};
    auto beam=[&](glm::vec3 a,glm::vec3 b,float width,float tag) {
        const auto axis=glm::normalize(b-a);
        const auto right=glm::normalize(glm::cross(axis,std::abs(axis.y)>.9f?glm::vec3(1,0,0):glm::vec3(0,1,0)))*width;
        const auto up=glm::normalize(glm::cross(axis,right))*width;
        quad(a-right-up,b-right-up,b+right-up,a+right-up,tag);
        quad(a+right-up,b+right-up,b+right+up,a+right+up,tag);
        quad(a+right+up,b+right+up,b-right+up,a-right+up,tag);
        quad(a-right+up,b-right+up,b-right-up,a-right-up,tag);
    };
    // Three physically separate circular sectors share only their perspective
    // projection from the authored eye. No camera-facing billboard or screen mask.
    const auto forward=glm::normalize(scene.focalPoint-scene.observerEye);
    const auto right=glm::normalize(glm::cross(forward,glm::vec3(0,1,0)));
    const auto up=glm::normalize(glm::cross(right,forward));
    for(int piece=0;piece<3;++piece) {
        const float depth=scene.fragmentDepths[piece];
        const auto center=glm::mix(scene.observerEye,scene.focalPoint,depth);
        const float radius=scene.ringRadius*depth;
        for(int i=0;i<16;++i){
            // Near sector faces upward to keep all reachable space clear.
            const float a=(30.f+piece*120.f+i*7.5f)*pi/180.f;
            const float b=(30.f+piece*120.f+(i+1)*7.5f)*pi/180.f;
            auto radial=[&](float t){return right*std::cos(t)+up*std::sin(t);};
            quad(center+radial(a)*radius,center+radial(b)*radius,
                 center+radial(b)*(radius+depth*.22f),center+radial(a)*(radius+depth*.22f),10.f+piece);
        }
    }
    // A broken, hollow optical vessel. The wide shoulder, pointed lower keel,
    // empty sectors and upper crown read at scene resolution without crystal spam.
    const auto c=scene.landmarkPosition;
    const auto scale=scene.landmarkScale;
    const float heights[]={-.5f,-.12f,.22f,.5f};
    const float radii[]={.08f,.52f,.4f,.12f};
    for(int j=0;j<12;++j){
        if(j==2 || j==3 || j==8)continue;
        const float a=2*pi*j/12.f,b=2*pi*(j+1)/12.f;
        for(int k=0;k<3;++k){
            auto point=[&](float t,int level){return c+glm::vec3(std::cos(t)*scale.x*radii[level],scale.y*heights[level],std::sin(t)*scale.z*radii[level]);};
            quad(point(a,k),point(b,k),point(b,k+1),point(a,k+1),20.f+float(j%3)*.2f);
            beam(point(a,k),point(a,k+1),.035f,30.f);
        }
    }
    // Suspended armillary: large negative space, deliberately broken arc.
    for(int i=0;i<40;++i){
        if((i>4 && i<9)||(i>25 && i<29))continue;
        auto p=[&](int n){float a=2*pi*n/40.f;return c+glm::vec3(std::cos(a)*scale.x*1.1f,scale.y*.31f+std::sin(a)*1.1f,std::sin(a)*scale.z*1.1f);};
        beam(p(i),p(i+1),.10f,31.f);
    }
    for(const auto& region:scene.regions){
        const auto lo=region.minimum,hi=region.maximum;
        if(region.material==0){
            // High baffles compress sky without adding unreachable collision.
            for(int i=0;i<4;++i){float z=glm::mix(lo.y,hi.y,(i+.5f)/4.f);
                beam({lo.x,scene.floorY+5.5f,z},{hi.x,scene.floorY+5.5f,z},.25f,40.f);}
        }
        if(region.material==1){
            // One broken meridian above the low north parapet. Its broad arc
            // opens the court skyline and gives the pale room an optical purpose.
            const float centerX=(lo.x+hi.x)*.5f;
            const float radius=(hi.x-lo.x)*.48f;
            const float z=scene.minimumXZ.y-.6f;
            for(int i=0;i<24;++i){
                if(i>=9 && i<=12)continue;
                auto point=[&](int k){float a=pi*k/24.f;return glm::vec3(centerX+radius*std::cos(a),scene.floorY+4.f+radius*std::sin(a),z);};
                beam(point(i),point(i+1),.30f,32.f);
            }
            // Two broken radial sighting vanes stay beyond the playable edge.
            beam({centerX-radius,scene.floorY+4.f,z},{centerX-radius-1.5f,scene.floorY+7.f,z},.18f,32.f);
            beam({centerX+radius,scene.floorY+4.f,z},{centerX+radius+1.2f,scene.floorY+6.f,z},.18f,32.f);
        }
        if(region.material==2){
            // Ceiling fins echo the thick authored screens at a larger scale.
            for(int i=0;i<3;++i){float x=glm::mix(lo.x,hi.x,(i+.5f)/3.f);
                beam({x,scene.floorY+10.f,lo.y},{x,scene.floorY+10.f,hi.y},.18f,32.f);}
        }
    }
    // Far architecture sits beyond the map boundary; sparse leaning plates,
    // not a repeated ring of small crystals enclosing another square arena.
    for(int i=0;i<7;++i){
        float x=glm::mix(scene.minimumXZ.x-8.f,scene.maximumXZ.x+8.f,i/6.f);
        float z=scene.minimumXZ.y-12.f-float(i%3)*5.f;
        glm::vec3 a(x,scene.floorY-2.f,z),b(x+4.f,scene.floorY-2.f,z+1.f);
        quad(a,b,b+glm::vec3(2,18.f+i%3*5,0),a+glm::vec3(2,18.f+i%3*5,0),41.f);
    }
    return data;
}

static std::vector<float> buildCombatWorldGeometry(const EnvironmentScene& scene) {
    EnvironmentScene apertures=scene; apertures.hasFloor=false;
    auto data=buildStarryCrystalGeometry(apertures);
    constexpr float pi=3.14159265359f;
    auto tri=[&](glm::vec3 a,glm::vec3 b,glm::vec3 c,float tint){
        auto n=glm::normalize(glm::cross(b-a,c-a));
        for(auto p:{a,b,c})data.insert(data.end(),{p.x,p.y,p.z,n.x,n.y,n.z,tint});
        for(auto p:{c,b,a})data.insert(data.end(),{p.x,p.y,p.z,-n.x,-n.y,-n.z,tint});
    };
    auto mid=(scene.minimumXZ+scene.maximumXZ)*.5f;
    // Two incomplete orbital bands behind the north skyline, always outside
    // the authored floor. Broad silhouettes, no dense ring of small crystals.
    for(int band=0;band<2;++band){
        float radius=28.f+band*10.f;
        glm::vec3 center(mid.x,scene.floorY+8.f,scene.minimumXZ.y-18.f-band*12.f);
        for(int i=0;i<40;++i){
            if((i>11&&i<16)||(i>29&&i<34))continue;
            auto p=[&](float a,float r){return center+glm::vec3(std::cos(a)*r,std::sin(a)*r,0);};
            float a=i*pi/40.f,b=(i+1)*pi/40.f;
            auto u=p(a,radius),v=p(b,radius),w=p(b,radius+1.0f),x=p(a,radius+1.0f);
            tri(u,v,w,50.f+band);tri(u,w,x,50.f+band);
        }
    }
    return data;
}

std::vector<float> buildEnvironmentGeometry(const EnvironmentScene& scene) {
    if(scene.combatWorld) return buildCombatWorldGeometry(scene);
    if(scene.observatory && scene.settings.preset==EnvironmentPreset::StarryCrystal) return buildObservatoryGeometry(scene);
    switch(scene.settings.preset) {
        case EnvironmentPreset::HollowOrbit: return buildHollowOrbitGeometry(scene);
        case EnvironmentPreset::StarryCrystal: return buildStarryCrystalGeometry(scene);
        case EnvironmentPreset::None: return {};
    }
    return {};
}

EnvironmentShaderPaths environmentShaderPaths(EnvironmentPreset preset) {
    if(preset==EnvironmentPreset::HollowOrbit)
        return {"res/shaders/environment_map.frag", "res/shaders/environment_sky.frag",
                "res/shaders/environment_crystal.frag", "res/shaders/environment_particle.vert",
                "res/shaders/environment_particle.frag"};
    return {"res/shaders/starry_crystal_map.frag", "res/shaders/starry_crystal_sky.frag",
            "res/shaders/starry_crystal_detail.frag", "res/shaders/starry_crystal_particle.vert",
            "res/shaders/starry_crystal_particle.frag"};
}

glm::mat4 environmentRiftTransform(const glm::vec3& position,
                                   const glm::vec3& rotationDegrees,
                                   const glm::vec3& scale) {
    glm::mat4 transform = glm::translate(glm::mat4(1.f),position);
    // Authored XYZ Euler rotation, applied X then Y then Z to local points.
    transform = glm::rotate(transform,glm::radians(rotationDegrees.z),glm::vec3(0,0,1));
    transform = glm::rotate(transform,glm::radians(rotationDegrees.y),glm::vec3(0,1,0));
    transform = glm::rotate(transform,glm::radians(rotationDegrees.x),glm::vec3(1,0,0));
    return glm::scale(transform,scale);
}

float environmentPhase(const EnvironmentSettings& settings, float seconds) {
    if (!std::isfinite(seconds) || settings.preset == EnvironmentPreset::None) return 0.f;
    if (seconds < settings.transformationStart) return 0.f;
    if (settings.transformationDuration <= 0.f) return 1.f;
    const float t = std::clamp((seconds-settings.transformationStart)/settings.transformationDuration,0.f,1.f);
    return t*t*(3.f-2.f*t);
}

EnvironmentScene deriveEnvironmentScene(const Map& map) {
    EnvironmentScene scene;
    scene.settings = map.getEnvironment();
    const auto& layout = map.getArenaLayout();
    scene.hasFloor = layout.hasFloor;
    scene.minimumXZ = layout.minXZ;
    scene.maximumXZ = layout.maxXZ;
    scene.floorY = layout.floorY;
    scene.combatWorld = scene.settings.preset==EnvironmentPreset::StarryCrystal && scene.settings.architecture=="crystal_combat";
    if(scene.combatWorld) {
        scene.landmarkPosition=scene.settings.landmarkPosition;
        scene.landmarkScale=scene.settings.landmarkScale;
        for(const auto& z:map.getSpatialSettings().regions) {
            int kind=z.id=="ravine"?1:z.id=="upper_walk"?2:z.id=="support_court"?3:z.id=="sky_passage"?4:0;
            scene.regions.push_back({{z.minimum.x,z.minimum.z},{z.maximum.x,z.maximum.z},kind});
        }
    }
    scene.observatory = !scene.combatWorld && map.getGameMode()==GameMode::Exploration && scene.settings.preset==EnvironmentPreset::StarryCrystal;
    if(scene.observatory) {
        const auto& e=map.getExploration();
        scene.observerEye=e.observerPosition+glm::vec3(0,e.observerEyeHeight,0);
        scene.focalPoint=e.target; scene.landmarkPosition=e.landmarkPosition;
        scene.landmarkScale=e.landmarkScale; scene.fragmentDepths=e.fragmentDepths;
        scene.ringRadius=e.ringRadius; scene.guidePoints=e.guidePoints;
        scene.exitCenter=(e.exit.minimum+e.exit.maximum)*.5f;
        for(const auto& z:e.zones) {
            int kind=z.id=="court"?1:z.id=="gallery"?2:z.id=="observation"?3:z.id=="well"?4:0;
            scene.regions.push_back({{z.minimum.x,z.minimum.z},{z.maximum.x,z.maximum.z},kind});
        }
    }
    // Rift slots are semantic identity shared with the director, never a hint
    // from material/name or assumed cardinal world coordinates.
    for (std::size_t i = 0; i < scene.rifts.size(); ++i) {
        auto& rift = scene.rifts[i];
        rift.active = scene.hasFloor && layout.riftActive[i];
        rift.position = layout.rifts[i];
        rift.scale = layout.riftScales[i];
        if (rift.active) {
            rift.localToWorld = environmentRiftTransform(rift.position,layout.riftRotations[i],rift.scale);
            rift.worldToLocal = glm::inverse(rift.localToWorld);
        }
    }
    return scene;
}

} // namespace silic2
