#include "enemy/enemy.h"
#include "enemy/crystal_combat.h"
#include "enemy/enemy_geometry.h"
#include "game/arena_definition.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

namespace silic2 {
namespace {
using V=glm::vec3;
V flat(V v) { v.y=0; return v; }
V unit(V v,V fallback={0,0,1}) {
    const float length=glm::length(v);
    return length>.0001f ? v/length : fallback;
}
V side(V forward) { return {forward.z,0,-forward.x}; }
// Both locomotion and planted windup turn the physical body. The same heading
// reaches the model, directional hit volumes and the eventual dash.
V turnHound(V current,V target,float maxTurn,float flank) {
    const float across=glm::dot(side(current),target), along=glm::dot(current,target);
    float angle=std::atan2(across,along);
    if(along<0 && std::abs(across)<.000001f) angle=flank*glm::pi<float>();
    if(std::abs(angle)<=maxTurn) return target;
    angle=std::copysign(maxTurn,angle);
    return unit(current*std::cos(angle)+side(current)*std::sin(angle),current);
}
constexpr V up(float amount) { return {0,amount,0}; }
V rotateZ(V point,V pivot,float angle) {
    const V p=point-pivot;const float c=std::cos(angle),s=std::sin(angle);
    return pivot+V(c*p.x-s*p.y,s*p.x+c*p.y,p.z);
}
float eased(float t) {t=std::clamp(t,0.f,1.f);return t*t*(3-2*t);}
bool activePhase(AttackPhase p) { return p==AttackPhase::Windup || p==AttackPhase::Attack; }
float floorAt(const std::shared_ptr<const ArenaDefinition>& arena) { return arena ? arena->floorY : 0; }
// Flying actors stay over the authored circular fighting floor, including where its
// low rim doesn't physically extend up to their cruising height.
glm::vec2 flightExtent(const ArenaDefinition& arena,float margin,bool aboveRim=false) {
    auto half=(arena.maxXZ-arena.minXZ)*.5f;
    if(!aboveRim && arena.encounterProfile==EncounterProfile::CrystalSurvival)
        half=glm::min(half,glm::vec2(crystalCombat::CrystalFlightRadius));
    return glm::max(half-glm::vec2(margin),glm::vec2(1));
}
bool insideFlightFootprint(V target,const ArenaDefinition* arena,float margin,bool aboveRim=false) {
    if(!arena) return true;
    const glm::vec2 center=(arena->maxXZ+arena->minXZ)*.5f;
    const glm::vec2 extent=flightExtent(*arena,margin,aboveRim);
    const glm::vec2 p=(glm::vec2(target.x,target.z)-center)/extent;
    return glm::dot(p,p)<1;
}
V flightLimit(V target,const ArenaDefinition* arena,float margin) {
    if(!arena) return target;
    const glm::vec2 center=(arena->maxXZ+arena->minXZ)*.5f;
    const glm::vec2 extent=flightExtent(*arena,margin+.1f);
    glm::vec2 p=(glm::vec2(target.x,target.z)-center)/extent;
    if(glm::length(p)>1) {
        p=center+extent*glm::normalize(p);
        target.x=p.x; target.z=p.y;
    }
    return target;
}

bool fitsFlyingBody(V root,V forward,EnemyType type,const ArenaDefinition* arena) {
    using namespace crystalCombat;
    if(type!=EnemyType::VeilRay) return insideFlightFootprint(root,arena,CenserSize.x*.5f);
    const V wing=side(forward)*(RaySize.x*.5f);
    const V nose=forward*(RaySize.z*.5f);
    return insideFlightFootprint(root,arena,0) &&
           insideFlightFootprint(root-wing,arena,.12f,true) && insideFlightFootprint(root+wing,arena,.12f,true) &&
           insideFlightFootprint(root-nose,arena,.12f,true) && insideFlightFootprint(root+nose,arena,.12f,true);
}

bool clearFlyingBody(V start,V end,V forward,EnemyType type,const Map* map) {
    using namespace crystalCombat;
    if(!map) return true;
    const float scale=bodyScale(type);
    const V right=side(forward);
    auto sweep=[&](V local,float radius) {
        V offset=(local.x*right+up(local.y)+local.z*forward)*scale;
        return enemyGeometry::clear(start+offset,end+offset,map,radius*scale);
    };
    if(type==EnemyType::VeilRay) {
        // Most cruise routes have ample space. Near ground and landmarks, sweep
        // the thin wings front-to-back at their actual spans and thicknesses.
        // A solid crossing the spine's center also blocks its finite-radius sweep.
        if(glm::dot(end-start,end-start)>36.f &&
           !enemyGeometry::clear(start+up(.33f*scale),end+up(.33f*scale),map)) return false;
        if(enemyGeometry::clear(start+up(RaySize.y*.5f),end+up(RaySize.y*.5f),map,RaySize.x*.52f)) return true;
        for(float sign:{-1.f,1.f}) {
            if(!sweep({sign*.44f,.29f,-.06f},.24f) || !sweep({sign*.91f,.27f,-.12f},.24f) ||
               !sweep({sign*1.35f,.22f,-.22f},.21f) || !sweep({sign*1.63f,.16f,-.34f},.13f)) return false;
        }
        return sweep({0,.33f,0},.3f) && sweep({0,.29f,.60f},.24f) && sweep({0,.28f,-.67f},.23f);
    }
    // Sphere sweeps follow the tapered shell and tail. The low and high tips
    // keep a small world-space skin so the tail can lift off a floor or perch.
    if(!sweep({0,1.68f,-.02f},.78f) || !sweep({0,1.01f,-.18f},.52f) ||
       !sweep({0,.42f,-.16f},.18f)) return false;
    for(float height:{.16f,CenserSize.y-.16f}) {
        const V offset=up(height)-forward*(.16f*scale);
        if(!enemyGeometry::clear(start+offset,end+offset,map,.12f)) return false;
    }
    return true;
}
// Exact segment/ellipsoid in the creature's local frame. The first visible
// surface owns the hit, so the far side of a shell cannot expose a weakpoint.
bool ellipsoid(V from,V to,V center,V radii,float& t) {
    const V a=(from-center)/radii, d=(to-from)/radii;
    const float dd=glm::dot(d,d), ad=glm::dot(a,d), c=glm::dot(a,a)-1;
    if(c<=0) { t=0; return true; }
    const float disc=ad*ad-dd*c;
    if(dd<1e-12f || disc<0) return false;
    t=(-ad-std::sqrt(disc))/dd;
    return t>=0 && t<=1;
}
}

glm::vec3 Enemy::getDimensions() const {
    switch(type) {
        case EnemyType::ForkHound:return crystalCombat::HoundSize;
        case EnemyType::VeilRay:return crystalCombat::RaySize;
        case EnemyType::HangingCenser:return crystalCombat::CenserSize;
        case EnemyType::Sentry:return {1.2f,1,1.2f};
        default:return {.6f,1.8f,.6f};
    }
}

bool Enemy::intersectShot(V start,V end,float& fraction,bool& gland) const {
    gland=false;
    if(!enemyGeometry::segment(start,end,getAABB(),fraction)) return false;
    if(!crystalCombat::isCrystalEnemy(type)) return true;
    const V forward=unit(flat(heading)), right=side(forward);
    auto local=[&](V p) {
        p-=position;
        return V(glm::dot(p,right),p.y,glm::dot(p,forward))/crystalCombat::bodyScale(type);
    };
    const V a=local(start),b=local(end);
    fraction=2;
    auto hitRegion=[&](V from,V to,V c,V r,bool weak=false) {
        float time;
        if(ellipsoid(from,to,c,r,time) && time<fraction) { fraction=time; gland=weak; }
    };
    auto surface=[&](V c,V r,bool weak=false) { hitRegion(a,b,c,r,weak); };
    switch(type) {
        case EnemyType::ForkHound:
            surface({0,.73f,0},{.52f,.49f,1.16f});
            surface({0,.90f,1.00f},{.34f,.32f,.40f});
            surface({0,1.02f,.38f},{.33f,.29f,.49f});
            for(float sign:{-1.f,1.f}) {
                surface({.36f*sign,.48f,.50f},{.185f,.48f,.30f});
                surface({.32f*sign,.34f,-.80f},{.16f,.34f,.27f});
            }
            break;
        case EnemyType::VeilRay: {
            const float p=getPhaseProgress();
            const float bank=phase==AttackPhase::Recovery ? .37f*std::sin(p*3.14159265f) : 0;
            const float fold=phase==AttackPhase::Windup ? .40f*eased(p*4) :
                             phase==AttackPhase::Attack ? .38f*(1-p) : 0;
            const V baseA=rotateZ(a,{0,.31f,0},-bank),baseB=rotateZ(b,{0,.31f,0},-bank);
            surface({0,.32f,0},{.29f,.32f,.95f});
            for(float sign:{-1.f,1.f}) {
                V pivot={sign*.13f,.36f,.05f};
                V a1=rotateZ(baseA,pivot,-sign*fold),b1=rotateZ(baseB,pivot,-sign*fold);
                hitRegion(a1,b1,{sign*.55f,.29f,-.08f},{.55f,.19f,.51f});
                pivot={sign*.85f,.25f,-.10f};
                V a2=rotateZ(a1,pivot,-sign*fold*.24f),b2=rotateZ(b1,pivot,-sign*fold*.24f);
                hitRegion(a2,b2,{sign*1.08f,.24f,-.16f},{.45f,.19f,.34f});
                hitRegion(a2,b2,{sign*1.49f,.18f,-.29f},{.27f,.17f,.16f});
            }
            break;
        }
        case EnemyType::HangingCenser: {
            surface({0,1.71f,-.02f},{.78f,.69f,.76f});
            surface({0,1.02f,-.25f},{.60f,.57f,.44f});
            surface({0,.46f,-.25f},{.18f,.46f,.18f});
            for(float sign:{-1.f,1.f}) surface({.5f*sign,1.07f,.33f},{.26f,.48f,.35f});
            const V mouth=crystalCombat::CenserMouth,normal=unit({0,-.4f,.92f});
            const V lipUp=glm::cross(normal,V(1,0,0));
            auto apertureSpace=[&](V v){v-=mouth;return V(v.x,glm::dot(v,lipUp),glm::dot(v,normal));};
            V u=apertureSpace(a),v=apertureSpace(b);
            if(phase==AttackPhase::Windup) {
                // Three hinged petals fold back outside the sac. A grazing
                // shot meets their thick roots before reaching the sac.
                for(int i=0;i<3;++i) {
                    const float angle=1.5707963f+i*2.0943951f;
                    const V radial(std::cos(angle),std::sin(angle),0);
                    const V tangent(-radial.y,radial.x,0);
                    auto petal=[&](V x){return V(glm::dot(x,tangent),glm::dot(x,radial),x.z);};
                    hitRegion(petal(u),petal(v),{0,.45f,-.09f},{.22f,.17f,.19f});
                }
                hitRegion(u,v,{0,0,0},{.30f,.29f,.14f},glm::dot(unit(a-b),normal)>.18f);
            } else hitRegion(u,v,{0,0,.07f},{.4f,.4f,.18f});
            break;
        }
        default:break;
    }
    return fraction<=1;
}

void Enemy::receiveShot(int amount,bool gland) {
    if(amount<=0 || isDead()) return;
    takeDamage(amount);
    if(!isDead() && gland && type==EnemyType::HangingCenser && phase==AttackPhase::Windup) {
        if(++glandHits==crystalCombat::GlandHitLimit) {
            interruptHeight=position.y;
            setPhase(AttackPhase::Interrupted,crystalCombat::InterruptTime);
            reportInterrupt=true;
        }
    }
}
void Enemy::setPhase(AttackPhase next,float duration) {
    phase=next;phaseTime=0;phaseLength=duration;
}
float Enemy::getPhaseProgress() const {
    return phaseLength>0 ? std::clamp(phaseTime/phaseLength,0.f,1.f) : 0;
}
void Enemy::crystalEvent(EnemyAction::Kind kind,float damage) {
    const auto mouth=crystalCombat::CenserMouthOffset;
    const V socket=type==EnemyType::HangingCenser ? position+up(mouth.y)+heading*mouth.z : getAABB().getCenter();
    pendingActions.push_back({kind,id,socket,heading,type,lockedTarget,damage});
}

bool Enemy::flyToward(V target,float distance,const Map* map) {
    const float footprint=getDimensions().x*.5f;
    if(!fitsFlyingBody(target,heading,type,arenaDefinition.get()))
        target=flightLimit(target,arenaDefinition.get(),footprint);
    const V delta=target-position;
    const float length=glm::length(delta);
    if(length<.001f) return true;
    const V desired=delta*(std::min(length,distance)/length);
    const bool steering=type==EnemyType::VeilRay && !activePhase(phase);
    auto tryMove=[&](V motion,V facing) {
        if(!fitsFlyingBody(position+motion,facing,type,arenaDefinition.get()) ||
           !clearFlyingBody(position,position+motion,facing,type,map)) return false;
        position+=motion;
        heading=facing;
        return true;
    };
    const V plannedHeading=steering ? unit(flat(desired),heading) : heading;
    if(tryMove(desired,plannedHeading)) return true;
    // A wide wing cannot rotate through the crystal to face its new route.
    // Slide away in the last clear orientation until the turn fits.
    if(steering && glm::dot(plannedHeading,heading)<.999f && tryMove(desired,heading)) return true;
    if(activePhase(phase) || phase==AttackPhase::Interrupted) return false;
    // Follow the obstacle edge on a stable side until the desired heading clears.
    for(float sign:{(id&1)?1.f:-1.f,(id&1)?-1.f:1.f}) {
        const V turn=side(unit(flat(desired),heading))*sign;
        const V tangent=turn*glm::length(desired);
        if(tryMove(tangent,steering ? turn : heading) || (steering && tryMove(tangent,heading))) return false;
    }
    return false;
}

void Enemy::driftCenser(float dt,V player,const Map* map) {
    using namespace crystalCombat;
    const V toPlayer=flat(player-position);
    const float distance=glm::length(toPlayer);
    heading=unit(toPlayer,heading);
    V velocity=side(heading)*((id%2 ? 1.f : -1.f)*CenserOrbitSpeed);
    if(distance>CenserMaxRange) velocity=heading*CenserCruise;
    else if(distance<CenserMinRange) velocity=-heading*CenserDrift;
    driftTime+=dt;
    const float bob=CenserBobHeight*std::sin(driftTime*CenserBobRate+float(id%8)*.785398f);
    const float lift=std::clamp(flightAltitude+bob-position.y,-CenserLiftSpeed*dt,CenserLiftSpeed*dt);
    const V step=velocity*dt+up(lift);
    flyToward(position+step,glm::length(step),map);
}

void Enemy::moveHound(V target,float dt,const Map* map) {
    using namespace crystalCombat;
    const float radius=.56f;
    if(arenaDefinition && !enemyGeometry::clear(position+up(.8f),target+up(.8f),map,radius)) {
        navigationRefresh-=dt;
        if(navigationRefresh<=0 || glm::length(flat(navigationTarget-position))<.4f) {
            navigationTarget=arenaDefinition->nextWaypoint(position,target,radius);
            navigationRefresh=.2f;
        }
        target=navigationTarget;
    }
    V delta=flat(target-position);
    if(glm::length(delta)<.01f) return;
    const V course=unit(delta,heading);
    heading=turnHound(heading,course,glm::radians(330.f)*dt,id%2 ? 1.f : -1.f);
    // Plant the feet to pivot instead of advancing away from a close waypoint.
    const float speed=HoundApproach*std::max(0.f,glm::dot(heading,course));
    const V desired=heading*std::min(speed*dt,glm::length(delta));
    if(map && map->getSpatialWorld()) position=map->getSpatialWorld()->moveActor(position,desired,radius,getDimensions().y,true).position;
    else position+=resolveHorizontalCollision(desired,map);
}

V Enemy::findHoundEntry(V player,V forward,float flank,const Map* map) const {
    using namespace crystalCombat;
    const V away=-forward, tangent=side(forward)*flank;
    float best=1.e9f;
    const float orbitRadius=(HoundMinRange+HoundMaxRange)*.5f;
    V target=player+tangent*orbitRadius;
    for(V radial:{away,tangent,unit(away+tangent),unit(tangent-away),-tangent,
                  unit(away-tangent),unit(-tangent-away),-away}) {
        V entry=player+radial*orbitRadius;
        entry.y=position.y;
        if(arenaDefinition && !arenaDefinition->isWalkable(entry,.56f)) continue;
        // The entry must provide room on both sides of the player, including
        // the overshoot. At the rim this naturally picks a tangential run.
        if(!enemyGeometry::clear(entry+up(.8f),entry+up(.8f)-radial*DashLength,map,.56f)) continue;
        float cost=glm::length(flat(entry-position));
        if(!enemyGeometry::clear(position+up(.8f),entry+up(.8f),map,.56f)) cost+=20.f;
        if(cost<best) { best=cost;target=entry; }
    }
    return target;
}

void Enemy::approachCrystal(float dt,V player,const Map* map) {
    using namespace crystalCombat;
    approachTime+=dt;
    const V toPlayer=flat(player-position);
    const float distance=glm::length(toPlayer),flank=(id%2 ? 1.f : -1.f);
    const V forward=unit(toPlayer,heading), lateral=side(forward)*flank;
    const float ground=floorAt(arenaDefinition);
    if(type==EnemyType::ForkHound) {
        const bool inRange=distance>=HoundMinRange && distance<=HoundMaxRange;
        const bool clearLane=distance<=HoundMaxRange &&
            enemyGeometry::clear(position+up(.8f),position+up(.8f)+forward*DashLength,map,.56f);
        if(attackPermit && approachTime>=.30f && inRange && clearLane) {
            hasFlankTarget=false;
            lockedDirection=heading;
            lockedTarget=player;
            setPhase(AttackPhase::Windup,HoundWindup);
            crystalEvent(EnemyAction::Kind::Windup);
        } else {
            // Maintain one continuous flanking course on either side of the
            // attack range. The minimum dash distance only gates the attack.
            const float orbitRadius=(HoundMinRange+HoundMaxRange)*.5f;
            V target=position+forward*(distance-orbitRadius)+lateral*3.5f+flat(playerVelocity)*.18f;
            if(distance<=HoundMaxRange) {
                const float targetRange=glm::length(flat(houndFlankTarget-player));
                if(targetRange<HoundMinRange || targetRange>HoundMaxRange ||
                   glm::length(flat(houndFlankTarget-position))<.35f) hasFlankTarget=false;
                // A dash past a rim player can end with clear sight inward
                // while the outward flanking course itself leaves the floor.
                if(!hasFlankTarget && (!clearLane || (arenaDefinition && !arenaDefinition->isWalkable(target,.56f)))) {
                    houndFlankTarget=findHoundEntry(player,forward,flank,map);
                    hasFlankTarget=true;
                }
                if(hasFlankTarget) target=houndFlankTarget;
            } else hasFlankTarget=false;
            moveHound(target,dt,map);
        }
        return;
    }
    flightAltitude=ground+(type==EnemyType::VeilRay ? RayAltitude : CenserAltitude);
    const float takeoffTolerance=type==EnemyType::HangingCenser ? CenserBobHeight+.04f : .12f;
    if(position.y<flightAltitude-takeoffTolerance) {
        flyToward({position.x,flightAltitude,position.z},dt*FlightRise,map);
        return;
    }
    if(type==EnemyType::HangingCenser) {
        heading=forward;
        if(attackPermit && distance>=CenserMinRange && distance<=CenserMaxRange &&
           enemyGeometry::clear(position+up(CenserMouthOffset.y)+heading*CenserMouthOffset.z,player+up(.85f),map,.12f)) {
            lockedTarget=player;lockedTarget.y=ground+.04f;
            glandHits=0;setPhase(AttackPhase::Windup,CenserWindup);
            crystalEvent(EnemyAction::Kind::Windup);
            return;
        }
        driftCenser(dt,player,map);
        return;
    }
    auto stageRay=[&] {
        const V direction=unit(sweepEnd-sweepStart,heading);
        const V entry=flat(sweepStart)+up(flightAltitude);
        if(hasRayLane && attackPermit && glm::distance(position,entry)<RayStagingDistance &&
           clearFlyingBody(position,entry,direction,type,map)) {
            heading=direction;
            lockedTarget=(sweepStart+sweepEnd)*.5f;
            setPhase(AttackPhase::Windup,RayWindup);
            crystalEvent(EnemyAction::Kind::Windup);
            rayReplanTime=0;
        } else {
            flyToward(navigationTarget,dt*RayCruise,map);
        }
    };
    rayReplanTime-=dt;
    if(rayReplanTime>0) {
        stageRay();
        return;
    }
    rayReplanTime=.15f;
    // Stage the run on the player's near side. A moving player crosses the
    // strip rather than spending its whole length just outrunning the ray.
    V crossing=forward;
    if(glm::length(playerVelocity)>2.f) {
        crossing=side(unit(flat(playerVelocity)));
        if(glm::dot(crossing,forward)<0) crossing=-crossing;
    }
    V middle=flat(player+playerVelocity*RayLockLead)+up(ground+.04f);
    middle=flightLimit(middle,arenaDefinition.get(),SweepWidth*.5f);
    // From this far away every possible entry is still outside windup range.
    // Cruise in short, body-checked legs until an entire attack can be staged.
    constexpr float stagingRadius=SweepLength+RayStagingDistance+9.f;
    if(glm::length(flat(middle-position))>stagingRadius) {
        hasRayLane=false;
        navigationTarget=position+unit(flat(middle-position),heading)*8.f;
        stageRay();
        return;
    }
    const V tangent=arenaDefinition ? side(unit(flat(middle)-V(
        (arenaDefinition->maxXZ.x+arenaDefinition->minXZ.x)*.5f,0,
        (arenaDefinition->maxXZ.y+arenaDefinition->minXZ.y)*.5f))) : side(crossing);
    // Try alternate strips around the crystal and rim before committing. Only
    // a checked entrance and exit can be advertised to the player.
    float best=1.e9f;
    V staging=flat(middle-crossing*(SweepLength*.5f))+up(flightAltitude);
    V bestBegin{},bestEnd{},bestDirection{};
    const V diagonal=unit(crossing+side(crossing));
    const V directions[]={crossing,-crossing,tangent,-tangent,side(crossing),-side(crossing),
                          V(1,0,0),V(-1,0,0),V(0,0,1),V(0,0,-1),diagonal,-diagonal,side(diagonal),-side(diagonal)};
    // Near a spire or rim the player may be at one end of the strip. Moving
    // the entrance and exit together preserves the full seven-section attack.
    auto consider=[&](V dir,float crossingFraction,float lateralOffset) {
            const V begin=middle+side(dir)*lateralOffset-dir*(SweepLength*crossingFraction), end=begin+dir*SweepLength;
            const V entry=flat(begin)+up(flightAltitude), exit=flat(end+dir*3.f)+up(flightAltitude);
            if(!fitsFlyingBody(entry,dir,type,arenaDefinition.get()) || !fitsFlyingBody(exit,dir,type,arenaDefinition.get())) return;
            float cost=glm::length(position-entry);
            if(cost>=best || !enemyGeometry::clear(begin+up(1.3f),end+up(1.3f),map,SweepWidth*.5f) ||
               !clearFlyingBody(entry,exit,dir,type,map)) return;
            if(!clearFlyingBody(position,entry,dir,type,map)) cost+=100;
            if(cost<best) {
                best=cost;staging=entry;bestBegin=begin;bestEnd=end;bestDirection=dir;
            }
    };
    for(float crossingFraction:{.5f,0.f,1.f}) {
        for(int i=0;i<14;++i) {
            if(i==6 && best<1.e9f) break;
            consider(directions[i],crossingFraction,0);
            if(best<RayStagingDistance) break;
        }
        if(best<1.e9f) break;
    }
    if(best==1.e9f) {
        // Against a wall the predicted player can lie along the side of the
        // warning strip. The wing and ground checks still cover the full run.
        for(int i=0;i<16;++i) {
            const float angle=i*.39269908f;
            const V dir(std::cos(angle),0,std::sin(angle));
            for(float offset:{SweepWidth*.475f,-SweepWidth*.475f})
                for(float fraction:{.5f,0.f,1.f}) consider(dir,fraction,offset);
            if(best<RayStagingDistance) break;
        }
    }
    hasRayLane=best<1.e9f;
    if(hasRayLane) {
        sweepStart=bestBegin;sweepEnd=bestEnd;
        if(!clearFlyingBody(position,staging,bestDirection,type,map)) {
            const V entry=staging;
            float shortest=1.e9f;
            // The wings must clear the last turn onto the strip as well. Look
            // for a visible intermediate point on the open side of the entry.
            for(float radius:{7.f,13.f}) for(int i=0;i<8;++i) {
                const float angle=i*.78539816f;
                const V via=entry+V(std::cos(angle),0,std::sin(angle))*radius;
                const V approach=unit(flat(via-position),heading);
                const float cost=glm::distance(position,via)+radius;
                if(cost<shortest && fitsFlyingBody(via,approach,type,arenaDefinition.get()) &&
                   clearFlyingBody(position,via,approach,type,map) &&
                   clearFlyingBody(via,entry,bestDirection,type,map)) {
                    staging=via;shortest=cost;
                }
            }
        }
    }
    navigationTarget=staging;
    stageRay();
}

void Enemy::updateWindup(float dt,V player,const Map* map) {
    using namespace crystalCombat;
    if(type==EnemyType::ForkHound && phaseTime<HoundTracking) {
        const float trackingStep=std::min(dt,HoundTracking-phaseTime);
        heading=turnHound(heading,unit(flat(player-position),heading),glm::radians(640.f)*trackingStep,id%2 ? 1.f : -1.f);
        lockedDirection=heading;
        lockedTarget=player;
    } else if(type==EnemyType::VeilRay) {
        flyToward(flat(sweepStart)+up(flightAltitude),dt*RayCruise,map);
    }
    phaseTime+=dt;
    if(phaseTime+.00001f<phaseLength) return;
    damageDelivered=false; nextSegment=0;
    if(type==EnemyType::HangingCenser) {
        crystalEvent(EnemyAction::Kind::Release);
        setPhase(AttackPhase::Recovery,CenserRecovery);
    } else {
        setPhase(AttackPhase::Attack,type==EnemyType::ForkHound ? DashLength/DashSpeed : SweepTime);
        crystalEvent(EnemyAction::Kind::Release);
    }
}
void Enemy::updateStrike(float dt,const Map* map) {
    using namespace crystalCombat;
    const V center=playerBounds.getCenter();
    bool blocked=false;
    if(type==EnemyType::ForkHound) {
        const V start=position, displacement=lockedDirection*(DashSpeed*dt);
        if(map && map->getSpatialWorld()) {
            auto hit=map->getSpatialWorld()->raycast(position+up(.75f),position+up(.75f)+displacement+lockedDirection*1.15f,.54f);
            blocked=hit.hit;
        }
        if(!blocked) {
            const V step=map && map->getSpatialWorld()
                ? map->getSpatialWorld()->moveActor(position,displacement,.55f,1.35f,true).position-position
                : resolveHorizontalCollision(displacement,map);
            position+=step;
            blocked=glm::dot(step,lockedDirection)<glm::length(displacement)-.015f;
        }
        const V body=up(.70f);
        AABB expanded(playerBounds.min-V(.45f,.48f,.45f),playerBounds.max+V(.45f,.48f,.45f));
        float t;
        if(!damageDelivered && enemyGeometry::segment(start+body,position+body+lockedDirection*.95f,expanded,t)
           && enemyGeometry::clear(position+body,center,map)) {
            damageDelivered=true;crystalEvent(EnemyAction::Kind::Strike,DashDamage);
        }
    } else {
        const float progress=std::min(1.f,(phaseTime+dt)/SweepTime);
        const V floorPosition=glm::mix(sweepStart,sweepEnd,progress);
        position=flat(floorPosition)+up(flightAltitude);
        while(nextSegment<SweepSegments && (nextSegment+.5f)/SweepSegments<=progress+.00001f) {
            const V direction=unit(sweepEnd-sweepStart,heading);
            const V section=sweepStart+direction*((nextSegment+.5f)*SweepLength/SweepSegments);
            const V d=flat(center-section);
            const V half=playerBounds.getSize()*.5f;
            auto projection=[&](V axis){return std::abs(axis.x)*half.x+std::abs(axis.z)*half.z;};
            if(!damageDelivered && playerBounds.min.y<section.y+1.35f &&
               std::abs(glm::dot(d,direction))<=SweepLength/(2*SweepSegments)+projection(direction) &&
               std::abs(glm::dot(d,side(direction)))<=SweepWidth*.5f+projection(side(direction)) &&
               enemyGeometry::clear(section+up(1),center,map)) {
                damageDelivered=true;crystalEvent(EnemyAction::Kind::Strike,SweepDamage);
            }
            ++nextSegment;
        }
    }
    phaseTime+=dt;
    if(blocked || phaseTime+.00001f>=phaseLength)
        setPhase(AttackPhase::Recovery,type==EnemyType::ForkHound ? DashRecovery : RayRecovery);
}
void Enemy::advanceCrystal(float dt,V player,const Map* map) {
    using namespace crystalCombat;
    if(phase==AttackPhase::Approach) { approachCrystal(dt,player,map);return; }
    if(phase==AttackPhase::Windup) { updateWindup(dt,player,map);return; }
    if(phase==AttackPhase::Attack) { updateStrike(dt,map);return; }
    if(phase==AttackPhase::Interrupted) {
        V lowered=position;lowered.y=interruptHeight-CenserDropDistance;
        flyToward(lowered,CenserDropSpeed*dt,map);
    }
    if(phase==AttackPhase::Recovery && type==EnemyType::HangingCenser) driftCenser(dt,player,map);
    if(phase==AttackPhase::Recovery && type==EnemyType::VeilRay) {
        const float sign=id%2 ? 1.f : -1.f;
        const V turn=unit(heading+side(heading)*dt*sign,heading);
        flyToward(position+turn*RayBankSpeed,dt*RayBankSpeed,map);
    }
    phaseTime+=dt;
    if(phaseTime+.00001f>=phaseLength) {
        if(phase==AttackPhase::Interrupted) setPhase(AttackPhase::Recovery,CenserRecovery);
        else { approachTime=0;glandHits=0;setPhase(AttackPhase::Approach); }
    }
}
void Enemy::updateCrystal(float dt,V player,const Map* map) {
    if(!hasContext) playerBounds={player+V(-.30f,0,-.30f),player+V(.30f,1.8f,.30f)};
    if(reportInterrupt) { crystalEvent(EnemyAction::Kind::Interrupt);reportInterrupt=false; }
    // Use small continuous motion steps, splitting exactly at short attack
    // boundaries. A late frame must not stretch a dash or skip a needle.
    for(float left=dt;left>.000001f;) {
        float step=std::min(left,1.f/60);
        if(phaseLength>0) step=std::min(step,std::max(.00001f,phaseLength-phaseTime));
        const AttackPhase before=phase;
        advanceCrystal(step,player,map);
        if(before==AttackPhase::Approach && phase==AttackPhase::Windup) attackPermit=false;
        left-=step;
    }
    if(!hasContext) attackPermit=true;
}
}
