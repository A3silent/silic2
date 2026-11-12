#include "engine/aim_state.h"
#include "engine/camera.h"
#include "player/player.h"
#include "player/weapon.h"
#include "enemy/enemy_manager.h"
#include "hud/bitmap_font_renderer.h"
#include "test_support.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <iostream>
using namespace silic2;
using namespace silic2::test;
int main(){try {
    AimState aim;
    require(aim.blend()==0 && aim.fov(45)==45 && aim.mouseScale(45)==1,"hip baseline exact");
    aim.update(true,.08f);requireNear(aim.blend(),.5f,.00001f,"half-transition smooth blend");
    const auto frozen=aim.blend();aim.update(false,10,false);require(aim.blend()==frozen,"paused input cannot advance blend");
    aim.update(false,0);require(aim.blend()==frozen,"zero dt leaves state unchanged");
    aim.update(false,.08f);require(aim.blend()==0,"release reverses partial entry without residue");
    for(int rate:{30,60,144,240}) {
        AimState a;float elapsed=0;while(elapsed<.16f){float dt=std::min(1.f/rate,.16f-elapsed);a.update(true,dt);elapsed+=dt;}
        requireNear(a.blend(),1,.00001f,"entry duration stable across dt");
        requireNear(a.fov(70),32,.0001f,"sprint FOV also reaches target");a.reset();require(a.blend()==0,"reset discards target and progress");
    }
    aim.update(true,.2f);Camera camera({1,2,3});camera.setDirection({.4f,.1f,-1});
    Player player;const float yaw=camera.getYaw();player.processMouseMovement(&camera,10*aim.mouseScale(45),0);
    requireNear(camera.getYaw()-yaw,10*GameConfig::getInstance().player.mouseSensitivity*aim.mouseScale(45),.0001f,"actual player mouse path respects ADS multiplier");
    auto feet=glm::vec3(0);camera.resetMovementLean(feet);feet+=camera.getRight()*.1f;camera.updateMovementLean(feet,.02f);
    for(float fov:{45.f,aim.fov(45)}) {
        const auto p=camera.getProjectionMatrix(1.6f,fov,600);
        auto center=glm::inverse(p*camera.getViewMatrix())*glm::vec4(0,0,-1,1);
        require(glm::length(glm::normalize(glm::vec3(center)/center.w-camera.getPosition())-camera.getFront())<.00001f,"zoom and roll preserve logical center ray");
    }
    Weapon weapon;require(weapon.fire(camera,true),"actual ADS fire accepted");
    auto shot=weapon.getBullets().back();require(shot.position==camera.getPosition() && glm::length(shot.direction-camera.getFront())<.000001f && shot.speed==195,"ADS shot starts at eye and follows center at195");
    require(!weapon.fire(camera,true),"ADS retains firing cooldown");weapon.reset();require(weapon.fire(camera),"hip fire still accepted");
    auto hip=weapon.getBullets().back();const auto oldStart=camera.getPosition()+camera.getFront()*.3f+camera.getRight()*.2f-camera.getUp()*.15f;
    require(glm::length(hip.position-oldStart)<.000001f,"hip muzzle offset preserved");
    require(glm::length(hip.direction-glm::normalize(camera.getPosition()+camera.getFront()*100.f-oldStart))<.000001f,"hip convergence preserved");
    Map nearWall;require(nearWall.loadFromJson(SimpleJson::parse(R"({"schemaVersion":2,"gameMode":"legacy","environmentPreset":"none","geometry":{"objects":[{"id":1,"shape":"box","position":[0,1.6,-0.1],"size":[2,3,0.04]}]},"entities":[],"lights":[]})")),"near wall fixture loads: "+nearWall.getLastError());
    camera.setPosition({0,1.6,0});camera.setDirection({0,0,-1});weapon.reset();weapon.fire(camera,true);weapon.update(.01f,&nearWall);
    require(weapon.getActiveBulletCount()==0,"ADS swept projectile cannot skip wall within .3m");
    auto spatialDoc=nearWall.toJson();spatialDoc["schemaVersion"]=SimpleJson(3.0);spatialDoc["spatial"]=SimpleJson::parse(R"({"mode":"triangles","navigation":{"nodes":[],"links":[]}})");
    const bool spatialLoaded=nearWall.loadFromJson(spatialDoc);require(spatialLoaded,"spatial near-wall fixture loads: "+nearWall.getLastError());weapon.reset();weapon.fire(camera,true);weapon.update(.005f,&nearWall);
    require(weapon.getActiveBulletCount()==0,"ADS first swept segment hits spatial triangle wall");
    EnemyManager enemies;enemies.spawn(EnemyType::Chaser,{0,0,-1});weapon.reset();weapon.fire(camera,true);
    const auto result=weapon.update(.01f,nullptr,&enemies);
    require(result.hits==1 && enemies.getEnemies().front().getHp()==5,"ADS first segment hits near enemy with unchanged one damage");
    weapon.reset();weapon.fire(camera);weapon.update(.005f,&nearWall);require(weapon.getActiveBulletCount()==1,"hip collision grace remains unchanged");
    HudViewModel model;model.phase=HudPhase::Playing;HudBatch normal,ads,hit;buildHudGeometry(normal,model,1280,720);model.crosshairOpacity=0;buildHudGeometry(ads,model,1280,720);
    require(ads.vertices.size()<normal.vertices.size(),"normal crosshair disappears in ADS");model.hitAge=0;buildHudGeometry(hit,model,1280,720);require(hit.vertices.size()>ads.vertices.size(),"hitmarker survives ADS");
    std::cout<<"ADS CPU passed: dt/pause/reset, FOV/mouse, center ray, actual bullet/cooldown/hip parity/near-wall, HUD hitmarker\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"ADS CPU failure: "<<e.what()<<'\n';return 1;}}
