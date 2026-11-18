#include "engine/aim_state.h"
#include "engine/camera.h"
#include "engine/simple_json.h"
#include "engine/view_lens.h"
#include "player/player.h"
#include "player/weapon.h"
#include "enemy/enemy_manager.h"
#include "hud/bitmap_font_renderer.h"
#include "test_support.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
using namespace silic2;
using namespace silic2::test;

static void checkLensProjection() {
    constexpr float aspect = 800.f / 600.f;
    const auto hip = ViewLens::forPlayer(0.f, aspect);
    const auto transition = ViewLens::forPlayer(.5f, aspect);
    const auto aimed = ViewLens::forPlayer(1.f, aspect);
    require(hip.strength > transition.strength && transition.strength > aimed.strength,
            "peripheral distortion decreases as the player aims");

    const glm::vec2 targetNdc(.75f, -.5f);
    // On an 800x600 screen this target is (300,-150) pixels from the center.
    const float radiusSquared = (300.f * 300.f + 150.f * 150.f) / (400.f * 400.f + 300.f * 300.f);
    for (const auto& lens : {hip, transition, aimed}) {
        const glm::vec2 ray = targetNdc * (1.f + lens.strength * radiusSquared);
        const auto captured = lens.overscan(glm::mat4(1.f)) * glm::vec4(ray, 0.f, 1.f);
        const auto anchor = lens.displayNdc(glm::vec2(captured) / captured.w);
        require(glm::length(anchor - targetNdc) < .0001f, "HUD anchor matches the off-axis scene sample");
        require(glm::length(lens.displayNdc(glm::vec2(0.f))) < .00001f, "aim remains at the lens center");
        require(lens.displayNdc({0.f, 1.f}).y > 1.f, "unused upper overscan margin projects outside the HUD");
    }
}

static std::string readProfile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    require(file.is_open(), "expected a saved profile");
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

static void checkFisheyeSettings() {
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto folder = std::filesystem::temp_directory_path() / ("silic2-settings-" + std::to_string(unique));
    require(std::filesystem::create_directory(folder), "isolated profile directory created");
    const auto file = folder / "generalconfig.local.json";
    const auto legacy = folder / "fisheye.local.json";
    auto settings = FisheyeSettings::load(file);
    require(settings.enabled && settings.hipStrength == .33 && !std::filesystem::exists(file),
            "startup defaults do not write preferences");

    std::ofstream(legacy) << R"({"enabled":false,"hipStrength":0.375})";
    settings = FisheyeSettings::load(file);
    const auto migrated = SimpleJson::parse(readProfile(file));
    require(!settings.enabled && settings.hipStrength == .375 &&
            !migrated["fisheye"].getBool("enabled", true) && migrated["fisheye"].getNumber("hipStrength") == .375,
            "legacy settings migrate to the nested general profile with their exact values");
    require(!std::filesystem::exists(legacy), "successful migration retires the legacy profile");
    std::ofstream(legacy) << R"({"enabled":true,"hipStrength":1.25})";
    settings = FisheyeSettings::load(file);
    require(!settings.enabled && settings.hipStrength == .375, "existing general profile wins over stale legacy values");

    // Another editor adds settings after launch, before the next hotkey save.
    std::ofstream(file) << R"({"audio":{"muted":true},"fisheye":{"enabled":false,"hipStrength":0.375,"tint":"blue"}})";
    require(settings.adjustHip(true) && std::abs(settings.hipStrength - .425) < 1e-9,
            "one bracket step preserves a custom fractional strength");
    settings.save(file);
    const auto merged = SimpleJson::parse(readProfile(file));
    require(merged["audio"].getBool("muted") && merged["fisheye"].getString("tint") == "blue",
            "saving a hotkey adjustment preserves external edits at root and section level");
    const auto saved = FisheyeSettings::load(file);
    require(!saved.enabled && saved.hipStrength == settings.hipStrength, "disabled strength survives reload");
    for (const float aimBlend : {0.f, .5f, 1.f})
        require(ViewLens::forPlayer(aimBlend, 1.6f, saved).strength == 0.f, "OFF bypasses hip and ADS");

    std::ofstream(file) << R"({"fisheye":{"enabled":"bad value","hipStrength":9.0}})";
    settings = FisheyeSettings::load(file);
    require(settings.enabled && settings.hipStrength == 1.5 && !settings.adjustHip(true),
            "invalid toggle defaults and numeric upper limit holds");
    const auto maximum = ViewLens::forPlayer(0.f, 1.6f, settings);
    require(maximum.strength == 1.5f && glm::length(maximum.displayNdc({1.f, 1.f}) - glm::vec2(1.f)) < 1.e-5f,
            "maximum strength maps the capture corner back to the display corner");
    require(ViewLens::forPlayer(1.f, 1.6f, settings).strength == .08f, "full ADS is independent of the hip setting");
    std::ofstream(file) << R"({"fisheye":{"hipStrength":-1}})";
    settings = FisheyeSettings::load(file);
    require(settings.hipStrength == 0.0 && !settings.adjustHip(false), "lower setting limit holds");

    const std::string invalid = "{unfinished";
    std::ofstream(file) << invalid;
    settings = FisheyeSettings::load(file);
    require(settings.enabled && settings.hipStrength == .33, "malformed general profile falls back to defaults");
    bool saveFailed = false;
    try { settings.save(file); } catch (const std::exception&) { saveFailed = true; }
    require(saveFailed && readProfile(file) == invalid, "a failed save leaves the malformed profile intact");
    std::filesystem::remove_all(folder);
}

int main(){try {
    checkLensProjection();
    checkFisheyeSettings();
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
