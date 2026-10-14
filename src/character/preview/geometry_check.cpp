// CPU-only geometry contract check: no window/context/device creation.
#include "character/character_presentation.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
namespace silic2 {
struct CharacterGeometryProbe {
    static size_t check(CharacterPresentation& p, const CharacterView& c) {
        p.vertices.clear();
        p.addCharacter(c);
        assert(!p.vertices.empty());
        assert(p.vertices.size()%3==0);
        for(const auto& vertex:p.vertices) {
            const auto local=vertex.position-c.position;
            for(int axis=0;axis<3;++axis) {
                assert(std::isfinite(vertex.position[axis]));
                assert(std::isfinite(vertex.normal[axis]));
            }
            assert(std::abs(local.x)<=c.dimensions.x*.5f+1e-5f);
            assert(local.y>=-1e-5f && local.y<=c.dimensions.y+1e-5f);
            assert(std::abs(local.z)<=c.dimensions.z*.5f+1e-5f);
            assert(std::abs(glm::length(vertex.normal)-1)<1e-4f);
        }
        return p.vertices.size();
    }
};
}
int main() {
    using namespace silic2;
    CharacterPresentation p;
    FirstPersonView player;
    size_t chaserMax=0,sentryMax=0;
    for(int tick=0;tick<40;++tick) {
        p.update(.025f,player);
        for(int yaw=0;yaw<32;++yaw) for(int state=0;state<4;++state) {
            CharacterView c;
            float angle=yaw*6.28318530718f/32;
            c.position={1.3f,-.1f,-2.7f};c.heading={std::sin(angle),0,std::cos(angle)};
            c.movement=1;c.charging=state>0;c.aimLocked=state>1;c.hitFlash=state==3?1.f:0.f;
            c.chargeProgress=tick/39.f;
            chaserMax=std::max(chaserMax,CharacterGeometryProbe::check(p,c));
            c.type=EnemyType::Sentry;c.dimensions={1.2f,1,1.2f};
            sentryMax=std::max(sentryMax,CharacterGeometryProbe::check(p,c));
        }
    }
    // 36 vertices per dry fragment; all caps charged/active simultaneously.
    const size_t worst=18*chaserMax+3*sentryMax+36*p.EffectCapacity;
    assert(worst<=65536);
    for(int i=0;i<100;++i)p.emit(CharacterEffect::Death,{0,0,0});
    assert(p.activeEffectCount()==p.EffectCapacity);
    p.update(0,player);assert(p.activeEffectCount()==p.EffectCapacity);
    p.update(std::numeric_limits<float>::quiet_NaN(),player);assert(p.activeEffectCount()==p.EffectCapacity);
    p.update(1,player);assert(p.activeEffectCount()==0);
    p.reset();assert(p.activeEffectCount()==0);
    std::cout<<"CPU: 10240 animated/type/state/yaw bounds checks passed; Chaser vertices="
             <<chaserMax<<", Sentry vertices="<<sentryMax<<", capped world vertices="<<worst
             <<"/65536; effect capacity/freeze/expiry/reset passed\n";
}
