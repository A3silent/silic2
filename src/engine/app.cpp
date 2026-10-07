#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "engine/app.h"
#include "engine/camera.h"
#include "engine/map.h"
#include "engine/map_renderer.h"
#include "engine/pixel_renderer.h"
#include "engine/shader.h"
#include "engine/texture.h"
#include "engine/game_config.h"
#include "engine/collision.h"
#include "player/player.h"
#include "player/weapon.h"
#include "effects/groundparticle.h"
#include "enemy/enemy_manager.h"
#include "enemy/enemy_projectile_system.h"
#include "game/run_controller.h"
#include "game/encounter_director.h"
#include "hud/hud_renderer.h"
#include "hud/hud_view_model.h"
#include "audio/audio_system.h"
#include "character/character_presentation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#ifdef __APPLE__
#include <mach/mach.h>
#endif

namespace silic2 {
namespace {
uint64_t residentBytes() {
#ifdef __APPLE__
    mach_task_basic_info_data_t info{}; mach_msg_type_number_t count=MACH_TASK_BASIC_INFO_COUNT;
    if(task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count)==KERN_SUCCESS) return info.resident_size;
#endif
    return 0;
}
AABB brushBounds(const Brush& b) {
    glm::vec3 low=b.vertices.front(),high=low;
    for(const auto& v:b.vertices){low=glm::min(low,v);high=glm::max(high,v);} return {low,high};
}
}
App::App(uint64_t seed):App([&]{AppOptions o;o.seed=seed;return o;}()){}
App::App(const AppOptions& config):options(config) {
    auto& gc=GameConfig::getInstance(); gc.window.width=options.width;gc.window.height=options.height;gc.render.enableVSync=options.vsync;
    try {
        if(!initWindow()) throw std::runtime_error("GLFW initialization failed");
        if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) throw std::runtime_error("GLAD initialization failed");
        std::cout<<"OpenGL "<<glGetString(GL_VERSION)<<'\n';
        glEnable(GL_DEPTH_TEST);
        std::filesystem::create_directories(options.outputDirectory);
        runLog.open(options.outputDirectory+"/runs.jsonl",std::ios::app);
        if(!runLog) throw std::runtime_error("Cannot open run log");
        if(!options.acceptance.empty()) {
            acceptanceLog.open(options.outputDirectory+"/"+options.acceptance+".jsonl");
            if(!acceptanceLog) throw std::runtime_error("Cannot open acceptance log");
            std::cout<<"ACCEPTANCE synthetic input: "<<options.acceptance<<" (not a human playtest)\n";
        }
        runController=std::make_unique<RunController>(options.seed);
        currentMap=std::make_unique<Map>(); mapRenderer=std::make_unique<MapRenderer>();
        pixelRenderer=std::make_unique<PixelRenderer>();
        if(!pixelRenderer->init(320,200)) throw std::runtime_error("Pixel framebuffer initialization failed");
        camera=std::make_unique<Camera>(); player=std::make_unique<Player>();
        weapon=std::make_unique<Weapon>();weapon->init();
        groundParticles=createEnhancedGroundParticleSystem(2000,GroundParticleSystem::GParticleMode::FIRE);
        enemyManager=std::make_unique<EnemyManager>();enemyManager->init();
        hostileProjectiles=std::make_unique<EnemyProjectileSystem>();
        director=std::make_unique<EncounterDirector>(ArenaDefinition{});
        hudRenderer=std::make_unique<HudRenderer>();hudRenderer->init();
        characters=std::make_unique<CharacterPresentation>();
        if(!characters->init())throw std::runtime_error("Character rendering initialization failed");
        audio=std::make_unique<AudioSystem>();audio->initialize();
        projectileShader=std::make_unique<Shader>("res/shaders/enemy.vert","res/shaders/enemy.frag");
        // A bright octahedron gives hostile projectiles a readable .24-unit diameter.
        const glm::vec3 p[6]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
        const int faces[24]={2,4,0,2,1,4,2,5,1,2,0,5,3,0,4,3,4,1,3,1,5,3,5,0};
        std::vector<float> verts;for(int i:faces){for(int a=0;a<3;++a)verts.push_back(p[i][a]);for(int a=0;a<3;++a)verts.push_back(p[i][a]);}
        glGenVertexArrays(1,&projectileVAO);glGenBuffers(1,&projectileVBO);glBindVertexArray(projectileVAO);glBindBuffer(GL_ARRAY_BUFFER,projectileVBO);
        glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(float),verts.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),nullptr);glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));glEnableVertexAttribArray(1);glBindVertexArray(0);
        glfwSetWindowUserPointer(window,this);glfwSetFramebufferSizeCallback(window,framebufferSizeCallback);
        glfwSetCursorPosCallback(window,mouseCallback);glfwSetScrollCallback(window,scrollCallback);glfwSetWindowFocusCallback(window,focusCallback);
        setState(GameState::READY);
        int fw,fh;glfwGetFramebufferSize(window,&fw,&fh);
        if(acceptanceLog) acceptanceLog<<"{\"event\":\"configuration\",\"logical_width\":"<<options.width<<",\"logical_height\":"<<options.height<<",\"framebuffer_width\":"<<fw<<",\"framebuffer_height\":"<<fh<<",\"internal_width\":320,\"internal_height\":200,\"vsync\":"<<(options.vsync?"true":"false")<<",\"audio_available\":"<<(audio->available()?"true":"false")<<",\"synthetic_input\":true}\n";
    }catch(...){cleanup();throw;}
}
App::~App(){cleanup();}
bool App::initWindow(){
    if(!glfwInit())return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
#endif
    window=glfwCreateWindow(options.width,options.height,"SILIC2 | STARRY CRYSTAL",nullptr,nullptr);
    if(!window)return false;
    glfwMakeContextCurrent(window);glfwSwapInterval(options.vsync?1:0);return true;
}
void App::cleanup(){
    // Destroy all GL owners with their context current, and voices before backend.
    if(window)glfwMakeContextCurrent(window);
    if(audio)audio->shutdown();audio.reset();
    characters.reset();hudRenderer.reset();weapon.reset();enemyManager.reset();groundParticles.reset();
    const bool hadRenderer=static_cast<bool>(mapRenderer);
    mapRenderer.reset();
    if(hadRenderer)TextureManager::getInstance().clear();
    pixelRenderer.reset();projectileShader.reset();
    if(projectileVBO)glDeleteBuffers(1,&projectileVBO);if(projectileVAO)glDeleteVertexArrays(1,&projectileVAO);
    projectileVAO=projectileVBO=0;
    if(window){glfwDestroyWindow(window);window=nullptr;}glfwTerminate();
}
bool App::loadMap(const std::string& path){
    auto candidate=std::make_unique<Map>();
    if(!candidate->loadFromFile(path)) {
        std::cerr << "Map load failed: " << candidate->getLastError() << '\n';
        return false;
    }
    const bool candidateArena=candidate->getGameMode()==GameMode::ArenaSurvival;
    std::shared_ptr<const ArenaDefinition> definition;
    if(candidateArena) {
        definition=std::make_shared<ArenaDefinition>(ArenaDefinition::fromMap(*candidate));
        if(!definition->hasConnectedWaypoints(.34f)) {
            std::cerr << "Map load failed: arena navigation is disconnected\n";
            return false;
        }
    }
    if(!options.acceptance.empty()&&!candidateArena&&options.acceptance!="screenshots")
        throw std::runtime_error("This acceptance mode requires an arena_survival map");
    // Build a complete candidate renderer while the previous scene stays alive.
    auto candidateRenderer=std::make_unique<MapRenderer>();
    if(!candidateRenderer->loadMap(*candidate))return false;
    currentMap=std::move(candidate);
    mapRenderer=std::move(candidateRenderer);
    currentMapFile=path;arena=candidateArena;
    director=std::make_unique<EncounterDirector>(definition?*definition:ArenaDefinition{});
    enemyManager->setArenaDefinition(definition);
    groundParticles->initialize(*currentMap);groundParticles->setEnabled(!arena&&GameConfig::getInstance().effects.enableGroundParticles);
    runController->restartRun(path);resetRuntimeState();setState(GameState::READY);return true;
}

void App::setState(GameState next){
    gameState=next;stateTimer=0;camera->setFirstMouse(true);
    glfwSetInputMode(window,GLFW_CURSOR,next==GameState::PLAYING?GLFW_CURSOR_DISABLED:GLFW_CURSOR_NORMAL);
    audio->setPaused(next==GameState::PAUSED||next==GameState::READY);
    if(next==GameState::PLAYING) refocused=true;
}
void App::run(){
    double previous=glfwGetTime();modeStarted=previous;flowPlaybackSeconds=0;
    if(!options.acceptance.empty()&&options.acceptance!="screenshots"&&options.acceptance!="flow")setState(GameState::PLAYING);
    while(!glfwWindowShouldClose(window)){
        frameStarted=glfwGetTime();const double raw=frameStarted-previous;previous=frameStarted;
        // Simulation stability clamp is independent of the uncapped performance record.
        deltaTime=static_cast<float>(std::min(raw,0.05));
        const bool measuring=gameState==GameState::PLAYING&&!refocused;
        // Synthetic flow inputs and assertions share a bounded frame clock.
        // GPU readback / screenshot I/O must not skip an input's press window.
        if(options.acceptance=="flow")flowPlaybackSeconds+=deltaTime;
        refocused=false;processInput();
        if(!options.acceptance.empty())updateAcceptance(deltaTime);
        update(deltaTime);render();
        glfwSwapBuffers(window);glfwPollEvents();
        if(measuring&&gameState==GameState::PLAYING&&(options.acceptance=="performance"||options.acceptance=="stress"))
            frameTimes.push_back((glfwGetTime()-frameStarted)*1000.0);
    }
    if(runController->state().active){runController->endRoom(RoomResult::Abandoned);runController->endRun(RunEndReason::Quit);runLog<<runController->stats().formatRunResult()<<'\n';}
    if(acceptanceFailed)throw std::runtime_error("Acceptance checks failed; inspect JSONL");
}
void App::processInput(){
    bool esc=glfwGetKey(window,GLFW_KEY_ESCAPE)==GLFW_PRESS;
    bool enter=glfwGetKey(window,GLFW_KEY_ENTER)==GLFW_PRESS;
    const bool r=glfwGetKey(window,GLFW_KEY_R)==GLFW_PRESS;
    bool left=glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS;
    if(options.acceptance=="flow") {
        const double t=flowPlaybackSeconds;
        enter=t>=.3&&t<.45;
        esc=(t>=.8&&t<.95)||(t>=1.3&&t<1.45)||(t>=2.2&&t<2.35);
        left=(t>=2.4&&t<4.2)||(t>=4.4&&t<4.55);
    }
    if(esc&&!escWasPressed){if(gameState==GameState::PLAYING)setState(GameState::PAUSED);else if(gameState==GameState::PAUSED)setState(GameState::PLAYING);}
    if(enter&&!enterWasPressed&&gameState==GameState::READY){setState(GameState::PLAYING);audio->play(AudioCue::UiConfirm,{},0,false);}
    if((gameState==GameState::ROOM_CLEARED||gameState==GameState::PLAYER_DEAD)&&stateTimer>=1&&((r&&!rWasPressed)||(left&&!leftMouseWasPressed)))restartCurrentRun();
    escWasPressed=esc;enterWasPressed=enter;rWasPressed=r;leftMouseWasPressed=left;
    if(gameState==GameState::PLAYING&&(options.acceptance.empty()||options.acceptance=="flow")){
        player->processInput(window,camera.get(),deltaTime);if(left)firePrimary();
    }
}
void App::firePrimary(){if(weapon->fire(*camera)){runController->stats().recordShot();characters->onPrimaryShot();audio->play(AudioCue::PlayerShot,{},0,false);}}
void App::update(float dt){
    if(gameState!=GameState::PAUSED)stateTimer+=dt;
    if(gameState==GameState::PLAYING){runController->tick(dt,true);updatePlaying(dt);}
    audio->setListener(camera->getPosition(),camera->getFront(),camera->getUp());
}
bool App::visible(const glm::vec3& from,const glm::vec3& to)const{
    const float distance=glm::length(to-from);if(distance<.001f)return true;
    for(const auto& b:currentMap->getBrushes())if(!b.vertices.empty()){
        auto hit=CollisionSystem::raycastAABB(Ray(from,to-from),brushBounds(b),distance);
        if(hit.collided&&hit.distance<distance-.02f)return false;
    }return true;
}
void App::applyDamage(float amount,DamageSource source,const glm::vec3& origin,bool newEvent){
    // Invulnerability is confined to explicitly labelled synthetic acceptance input.
    const bool syntheticInvulnerable=options.acceptance=="performance"||options.acceptance=="stress"||options.acceptance=="restarts";
    const float actual=syntheticInvulnerable?0:player->takeDamage(amount);
    runController->stats().recordDamage(actual,source,newEvent);
    if(actual>0){damageAge=0;auto dir=origin-camera->getPosition();damageDirection=std::atan2(glm::dot(dir,camera->getRight()),glm::dot(dir,camera->getFront()));if(newEvent)audio->play(AudioCue::PlayerHurt,{},0,false);}
    if(player->isDead())runController->stats().recordFatalSource(source);
}
void App::updatePlaying(float dt){
    hitAge+=dt;killAge+=dt;damageAge+=dt;
    player->update(dt,currentMap.get());
    auto pos=player->getPosition();const auto movement=pos-previousPlayerPosition;
    runController->stats().recordMovement(glm::length(glm::vec2(movement.x,movement.z)),player->getState()==PlayerState::SLIDING?MovementState::Sliding:MovementState::Other,dt);
    previousPlayerPosition=pos;camera->setPosition(player->getEyePosition()+player->getCameraOffset());camera->update();
    const auto result=weapon->update(dt,currentMap.get(),enemyManager.get());
    for(int i=0;i<result.hits;++i){runController->stats().recordHit();hitAge=0;audio->play(AudioCue::Hit,{},0,false);}
    for(auto type:result.killedEnemyTypes){runController->stats().recordKill(type);killAge=0;audio->play(AudioCue::Kill,{},0,false);}
    for(const auto& enemy:enemyManager->getEnemies()){
        if(enemy.isDead()){audio->stopSource(10000+enemy.getId());characters->emit(CharacterEffect::Death,enemy.getAABB().getCenter());}
        else if(enemy.getSecondsSinceDamage()==0)characters->emit(CharacterEffect::Impact,enemy.getAABB().getCenter());
    }
    if(arena){
        DirectorInput input;input.playerPosition=pos;input.liveChasers=enemyManager->count(EnemyType::Chaser);input.liveSentries=enemyManager->count(EnemyType::Sentry);
        for(const auto& enemy:enemyManager->getEnemies())if(!enemy.isDead()&&enemy.getType()==EnemyType::Sentry)
            for(size_t i=0;i<3;++i)if(glm::length(enemy.getPosition()-director->arenaDefinition().sentryPositions[i])<1)input.occupiedSentrySlots[i]=true;
        for(const auto& event:director->update(dt,input)){
            switch(event.kind){
                case DirectorEvent::Kind::WarningStarted:audio->play(AudioCue::RiftWarning,event.position,event.requestId);break;
                case DirectorEvent::Kind::WarningCancelled:audio->stopSource(event.requestId);break;
                case DirectorEvent::Kind::Spawn:enemyManager->spawn(event.type,event.position);audio->stopSource(event.requestId);break;
                case DirectorEvent::Kind::Deferred:runController->stats().recordSpawnDelay(toString(event.reason));break;
                case DirectorEvent::Kind::Cancelled:runController->stats().recordSpawnCancellation(toString(event.reason));audio->stopSource(event.requestId);break;
            }
        }
    }
    enemyManager->update(dt,pos,currentMap.get());
    for(const auto& action:enemyManager->actions()){
        if(action.kind==EnemyAction::Kind::Charge)audio->play(AudioCue::SentryCharge,action.position,10000+action.sourceId);
        else if(action.kind==EnemyAction::Kind::CancelCharge)audio->stopSource(10000+action.sourceId);
        else if(action.kind==EnemyAction::Kind::Fire){
            audio->stopSource(10000+action.sourceId);
            if(hostileProjectiles->spawn(action.position,action.direction,action.sourceId)){
                runController->stats().recordHostileShot();audio->play(AudioCue::SentryFire,action.position);characters->emit(CharacterEffect::SentryFire,action.position,action.direction);
            }
        }
    }
    const float dps=enemyManager->getContactDps(pos);
    if(dps>0){glm::vec3 origin=pos;for(const auto& e:enemyManager->getEnemies())if(e.isTouchingPlayer(pos)){origin=e.getPosition();break;}applyDamage(dps*dt,DamageSource::EnemyContact,origin,!contactDamageActive);}contactDamageActive=dps>0;
    if(!player->isDead())for(const auto& hit:hostileProjectiles->update(dt,currentMap.get(),player->getAABB())){runController->stats().recordHostileHit();applyDamage(hit.damage,DamageSource::EnemyProjectile,hit.origin);if(player->isDead())break;}
    const float time=runController->stats().snapshot().runTime;
    if(currentMap->getEnvironmentPreset()==EnvironmentPreset::StarryCrystal&&!transformationPlayed&&time>=currentMap->getEnvironment().transformationStart){audio->play(AudioCue::Transform,{},0,false);transformationPlayed=true;}
    FirstPersonView fp;fp.speed=glm::length(glm::vec2(player->getVelocity().x,player->getVelocity().z));fp.feet=pos;fp.forward=camera->getFront();fp.grounded=player->isOnGround();fp.sliding=player->getState()==PlayerState::SLIDING;
    if(fp.sliding&&!wasSliding)audio->play(AudioCue::Slide,{},0,false);wasSliding=fp.sliding;characters->update(dt,fp);
    if(!arena)groundParticles->update(dt);
    if(player->isDead())finishCurrentRun(RoomResult::PlayerDied,RunEndReason::PlayerDied);
    else if((arena&&director->complete(enemyManager->getLiveCount()))||(!arena&&enemyManager->hasSpawnedEnemies()&&enemyManager->allEnemiesDead()))finishCurrentRun(RoomResult::Cleared,RunEndReason::Completed);
}
void App::finishCurrentRun(RoomResult room,RunEndReason reason){
    if(!runController->state().active)return;
    runController->endRoom(room);runController->endRun(reason);
    const auto line=runController->stats().formatRunResult();runLog<<line<<'\n';runLog.flush();std::cout<<line<<'\n';
    audio->stopAll();hostileProjectiles->clear();
    setState(reason==RunEndReason::Completed?GameState::ROOM_CLEARED:GameState::PLAYER_DEAD);
    audio->play(reason==RunEndReason::Completed?AudioCue::Victory:AudioCue::PlayerDeath,{},0,false);
}
glm::vec3 App::getPlayerStartPosition()const{auto* start=currentMap->getPlayerStart();return start?start->position:glm::vec3(0,2,0);}
void App::resetRuntimeState(){
    runController->stats().setExecutionContext(!options.acceptance.empty(),options.acceptance);
    runController->stats().setContentVersion(currentMap->getContentVersion());
    audio->reset();hostileProjectiles->clear();enemyManager->clear();weapon->reset();characters->reset();groundParticles->reset();mapRenderer->resetEnvironment();
    director->reset(runController->state().seed);player->respawn(getPlayerStartPosition());previousPlayerPosition=player->getPosition();
    camera->resetOrientation();
    if(const auto* start=currentMap->getPlayerStart()) {
        const float yaw=glm::radians(start->rotation.y),pitch=glm::radians(start->rotation.x);
        camera->setDirection({std::cos(yaw)*std::cos(pitch),std::sin(pitch),std::sin(yaw)*std::cos(pitch)});
    }
    camera->setPosition(player->getEyePosition());
    if(!arena)enemyManager->spawnFromMap(*currentMap);
    contactDamageActive=false;transformationPlayed=false;wasSliding=false;hitAge=killAge=damageAge=1000;damageDirection=0;
    if(options.acceptance=="restarts")logReset();
    audio->play(AudioCue::Ambience,{},0,false,.5f);
}
void App::restartCurrentRun(){
    // Stop sound/runtime first, then seed/stats, then entities/presentation/input.
    audio->stopAll();hostileProjectiles->clear();
    if(runController->state().active){runController->endRoom(RoomResult::Abandoned);runController->endRun(RunEndReason::Restarted);runLog<<runController->stats().formatRunResult()<<'\n';}
    runController->restartRun(currentMapFile);resetRuntimeState();setState(GameState::PLAYING);
}
HudViewModel App::makeHud(const glm::mat4& view,const glm::mat4& projection)const{
    HudViewModel vm;const auto& s=runController->stats().snapshot();
    vm.arenaSurvival=arena;
    vm.hasTransformation=currentMap->getEnvironmentPreset()==EnvironmentPreset::StarryCrystal;
    vm.transformationStart=currentMap->getEnvironment().transformationStart;
    vm.transformationDuration=currentMap->getEnvironment().transformationDuration;
    switch(gameState){case GameState::READY:vm.phase=HudPhase::Ready;break;case GameState::PLAYING:vm.phase=HudPhase::Playing;break;case GameState::PAUSED:vm.phase=HudPhase::Paused;break;case GameState::ROOM_CLEARED:vm.phase=HudPhase::Won;break;case GameState::PLAYER_DEAD:vm.phase=HudPhase::Dead;break;}
    vm.hp=player->getHp();vm.maxHp=player->getMaxHp();vm.combatSeconds=s.runTime;vm.cleanup=arena&&director->generationFinished();vm.livingEnemies=static_cast<int>(enemyManager->getLiveCount());
    vm.resultLockRemaining=std::max(0.f,1-stateTimer);vm.kills=s.kills;vm.shotsFired=s.shotsFired;vm.shotsHit=s.shotsHit;vm.actualHpLoss=s.damageTaken;vm.fatalSource=toString(s.fatalSource);vm.seed=s.seed;vm.audioStatus=!audio->available()?HudAudioStatus::Unavailable:(audio->muted()?HudAudioStatus::Muted:HudAudioStatus::Available);
    vm.hitAge=hitAge;vm.killAge=killAge;vm.damageAge=damageAge;vm.damageDirectionRadians=damageDirection;
    auto p=player->getPosition(),f=camera->getFront();vm.playerXZ={p.x,p.z};vm.playerForwardXZ={f.x,f.z};
    bool bounds=false;
    for(const auto& b:currentMap->getBrushes())if(!b.vertices.empty()){
        auto box=brushBounds(b);if(!bounds){vm.mapMinXZ={box.min.x,box.min.z};vm.mapMaxXZ={box.max.x,box.max.z};bounds=true;}
        else {vm.mapMinXZ=glm::min(vm.mapMinXZ,glm::vec2(box.min.x,box.min.z));vm.mapMaxXZ=glm::max(vm.mapMaxXZ,glm::vec2(box.max.x,box.max.z));}
        if(box.max.y-box.min.y>.2f)vm.terrain.push_back({{box.min.x,box.min.z},{box.max.x,box.max.z}});
    }
    const auto& layout=currentMap->getArenaLayout();
    if(layout.hasFloor){vm.mapMinXZ=layout.minXZ;vm.mapMaxXZ=layout.maxXZ;}
    if(arena){auto warnings=director->warningProgress();for(size_t i=0;i<4;++i){auto r=layout.rifts[i];vm.rifts.push_back({{r.x,r.z},warnings[i]>=0});}}
    for(const auto& enemy:enemyManager->getEnemies()){
        HudEnemy e;auto pos=enemy.getPosition(),center=enemy.getAABB().getCenter();e.worldXZ={pos.x,pos.z};e.hp=enemy.getHp();e.maxHp=enemy.getMaxHp();e.alive=!enemy.isDead();e.sentry=enemy.getType()==EnemyType::Sentry;e.secondsSinceDamage=enemy.getSecondsSinceDamage();
        auto clip=projection*view*glm::vec4(center,1);e.inView=clip.w>0&&std::abs(clip.x)<=clip.w&&std::abs(clip.y)<=clip.w&&clip.z>=-clip.w&&clip.z<=clip.w;
        e.unobstructed=e.inView&&visible(camera->getPosition(),center);
        auto anchor=projection*view*glm::vec4(pos+glm::vec3(0,enemy.getAABB().getSize().y+.2f,0),1);
        if(anchor.w>0)e.healthAnchor={anchor.x/anchor.w*.5f+.5f,.5f-anchor.y/anchor.w*.5f};vm.enemies.push_back(e);
    }return vm;
}
void App::renderProjectiles(const glm::mat4& view,const glm::mat4& projection){
    if(hostileProjectiles->size()==0)return;
    projectileShader->use();projectileShader->setMat4("view",view);projectileShader->setMat4("projection",projection);projectileShader->setInt("numLights",0);projectileShader->setVec3("ambientLight",glm::vec3(1));projectileShader->setVec3("enemyColor",{1,.3f,.08f});
    glBindVertexArray(projectileVAO);for(const auto& p:hostileProjectiles->projectiles()){
        auto model=glm::scale(glm::translate(glm::mat4(1),p.position),glm::vec3(EnemyProjectileSystem::RADIUS));projectileShader->setMat4("model",model);glDrawArrays(GL_TRIANGLES,0,24);
    }glBindVertexArray(0);
}
void App::render(){
    int fw,fh,lw,lh;glfwGetFramebufferSize(window,&fw,&fh);glfwGetWindowSize(window,&lw,&lh);if(fw<=0||fh<=0)return;
    if(options.acceptance=="stress") {
        while(hostileProjectiles->size()<32) hostileProjectiles->spawn({float(hostileProjectiles->size()%8)-4,1,-8},{0,0,1});
    }
    pixelRenderer->beginPixelRender();glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);
    const auto view=camera->getViewMatrix(),projection=camera->getProjectionMatrix(320.f/200.f,player->getCurrentFov());
    const auto& ws=currentMap->getWorldSettings();glClearColor(ws.backgroundColor.r,ws.backgroundColor.g,ws.backgroundColor.b,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    mapRenderer->clearDynamicLights();for(const auto& light:weapon->getActiveLights())mapRenderer->addDynamicLight(light.first,light.second,1,3);
    mapRenderer->setEnvironmentTime(runController->stats().snapshot().runTime);mapRenderer->setRiftWarnings(arena?director->warningProgress():std::array<float,4>{-1,-1,-1,-1});
    mapRenderer->render(view,projection);weapon->render(view,projection);
    std::vector<CharacterView> cv;for(const auto& e:enemyManager->getEnemies())if(!e.isDead()){
        CharacterView c;c.type=e.getType();c.position=e.getPosition();c.heading=e.getHeading();c.dimensions=e.getAABB().getSize();c.charging=e.isCharging();c.chargeProgress=e.getChargeProgress();c.aimLocked=e.isAimLocked();c.hitFlash=e.getHitFlash();c.phase=e.getId()*.731f;c.movement=glm::clamp(e.getMovementSpeed()/3.f,0.f,1.f);cv.push_back(c);
    }
    characters->renderWorld(view,projection,cv);renderProjectiles(view,projection);
    if(!arena&&GameConfig::getInstance().effects.enableGroundParticles)groundParticles->render(view,projection);
    characters->renderFirstPerson(projection);pixelRenderer->endPixelRender(fw,fh);
    hudRenderer->render(makeHud(view,projection),lw,lh,fw,fh);
    if(options.acceptance=="flow") {
        if(screenshotStage==0&&gameState==GameState::READY){captureScreenshot("flow-ready");++screenshotStage;}
        else if(screenshotStage==1&&gameState==GameState::PAUSED){captureScreenshot("flow-paused");++screenshotStage;}
        else if(screenshotStage==2&&gameState==GameState::PLAYER_DEAD){captureScreenshot("flow-dead");++screenshotStage;}
        else if(screenshotStage==3&&runController->state().runIndex==2){captureScreenshot("flow-restart");++screenshotStage;}
    }
    if(options.acceptance=="performance"&&gameState==GameState::ROOM_CLEARED) captureScreenshot("won-run-"+std::to_string(acceptanceRuns+1));
    if(options.acceptance=="screenshots"){
        const double elapsed=glfwGetTime()-modeStarted;
        if(screenshotStage==0){captureScreenshot("ready");++screenshotStage;}
        else if(screenshotStage==1&&elapsed>.8){captureScreenshot("playing");++screenshotStage;}
        else if(screenshotStage==2&&gameState==GameState::PAUSED){captureScreenshot("paused");++screenshotStage;}
        else if(screenshotStage==3&&elapsed>4){captureScreenshot("combat");++screenshotStage;}
    }
}
void App::captureScreenshot(const std::string& name){
    int w,h;glfwGetFramebufferSize(window,&w,&h);if(w<=0||h<=0)return;
    std::vector<unsigned char> pixels(static_cast<size_t>(w)*h*3);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    const std::string path=options.outputDirectory+"/"+name+".ppm";std::ofstream out(path,std::ios::binary);out<<"P6\n"<<w<<' '<<h<<"\n255\n";
    for(int y=h-1;y>=0;--y)out.write(reinterpret_cast<char*>(pixels.data()+static_cast<size_t>(y)*w*3),w*3);
    std::cout<<"Screenshot: "<<path<<'\n';
}
void App::logReset(){
    glFinish(); // Complete queued rendering before comparing resident memory.
    const auto& s=runController->stats().snapshot();const auto ad=audio->diagnostics();auto warnings=director->warningProgress();
    const size_t warningCount=std::count_if(warnings.begin(),warnings.end(),[](float p){return p>=0;});
    const bool clean=enemyManager->getLiveCount()==0&&hostileProjectiles->size()==0&&weapon->getActiveBulletCount()==0&&weapon->getActiveImpactLightCount()==0&&characters->activeEffectCount()==0&&ad.activeVoices==0&&warningCount==0&&s.kills==0&&s.shotsFired==0&&s.runTime==0&&s.damageTaken==0&&s.hostileHits==0&&s.hostileShots==0&&s.fatalSource==DamageSource::Unknown&&director->combatSeconds()==0&&mapRenderer->getEnvironmentTime()==0&&player->getHp()==100;
    acceptanceFailed|=!clean;
    acceptanceLog<<"{\"event\":\"reset\",\"run_index\":"<<s.runIndex<<",\"clean\":"<<(clean?"true":"false")<<",\"entities\":"<<enemyManager->getLiveCount()<<",\"hostile_projectiles\":"<<hostileProjectiles->size()<<",\"player_projectiles\":"<<weapon->getActiveBulletCount()<<",\"impact_lights\":"<<weapon->getActiveImpactLightCount()<<",\"character_effects\":"<<characters->activeEffectCount()<<",\"warnings\":"<<warningCount<<",\"voices\":"<<ad.activeVoices<<",\"stats_time\":"<<s.runTime<<",\"stats_kills\":"<<s.kills<<",\"stats_shots\":"<<s.shotsFired<<",\"stats_damage\":"<<s.damageTaken<<",\"stats_hostile_shots\":"<<s.hostileShots<<",\"stats_hostile_hits\":"<<s.hostileHits<<",\"fatal_source\":\""<<toString(s.fatalSource)<<"\",\"director_time\":"<<director->combatSeconds()<<",\"environment_time\":"<<mapRenderer->getEnvironmentTime()<<",\"hp\":"<<player->getHp()<<",\"rss_bytes\":"<<residentBytes()<<"}\n";acceptanceLog.flush();
}
void App::recordPerformance(){
    if(frameTimes.empty())return;
    auto sorted=frameTimes;std::sort(sorted.begin(),sorted.end());auto quantile=[&](double p){return sorted[static_cast<size_t>((sorted.size()-1)*p)];};
    size_t streak=0,longest=0;for(double ms:frameTimes){streak=ms>50?streak+1:0;longest=std::max(longest,streak);}
    if(options.acceptance=="stress") acceptanceLog<<"{\"event\":\"stress_load\",\"chasers\":"<<enemyManager->count(EnemyType::Chaser)<<",\"sentries\":"<<enemyManager->count(EnemyType::Sentry)<<",\"hostile_projectiles\":"<<hostileProjectiles->size()<<",\"environment_particles\":"<<mapRenderer->getEnvironmentParticleCount()<<"}\n";
    const bool pass=quantile(.95)<=20&&quantile(.99)<=33.3&&longest<3;
    acceptanceFailed|=!pass;
    acceptanceLog<<std::setprecision(8)<<"{\"event\":\"performance\",\"run\":"<<acceptanceRuns+1<<",\"combat_seconds\":"<<runController->stats().snapshot().runTime<<",\"frames\":"<<frameTimes.size()<<",\"p95_ms\":"<<quantile(.95)<<",\"p99_ms\":"<<quantile(.99)<<",\"max_ms\":"<<sorted.back()<<",\"longest_over_50ms\":"<<longest<<",\"passed\":"<<(pass?"true":"false")<<",\"rss_bytes\":"<<residentBytes()<<",\"audio_frames\":"<<audio->diagnostics().renderedFrames<<"}\n";
    std::ofstream raw(options.outputDirectory+"/frames-"+options.acceptance+"-"+std::to_string(acceptanceRuns+1)+".csv");raw<<"frame,wall_clock_ms\n";for(size_t i=0;i<frameTimes.size();++i)raw<<i<<','<<frameTimes[i]<<'\n';
    acceptanceLog.flush();frameTimes.clear();
}
void App::updateAcceptance(float dt){
    const double elapsed=options.acceptance=="flow"?flowPlaybackSeconds:glfwGetTime()-modeStarted;
    if(options.acceptance=="flow") {
        auto check=[&](const char* name,bool passed){acceptanceFailed|=!passed;acceptanceLog<<"{\"event\":\"flow_check\",\"check\":\""<<name<<"\",\"passed\":"<<(passed?"true":"false")<<"}\n";};
        const auto& stats=runController->stats().snapshot();
        if(flowStep==0&&elapsed>.15){check("ready_excludes_time",gameState==GameState::READY&&stats.runTime==0);++flowStep;}
        else if(flowStep==1&&elapsed>1.0){check("escape_pauses",gameState==GameState::PAUSED);flowFrozenTime=stats.runTime;flowFrozenPosition=player->getPosition();flowAudioFrames=audio->diagnostics().renderedFrames;++flowStep;}
        else if(flowStep==2&&elapsed>1.2){check("pause_freezes_simulation_and_audio",stats.runTime==flowFrozenTime&&player->getPosition()==flowFrozenPosition&&audio->diagnostics().renderedFrames==flowAudioFrames);++flowStep;}
        else if(flowStep==3&&elapsed>1.8){check("escape_resumes",gameState==GameState::PLAYING);focusCallback(window,0);check("focus_callback_pauses_and_releases_cursor",gameState==GameState::PAUSED&&camera->isFirstMouse()&&glfwGetInputMode(window,GLFW_CURSOR)==GLFW_CURSOR_NORMAL);++flowStep;}
        else if(flowStep==4&&elapsed>2.5){check("resume_recaptures_cursor",gameState==GameState::PLAYING&&glfwGetInputMode(window,GLFW_CURSOR)==GLFW_CURSOR_DISABLED);applyDamage(200,DamageSource::EnemyProjectile,player->getPosition()+glm::vec3(0,0,-3));++flowStep;}
        else if(flowStep==5&&elapsed>2.8){check("death_actual_damage_and_source",gameState==GameState::PLAYER_DEAD&&stats.damageTaken==100&&stats.fatalSource==DamageSource::EnemyProjectile);++flowStep;}
        else if(flowStep==6&&elapsed>3.8){check("held_fire_cannot_skip_result",gameState==GameState::PLAYER_DEAD&&stats.runIndex==1);++flowStep;}
        else if(flowStep==7&&elapsed>4.7){check("fresh_click_same_seed_restart",gameState==GameState::PLAYING&&stats.runIndex==2&&stats.seed==options.seed&&stats.damageTaken==0&&stats.fatalSource==DamageSource::Unknown&&player->getHp()==100&&director->combatSeconds()<.5);++flowStep;}
        if(elapsed>5){check("all_flow_stages_completed",flowStep==8);glfwSetWindowShouldClose(window,GL_TRUE);}return;
    }
    if(options.acceptance=="screenshots"){
        if(elapsed>.3&&elapsed<1.5&&gameState==GameState::READY)setState(GameState::PLAYING);
        if(elapsed>1.5&&elapsed<2.5&&gameState==GameState::PLAYING)setState(GameState::PAUSED);
        if(elapsed>2.5&&gameState==GameState::PAUSED)setState(GameState::PLAYING);
        if(elapsed>5)glfwSetWindowShouldClose(window,GL_TRUE);return;
    }
    if(gameState==GameState::PAUSED) {
        acceptanceLog<<"{\"event\":\"synthetic_resume\",\"reason\":\"acceptance automation resumes after focus pause\"}\n";
        setState(GameState::PLAYING);
    }
    if(options.acceptance=="restarts"){
        if(acceptanceFrames++==0){
            enemyManager->spawn(EnemyType::Chaser,{3,0,10});enemyManager->spawn(EnemyType::Sentry,{8,0,10});
            hostileProjectiles->spawn({0,1,10},{0,0,1});firePrimary();characters->emit(CharacterEffect::Impact,{0,1,11});audio->play(AudioCue::SentryCharge,{8,1,10},10001);
        }
        if(stateTimer>=(acceptanceRuns%2==0?.4f:1.5f)){
            // Re-populate transient state immediately before reset; alternating
            // cycles exercise both active 1.2s warnings and materialized spawns.
            firePrimary();hostileProjectiles->spawn({0,1,10},{0,0,1});
            characters->emit(CharacterEffect::Impact,{0,1,11});
            audio->play(AudioCue::PlayerHurt,{},0,false);
            const float actual=player->takeDamage(15);
            runController->stats().recordDamage(actual,DamageSource::EnemyProjectile);
            runController->stats().recordHostileShot();runController->stats().recordHostileHit();
            auto warningProgress=director->warningProgress();
            const auto warningCount=std::count_if(warningProgress.begin(),warningProgress.end(),[](float p){return p>=0;});

            acceptanceLog<<"{\"event\":\"before_reset\",\"cycle\":"<<acceptanceRuns+1<<",\"entities\":"<<enemyManager->getLiveCount()<<",\"hostile_projectiles\":"<<hostileProjectiles->size()<<",\"player_projectiles\":"<<weapon->getActiveBulletCount()<<",\"warnings\":"<<warningCount<<",\"damage_taken\":"<<runController->stats().snapshot().damageTaken<<",\"stats_shots\":"<<runController->stats().snapshot().shotsFired<<",\"voices\":"<<audio->diagnostics().activeVoices<<",\"audio_frames\":"<<audio->diagnostics().renderedFrames<<",\"rss_bytes\":"<<residentBytes()<<"}\n";
            ++acceptanceRuns;restartCurrentRun();acceptanceFrames=0;if(acceptanceRuns>=options.restartCycles)glfwSetWindowShouldClose(window,GL_TRUE);
        }return;
    }
    if(options.acceptance=="performance"&&gameState==GameState::ROOM_CLEARED){
        recordPerformance();++acceptanceRuns;if(acceptanceRuns>=3){glfwSetWindowShouldClose(window,GL_TRUE);return;}restartCurrentRun();return;
    }
    if(options.acceptance=="performance"&&runController->stats().snapshot().runTime>180){acceptanceFailed=true;recordPerformance();glfwSetWindowShouldClose(window,GL_TRUE);return;}
    if(options.acceptance=="stress"){
        mapRenderer->setEnvironmentParticleCount(2000);
        const auto& a=director->arenaDefinition();
        while(enemyManager->count(EnemyType::Chaser)<18)enemyManager->spawn(EnemyType::Chaser,{float(enemyManager->count(EnemyType::Chaser)%6)*2-5,0,-12});
        while(enemyManager->count(EnemyType::Sentry)<3)enemyManager->spawn(EnemyType::Sentry,a.sentryPositions[enemyManager->count(EnemyType::Sentry)]);
        while(hostileProjectiles->size()<32)hostileProjectiles->spawn({float(hostileProjectiles->size()%8)-4,1,-8},{0,0,1});
        if(elapsed>30){captureScreenshot("stress");recordPerformance();glfwSetWindowShouldClose(window,GL_TRUE);return;}
    }
    if(gameState!=GameState::PLAYING)return;
    // Deterministic synthetic orbit and aim; uses ordinary Weapon::fire and collisions.
    const float t=runController->stats().snapshot().runTime;const float angle=t*.24f;
    player->setPosition({14*std::sin(angle),.05f,14*std::cos(angle)});camera->setPosition(player->getEyePosition());
    float best=1e9;glm::vec3 target(0,1,0);bool found=false;
    for(const auto& e:enemyManager->getEnemies())if(!e.isDead()){
        auto center=e.getAABB().getCenter();float distance=glm::length(center-camera->getPosition());
        if(distance<best&&visible(camera->getPosition(),center)){best=distance;target=center;found=true;}
    }
    camera->setDirection(target-camera->getPosition());if(found&&options.acceptance=="performance")firePrimary();
    (void)dt;
}
void App::framebufferSizeCallback(GLFWwindow*,int w,int h){glViewport(0,0,w,h);}
void App::mouseCallback(GLFWwindow* window,double x,double y){
    auto* app=static_cast<App*>(glfwGetWindowUserPointer(window));if(!app||app->gameState!=GameState::PLAYING||!app->options.acceptance.empty())return;
    auto& camera=*app->camera;if(camera.isFirstMouse()){camera.setLastMousePos(x,y);camera.setFirstMouse(false);return;}
    float dx=static_cast<float>(x)-camera.getLastX(),dy=camera.getLastY()-static_cast<float>(y);camera.setLastMousePos(x,y);app->player->processMouseMovement(&camera,dx,dy);
}
void App::scrollCallback(GLFWwindow* window,double,double y){auto* app=static_cast<App*>(glfwGetWindowUserPointer(window));if(app&&app->gameState==GameState::PLAYING)app->camera->processMouseScroll(y);}
void App::focusCallback(GLFWwindow* window,int focused){auto* app=static_cast<App*>(glfwGetWindowUserPointer(window));if(!app)return;if(!focused&&app->gameState==GameState::PLAYING)app->setState(GameState::PAUSED);app->camera->setFirstMouse(true);app->refocused=true;}
}
