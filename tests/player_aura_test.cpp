#include "effects/groundparticle.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace silic2;
static void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
namespace silic2 {
struct GroundParticleProbe {
    static std::vector<Particle> live(const GroundParticleSystem& g){
        std::vector<Particle> result;for(const auto& p:g.particleSystem->particles)if(p.isAlive())result.push_back(p);return result;
    }
    static bool soft(const GroundParticleSystem& g){return !g.particleSystem->use3DBoxes;}
};
}
int main(){try {
    auto aura=createEnhancedGroundParticleSystem(96,GroundParticleSystem::GParticleMode::CRYSTAL);
    const glm::vec3 feet(20,2,30);aura->setEmitterPosition(feet);aura->update(.2f);
    const auto first=GroundParticleProbe::live(*aura);
    require(!first.empty()&&first.size()<=3&&GroundParticleProbe::soft(*aura),"Crystal starts sparse soft points without requiring a floor mesh");
    for(const auto& p:first){
        const float radius=glm::length(glm::vec2(p.pPosition.x-feet.x,p.pPosition.z-feet.z));
        require(radius>=.9f&&radius<=4.2f&&p.pPosition.y>feet.y,"Motes spawn in local annulus above actual feet");
        require(p.pVelocity.y>=.65f&&p.pVelocity.y<=1.1f&&p.pGravity==0,"Motes rise gently without falling");
        require(p.pColor.b>p.pColor.g&&p.pColor.g>p.pColor.r&&p.pSize<=.20f,"Blue small point shader inputs preserve center visibility");
    }
    aura->update(0);const auto frozen=GroundParticleProbe::live(*aura);
    require(first.size()==frozen.size(),"Zero simulation delta does not emit");
    for(size_t i=0;i<first.size();++i)require(first[i].pPosition==frozen[i].pPosition&&first[i].pLife==frozen[i].pLife&&first[i].pColor==frozen[i].pColor,"Pause delta leaves motion/life/color exact");
    aura->setEmissionRate(0);aura->update(.5f);const auto risen=GroundParticleProbe::live(*aura);
    require(risen.size()==first.size(),"Existing motes persist while emission stops");
    for(size_t i=0;i<first.size();++i)require(risen[i].pPosition.y>first[i].pPosition.y+.3f,"Actual particles move upward");
    for(int i=0;i<200;++i){aura->update(.02f);for(const auto& p:GroundParticleProbe::live(*aura))require(std::isfinite(p.pColor.r)&&std::isfinite(p.pColor.g)&&std::isfinite(p.pColor.b),"Fade tail remains finite");}
    require(aura->getActiveParticles()==0,"Motes expire within their three-second lifetime");
    aura->reset();aura->setEmissionRate(18);aura->update(.2f);const auto repeated=GroundParticleProbe::live(*aura);
    require(first.size()==repeated.size(),"Reset clears emission timer and restores deterministic local seed");
    for(size_t i=0;i<first.size();++i)require(first[i].pPosition==repeated[i].pPosition&&first[i].pVelocity==repeated[i].pVelocity,"Same reset and input reproduce emitter");
    const glm::vec3 moved(50,4,60);aura->setEmitterPosition(moved);require(aura->getActiveParticles()==0&&aura->getEmitterPosition()==moved,"Teleport clears old wake and reanchors");aura->update(.1f);
    require(aura->getActiveParticles()>0,"Emission resumes at new anchor");
    auto capped=createEnhancedGroundParticleSystem(2000,GroundParticleSystem::GParticleMode::CRYSTAL);capped->setEmissionRate(10000);capped->update(1000);require(capped->getActiveParticles()<=96,"Long frame and large backing pool still cap crystal particles at96");
    for(int i=0;i<60;++i){capped->update(.05f);require(capped->getActiveParticles()<=96,"Sustained emission stays bounded");}
    Map empty;auto legacy=createEnhancedGroundParticleSystem(2000,GroundParticleSystem::GParticleMode::FIRE);legacy->initialize(empty);legacy->update(.2f);
    require(!GroundParticleProbe::soft(*legacy)&&legacy->getActiveParticles()>0,"Legacy fire still uses rising boxes");
    legacy->reset();legacy->setEnabled(false);legacy->update(.2f);require(legacy->getActiveParticles()==0,"Disabled arena/Hollow mode does not emit");
    std::cout<<"Player aura CPU passed: local soft rising particles, zero-delta freeze, lifetime/finite fade, reset/determinism/teleport,96cap and legacy fire isolation.\n";
}catch(const std::exception& e){std::cerr<<"Player aura CPU failed: "<<e.what()<<'\n';return 1;}}
