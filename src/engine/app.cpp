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
#include "enemy/crystal_combat.h"
#include "enemy/enemy_projectile_system.h"
#include "game/run_controller.h"
#include "game/encounter_director.h"
#include "game/exploration_controller.h"
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
#include <optional>
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
std::optional<glm::vec2> hudPosition(const glm::mat4& viewProjection,
                                     const glm::vec3& position, const ViewLens& lens) {
    const glm::vec4 clip = viewProjection * glm::vec4(position, 1.f);
    if (clip.w <= 0.f || glm::any(glm::greaterThan(glm::abs(glm::vec3(clip)), glm::vec3(clip.w))))
        return std::nullopt;
    const glm::vec2 display = lens.displayNdc(glm::vec2(clip) / clip.w);
    if (std::abs(display.x) > 1.f || std::abs(display.y) > 1.f)
        return std::nullopt;
    return glm::vec2(.5f + .5f * display.x, .5f - .5f * display.y);
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
        generalConfigFile = "generalconfig.local.json";
        if (!options.acceptance.empty())
            generalConfigFile = std::filesystem::path(options.outputDirectory) / generalConfigFile;
        fisheye = FisheyeSettings::load(generalConfigFile);
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
        const int sceneWidth=isCrystalRoute()?960:gc.render.pixelWidth;
        const int sceneHeight=isCrystalRoute()?600:gc.render.pixelHeight;
        if(!pixelRenderer->init(sceneWidth,sceneHeight)) throw std::runtime_error("Pixel framebuffer initialization failed");
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
        glfwSetKeyCallback(window, keyCallback);
        glfwSetCursorPosCallback(window,mouseCallback);glfwSetScrollCallback(window,scrollCallback);
        // This hidden synthetic route invokes focusCallback explicitly; OS focus
        // delivery is covered by ordinary gameplay, not this deterministic harness.
        if(options.acceptance!="rifle-ads" && !isCrystalRoute())glfwSetWindowFocusCallback(window,focusCallback);
        setState(GameState::READY);
        int fw,fh;glfwGetFramebufferSize(window,&fw,&fh);
        if(acceptanceLog) acceptanceLog<<"{\"event\":\"configuration\",\"logical_width\":"<<options.width<<",\"logical_height\":"<<options.height<<",\"framebuffer_width\":"<<fw<<",\"framebuffer_height\":"<<fh<<",\"internal_width\":"<<pixelRenderer->getPixelWidth()<<",\"internal_height\":"<<pixelRenderer->getPixelHeight()<<",\"vsync\":"<<(options.vsync?"true":"false")<<",\"audio_available\":"<<(audio->available()?"true":"false")<<",\"synthetic_input\":true}\n";
    }catch(...){cleanup();throw;}
}
App::~App(){cleanup();}
bool App::initWindow(){
    if(!glfwInit())return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
#endif
    if(options.acceptance=="rifle-ads" || isCrystalRoute())glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
    window=glfwCreateWindow(options.width,options.height,"SILIC2",nullptr,nullptr);
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
    if(!candidate->getSpatialDiagnostics().empty()) {
        for(const auto& message:candidate->getSpatialDiagnostics())std::cerr<<"Spatial map: "<<message<<'\n';
        return false;
    }
    const bool candidateArena=candidate->getGameMode()==GameMode::ArenaSurvival;
    std::shared_ptr<const ArenaDefinition> definition;
    const bool candidateExploration=candidate->getGameMode()==GameMode::Exploration;
    if(candidateArena || candidateExploration) {
        definition=std::make_shared<ArenaDefinition>(ArenaDefinition::fromMap(*candidate));
        if(!definition->hasConnectedWaypoints(.34f)) {
            std::cerr << "Map load failed: arena navigation is disconnected\n";
            return false;
        }
    }
    if(!options.acceptance.empty()&&!candidateArena&&!candidateExploration&&options.acceptance!="screenshots")
        throw std::runtime_error("This acceptance mode requires an arena_survival map");
    if(options.acceptance=="spatial-combat"&&(!candidateArena||!candidate->getSpatialWorld()))throw std::runtime_error("Spatial combat acceptance requires 3D survival map");
    if(options.acceptance=="exploration"&&!candidateExploration) throw std::runtime_error("Exploration acceptance requires exploration map");
    // Build a complete candidate renderer while the previous scene stays alive.
    auto candidateRenderer=std::make_unique<MapRenderer>();
    if(!candidateRenderer->loadMap(*candidate))return false;
    currentMap=std::move(candidate);
    mapRenderer=std::move(candidateRenderer);
    currentMapFile=path;arena=candidateArena;
    combatLog.close();
    if (arena && currentMap->getArenaLayout().encounterProfile==EncounterProfile::CrystalSurvival) {
        combatLog.open(options.outputDirectory+"/combat-events.jsonl",std::ios::app);
        if (!combatLog) throw std::runtime_error("Cannot open Crystal combat event log");
    }
    glfwSetWindowTitle(window,(std::string("SILIC2 | ")+currentMap->getDisplayName()).c_str());
    director=std::make_unique<EncounterDirector>(definition?*definition:ArenaDefinition{});
    exploration=candidateExploration?std::make_unique<ExplorationController>(*currentMap):nullptr;
    enemyManager->setArenaDefinition(definition);
    const bool crystalAura=currentMap->getEnvironmentPreset()==EnvironmentPreset::ForestCrystal;
    groundParticles=createEnhancedGroundParticleSystem(crystalAura?96:2000,crystalAura?GroundParticleSystem::GParticleMode::CRYSTAL:GroundParticleSystem::GParticleMode::FIRE);
    groundParticles->initialize(*currentMap);
    groundParticles->setEnabled((crystalAura||(!arena&&!exploration))&&GameConfig::getInstance().effects.enableGroundParticles);
    runController->restartRun(path);resetRuntimeState();setState(GameState::READY);return true;
}

void App::setState(GameState next){
    gameState=next;stateTimer=0;camera->setFirstMouse(true);
    if(next!=GameState::PLAYING && next!=GameState::PAUSED) {
        aim.reset();aimHeld=false;updateFirstPerson(0);
    }
    glfwSetInputMode(window,GLFW_CURSOR,next==GameState::PLAYING?GLFW_CURSOR_DISABLED:GLFW_CURSOR_NORMAL);
    audio->setPaused(next==GameState::PAUSED||next==GameState::READY);
    if(next==GameState::PLAYING) {
        refocused=true;
        camera->syncMovementLeanPosition(player->getPosition());
        player->syncRollButtons(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS,
                                glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
    }
}
void App::run(){
    std::ofstream completeFrames;
    if(options.acceptance=="spatial-combat" || options.acceptance=="crystal-horde"){
        completeFrames.open(options.outputDirectory+"/frames-complete.csv");
        completeFrames<<"frame,wall_clock_ms,simulation_seconds,captured,measuring,playing,x,y,z,living\n";
    }
    size_t completeFrame=0;
    double previous=glfwGetTime();modeStarted=previous;flowPlaybackSeconds=0;
    if(!options.acceptance.empty()&&options.acceptance!="screenshots"&&options.acceptance!="flow")setState(GameState::PLAYING);
    while(!glfwWindowShouldClose(window)){
        frameStarted=glfwGetTime();const double raw=frameStarted-previous;previous=frameStarted;
        // Simulation stability clamp is independent of the uncapped performance record.
        deltaTime=static_cast<float>(std::min(raw,0.05));
        if(isCrystalRoute()) deltaTime=1.f/30.f;
        const bool measuring=gameState==GameState::PLAYING&&!refocused;
        // Synthetic flow inputs and assertions share a bounded frame clock.
        // GPU readback / screenshot I/O must not skip an input's press window.
        if(options.acceptance=="flow"||options.acceptance=="camera-lean"||options.acceptance=="rifle-ads")flowPlaybackSeconds+=deltaTime;
        frameCaptured=false;refocused=false;processInput();
        if(!options.acceptance.empty())updateAcceptance(deltaTime);
        update(deltaTime);render();
        glfwSwapBuffers(window);glfwPollEvents();
        if(completeFrames.is_open()) {
            auto p=player->getPosition();
            completeFrames<<completeFrame++<<','<<(glfwGetTime()-frameStarted)*1000.0<<','<<runController->stats().snapshot().runTime<<','<<frameCaptured<<','<<measuring<<','<<(gameState==GameState::PLAYING)<<','<<p.x<<','<<p.y<<','<<p.z<<','<<enemyManager->getLiveCount()<<'\n';
        }
        if(measuring&&gameState==GameState::PLAYING&&!frameCaptured&&(options.acceptance=="performance"||options.acceptance=="stress"||options.acceptance=="exploration"||options.acceptance=="spatial-combat"))
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
    if(gameState==GameState::PLAYING && options.acceptance!="rifle-ads" && !isCrystalRoute())
        aimHeld=glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS;
    if(gameState==GameState::PLAYING&&(options.acceptance.empty()||options.acceptance=="flow")){
        player->processInput(window,camera.get(),deltaTime);if(left)firePrimary();
    }
}
void App::firePrimary(){if(weapon->fire(*camera,aimHeld || aim.blend()>0)){runController->stats().recordShot();characters->onPrimaryShot();audio->play(AudioCue::PlayerShot,{},0,false);}}
void App::update(float dt){
    fisheyeFeedbackSeconds = std::max(0.f, fisheyeFeedbackSeconds - dt);
    if(gameState!=GameState::PAUSED)stateTimer+=dt;
    if(gameState==GameState::PLAYING){runController->tick(dt,true);updatePlaying(dt);}
    audio->setListener(camera->getPosition(),camera->getFront(),camera->getUp());
}
bool App::visible(const glm::vec3& from,const glm::vec3& to)const{
    if(auto world=currentMap->getSpatialWorld())return !world->raycast(from,to).hit;
    const float distance=glm::length(to-from);if(distance<.001f)return true;
    for(const auto& b:currentMap->getBrushes())if(!b.vertices.empty()){
        auto hit=CollisionSystem::raycastAABB(Ray(from,to-from),brushBounds(b),distance);
        if(hit.collided&&hit.distance<distance-.02f)return false;
    }return true;
}
float App::applyDamage(float amount,DamageSource source,const glm::vec3& origin,bool newEvent){
    // Invulnerability is confined to explicitly labelled synthetic acceptance input.
    const bool syntheticInvulnerable=options.acceptance=="performance"||options.acceptance=="stress"||
        options.acceptance=="restarts"||options.acceptance=="crystal-horde";
    const float actual=syntheticInvulnerable?0:player->takeDamage(amount);
    runController->stats().recordDamage(actual,source,newEvent);
    if(actual>0){damageAge=0;auto dir=origin-camera->getPosition();damageDirection=std::atan2(glm::dot(dir,camera->getRight()),glm::dot(dir,camera->getFront()));if(newEvent)audio->play(AudioCue::PlayerHurt,{},0,false);}
    if(player->isDead())runController->stats().recordFatalSource(source);
    return actual;
}
void App::updatePlaying(float dt){
    aim.update(aimHeld,dt);
    hitAge+=dt;killAge+=dt;damageAge+=dt;
    // The final roll displacement may land in a step that ends standing or jumping.
    const bool rollStep = player->getState() == PlayerState::ROLLING;
    player->update(dt,currentMap.get());
    auto pos=player->getPosition();const auto movement=pos-previousPlayerPosition;
    runController->stats().recordMovement(glm::length(glm::vec2(movement.x,movement.z)),player->getState()==PlayerState::SLIDING?MovementState::Sliding:MovementState::Other,dt);
    previousPlayerPosition=pos;camera->setPosition(player->getEyePosition()+player->getCameraOffset());camera->update();
    camera->updateMovementLean(pos,dt,rollStep);
    const auto result=weapon->update(dt,currentMap.get(),enemyManager.get());
    for(int i=0;i<result.hits;++i){runController->stats().recordHit();hitAge=0;audio->play(AudioCue::Hit,{},0,false);}
    for(auto type:result.killedEnemyTypes){runController->stats().recordKill(type);killAge=0;audio->play(AudioCue::Kill,{},0,false);}
    for(const auto& enemy:enemyManager->getEnemies()){
        if(enemy.isDead()){
            if (crystalCombat::isCrystalEnemy(enemy.getType()))
                logCombat(enemy.getAttackPhase()==AttackPhase::Windup?"killed_during_windup":"killed",enemy.getType(),enemy.getId());
            awaitingFirstThreat.erase(enemy.getId());audio->stopSource(actorAudioSource(enemy.getId()));characters->emit(CharacterEffect::Death,enemy.getAABB().getCenter());
        }
        else if(enemy.getSecondsSinceDamage()==0)characters->emit(CharacterEffect::Impact,enemy.getAABB().getCenter());
    }
    if(arena){
        DirectorInput input;
        input.playerPosition = pos;
        input.playerVelocity = player->getVelocity();
        const auto& layout = currentMap->getArenaLayout();
        if (layout.encounterProfile==EncounterProfile::CrystalSurvival)
            input.occupiedRifts.resize(layout.rifts.size());
        for (const auto& enemy : enemyManager->getEnemies()) {
            if (enemy.isDead()) continue;
            ++input.liveByType[static_cast<size_t>(enemy.getType())];
            if (layout.encounterProfile==EncounterProfile::CrystalSurvival)
                input.liveBounds.push_back(enemy.getAABB());
            if (enemy.getType()==EnemyType::Sentry) {
                for (size_t i=0; i<layout.sentryPositions.size(); ++i)
                    if (glm::distance(enemy.getPosition(),layout.sentryPositions[i])<1.f) input.occupiedSentrySlots[i]=true;
            }
            if (enemy.getType()!=EnemyType::VeilRay && enemy.getType()!=EnemyType::HangingCenser) {
                for (size_t i=0; i<input.occupiedRifts.size(); ++i)
                    if (glm::distance(enemy.getPosition(),layout.rifts[i])<2.f) input.occupiedRifts[i]=true;
            }
        }
        input.liveChasers = input.liveByType[static_cast<size_t>(EnemyType::Chaser)];
        input.liveSentries = input.liveByType[static_cast<size_t>(EnemyType::Sentry)];
        for(const auto& event:director->update(dt,input)){
            const uint32_t riftAudioSource=event.requestId & 0x7fffffffu;
            switch(event.kind){
                case DirectorEvent::Kind::WarningStarted:audio->play(AudioCue::RiftWarning,event.position,riftAudioSource);break;
                case DirectorEvent::Kind::WarningCancelled:audio->stopSource(riftAudioSource);break;
                case DirectorEvent::Kind::Spawn:
                    if (enemyManager->spawn(event.type,event.position)) {
                        const auto id=enemyManager->getEnemies().back().getId();
                        logCombat("spawn", event.type, id);
                        if (crystalCombat::isCrystalEnemy(event.type))
                            awaitingFirstThreat[id]=runController->stats().snapshot().runTime;
                    }
                    audio->stopSource(riftAudioSource);
                    break;
                case DirectorEvent::Kind::Deferred:runController->stats().recordSpawnDelay(toString(event.reason));break;
                case DirectorEvent::Kind::Cancelled:runController->stats().recordSpawnCancellation(toString(event.reason));audio->stopSource(riftAudioSource);break;
            }
        }
        enemyManager->setPressureStage(director->pressureStage());
        // The player's bullets resolve first. In finite encounters the last survivor wins
        // immediately, including when an older volley would land this tick.
        if (director->complete(enemyManager->getLiveCount())) {
            finishCurrentRun(RoomResult::Cleared,RunEndReason::Completed);
            return;
        }
    }
    if(exploration) {
        ExplorationInput input; input.feet=pos; input.eye=camera->getPosition(); input.forward=camera->getFront(); input.livingEnemies=enemyManager->getLiveCount();
        const auto events=exploration->update(dt,true,input);
        for(const auto& spawn:events.spawns) enemyManager->spawn(spawn.type=="sentry"?EnemyType::Sentry:EnemyType::Chaser,spawn.position);
        if(events.calibrated) audio->play(AudioCue::Transform,{},0,false);
        if(events.completed) { finishCurrentRun(RoomResult::Cleared,RunEndReason::Completed); return; }
    }
    const auto playerBounds=player->getAABB();
    enemyManager->update(dt,pos,currentMap.get(),&playerBounds,hostileProjectiles->activeVolleyCount());
    for(const auto& action : enemyManager->actions()) handleEnemyAction(action);
    const float dps=enemyManager->getContactDps(pos);
    if(dps>0 && !player->isDead()){glm::vec3 origin=pos;for(const auto& e:enemyManager->getEnemies())if(e.isTouchingPlayer(pos)){origin=e.getPosition();break;}applyDamage(dps*dt,DamageSource::EnemyContact,origin,!contactDamageActive);}contactDamageActive=dps>0;
    if(!player->isDead())for(const auto& hit:hostileProjectiles->update(dt,currentMap.get(),player->getAABB())){runController->stats().recordHostileHit();const auto actual=applyDamage(hit.damage,hit.source,hit.origin);
        logCombat("damage",hit.source==DamageSource::CenserBurst?EnemyType::HangingCenser:EnemyType::Sentry,hit.sourceId,actual);if(player->isDead())break;}
    const float time=runController->stats().snapshot().runTime;
    if(!exploration&&currentMap->getEnvironmentPreset()!=EnvironmentPreset::None&&!transformationPlayed&&time>=currentMap->getEnvironment().transformationStart){audio->play(AudioCue::Transform,{},0,false);transformationPlayed=true;}
    updateFirstPerson(dt);
    if(currentMap->getEnvironmentPreset()==EnvironmentPreset::ForestCrystal) {
        groundParticles->setEmitterPosition(player->getPosition());groundParticles->update(dt);
    } else if(!arena&&!exploration)groundParticles->update(dt);
    if(player->isDead())finishCurrentRun(RoomResult::PlayerDied,RunEndReason::PlayerDied);
    else if(!arena&&!exploration&&enemyManager->hasSpawnedEnemies()&&enemyManager->allEnemiesDead())finishCurrentRun(RoomResult::Cleared,RunEndReason::Completed);
}
void App::updateFirstPerson(float dt){
    const auto pos=player->getPosition();
    FirstPersonView fp;fp.aimBlend=aim.blend();fp.speed=glm::length(glm::vec2(player->getVelocity().x,player->getVelocity().z));fp.feet=pos;fp.forward=camera->getFront();fp.grounded=player->isOnGround();fp.sliding=player->getState()==PlayerState::SLIDING;
    fp.rollProgress=player->getRollProgress();
    if(dt>0 && fp.sliding&&!wasSliding)audio->play(AudioCue::Slide,{},0,false);wasSliding=fp.sliding;characters->update(dt,fp);
}
void App::finishCurrentRun(RoomResult room,RunEndReason reason){
    if(!runController->state().active)return;
    runController->endRoom(room);runController->endRun(reason);
    if (combatLog.is_open()) combatLog.flush();
    const auto line=runController->stats().formatRunResult();runLog<<line<<'\n';runLog.flush();std::cout<<line<<'\n';
    audio->stopAll();hostileProjectiles->clear();
    setState(reason==RunEndReason::Completed?GameState::ROOM_CLEARED:GameState::PLAYER_DEAD);
    audio->play(reason==RunEndReason::Completed?AudioCue::Victory:AudioCue::PlayerDeath,{},0,false);
}
glm::vec3 App::getPlayerStartPosition() const {
    const auto* start=currentMap->getPlayerStart();
    if (!start) return {0,2,0};
    auto feet=start->position;
    const auto& layout=currentMap->getArenaLayout();
    // Legacy brush sweeps treat touching a box as intersecting it. Leave the
    // usual support clearance when an arena spawn lies exactly on its floor.
    if (arena && layout.hasFloor && !currentMap->getSpatialWorld() &&
        std::abs(feet.y-layout.floorY)<.0001f) feet.y=layout.floorY+.015f;
    return feet;
}
void App::resetRuntimeState(){
    runController->stats().setExecutionContext(!options.acceptance.empty(),options.acceptance);
    runController->stats().setContentVersion(currentMap->getContentVersion());
    aim.reset();aimHeld=false;
    awaitingFirstThreat.clear();witnessedTypes.clear();censerInterrupts=0;
    crystalPeakLive=crystalFinalPeak=0;crystalNextSample=0;
    crystalOuterSeconds=crystalSprintSeconds=0;
    audio->reset();hostileProjectiles->clear();enemyManager->clear();weapon->reset();characters->reset();groundParticles->reset();mapRenderer->resetEnvironment();
    if(exploration)exploration->reset();
    director->reset(runController->state().seed);player->respawn(getPlayerStartPosition());previousPlayerPosition=player->getPosition();
    groundParticles->setEmitterPosition(player->getPosition());
    camera->resetOrientation();
    if(const auto* start=currentMap->getPlayerStart()) {
        const float yaw=glm::radians(start->rotation.y),pitch=glm::radians(start->rotation.x);
        camera->setDirection({std::cos(yaw)*std::cos(pitch),std::sin(pitch),std::sin(yaw)*std::cos(pitch)});
    }
    camera->setPosition(player->getEyePosition());
    camera->resetMovementLean(player->getPosition());
    if(!arena&&!exploration)enemyManager->spawnFromMap(*currentMap);
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
HudViewModel App::makeHud(const glm::mat4& view, const glm::mat4& projection,
                          const ViewLens& lens) const {
    HudViewModel vm;const auto& s=runController->stats().snapshot();
    vm.arenaSurvival=arena;vm.mapName=currentMap->getDisplayName();
    vm.endlessSurvival=arena && director->isEndless();
    if (vm.endlessSurvival) {
        vm.pressureStage=director->pressureStage();
        if (vm.pressureStage<4) vm.nextSurgeSeconds=static_cast<int>((vm.pressureStage+1)*EncounterDirector::PressureInterval);
    } else vm.generationSeconds=static_cast<int>(director->generationDeadline());
    vm.hasTransformation=!exploration&&currentMap->getEnvironmentPreset()!=EnvironmentPreset::None;
    if(exploration) { vm.exploration=true; vm.zoneName=exploration->snapshot().zoneName; vm.objective=exploration->objective(); vm.observationProgress=exploration->snapshot().observationProgress; vm.observationAvailable=exploration->snapshot().phase==ExplorationPhase::Observe; vm.calibrated=exploration->snapshot().calibrated; }
    vm.transformationStart=currentMap->getEnvironment().transformationStart;
    vm.transformationDuration=currentMap->getEnvironment().transformationDuration;
    switch(gameState){case GameState::READY:vm.phase=HudPhase::Ready;break;case GameState::PLAYING:vm.phase=HudPhase::Playing;break;case GameState::PAUSED:vm.phase=HudPhase::Paused;break;case GameState::ROOM_CLEARED:vm.phase=HudPhase::Won;break;case GameState::PLAYER_DEAD:vm.phase=HudPhase::Dead;break;}
    vm.hp=player->getHp();vm.maxHp=player->getMaxHp();vm.combatSeconds=s.runTime;vm.cleanup=arena&&director->generationFinished();vm.livingEnemies=static_cast<int>(enemyManager->getLiveCount());
    vm.resultLockRemaining=std::max(0.f,1-stateTimer);vm.kills=s.kills;vm.shotsFired=s.shotsFired;vm.shotsHit=s.shotsHit;vm.actualHpLoss=s.damageTaken;vm.fatalSource=toString(s.fatalSource);vm.seed=s.seed;vm.audioStatus=!audio->available()?HudAudioStatus::Unavailable:(audio->muted()?HudAudioStatus::Muted:HudAudioStatus::Available);
    vm.crosshairOpacity=1.f-aim.blend();
    vm.fisheyeEnabled = fisheye.enabled;
    vm.fisheyeHipStrength = static_cast<float>(fisheye.hipStrength);
    vm.showFisheyeFeedback = gameState == GameState::PLAYING && fisheyeFeedbackSeconds > 0.f;
    vm.hitAge=hitAge;vm.killAge=killAge;vm.damageAge=damageAge;vm.damageDirectionRadians=damageDirection;
    auto p=player->getPosition(),f=camera->getFront();vm.playerXZ={p.x,p.z};vm.playerForwardXZ={f.x,f.z};
    bool bounds=false;
    for(const auto& b:currentMap->getBrushes())if(!b.vertices.empty()){
        auto box=brushBounds(b);if(!bounds){vm.mapMinXZ={box.min.x,box.min.z};vm.mapMaxXZ={box.max.x,box.max.z};bounds=true;}
        else {vm.mapMinXZ=glm::min(vm.mapMinXZ,glm::vec2(box.min.x,box.min.z));vm.mapMaxXZ=glm::max(vm.mapMaxXZ,glm::vec2(box.max.x,box.max.z));}
        if(!currentMap->getSpatialWorld() && box.max.y-box.min.y>.2f)vm.terrain.push_back({{box.min.x,box.min.z},{box.max.x,box.max.z}});
    }
    if(auto world=currentMap->getSpatialWorld())for(const auto& t:world->triangles()) {
        if(std::abs(t.normal.y)>.65f)continue;
        auto minimum=glm::min(t.a,glm::min(t.b,t.c)),maximum=glm::max(t.a,glm::max(t.b,t.c));
        if(maximum.y<player->getPosition().y || minimum.y>player->getPosition().y+1.8f)continue;
        vm.terrain.push_back({{minimum.x,minimum.z},{maximum.x,maximum.z}});
    }
    const auto& layout=currentMap->getArenaLayout();
    if(layout.hasFloor){vm.mapMinXZ=layout.minXZ;vm.mapMaxXZ=layout.maxXZ;}
    if(arena){auto warnings=director->warningProgress();for(size_t i=0;i<layout.rifts.size();++i){auto r=layout.rifts[i];vm.rifts.push_back({{r.x,r.z},warnings[i]>=0});}}
    const glm::mat4 viewProjection = projection * view;
    for(const auto& enemy:enemyManager->getEnemies()){
        const auto bounds=enemy.getAABB();
        HudEnemy e;auto pos=enemy.getPosition(),center=bounds.getCenter();e.worldXZ={pos.x,pos.z};e.hp=enemy.getHp();e.maxHp=enemy.getMaxHp();e.alive=!enemy.isDead();e.sentry=enemy.getType()==EnemyType::Sentry;e.secondsSinceDamage=enemy.getSecondsSinceDamage();
        e.inView = hudPosition(viewProjection, center, lens).has_value();
        e.unobstructed=e.inView&&visible(camera->getPosition(),center);
        const glm::vec3 above(center.x,bounds.max.y+.2f,center.z);
        e.healthAnchor = hudPosition(viewProjection, above, lens).value_or(glm::vec2(-1.f));
        vm.enemies.push_back(e);
    }return vm;
}
void App::renderProjectiles(const glm::mat4& view,const glm::mat4& projection){
    if(hostileProjectiles->size()==0)return;
    projectileShader->use();projectileShader->setMat4("view",view);projectileShader->setMat4("projection",projection);projectileShader->setInt("numLights",0);projectileShader->setVec3("ambientLight",glm::vec3(1));projectileShader->setVec3("enemyColor",{1,.3f,.08f});
    glBindVertexArray(projectileVAO);for(const auto& p:hostileProjectiles->projectiles()){
        auto model=glm::scale(glm::translate(glm::mat4(1),p.position),glm::vec3(p.ballistic ? .22f : EnemyProjectileSystem::RADIUS));projectileShader->setMat4("model",model);glDrawArrays(GL_TRIANGLES,0,24);
    }glBindVertexArray(0);
}
void App::render(){
    int fw,fh,lw,lh;glfwGetFramebufferSize(window,&fw,&fh);glfwGetWindowSize(window,&lw,&lh);if(fw<=0||fh<=0)return;
    if(options.acceptance=="stress") {
        while(hostileProjectiles->size()<32) hostileProjectiles->spawn({float(hostileProjectiles->size()%8)-4,1,-8},{0,0,1});
    }
    pixelRenderer->beginPixelRender();glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);
    const float aspect = static_cast<float>(fw) / fh;
    const auto lens = ViewLens::forPlayer(aim.blend(), aspect, fisheye);
    const auto view = camera->getViewMatrix();
    const auto projection = lens.overscan(camera->getProjectionMatrix(
        aspect, aim.fov(player->getCurrentFov()), currentMap->getWorldSettings().farPlane));
    const auto& ws=currentMap->getWorldSettings();glClearColor(ws.backgroundColor.r,ws.backgroundColor.g,ws.backgroundColor.b,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    mapRenderer->clearDynamicLights();for(const auto& light:weapon->getActiveLights())mapRenderer->addDynamicLight(light.first,light.second,1,3);
    // Continuous self-emission in Forest; distant visibility remains ambient-lit.
    if(currentMap->getEnvironmentPreset()==EnvironmentPreset::ForestCrystal)
        mapRenderer->addDynamicLight(player->getPosition()+glm::vec3(0,1.4f,0),{.28f,.72f,1.f},.9f,18.f);
    if(exploration)mapRenderer->setObservationState(exploration->snapshot().observationProgress,exploration->snapshot().calibrated);
    mapRenderer->setEnvironmentTime(runController->stats().snapshot().runTime);mapRenderer->setRiftWarnings(arena?director->warningProgress():std::vector<float>{});
    mapRenderer->render(view,projection);
    const bool translucentForest=currentMap->getEnvironmentPreset()==EnvironmentPreset::ForestCrystal;
    if(!translucentForest)weapon->render(view,projection);
    std::vector<CharacterView> cv;for(const auto& e:enemyManager->getEnemies())if(!e.isDead()){
        CharacterView c;c.type=e.getType();c.position=e.getPosition();c.heading=e.getHeading();c.dimensions=e.getDimensions();
        c.crystalMinion=arena && director->isEndless();
        c.attackPhase=e.getAttackPhase();c.attackProgress=e.getPhaseProgress();c.interruptHits=e.getInterruptHits();
        c.attackStart=e.getSweepStart();c.attackEnd=e.getType()==EnemyType::VeilRay?e.getSweepEnd():e.getLockedTarget();
        c.charging=e.isCharging();c.chargeProgress=e.getChargeProgress();c.aimLocked=e.isAimLocked();c.hitFlash=e.getHitFlash();c.phase=e.getId()*.731f;c.movement=glm::clamp(e.getMovementSpeed()/(e.getType()==EnemyType::ForkHound?crystalCombat::HoundApproach:3.f),0.f,1.f);cv.push_back(c);
    }
    characters->renderWorld(view,projection,cv);
    std::vector<glm::vec3> landingPositions;
    for (const auto& orb : hostileProjectiles->projectiles())
        if (orb.ballistic) landingPositions.push_back(orb.target);
    if (!landingPositions.empty()) characters->renderImpactWarnings(view,projection,landingPositions);
    renderProjectiles(view,projection);
    if(!translucentForest&&!arena&&!exploration&&GameConfig::getInstance().effects.enableGroundParticles)groundParticles->render(view,projection);
    if(translucentForest){
        mapRenderer->renderTranslucent(view,projection);weapon->render(view,projection);
        groundParticles->render(view,projection);
    }
    characters->renderFirstPerson(projection, lens.overscanFactor());
    if(!pendingScreenshot.empty())captureScreenshot(pendingScreenshot+"-scene",pixelRenderer->getPixelWidth(),pixelRenderer->getPixelHeight());
    pixelRenderer->endPixelRender(fw,fh,lens);
    hudRenderer->render(makeHud(view,projection,lens),lw,lh,fw,fh);
    if(!pendingScreenshot.empty()){captureScreenshot(pendingScreenshot);pendingScreenshot.clear();}
    if((isCrystalRoute()||options.acceptance=="flow"||options.acceptance=="screenshots"||options.acceptance=="spatial-combat"||options.acceptance=="camera-lean"||options.acceptance=="rifle-ads")&&glGetError()!=GL_NO_ERROR)
        throw std::runtime_error("OpenGL error during runtime acceptance render");
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
void App::captureScreenshot(const std::string& name,int width,int height){
    frameCaptured=true;
    int w=width,h=height;if(!w||!h)glfwGetFramebufferSize(window,&w,&h);if(w<=0||h<=0)return;
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
// Explicit bounded synthetic functional route, never ordinary gameplay.
void App::updateAdsAcceptance(float dt){
    const float t=static_cast<float>(flowPlaybackSeconds);
    auto check=[&](const char* name,bool passed){
        acceptanceFailed|=!passed;
        acceptanceLog<<"{\"event\":\"ads_check\",\"check\":\""<<name<<"\",\"passed\":"<<(passed?"true":"false")
            <<",\"state\":"<<static_cast<int>(gameState)<<",\"blend\":"<<aim.blend()<<",\"fov\":"<<aim.fov(player->getCurrentFov())<<",\"mouse_scale\":"<<aim.mouseScale(player->getCurrentFov())<<"}\n";
    };
    if(flowStep==0 && t>.15f){
        // Start the hidden synthetic route; explicit focusCallback below still
        // exercises the same pause path used by ordinary visible gameplay.
        setState(GameState::PLAYING);
        check("hip_starts_clear",gameState==GameState::PLAYING && aim.blend()==0);pendingScreenshot="hip";++flowStep;
    }else if(flowStep==1 && t>.3f){
        aimHeld=true;flowFrozenPosition=player->getPosition();++flowStep;
    }else if(flowStep==2 && t>.6f){
        check("held_aim_reaches_target",aim.blend()==1 && std::abs(aim.fov(player->getCurrentFov())-32)<.001f);
        const auto front=camera->getFront();camera->setFirstMouse(true);
        processMousePosition(100,100);check("first_mouse_sample_does_not_jump",camera->getFront()==front);
        const auto yaw=camera->getYaw();processMousePosition(110,100);
        check("mouse_sensitivity_tracks_zoom",std::abs(camera->getYaw()-yaw-10*GameConfig::getInstance().player.mouseSensitivity*aim.mouseScale(player->getCurrentFov()))<.001f);
        camera->setDirection(front);camera->setFirstMouse(true);
        pendingScreenshot="ads";++flowStep;
    }else if(flowStep==3 && t>1.f){
        check("moving_while_aimed",aim.blend()==1 && glm::length(player->getPosition()-flowFrozenPosition)>.2f);
        auto projected=camera->getProjectionMatrix(1.6f,aim.fov(player->getCurrentFov()))*camera->getViewMatrix()*glm::vec4(camera->getPosition()+camera->getFront()*5.f,1);
        check("lean_keeps_center_ray",std::abs(projected.x/projected.w)<.00001f && std::abs(projected.y/projected.w)<.00001f);
        firePrimary();const auto& bullets=weapon->getBullets();
        check("aimed_shot_follows_eye_ray",!bullets.empty() && glm::length(bullets.back().position-camera->getPosition())<.00001f && glm::dot(bullets.back().direction,camera->getFront())>.99999f && bullets.back().speed==195);
        check("sustained_aim_fire_works",runController->stats().snapshot().shotsFired>=3);
        pendingScreenshot="ads-moving-fire";++flowStep;
    }else if(flowStep==4 && t>1.25f){
        adsFrozenBlend=aim.blend();flowFrozenTime=runController->stats().snapshot().runTime;
        focusCallback(window,0);pendingScreenshot="ads-paused";++flowStep;
    }else if(flowStep==5 && t>1.55f){
        aimHeld=false;check("focus_pause_freezes_ads",gameState==GameState::PAUSED && adsFrozenBlend==1 && aim.blend()==adsFrozenBlend && runController->stats().snapshot().runTime==flowFrozenTime);
        setState(GameState::PLAYING);check("resume_resets_mouse_sampling",camera->isFirstMouse());++flowStep;
    }else if(flowStep==6 && t>1.8f){
        check("released_during_pause_returns_to_hip",aim.blend()==0);pendingScreenshot="released";++flowStep;
    }else if(flowStep==7 && t>1.95f){aimHeld=true;++flowStep;
    }else if(flowStep==8 && t>2.25f){
        check("reentry_reaches_ads",aim.blend()==1);applyDamage(200,DamageSource::EnemyProjectile,player->getPosition());++flowStep;
    }else if(flowStep==9 && t>2.4f){
        check("death_clears_ads",gameState==GameState::PLAYER_DEAD && aim.blend()==0 && !aimHeld);pendingScreenshot="ads-dead";++flowStep;
    }else if(flowStep==10 && t>2.6f){
        restartCurrentRun();check("restart_clears_ads",aim.blend()==0 && !aimHeld);pendingScreenshot="ads-restart";++flowStep;
    }else if(flowStep==11 && t>2.9f){
        aimHeld=true;aim.update(true,.16f);const auto original=currentMapFile;
        const bool switched=loadMap("res/maps/hollow_orbit_arena.json");
        check("map_switch_clears_ads",switched && aim.blend()==0 && !aimHeld && gameState==GameState::READY);
        const bool returned=loadMap(original);check("forest_return_clears_ads",returned && aim.blend()==0 && !aimHeld);
        setState(GameState::PLAYING);++flowStep;
    }else if(flowStep==12 && t>3.15f){
        check("all_ads_stages_completed",flowStep==12);glfwSetWindowShouldClose(window,GL_TRUE);++flowStep;
    }
    if(gameState==GameState::PLAYING){
        if(t>.65f && t<.90f)firePrimary();
        PlayerInput input;input.right=t>.65f && t<1.1f;
        player->processInput(input,camera.get(),dt);
    }
}

void App::updateAcceptance(float dt){
    if(isCrystalRoute()){updateCrystalAcceptance(dt);return;}
    if(options.acceptance=="rifle-ads"){updateAdsAcceptance(dt);return;}
    if(options.acceptance=="camera-lean") {
        const float t=float(flowPlaybackSeconds);
        auto check=[&](const char* name,bool passed){acceptanceFailed|=!passed;acceptanceLog<<"{\"event\":\"lean_check\",\"check\":\""<<name<<"\",\"passed\":"<<(passed?"true":"false")<<",\"seconds\":"<<t<<",\"roll_degrees\":"<<camera->getMovementRollDegrees()<<"}\n";};
        if(flowStep==0&&t>.7f){check("left_head_tilts_left",camera->getMovementRollDegrees()<-.5f);pendingScreenshot="lean-left";++flowStep;}
        else if(flowStep==1&&t>1.15f){check("braking_rebounds_right",camera->getMovementRollDegrees()>0);pendingScreenshot="lean-brake";++flowStep;}
        else if(flowStep==2&&t>1.95f){check("right_head_tilts_right",camera->getMovementRollDegrees()>.5f);pendingScreenshot="lean-right";++flowStep;}
        else if(flowStep==3&&t>2.1f){leanFrozenRoll=camera->getMovementRollDegrees();setState(GameState::PAUSED);pendingScreenshot="lean-paused";++flowStep;}
        else if(flowStep==4&&t>2.4f){check("paused_roll_frozen",camera->getMovementRollDegrees()==leanFrozenRoll);setState(GameState::PLAYING);++flowStep;}
        else if(flowStep==5&&t>2.85f){restartCurrentRun();check("restart_roll_zero",camera->getMovementRollDegrees()==0);pendingScreenshot="lean-restart";++flowStep;}
        else if(flowStep==6&&t>3.0f){glfwSetWindowShouldClose(window,GL_TRUE);}
        if(gameState==GameState::PLAYING){PlayerInput keys;keys.left=t<.8f;keys.right=t>=1.3f&&t<2.1f;player->processInput(keys,camera.get(),dt);}
        return;
    }
    if(options.acceptance=="spatial-combat"){updateSpatialCombatAcceptance(dt);return;}
    if(options.acceptance=="exploration"){updateExplorationAcceptance(dt);return;}
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
    if(options.acceptance=="performance"){
        const float seconds=runController->stats().snapshot().runTime;
        if(gameState==GameState::ROOM_CLEARED || (director->isEndless() && seconds>=210.f)){
            recordPerformance();++acceptanceRuns;
            if(acceptanceRuns>=3)glfwSetWindowShouldClose(window,GL_TRUE);
            else restartCurrentRun();
            return;
        }
        if(!director->isEndless() && seconds>180){
            acceptanceFailed=true;recordPerformance();glfwSetWindowShouldClose(window,GL_TRUE);return;
        }
    }
    if(options.acceptance=="stress"){
        mapRenderer->setEnvironmentParticleCount(2000);
        const auto& a=director->arenaDefinition();
        const size_t chaserLimit=director->isEndless() ? EncounterDirector::crystalMaxAlive(EnemyType::Chaser) : EncounterDirector::ChaserCap;
        for(size_t i=enemyManager->count(EnemyType::Chaser);i<chaserLimit;++i)
            if(!enemyManager->spawn(EnemyType::Chaser,{float(i%6)*2-5,0,-12})) break;
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
void App::updateExplorationAcceptance(float dt) {
    const auto& settings=currentMap->getExploration();
    const auto& state=exploration->snapshot();
    const auto& stats=runController->stats().snapshot();
    auto check=[&](const char* name,bool passed) { acceptanceFailed|=!passed; acceptanceLog<<"{\"event\":\"journey_check\",\"check\":\""<<name<<"\",\"passed\":"<<(passed?"true":"false")<<"}\n"; };
    if(gameState==GameState::PAUSED) { acceptanceLog<<"{\"event\":\"synthetic_focus_resume\"}\n";setState(GameState::PLAYING); }
    if(gameState==GameState::PLAYER_DEAD || stats.runTime>150) {
        check("journey_completed_without_shortcut",false);recordPerformance();pendingScreenshot="journey-failure";glfwSetWindowShouldClose(window,GL_TRUE);return;
    }
    if(gameState==GameState::ROOM_CLEARED) {
        check("ordinary_combat_and_observation_then_exit",state.calibrated && state.phase==ExplorationPhase::Complete && stats.kills==static_cast<int>(settings.spawns.size()) && stats.shotsFired>0 && stats.runTime>=60 && stats.distanceTravelled>60);
        acceptanceLog<<"{\"event\":\"journey_end\",\"synthetic_input\":true,\"seconds\":"<<stats.runTime<<",\"kills\":"<<stats.kills<<",\"shots\":"<<stats.shotsFired<<",\"damage\":"<<stats.damageTaken<<",\"node\":"<<journeyNode<<",\"capture_frames_excluded\":true}\n";
        recordPerformance(); pendingScreenshot="journey-complete";glfwSetWindowShouldClose(window,GL_TRUE);return;
    }
    if(gameState!=GameState::PLAYING)return;
    if(journeyPhase!=static_cast<int>(state.phase)) {
        journeyPhase=static_cast<int>(state.phase);
        acceptanceLog<<"{\"event\":\"journey_phase\",\"phase\":"<<journeyPhase<<",\"seconds\":"<<stats.runTime<<",\"living\":"<<enemyManager->getLiveCount()<<",\"progress\":"<<state.observationProgress<<"}\n";
        if(state.calibrated)pendingScreenshot="star-calibrated";
    }
    PlayerInput keys;
    if(state.phase==ExplorationPhase::Encounter) {
        float best=1e9f; glm::vec3 target; bool found=false;
        for(const auto& enemy:enemyManager->getEnemies())if(!enemy.isDead()) {
            auto center=enemy.getAABB().getCenter();float distance=glm::length(center-camera->getPosition());
            if(distance<best && visible(camera->getPosition(),center)) {best=distance;target=center;found=true;}
        }
        if(found){camera->setDirection(target-camera->getPosition());firePrimary();}
        player->processInput(keys,camera.get(),dt);return;
    }
    const size_t node=std::min(journeyNode,settings.guidePoints.size()-1);
    auto goal=settings.guidePoints[node],delta=goal-player->getPosition();delta.y=0;
    const bool station=glm::length(glm::vec2(goal.x-settings.observerPosition.x,goal.z-settings.observerPosition.z))<.1f;
    if(glm::length(delta)>.18f && journeyHold==0) {camera->setDirection(delta);keys.forward=true;}
    else {
        if(journeyHold==0) {
            pendingScreenshot="route-node-"+std::to_string(node);
            const auto feet=player->getPosition();
            acceptanceLog<<"{\"event\":\"route_node\",\"node\":"<<node<<",\"seconds\":"<<stats.runTime<<",\"feet\":["<<feet.x<<','<<feet.y<<','<<feet.z<<"]}\n";
        }
        journeyHold+=dt;
        if(station)camera->setDirection(settings.target-camera->getPosition());
        else if(node+1<settings.guidePoints.size()) camera->setDirection(settings.guidePoints[node+1]+glm::vec3(0,settings.observerEyeHeight,0)-camera->getPosition());
        // Hold the aligned, busiest view for 20s, then sample the whole route for at least 65s.
        const bool longEnough=journeyHold>(station?20.f:1.8f);
        const bool exitAllowed=node+2<settings.guidePoints.size() || stats.runTime>=65;
        if(longEnough && (!station||state.calibrated) && exitAllowed && node+1<settings.guidePoints.size()){++journeyNode;journeyHold=0;}
    }
    player->processInput(keys,camera.get(),dt);
}
void App::updateSpatialCombatAcceptance(float dt) {
    const auto& stats=runController->stats().snapshot();const auto& route=currentMap->getSpatialSettings().route;
    auto finish=[&](bool passed,const char* reason) {
        acceptanceFailed|=!passed;
        acceptanceLog<<"{\"event\":\"spatial_combat_end\",\"passed\":"<<(passed?"true":"false")<<",\"reason\":\""<<reason<<"\",\"synthetic_input\":true,\"seconds\":"<<stats.runTime<<",\"route_laps\":"<<combatLap<<",\"route_node\":"<<journeyNode<<",\"kills\":"<<stats.kills<<",\"shots\":"<<stats.shotsFired<<",\"damage\":"<<stats.damageTaken<<",\"distance\":"<<stats.distanceTravelled<<",\"first_fire_seconds\":"<<firstFireSeconds<<",\"longest_no_visible_with_living_seconds\":"<<longestNoVisible<<",\"generation_gap_seconds\":"<<longestGenerationGap<<",\"cleanup_gap_seconds\":"<<longestCleanupGap<<",\"remaining_enemies\":"<<enemyManager->getLiveCount()<<"}\n";
        for(const auto& e:enemyManager->getEnemies())if(!e.isDead()){auto p=e.getPosition();acceptanceLog<<"{\"event\":\"remaining_enemy\",\"type\":\""<<toString(e.getType())<<"\",\"position\":["<<p.x<<','<<p.y<<','<<p.z<<"],\"speed\":"<<e.getMovementSpeed()<<"}\n";}
        recordPerformance();pendingScreenshot=passed?(director->isEndless()?"combat-result":"combat-cleared"):"combat-failed";glfwSetWindowShouldClose(window,GL_TRUE);
    };
    if(gameState==GameState::PAUSED){acceptanceLog<<"{\"event\":\"synthetic_focus_resume\"}\n";setState(GameState::PLAYING);}
    const bool exercisedSurvival=director->isEndless() && stats.kills>0 && stats.distanceTravelled>50.f;
    if(gameState==GameState::PLAYER_DEAD){
        finish(exercisedSurvival && player->getHp()<=0 && stats.fatalSource!=DamageSource::Unknown,"player_dead");
        return;
    }
    if(gameState==GameState::ROOM_CLEARED){finish(stats.runTime>=director->generationDeadline()&&combatLap>=1&&stats.kills>0&&stats.shotsFired>0,"generation_and_cleanup_complete");return;}
    if(route.empty()){finish(false,"missing_route");return;}
    if(stats.runTime>210){
        if(director->isEndless()) finish(exercisedSurvival && gameState==GameState::PLAYING && player->getHp()>0,"survived_observation_window");
        else finish(false,"timeout");
        return;
    }
    if(gameState!=GameState::PLAYING)return;
    glm::vec3 position=player->getPosition();const size_t node=journeyNode%route.size();auto delta=route[node]-position;delta.y=0;
    if(glm::length(delta)<.35f && std::abs(position.y-route[node].y)<.45f) {
        acceptanceLog<<"{\"event\":\"combat_route_node\",\"lap\":"<<combatLap<<",\"node\":"<<node<<",\"seconds\":"<<stats.runTime<<",\"frame\":"<<frameTimes.size()<<",\"feet\":["<<position.x<<','<<position.y<<','<<position.z<<"]}\n";
        if(combatLap==0 && (node==0||node==4||node==14||node==28||node==34||node==38))pendingScreenshot="combat-node-"+std::to_string(node);
        ++journeyNode;if(journeyNode>=route.size()){journeyNode=0;++combatLap;}
        delta=route[journeyNode]-position;delta.y=0;
    }
    bool found=false;glm::vec3 aim;float nearest=1e9f;
    for(const auto& e:enemyManager->getEnemies())if(!e.isDead()){auto center=e.getAABB().getCenter();float distance=glm::length(center-camera->getPosition());if(distance<nearest&&visible(camera->getPosition(),center)){nearest=distance;aim=center;found=true;}}
    if(found){
        if(noVisibleSeconds>=1)acceptanceLog<<"{\"event\":\"visibility_gap_end\",\"seconds\":"<<stats.runTime<<",\"duration\":"<<noVisibleSeconds<<"}\n";
        camera->setDirection(aim-camera->getPosition());firePrimary();if(firstFireSeconds<0)firstFireSeconds=stats.runTime;noVisibleSeconds=generationGap=cleanupGap=0;
        if(!combatLowerCaptured&&stats.runTime>25&&position.y<1&&aim.y<2){combatLowerCaptured=true;pendingScreenshot="combat-lower-engagement";}
    } else {
        if(enemyManager->getLiveCount()>0){
            float previousGap=noVisibleSeconds;noVisibleSeconds+=dt;longestNoVisible=std::max(longestNoVisible,noVisibleSeconds);
            if(stats.runTime<director->generationDeadline()){generationGap+=dt;longestGenerationGap=std::max(longestGenerationGap,generationGap);}else {cleanupGap+=dt;longestCleanupGap=std::max(longestCleanupGap,cleanupGap);}
            if(previousGap<5&&noVisibleSeconds>=5)for(const auto& e:enemyManager->getEnemies())if(!e.isDead()){auto p=e.getPosition();acceptanceLog<<"{\"event\":\"visibility_gap_enemy\",\"seconds\":"<<stats.runTime<<",\"type\":\""<<toString(e.getType())<<"\",\"position\":["<<p.x<<','<<p.y<<','<<p.z<<"]}\n";}
        }else noVisibleSeconds=generationGap=cleanupGap=0;
        // After the generation interval, approach the remaining enemy through the same
        // supported navigation used by actors; movement still uses ordinary WASD.
        if(stats.runTime>=director->generationDeadline()&&combatLap>=1){
            cleanupRefresh-=dt;
            if(cleanupRefresh<=0||glm::length(cleanupWaypoint-position)<.4f){
                float distance=1e9f;glm::vec3 target=position;
                for(const auto& e:enemyManager->getEnemies())if(!e.isDead()&&glm::length(e.getPosition()-position)<distance){distance=glm::length(e.getPosition()-position);target=e.getPosition();}
                cleanupWaypoint=currentMap->getSpatialWorld()->nextWaypoint(position,target,.34f);cleanupRefresh=.2f;
            }
            delta=cleanupWaypoint-position;delta.y=0;
        }
        camera->setDirection(delta);
    }
    PlayerInput keys;if(glm::length(delta)>.15f){auto desired=glm::normalize(delta),forward=camera->getFront(),right=camera->getRight();forward.y=right.y=0;if(glm::length(forward)<.001f)forward=desired;if(glm::length(right)<.001f)right=glm::cross(forward,glm::vec3(0,1,0));forward=glm::normalize(forward);right=glm::normalize(right);float f=glm::dot(desired,forward),r=glm::dot(desired,right);keys.forward=f>.35f;keys.backward=f<-.35f;keys.right=r>.35f;keys.left=r<-.35f;}
    player->processInput(keys,camera.get(),dt);
    if(!combatShiftBefore&&stats.runTime>=44){combatShiftBefore=true;pendingScreenshot="combat-before-shift";}
    if(!combatShiftAfter&&stats.runTime>=49){combatShiftAfter=true;pendingScreenshot="combat-after-shift";}
    if(stats.shotsFired!=combatLastShots){combatLastShots=stats.shotsFired;if(combatLastShots%25==0)acceptanceLog<<"{\"event\":\"combat_pressure\",\"seconds\":"<<stats.runTime<<",\"shots\":"<<stats.shotsFired<<",\"living\":"<<enemyManager->getLiveCount()<<",\"damage\":"<<stats.damageTaken<<"}\n";}
}
void App::framebufferSizeCallback(GLFWwindow*,int w,int h){glViewport(0,0,w,h);}
void App::mouseCallback(GLFWwindow* window,double x,double y){
    auto* app=static_cast<App*>(glfwGetWindowUserPointer(window));if(!app||app->gameState!=GameState::PLAYING||!app->options.acceptance.empty())return;
    app->processMousePosition(x,y);
}
void App::keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (!app || (app->gameState != GameState::PLAYING && app->gameState != GameState::PAUSED &&
                 app->gameState != GameState::READY)) return;

    bool changed = true;
    switch (key) {
        case GLFW_KEY_V: app->fisheye.enabled = !app->fisheye.enabled; break;
        case GLFW_KEY_LEFT_BRACKET: changed = app->fisheye.adjustHip(false); break;
        case GLFW_KEY_RIGHT_BRACKET: changed = app->fisheye.adjustHip(true); break;
        default: return;
    }
    app->fisheyeFeedbackSeconds = 2.f;
    if (changed) {
        try {
            app->fisheye.save(app->generalConfigFile);
        } catch (const std::exception& e) {
            std::cerr << "Could not save " << app->generalConfigFile << ": " << e.what() << '\n';
        }
    }
}
void App::processMousePosition(double x,double y){
    if(camera->isFirstMouse()){camera->setLastMousePos(x,y);camera->setFirstMouse(false);return;}
    const float dx=static_cast<float>(x)-camera->getLastX(),dy=camera->getLastY()-static_cast<float>(y);
    camera->setLastMousePos(x,y);
    const float scale=aim.mouseScale(player->getCurrentFov());
    player->processMouseMovement(camera.get(),dx*scale,dy*scale);
}
void App::scrollCallback(GLFWwindow* window,double,double y){auto* app=static_cast<App*>(glfwGetWindowUserPointer(window));if(app&&app->gameState==GameState::PLAYING)app->camera->processMouseScroll(y);}
void App::focusCallback(GLFWwindow* window,int focused){auto* app=static_cast<App*>(glfwGetWindowUserPointer(window));if(!app)return;if(app->options.acceptance=="rifle-ads")app->acceptanceLog<<"{\"event\":\"focus_callback\",\"focused\":"<<focused<<",\"seconds\":"<<app->flowPlaybackSeconds<<"}\n";if(!focused&&app->gameState==GameState::PLAYING)app->setState(GameState::PAUSED);app->camera->setFirstMouse(true);app->refocused=true;}
}
