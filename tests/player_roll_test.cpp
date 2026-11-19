#include "engine/camera.h"
#include "engine/map.h"
#include "player/player.h"
#include "test_support.h"

#include <algorithm>
#include <cmath>

using namespace silic2;
using silic2::test::require;
using silic2::test::requireNear;

namespace {

constexpr float rollDistance=5.4f;

Map rollArena(bool spatial) {
    // A thin wall faces the spawn at x=0. The lanes at x=8 are unobstructed.
    auto document=SimpleJson::parse(R"({
        "schemaVersion":3,"gameMode":"legacy","environmentPreset":"none",
        "geometry":{"objects":[
            {"id":1,"shape":"plane","position":[0,0,0],"size":[40,0,40],"type":"floor"},
            {"id":2,"shape":"box","position":[0,2,-2],"size":[3,4,0.1],"type":"wall"}
        ]}
    })");
    if(spatial)
        document["spatial"]=SimpleJson::parse(R"({"mode":"triangles","navigation":{"nodes":[],"links":[]}})");
    Map map;
    require(map.loadFromJson(document),"load roll fixture");
    require(bool(map.getSpatialWorld())==spatial,"roll uses requested collision backend");
    return map;
}

void advance(Player& player,Camera& camera,const Map& map,const PlayerInput& input,
             float duration,float maxStep=.01f) {
    while(duration>1e-6f) {
        const float dt=std::min(maxStep,duration);
        player.processInput(input,&camera,dt);
        player.update(dt,&map);
        duration-=dt;
    }
}

void ground(Player& player,Camera& camera,const Map& map) {
    advance(player,camera,map,{},.05f);
    require(player.isOnGround(),"roll fixture starts on the floor");
}

void directionAndDistance(const Map& map,float step) {
    Player player({8,.015f,0});
    Camera camera;
    camera.setDirection({0,.8f,-.6f});
    ground(player,camera,map);
    const auto start=player.getPosition();
    const float standingEye=player.getEyePosition().y-start.y;
    PlayerInput diagonal;
    diagonal.roll=diagonal.forward=diagonal.left=true;
    advance(player,camera,map,diagonal,step,step);
    require(player.getState()==PlayerState::ROLLING,"left diagonal starts a roll");
    camera.setDirection({1,0,0});
    PlayerInput opposite;
    opposite.right=true;
    advance(player,camera,map,opposite,.16f-step,step);
    requireNear(player.getRollProgress(),.5f,.005f,"roll midpoint tracks elapsed time");
    const auto midEye=player.getEyePosition();
    require(midEye.y-player.getPosition().y<standingEye-.1f,"eyes dip during roll");
    const float midProgress=player.getRollProgress();
    player.update(0,&map);
    require(player.getRollProgress()==midProgress && player.getEyePosition()==midEye,"zero simulation time freezes roll pose");
    requireNear(player.takeDamage(15),15,.001f,"player still takes damage during a roll");
    advance(player,camera,map,{},.16f,step);
    const auto displacement=player.getPosition()-start;
    const float axis=-rollDistance/std::sqrt(2.f);
    requireNear(displacement.x,axis,.015f,"diagonal roll distance is normalized");
    requireNear(displacement.z,axis,.015f,"pitch, steering and yaw do not redirect a committed roll");
    require(player.getState()!=PlayerState::ROLLING && player.getRollProgress()==0,"roll finishes in 0.32 seconds");
    requireNear(player.getEyePosition().y-player.getPosition().y,standingEye,.001f,"eye height recovers at roll end");
}

void buttonAndLifecycle(const Map& map) {
    Player player({8,.015f,0});
    Camera camera;
    camera.setDirection({0,0,-1});
    ground(player,camera,map);
    PlayerInput held;
    held.roll=true;
    player.syncRollButtons(true,false);
    advance(player,camera,map,held,.10f);
    requireNear(player.getPosition().z,0,.001f,"C held on entering play is consumed");
    advance(player,camera,map,{},.01f);
    advance(player,camera,map,held,.98f);
    requireNear(player.getPosition().z,-rollDistance,.015f,"stationary tap rolls along camera forward");
    advance(player,camera,map,{},.01f);
    advance(player,camera,map,held,.10f);
    requireNear(player.getPosition().z,-rollDistance,.015f,"press just before one second is consumed through cooldown expiry");
    advance(player,camera,map,{},.01f);
    advance(player,camera,map,held,.16f);
    require(player.getState()==PlayerState::ROLLING,"fresh tap after one second starts a new roll");
    advance(player,camera,map,held,1.20f);
    requireNear(player.getPosition().z,-2*rollDistance,.02f,"holding C across multiple cooldowns rolls once");

    advance(player,camera,map,{},.01f);
    advance(player,camera,map,held,.08f);
    player.respawn({8,2,0});
    require(player.getState()!=PlayerState::ROLLING && player.getRollProgress()==0,"respawn clears active roll");
    advance(player,camera,map,held,.7f);
    require(player.isOnGround() && std::abs(player.getPosition().z)<.001f,"airborne press never buffers a landing roll");
    advance(player,camera,map,{},.01f);
    advance(player,camera,map,held,.08f);
    require(player.getState()==PlayerState::ROLLING,"respawn clears prior roll cooldown");
}

void rollInterruptsSlide(const Map& map) {
    Player player({8,.015f,0});
    Camera camera;
    camera.setDirection({0,0,-1});
    ground(player,camera,map);
    PlayerInput sprint;
    sprint.forward=sprint.sprint=true;
    advance(player,camera,map,sprint,.05f);
    sprint.crouch=true;
    advance(player,camera,map,sprint,.03f);
    require(player.getState()==PlayerState::SLIDING,"Ctrl starts a sprint slide");
    const auto start=player.getPosition();
    PlayerInput sideways;
    sideways.right=sideways.crouch=sideways.roll=true;
    advance(player,camera,map,sideways,.08f);
    require(player.getState()==PlayerState::ROLLING,"C takes over an active slide");
    advance(player,camera,map,sprint,.22f);
    advance(player,camera,map,{},.02f);
    advance(player,camera,map,{},.12f);
    const auto atRest=player.getPosition();
    requireNear(atRest.x-start.x,rollDistance,.015f,"sideways roll ends without extra momentum");
    requireNear(atRest.z,start.z,.015f,"old slide momentum and steering cannot bend roll");
    PlayerInput walk;
    walk.left=true;
    advance(player,camera,map,walk,.05f);
    require(player.getState()==PlayerState::WALKING && player.getPosition().x<atRest.x-.1f,
            "normal walking resumes after the interrupted slide");
}

void wallStopsRoll(const Map& map) {
    for(float step:{.32f,1.f/30,1.f/144}) {
        Player player({0,.015f,0});
        Camera camera;
        camera.setDirection({0,0,-1});
        ground(player,camera,map);
        PlayerInput press;
        press.roll=true;
        advance(player,camera,map,press,.32f,step);
        require(player.getPosition().z<-.9f,"roll reaches the nearby wall at every frame step");
        require(player.getAABB().min.z>=-1.96f,"roll cannot tunnel through a thin wall");
    }
}

void runOutOfRoll(const Map& map) {
    struct Direction { bool PlayerInput::*key; glm::vec3 axis; };
    const Direction directions[]={
        {&PlayerInput::forward,{0,0,-1}}, {&PlayerInput::backward,{0,0,1}},
        {&PlayerInput::left,{-1,0,0}}, {&PlayerInput::right,{1,0,0}}
    };
    for(const auto& direction:directions) {
        Player player({8,.015f,0});
        Camera camera;
        camera.setDirection({0,0,-1});
        ground(player,camera,map);
        PlayerInput launch;
        launch.roll=true;
        advance(player,camera,map,launch,.16f);
        PlayerInput run;
        run.*direction.key=true;
        advance(player,camera,map,run,.16f);
        const auto exit=player.getPosition();
        require(player.isSprinting(),"held direction enters sprint at roll end without Shift");
        advance(player,camera,map,run,.06f);
        const auto distance=glm::dot(player.getPosition()-exit,direction.axis);
        requireNear(distance,.06f*GameConfig::getInstance().player.sprintSpeed,.003f,"first running steps use full directional speed");
        advance(player,camera,map,{},.5f);
        require(!player.isSprinting() && player.getState()==PlayerState::IDLE,"releasing movement ends the roll sprint");
        const auto stopped=player.getPosition();
        advance(player,camera,map,{},.12f);
        require(glm::length(player.getPosition()-stopped)<.001f,"released movement stays stopped");
    }
}

void jumpOutOfRoll(const Map& map,float pressTime) {
    Player player({8,.015f,0});
    Camera camera;
    camera.setDirection({0,0,-1});
    ground(player,camera,map);
    PlayerInput launch;
    launch.roll=true;
    advance(player,camera,map,launch,.16f);
    PlayerInput exit;
    exit.right=exit.backward=true;
    advance(player,camera,map,exit,pressTime-.16f);
    exit.jump=true;
    advance(player,camera,map,exit,.01f);
    exit.jump=false;
    if(pressTime<.31f)
        advance(player,camera,map,exit,.32f-pressTime-.01f);
    require(player.getVelocity().y>0 && !player.isOnGround(),"late tap or first post-roll tap immediately jumps");
    const auto takeoff=player.getPosition();
    advance(player,camera,map,{},.06f);
    const auto travel=player.getPosition()-takeoff;
    const float axis=.06f*GameConfig::getInstance().player.sprintSpeed/std::sqrt(2.f);
    requireNear(travel.x,axis,.035f,"jump carries lateral sprint speed after releasing keys");
    requireNear(travel.z,axis,.035f,"jump carries backward sprint speed normalized on the diagonal");
    require(travel.y>.2f,"roll jump rises from the arena floor");
}

void jumpBufferLifecycle(const Map& map) {
    Player player({8,.015f,0});
    Camera camera;
    camera.setDirection({0,0,-1});
    ground(player,camera,map);
    PlayerInput launch;
    launch.roll=true;
    PlayerInput jump;
    jump.jump=true;
    advance(player,camera,map,launch,.12f);
    advance(player,camera,map,jump,.25f);
    require(player.isOnGround(),"holding a jump pressed early never converts to a buffered tap");

    player.respawn({19,.015f,0});
    ground(player,camera,map);
    launch.right=true;
    advance(player,camera,map,launch,.25f);
    advance(player,camera,map,jump,.08f);
    require(!player.isOnGround() && player.getVelocity().y<=0,"late Space cannot jump after rolling off a ledge");

    player.respawn({8,.015f,0});
    ground(player,camera,map);
    advance(player,camera,map,launch,.25f);
    advance(player,camera,map,jump,.01f);
    player.respawn({8,.015f,0});
    ground(player,camera,map);
    advance(player,camera,map,launch,.34f);
    require(player.isOnGround(),"respawn clears a pending roll jump");

    for(bool queuedBeforePause:{false,true}) {
        player.respawn({8,.015f,0});
        ground(player,camera,map);
        advance(player,camera,map,launch,.25f);
        advance(player,camera,map,queuedBeforePause ? jump : PlayerInput{},.01f);
        // No simulation ticks in the menu. Resume with C released and Space held.
        player.syncRollButtons(false,true);
        advance(player,camera,map,jump,.08f);
        require((player.getVelocity().y>0)==queuedBeforePause,
                "resuming preserves a queued jump and consumes a Space press made in the menu");
    }
}

void rollViewPreservesAim(const Map& map, int startingSide) {
    Camera camera;
    camera.setDirection({.6f,.3f,-.7f});
    Player player({8,.015f,0});
    ground(player,camera,map);
    auto frame=[&](const PlayerInput& keys) {
        player.processInput(keys,&camera,.01f);
        const bool rollStep=player.getState()==PlayerState::ROLLING;
        player.update(.01f,&map);
        camera.setPosition(player.getEyePosition());
        camera.updateMovementLean(player.getPosition(),.01f,rollStep);
    };
    PlayerInput approach;
    approach.left=startingSide<0;
    approach.right=startingSide>0;
    // Enter while the lean is still changing, so stale angular velocity is observable on exit.
    for(int i=0;i<8;++i) frame(approach);
    const float entryLean=camera.getMovementRollDegrees();
    if(startingSide==0) require(entryLean==0,"stationary roll starts level");
    else require(entryLean*startingSide>.5f,"strafe establishes an entry angle on the requested side");
    const int exitSide=startingSide>0 ? -1 : 1;
    PlayerInput strafe;
    strafe.left=exitSide<0;
    strafe.right=exitSide>0;
    strafe.roll=true;
    for(int i=0;i<32;++i) {
        if(i==12) {
            const auto before=camera.getFront();
            camera.processMouseMovement(33.f,-10.f);
            require(glm::dot(before,camera.getFront())<.95f,"mouse look changes aim during the roll");
        }
        frame(strafe);
        const auto view=camera.getViewMatrix();
        const auto viewUp=glm::vec3(glm::inverse(view)[1]);
        const float renderedLean=glm::degrees(std::atan2(glm::dot(viewUp,camera.getRight()),
                                                        glm::dot(viewUp,camera.getUp())));
        requireNear(renderedLean,entryLean,.0002f,"rendered tilt holds the entry angle through the last roll displacement");
        const auto target=glm::vec4(camera.getPosition()+camera.getFront()*5.f,1.f);
        const auto clip=camera.getProjectionMatrix(1.6f)*view*target;
        require(glm::length(glm::vec2(clip)/clip.w)<1e-5f,"logical shot stays at reticle during a side roll");
    }
    require(player.getState()!=PlayerState::ROLLING,"last sampled roll frame includes the transition to running");
    frame(strafe);
    const float firstStep=(camera.getMovementRollDegrees()-entryLean)*exitSide;
    require(firstStep>0 && firstStep<.13f,"normal strafe resumes smoothly from the held angle in the new direction");
    for(int i=0;i<25;++i) frame(strafe);
    require(camera.getMovementRollDegrees()*exitSide>1,"ordinary strafe settles on the new side after the roll");
}

} // namespace

void runPlayerRollTests() {
    for(bool spatial:{false,true}) {
        const auto map=rollArena(spatial);
        for(float step:{1.f/30,1.f/144})
            directionAndDistance(map,step);
        buttonAndLifecycle(map);
        rollInterruptsSlide(map);
        wallStopsRoll(map);
        runOutOfRoll(map);
        jumpOutOfRoll(map,.24f);
        jumpOutOfRoll(map,.32f);
        jumpBufferLifecycle(map);
        for(int startingSide:{-1,0,1}) rollViewPreservesAim(map,startingSide);
    }
}
