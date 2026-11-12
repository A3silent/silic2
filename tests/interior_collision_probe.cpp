// CPU acceptance of the actual imported cover/proxy pairs. No generated fixtures
// are installed, and no gameplay tuning or test-only movement path is involved.
#include "engine/map.h"
#include "engine/camera.h"
#include "player/player.h"
#include "enemy/enemy.h"
#include "enemy/enemy_geometry.h"
#include "enemy/enemy_projectile_system.h"
#include "game/arena_definition.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
using namespace silic2;
namespace {
using J=SimpleJson;
J vector(glm::vec3 v){J a=J::array();for(int i=0;i<3;++i)a.push_back(J(double(v[i])));return a;}
struct RayPair{glm::vec3 start,end;};
J rayJson(const RayPair& ray){J j=J::object();j["start"]=vector(ray.start);j["end"]=vector(ray.end);return j;}
bool projectileReaches(const Map& map,const RayPair& ray) {
    EnemyProjectileSystem system;system.spawn(ray.start,ray.end-ray.start,1);
    const auto target=ray.end;
    const AABB player(target-glm::vec3(.16f),target+glm::vec3(.16f));
    bool hit=false;const int count=int(std::ceil(glm::length(ray.end-ray.start)/10.f/.01f))+50;
    for(int i=0;i<count;++i)hit|=!system.update(.01f,&map,player).empty();
    return hit;
}
}
int main(int argc,char** argv) {
    const std::string mapPath=argc>1?argv[1]:"res/maps/starry_crystal_arena.json";
    const std::string out=argc>2?argv[2]:"artifacts/acceptance/tripo-interior/cpu";
    bool baseline=false,skipTop=false;for(int i=3;i<argc;++i) {baseline|=std::string(argv[i])=="--baseline";skipTop|=std::string(argv[i])=="--skip-top";}
    std::filesystem::create_directories(out);
    J report=J::object();int failures=0;
    auto check=[&](const std::string& name,bool ok){std::cout<<"CHECK "<<name<<' '<<(ok?"PASS":"FAIL")<<std::endl;J c=J::object();c["check"]=J(name);c["passed"]=J(ok);report["checks"].push_back(c);if(!ok)++failures;};
    try {
        report["top_pursuit_skipped"]=J(skipTop);report["checks"]=J::array();report["covers"]=J::array();report["diagnostics"]=J::array();
        Map map;if(!map.loadFromFile(mapPath))throw std::runtime_error(map.getLastError());
        const auto world=map.getSpatialWorld();if(!world)throw std::runtime_error("requires triangle spatial world");
        for(const auto& diagnostic:map.getSpatialDiagnostics()){std::cout<<"DIAGNOSTIC "<<diagnostic<<'\n';report["diagnostics"].push_back(J(diagnostic));}
        check("map_diagnostics_empty",map.getSpatialDiagnostics().empty());check("navigation_connected",world->navigationConnected());
        report["collision_triangles"]=J(double(world->triangles().size()));report["nav_nodes"]=J(double(map.getSpatialSettings().nodes.size()));
        auto definition=std::make_shared<ArenaDefinition>(ArenaDefinition::fromMap(map));
        check("player_spawn_clear",world->isWalkable(definition->playerStart,.34f));
        for(size_t i=0;i<definition->rifts.size();++i)check("rift_"+std::to_string(i)+"_spawn_clear",world->isWalkable(definition->rifts[i],.65f));
        for(size_t i=0;i<definition->sentryPositions.size();++i)check("sentry_"+std::to_string(i)+"_spawn_clear",world->isWalkable(definition->sentryPositions[i],.65f));
        size_t count=0;std::set<std::string> kinds;
        for(const auto& model:map.getModels()) {
            if(!model.source.getBool("interiorCover",false) && !(baseline&&(model.id==31||model.id==32)))continue;
            ++count;const auto kind=model.source.getString("coverKind");kinds.insert(kind);
            const auto prefix=std::string("cover_")+std::to_string(model.id)+"_";
            auto ci=std::find_if(map.getCollisionBrushes().begin(),map.getCollisionBrushes().end(),[&](const Brush& b){return b.id==model.id;});
            auto vi=std::find_if(map.getBrushes().begin(),map.getBrushes().end(),[&](const Brush& b){return b.id==model.id;});
            check(prefix+"explicit_collision",!model.collisionAsset.empty() && ci!=map.getCollisionBrushes().end());
            if(ci==map.getCollisionBrushes().end() || vi==map.getBrushes().end())continue;
            SpatialWorld proxy({*ci},{},{}),visual({*vi},{},{});
            const auto box=enemyGeometry::bounds(*ci);const auto size=box.max-box.min;
            J item=J::object();item["id"]=J(double(model.id));item["kind"]=J(kind);item["bounds_min"]=vector(box.min);item["bounds_max"]=vector(box.max);item["triangles"]=J(double(ci->faces.size()/3));
            std::vector<RayPair> solids,holes;int observedRays=0,proxyOnly=0,visualOnly=0;
            std::ofstream sections(out+"/cover-"+std::to_string(model.id)+"-sections.csv");
            sections<<"axis,row,column,start_x,start_y,start_z,end_x,end_y,end_z,proxy_hit,visual_hit,proxy_fraction,visual_fraction\n";
            J mismatches=J::array();
            // Cross sections run through the central 90% of the authored AABB.
            // Hole candidates must sit between two genuine solid hits on the same
            // row, excluding trivial rays outside the object's silhouette.
            for(int axis:{0,2}) {
                const int across=axis==0?2:0;
                for(int row=1;row<=14;++row) {
                    const float y=box.min.y+size.y*float(row)/15.f;
                    bool priorSolid=false;std::vector<RayPair> pendingHoles;
                    for(int column=1;column<40;++column) {
                        glm::vec3 start=(box.min+box.max)*.5f,end=start;
                        start.y=end.y=y;start[across]=end[across]=box.min[across]+size[across]*float(column)/40.f;
                        start[axis]=box.min[axis]-.8f;end[axis]=box.max[axis]+.8f;
                        RayPair pair{start,end};const auto proxyHit=proxy.raycast(start,end),visualHit=visual.raycast(start,end);
                        const bool ph=proxyHit.hit,vh=visualHit.hit;
                        sections<<axis<<','<<row<<','<<column<<','<<start.x<<','<<start.y<<','<<start.z<<','<<end.x<<','<<end.y<<','<<end.z<<','<<ph<<','<<vh<<','<<proxyHit.fraction<<','<<visualHit.fraction<<'\n';
                        if(ph!=vh && mismatches.size()<16) {auto sample=rayJson(pair);sample["proxy_hit"]=J(ph);sample["visual_hit"]=J(vh);mismatches.push_back(sample);}
                        ++observedRays;if(ph&&!vh)++proxyOnly;if(!ph&&vh)++visualOnly;
                        if(ph&&vh) {
                            if(priorSolid)holes.insert(holes.end(),pendingHoles.begin(),pendingHoles.end());
                            pendingHoles.clear();priorSolid=true;
                            if(y>.45f && y<1.5f && world->raycast(start,end,.12f).objectId==model.id)solids.push_back(pair);
                        } else if(!ph&&!vh && priorSolid && !world->raycast(start,end,.12f).hit)pendingHoles.push_back(pair);
                    }
                }
            }
            item["mismatch_examples"]=mismatches;
            item["cross_section_rays"]=J(double(observedRays));item["proxy_only_hits"]=J(double(proxyOnly));item["visual_only_hits"]=J(double(visualOnly));item["clear_internal_projectile_rays"]=J(double(holes.size()));
            check(prefix+"solid_visual_and_proxy_ray",!solids.empty());
            if(!solids.empty()) {
                const auto ray=solids[solids.size()/2];item["solid_ray"]=rayJson(ray);
                check(prefix+"solid_los_blocked",!enemyGeometry::clear(ray.start,ray.end,&map));
                check(prefix+"solid_hostile_projectile_blocked",!projectileReaches(map,ray));
                const RayPair reverse{ray.end,ray.start};
                check(prefix+"solid_reverse_los_blocked",!enemyGeometry::clear(reverse.start,reverse.end,&map));
                check(prefix+"solid_reverse_hostile_projectile_blocked",!projectileReaches(map,reverse));
            }
            if(kind=="skeleton") {
                check(prefix+"bone_opening_retained",!holes.empty());
                if(!holes.empty()) {
                    const auto ray=holes[holes.size()/2];item["opening_ray"]=rayJson(ray);
                    check(prefix+"bone_opening_los",enemyGeometry::clear(ray.start,ray.end,&map,.12f));
                    check(prefix+"bone_opening_hostile_projectile",projectileReaches(map,ray));
                    const RayPair reverse{ray.end,ray.start};
                    check(prefix+"bone_opening_reverse_los",enemyGeometry::clear(reverse.start,reverse.end,&map,.12f));
                    check(prefix+"bone_opening_reverse_hostile_projectile",projectileReaches(map,reverse));
                }
            }
            // Pick a genuine blocked ground crossing, not a ray near a neighboring
            // wall; both endpoints must support actual actor bodies.
            bool foundMove=false;RayPair movement{};
            for(const auto& ray:solids) {
                auto a=ray.start,b=ray.end;a.y=b.y=.015f;
                if(world->isWalkable(a,.34f)&&world->isWalkable(b,.34f)&&!world->canTraverse(a,b,.34f)) {movement={a,b};foundMove=true;break;}
            }
            check(prefix+"ground_crossing_fixture",foundMove);
            if(foundMove) {
                item["movement"]=rayJson(movement);
                Player player;Camera camera;player.respawn(movement.start);camera.setDirection(movement.end-movement.start);PlayerInput input;input.forward=true;
                bool valid=true;float highest=0;
                for(int i=0;i<150;++i){player.processInput(input,&camera,.01f);player.update(.01f,&map);const auto feet=player.getPosition();highest=std::max(highest,feet.y);valid &= std::isfinite(feet.x)&&std::isfinite(feet.y)&&std::isfinite(feet.z);if(i>5 && !world->isWalkable(feet,.30f))valid=false;}
                check(prefix+"ordinary_player_never_penetrates",valid);item["straight_walk_end"]=vector(player.getPosition());item["straight_walk_highest"]=J(double(highest));
                Player slider;slider.respawn(movement.start);PlayerInput slideInput;slideInput.forward=true;slideInput.sprint=true;
                bool sawSlide=false,slideValid=true;
                for(int i=0;i<100;++i) {slideInput.crouch=i>=4;slider.processInput(slideInput,&camera,.01f);slider.update(.01f,&map);sawSlide|=slider.getState()==PlayerState::SLIDING;const auto feet=slider.getPosition();slideValid &= std::isfinite(feet.x)&&std::isfinite(feet.y)&&std::isfinite(feet.z);if(i>5&&!world->isWalkable(feet,.30f))slideValid=false;}
                check(prefix+"ordinary_input_slide_entered",sawSlide);check(prefix+"slide_never_penetrates",slideValid);item["slide_end"]=vector(slider.getPosition());
                Enemy enemy(movement.start);enemy.setArenaDefinition(definition);int frame=0;
                for(;frame<4500 && glm::length(enemy.getPosition()-movement.end)>.8f;++frame)enemy.update(.01f,movement.end,&map);
                check(prefix+"enemy_reaches_around",glm::length(enemy.getPosition()-movement.end)<=.8f);
                item["enemy_end"]=vector(enemy.getPosition());item["enemy_detour_seconds"]=J(double(frame)*.01);
            }
            J top=J::array();
            for(int x=1;x<8;++x)for(int z=1;z<8;++z) {
                glm::vec3 p{box.min.x+size.x*float(x)/8,box.max.y+.1f,box.min.z+size.z*float(z)/8};
                auto support=world->support(p,0,size.y+.2f,.34f);
                if(support.hit&&support.objectId==model.id&&support.height>.6f) {p.y=support.height+.015f;if(world->isWalkable(p,.34f))top.push_back(vector(p));}
            }
            item["standing_top_samples"]=top;
            // A standable sample is not automatically safe: ask a real Chaser
            // whether its ordinary pursuit reaches melee contact from the floor.
            J topThreats=J::array();
            if(!skipTop && foundMove && top.size())for(size_t index:{size_t(0),top.size()/2,top.size()-1}) {
                const auto& value=top[index];glm::vec3 goal{float(value[0].getNumber()),float(value[1].getNumber()),float(value[2].getNumber())};
                Enemy pursuer(movement.start);pursuer.setArenaDefinition(definition);int frame=0;
                for(;frame<3000 && !pursuer.isTouchingPlayer(goal);++frame)pursuer.update(.01f,goal,&map);
                J result=J::object();result["target"]=vector(goal);result["chaser_contact"]=J(pursuer.isTouchingPlayer(goal));result["seconds"]=J(frame*.01);result["chaser_end"]=vector(pursuer.getPosition());topThreats.push_back(result);
                if(top.size()==1)break;
            }
            item["standing_top_contact_checks"]=topThreats;
            report["covers"].push_back(item);
        }
        if(baseline)check("two_original_covers",count==2);
        else {check("four_or_five_covers",count>=4&&count<=5);check("ship_bone_crystal_present",kinds.count("ship")&&kinds.count("skeleton")&&kinds.count("crystal"));}
    }catch(const std::exception& e){report["error"]=J(e.what());std::cerr<<e.what()<<'\n';++failures;}
    report["failures"]=J(double(failures));std::ofstream(out+"/collision-report.json")<<report.toString()<<'\n';std::cout<<"INTERIOR_FAILURES "<<failures<<std::endl;return failures?1:0;
}
