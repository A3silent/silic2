#include "engine/camera.h"
#include "engine/game_config.h"
#include "test_support.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <filesystem>
#include <iostream>
#include <limits>
using namespace silic2;
using namespace silic2::test;
namespace {
void advance(Camera& camera,glm::vec3& feet,glm::vec3 velocity,int frames,float dt) {
    for(int i=0;i<frames;++i){feet+=velocity*dt;camera.updateMovementLean(feet,dt);}
}
}
void runCameraLeanTests() {
    auto& config=GameConfig::getInstance();const auto saved=config.camera;config.camera=CameraConfig{};
    Camera projectionCamera;
    const auto oldProjection=projectionCamera.getProjectionMatrix(1.6f,60);
    const auto forestProjection=projectionCamera.getProjectionMatrix(1.6f,60,600);
    auto depth=[](const glm::mat4& projection,float distance){auto clip=projection*glm::vec4(0,0,-distance,1);return clip.z/clip.w;};
    require(depth(oldProjection,140)>1 && depth(forestProjection,140)<1,"140m spire distance is inside forest frustum only");
    require(depth(forestProjection,599)<1 && depth(forestProjection,601)>1,"map far clipping uses authored600m limit");
    require(projectionCamera.getProjectionMatrix(1.6f,60)==oldProjection,"map projection override cannot leak into next legacy render");
    require(projectionCamera.getProjectionMatrix(1.6f,60,-1)==oldProjection,"invalid caller override safely falls back");
    Camera camera({2,3,4});glm::vec3 feet(0);camera.resetMovementLean(feet);
    const auto front=camera.getFront(),right=camera.getRight(),up=camera.getUp();
    advance(camera,feet,-right*5.f,120,1.f/120);
    requireNear(camera.getMovementRollDegrees(),-3,.002,"left strafe gives left camera head tilt");
    const auto inverse=glm::inverse(camera.getViewMatrix());
    require(glm::dot(glm::vec3(inverse[1]),right)<0,"rendered head/up tilts left, not merely named roll sign");
    require(camera.getFront()==front&&camera.getRight()==right&&camera.getUp()==up,"roll never modifies logical aim/movement/audio basis");
    auto unproject=glm::inverse(camera.getProjectionMatrix(1.6f)*camera.getViewMatrix())*glm::vec4(0,0,-1,1);
    auto ray=glm::normalize(glm::vec3(unproject)/unproject.w-camera.getPosition());
    require(glm::length(ray-front)<.00001f,"center crosshair ray agrees with unrolled firing direction");
    const float frozen=camera.getMovementRollDegrees();camera.updateMovementLean(feet+glm::vec3(1),0);
    require(camera.getMovementRollDegrees()==frozen,"zero simulation time freezes spring and sample");
    camera.syncMovementLeanPosition(feet);float rebound=0;
    for(int i=0;i<120;++i){camera.updateMovementLean(feet,1.f/120);rebound=std::max(rebound,camera.getMovementRollDegrees());}
    require(rebound>.15f&&rebound<.6f,"stop gives bounded small opposite rebound");
    require(std::abs(camera.getMovementRollDegrees())<.002f,"stopped camera settles");
    advance(camera,feet,right*5.f,120,1.f/120);
    requireNear(camera.getMovementRollDegrees(),3,.002,"right strafe symmetric");
    camera.resetMovementLean(feet);advance(camera,feet,front*5.f+glm::vec3(0,3,0),120,1.f/120);
    require(std::abs(camera.getMovementRollDegrees())<.0001f,"forward motion and vertical motion do not roll");
    camera.setDirection({1,0,0});camera.resetMovementLean(feet);advance(camera,feet,camera.getRight()*5.f,120,1.f/120);
    requireNear(camera.getMovementRollDegrees(),3,.002,"lateral motion is camera relative after yaw change");
    camera.resetOrientation();require(camera.getMovementRollDegrees()==0,"restart orientation clears lean");
    camera.resetMovementLean(feet);advance(camera,feet,right*5.f,60,1.f/60);
    feet+=glm::vec3(20,0,0);camera.updateMovementLean(feet,1.f/60);
    require(camera.getMovementRollDegrees()==0,"teleport resets roll and sample");
    camera.updateMovementLean(feet,1.f/60);require(camera.getMovementRollDegrees()==0,"teleport does not leak into next sample");
    advance(camera,feet,right*5.f,60,1.f/60);camera.updateMovementLean(feet,1.f);
    require(camera.getMovementRollDegrees()==0,"suspended clock does not produce roll impulse");
    advance(camera,feet,right*5.f,60,1.f/60);config.camera.leanMaxDegrees=0;camera.updateMovementLean(feet,1.f/60);
    require(camera.getMovementRollDegrees()==0,"zero amplitude disables effect immediately");config.camera=CameraConfig{};
    float baseline=0;
    for(int rate:{30,60,120,240}){Camera c;glm::vec3 p(0);c.resetMovementLean(p);advance(c,p,c.getRight()*5.f,rate/2,1.f/rate);advance(c,p,glm::vec3(0),rate/5,1.f/rate);if(rate==30)baseline=c.getMovementRollDegrees();else requireNear(c.getMovementRollDegrees(),baseline,.0002,"spring response is timestep consistent");}
    camera.resetMovementLean(feet);advance(camera,feet,right*50.f,240,1.f/240);
    require(std::abs(camera.getMovementRollDegrees())<4.5f,"high speed remains bounded");
    const auto path=(std::filesystem::temp_directory_path()/"silic2-camera-lean-config.json").string();
    config.camera.leanMaxDegrees=2.2f;config.camera.leanReferenceSpeed=7;config.camera.leanFrequency=16;config.camera.leanDamping=.65f;
    require(config.saveToFile(path),"lean config saves");config.camera=CameraConfig{};require(config.loadFromFile(path),"lean config reloads");
    requireNear(config.camera.leanMaxDegrees,2.2,.0001,"amplitude roundtrip");requireNear(config.camera.leanReferenceSpeed,7,.0001,"reference speed roundtrip");requireNear(config.camera.leanFrequency,16,.0001,"spring frequency roundtrip");requireNear(config.camera.leanDamping,.65,.0001,"damping roundtrip");
    std::filesystem::remove(path);config.camera=saved;
    std::cout<<"Camera lean passed: direction/head basis, center aim, braking, pause, reset/teleport, 30-240Hz and configuration\n";
}
