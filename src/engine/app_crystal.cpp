#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "engine/app.h"
#include "audio/audio_system.h"
#include "character/character_presentation.h"
#include "enemy/enemy_manager.h"
#include "enemy/enemy_projectile_system.h"
#include "enemy/crystal_combat.h"
#include "engine/camera.h"
#include "game/encounter_director.h"
#include "game/run_controller.h"
#include "player/player.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace silic2 {
namespace {
AudioCue warningCue(EnemyType type) {
    switch(type) {
        case EnemyType::ForkHound: return AudioCue::HoundBrace;
        case EnemyType::VeilRay: return AudioCue::RayFold;
        case EnemyType::HangingCenser: return AudioCue::CenserOpen;
        default: return AudioCue::SentryCharge;
    }
}

const char* attackName(EnemyType type) {
    switch(type) {
        case EnemyType::ForkHound: return "hound_dash";
        case EnemyType::VeilRay: return "ray_sweep";
        case EnemyType::HangingCenser: return "censer_burst";
        case EnemyType::Sentry: return "sentry_projectile";
        default: return "contact";
    }
}
}

void App::logCombat(const char* event, EnemyType type, uint32_t id, float value) {
    if (!combatLog.is_open()) return;
    const auto& run=runController->state();
    combatLog << "{\"event\":\"" << event << "\",\"type\":\"" << toString(type)
              << "\",\"attack\":\"" << attackName(type) << "\",\"id\":" << id
              << ",\"seconds\":" << runController->stats().snapshot().runTime
              << ",\"seed\":" << run.seed << ",\"run_index\":" << run.runIndex
              << ",\"mode\":\"" << options.acceptance << "\",\"value\":" << value << "}\n";
}

void App::handleEnemyAction(const EnemyAction& action) {
    using Kind=EnemyAction::Kind;
    const auto source=actorAudioSource(action.sourceId);
    switch(action.kind) {
        case Kind::Charge:
            audio->play(AudioCue::SentryCharge,action.position,source);
            break;
        case Kind::CancelCharge:
            audio->stopSource(source);
            break;
        case Kind::Fire:
            audio->stopSource(source);
            if(hostileProjectiles->spawn(action.position,action.direction,action.sourceId)) {
                runController->stats().recordHostileShot();
                audio->play(AudioCue::SentryFire,action.position);
                characters->emit(CharacterEffect::SentryFire,action.position,action.direction);
            }
            break;
        case Kind::Windup: {
            audio->play(warningCue(action.type),action.position,source);
            const auto spawned=awaitingFirstThreat.find(action.sourceId);
            if (spawned!=awaitingFirstThreat.end()) {
                logCombat("first_threat",action.type,action.sourceId,runController->stats().snapshot().runTime-spawned->second);
                awaitingFirstThreat.erase(spawned);
            }
            logCombat("windup",action.type,action.sourceId);
            if (isCrystalRoute()) witnessedTypes.insert(action.type);
            break;
        }
        case Kind::Interrupt:
            audio->stopSource(source);
            audio->play(AudioCue::CenserBreak,action.position,source);
            logCombat("interrupt",action.type,action.sourceId);
            if (isCrystalRoute()) {
                ++censerInterrupts;
                if (censerInterrupts==1) pendingScreenshot="censer-interrupted";
            }
            break;
        case Kind::Release:
            audio->stopSource(source);
            logCombat("release",action.type,action.sourceId);
            if (action.type==EnemyType::HangingCenser &&
                hostileProjectiles->spawnVolley(action.position,action.target,action.sourceId)) {
                for (int i=0;i<3;++i) runController->stats().recordHostileShot();
                audio->play(AudioCue::SentryFire,action.position);
            }
            break;
        case Kind::Strike: {
            if (player->isDead()) break;
            const auto damageSource=action.type==EnemyType::ForkHound?DamageSource::HoundDash:DamageSource::RaySweep;
            const float actual=applyDamage(action.damage,damageSource,action.position);
            logCombat("damage",action.type,action.sourceId,actual);
            break;
        }
    }
}

bool App::isCrystalRoute() const {
    return options.acceptance=="crystal-center" || options.acceptance=="crystal-outer" ||
           options.acceptance=="crystal-ads" || options.acceptance=="crystal-horde";
}

// End-to-end deterministic input for the three published routes. All combat,
// aim interpolation, bullet travel, movement, damage and run transitions use
// their ordinary runtime paths. A run proves engineering behavior, not fun.
void App::updateCrystalAcceptance(float dt) {
    if (!arena || currentMap->getArenaLayout().encounterProfile!=EncounterProfile::CrystalSurvival)
        throw std::runtime_error("Crystal acceptance requires the expanded Crystal map");
    const bool observer=options.acceptance=="crystal-horde";
    const bool outer=options.acceptance=="crystal-outer";
    const bool ads=options.acceptance=="crystal-ads";
    const auto& stats=runController->stats().snapshot();
    const auto pos=player->getPosition();
    auto combatSnapshot=[&] {
        std::vector<glm::vec4> state;
        for (const auto& enemy:enemyManager->getEnemies()) {
            state.emplace_back(enemy.getPosition(),enemy.getPhaseProgress());
            state.emplace_back(enemy.getLockedTarget(),static_cast<float>(enemy.getAttackPhase()));
        }
        for (const auto& projectile:hostileProjectiles->projectiles())
            state.emplace_back(projectile.position,projectile.remainingLife);
        for (float warning:director->warningProgress()) state.emplace_back(warning);
        return state;
    };
    if (gameState==GameState::PAUSED) {
        if (stats.runTime!=flowFrozenTime || glm::distance(pos,flowFrozenPosition)>.0001f ||
            crystalPauseSnapshot!=combatSnapshot())
            throw std::runtime_error("Crystal route advanced during pause");
        crystalPauseClock+=dt;
        if (crystalPauseClock>=.25f) {
            setState(GameState::PLAYING);
            restartCurrentRun();
            crystalRestarted=true;
            if (enemyManager->getLiveCount() || hostileProjectiles->size() ||
                !awaitingFirstThreat.empty() || director->combatSeconds()!=0 ||
                characters->activeEffectCount() || stats.damageTaken!=0 || player->getHp()!=100)
                throw std::runtime_error("Crystal route reset retained combat state");
            acceptanceLog<<"{\"event\":\"paused_and_restarted\",\"at\":"<<flowFrozenTime<<"}\n";
        }
        return;
    }
    const size_t live=enemyManager->getLiveCount();
    const unsigned stage=director->pressureStage();
    crystalPeakLive=std::max(crystalPeakLive,live);
    if (stage==4) crystalFinalPeak=std::max(crystalFinalPeak,live);
    // Independent observer captures the late crowd even if every normal input
    // route dies earlier. Only that named mode has artificial health protection.
    if (stats.runTime>=crystalNextSample) {
        unsigned near=0, occupiedSectors=0;
        for (const auto& enemy:enemyManager->getEnemies()) {
            if (enemy.isDead()) continue;
            auto delta=enemy.getPosition()-pos;
            if (glm::dot(glm::vec2(delta.x,delta.z),glm::vec2(delta.x,delta.z))>24.f*24.f) continue;
            ++near;
            const int octant=static_cast<int>((std::atan2(delta.z,delta.x)+3.141593f)*1.2732395f)%8;
            occupiedSectors|=1u<<octant;
        }
        unsigned directions=0;
        for (unsigned mask=occupiedSectors; mask; mask>>=1) directions+=mask&1u;
        acceptanceLog<<"{\"event\":\"pressure_sample\",\"seconds\":"<<stats.runTime
                     <<",\"stage\":"<<stage+1<<",\"observer\":"<<(observer?"true":"false")
                     <<",\"live\":"<<live<<",\"within_24m\":"<<near<<",\"occupied_octants\":"<<directions
                     <<",\"spawned\":"<<enemyManager->getTotalCount()<<",\"hp\":"<<player->getHp()
                     <<",\"damage\":"<<stats.damageTaken<<",\"by_type\":{";
        bool first=true;
        for (auto type:{EnemyType::Chaser,EnemyType::Sentry,EnemyType::ForkHound,EnemyType::VeilRay,EnemyType::HangingCenser}) {
            acceptanceLog<<(first?"":",")<<'\"'<<toString(type)<<"\":"<<enemyManager->count(type);
            first=false;
        }
        acceptanceLog<<"}}\n";
        crystalNextSample+=5.f;
    }
    const bool dead=gameState==GameState::PLAYER_DEAD;
    const float limit=observer?212.f:240.f;
    const bool survivedWindow=gameState==GameState::PLAYING && stats.runTime>=limit && player->getHp()>0;
    if (dead || gameState==GameState::ROOM_CLEARED || stats.runTime>=limit) {
        const bool combatDeath=dead && player->getHp()<=0 && stats.fatalSource!=DamageSource::Unknown;
        const bool passed=observer ? (survivedWindow && crystalRestarted && crystalFinalPeak>=35)
                                   : ((combatDeath || survivedWindow) && stats.shotsFired>0 && stats.distanceTravelled>50);
        acceptanceLog<<"{\"event\":\"crystal_route_end\",\"passed\":"<<(passed?"true":"false")
                     <<",\"observer\":"<<(observer?"true":"false")<<",\"natural_death\":"<<(dead?"true":"false")
                     <<",\"survived_window\":"<<(survivedWindow?"true":"false")
                     <<",\"state\":"<<static_cast<int>(gameState)<<",\"seconds\":"<<stats.runTime
                     <<",\"distance\":"<<stats.distanceTravelled<<",\"threat_types\":"<<witnessedTypes.size()
                     <<",\"outer_seconds\":"<<crystalOuterSeconds<<",\"sprint_seconds\":"<<crystalSprintSeconds
                     <<",\"interrupts\":"<<censerInterrupts<<",\"living\":"<<live
                     <<",\"peak_living\":"<<crystalPeakLive<<",\"peak_overrun\":"<<crystalFinalPeak
                     <<",\"fatal_source\":\""<<toString(stats.fatalSource)<<"\",\"damage\":"<<stats.damageTaken<<"}\n";
        acceptanceFailed=!passed;
        if (observer) pendingScreenshot="horde-observation";
        else if (dead) pendingScreenshot="survival-result";
        else pendingScreenshot=survivedWindow?"survival-window-end":"survival-route-failed";
        glfwSetWindowShouldClose(window,GL_TRUE);
        return;
    }
    if (gameState!=GameState::PLAYING) return;
    if (observer && !crystalPaused && stage>=1) {
        crystalPaused=true; flowFrozenTime=stats.runTime; flowFrozenPosition=pos;
        crystalPauseSnapshot=combatSnapshot();
        setState(GameState::PAUSED);
        pendingScreenshot="pause-live-attack";
        return;
    }

    if (glm::length(glm::vec2(pos.x,pos.z))>=72.f) crystalOuterSeconds+=dt;
    if (player->isSprinting()) crystalSprintSeconds+=dt;
    const float radius=outer?85.f:(ads?36.f:22.f);
    const float angle=std::atan2(pos.x,pos.z)+.18f;
    const glm::vec3 waypoint{radius*std::sin(angle),pos.y,radius*std::cos(angle)};
    glm::vec3 travel=glm::normalize(waypoint-pos);
    const Enemy* target=nullptr;
    float priority=-1e9f;
    const glm::vec3 eye=camera->getPosition();
    for (const auto& e:enemyManager->getEnemies()) {
        const auto center=e.getAABB().getCenter();
        if (e.isDead()) continue;
        float score=-glm::distance(eye,center);
        if (ads && e.getType()==EnemyType::HangingCenser && e.getAttackPhase()==AttackPhase::Windup) score+=130;

        if (score>priority && visible(eye,center)) { priority=score;target=&e; }
    }
    aimHeld=false;
    if (target) {
        glm::vec3 aimPoint=target->getAABB().getCenter();
        const bool gland=ads && target->getType()==EnemyType::HangingCenser &&
                         target->getAttackPhase()==AttackPhase::Windup;
        if (gland) {
            aimHeld=true;
            const auto mouth=crystalCombat::CenserMouthOffset;
            aimPoint=target->getPosition()+glm::vec3(0,mouth.y,0)+target->getHeading()*mouth.z;
        }
        camera->setDirection(aimPoint-eye);
        if (gland && aim.blend()>.85f && target->getInterruptHits()>0) {
            const std::string name="censer-ads-hit-"+std::to_string(target->getInterruptHits());
            if (crystalPoseCaptures.insert(name).second) pendingScreenshot=name;
        }
        // The measured routes keep firing at visible enemies, including during
        // their windup. Camerawork for the gallery is exclusive to the observer.
        firePrimary();
        if (observer && crystalRestarted && pendingScreenshot.empty()) {
            if ((target->getType()==EnemyType::Chaser || target->getType()==EnemyType::Sentry) &&
                glm::distance(eye,aimPoint)<24.f) {
                std::string name=std::string(toString(target->getType()))+"-encounter";
                if (crystalPoseCaptures.insert(name).second) pendingScreenshot=name;
            }
            const int seconds=static_cast<int>(stats.runTime);
            if (seconds==49 || seconds==97 || seconds==142 || seconds==187 || seconds==205) {
                std::string name="horde-"+std::to_string(seconds);
                if (crystalPoseCaptures.insert(name).second) {
                    // Look into the densest sector for one captured frame.
                    glm::vec3 best=travel;
                    float bestScore=-1.f;
                    for (int sector=0; sector<16; ++sector) {
                        const float yaw=sector*.3926991f;
                        const glm::vec3 heading{std::cos(yaw),0,std::sin(yaw)};
                        float score=0;
                        for (const auto& e:enemyManager->getEnemies()) {
                            auto to=e.getAABB().getCenter()-eye;
                            const float range=glm::length(to);
                            if (!e.isDead() && range>2.f && range<55.f)
                                score+=std::max(0.f,glm::dot(heading,to/range)-.82f);
                        }
                        if (score>bestScore) { bestScore=score;best=heading; }
                    }
                    camera->setDirection(best);
                    pendingScreenshot=name;
                }
            }
        }
    } else camera->setDirection(travel);

    PlayerInput keys;
    keys.sprint=outer;
    glm::vec3 front=camera->getFront(); front.y=0;
    if (glm::length(front)>.001f) {
        front=glm::normalize(front);
        const auto right=glm::cross(front,glm::vec3(0,1,0));
        const float f=glm::dot(front,travel),r=glm::dot(right,travel);
        keys.forward=f>.35f;keys.backward=f<-.35f;keys.right=r>.35f;keys.left=r<-.35f;
    }
    player->processInput(keys,camera.get(),dt);
}
} // namespace silic2
