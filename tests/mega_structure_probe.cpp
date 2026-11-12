// Functional checks for authored giant structures. Cases are world-space points
// selected from the actual map, kept in acceptance artifacts rather than runtime.
#include "engine/map.h"
#include "engine/camera.h"
#include "player/player.h"
#include "enemy/enemy.h"
#include "enemy/enemy_geometry.h"
#include "enemy/enemy_projectile_system.h"
#include "game/arena_definition.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
using namespace silic2;
namespace {
using J=SimpleJson;
glm::vec3 point(const J& j) {
    if(!j.isArray()||j.size()!=3)throw std::runtime_error("Expected three-coordinate point");
    return {float(j[0].getNumber()),float(j[1].getNumber()),float(j[2].getNumber())};
}
J vector(glm::vec3 p){J j=J::array();for(int i=0;i<3;++i)j.push_back(J(double(p[i])));return j;}
J read(const std::string& path){std::ifstream f(path);if(!f)throw std::runtime_error("Cannot read "+path);std::stringstream s;s<<f.rdbuf();return J::parse(s.str());}
bool projectile(const Map& map,glm::vec3 from,glm::vec3 to) {
    EnemyProjectileSystem shots;shots.spawn(from,to-from,1);bool hit=false;
    AABB target(to-glm::vec3(.16f),to+glm::vec3(.16f));
    const int frames=int(std::ceil(glm::length(to-from)/10.f/.01f))+50;
    for(int i=0;i<frames;++i)hit|=!shots.update(.01f,&map,target).empty();
    return hit;
}
struct Movement {bool valid=true,reached=false,slid=false;glm::vec3 end{0};float distance=0;};
Movement travel(const Map& map,const std::vector<glm::vec3>& points,bool slide,bool stopAtGoal) {
    Movement result;if(points.size()<2){result.valid=false;return result;}
    Player player;Camera camera;player.respawn(points.front());auto previous=points.front();int total=0;
    for(size_t node=1;node<points.size();++node) {
        bool reached=false;
        for(int frame=0;frame<1200;++frame,++total) {
            auto delta=points[node]-player.getPosition();delta.y=0;
            if(stopAtGoal&&glm::length(delta)<.15f&&std::abs(points[node].y-player.getPosition().y)<.3f){reached=true;break;}
            camera.setDirection(delta);PlayerInput keys;keys.forward=true;keys.sprint=slide&&total==0;keys.crouch=slide&&total>=4;
            player.processInput(keys,&camera,.01f);player.update(.01f,&map);
            auto p=player.getPosition();result.distance+=glm::length(p-previous);previous=p;
            result.valid&=std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
            if(total>5&&!map.getSpatialWorld()->isWalkable(p,.30f))result.valid=false;
            result.slid|=player.getState()==PlayerState::SLIDING;
            // A wall-approach case intentionally cannot reach its endpoint.
            if(!stopAtGoal&&frame>=149)break;
        }
        result.reached=reached;if(stopAtGoal&&!reached)break;
    }
    result.end=player.getPosition();return result;
}
}
int main(int argc,char** argv) {
    if(argc!=4){std::cerr<<"Usage: mega-structure-probe MAP CASES_JSON OUTPUT\n";return 2;}
    const std::string output=argv[3];std::filesystem::create_directories(output);
    J report=J::object();report["checks"]=J::array();report["structures"]=J::array();report["movements"]=J::array();report["rays"]=J::array();report["diagnostics"]=J::array();int failures=0;
    auto check=[&](const std::string& name,bool ok){J j=J::object();j["check"]=J(name);j["passed"]=J(ok);report["checks"].push_back(j);std::cout<<"CHECK "<<name<<' '<<(ok?"PASS":"FAIL")<<std::endl;if(!ok)++failures;};
    try {
        const auto cases=read(argv[2]);Map map;if(!map.loadFromFile(argv[1]))throw std::runtime_error(map.getLastError());
        auto world=map.getSpatialWorld();if(!world)throw std::runtime_error("Requires spatial map");
        auto arena=std::make_shared<ArenaDefinition>(ArenaDefinition::fromMap(map));
        // Read-only case discovery. These samples help choose actual surface and
        // opening coordinates; they deliberately make no acceptance assertions.
        if(cases.hasKey("queries")) {
            J discovery=J::object();discovery["mode"]=J("discovery, not acceptance");discovery["queries"]=J::array();
            for(size_t i=0;i<cases["queries"].size();++i) {
                const auto& query=cases["queries"][i];auto a=point(query["start"]),b=point(query["end"]);const auto id=uint32_t(query.getNumber("id"));
                auto vi=std::find_if(map.getBrushes().begin(),map.getBrushes().end(),[&](const auto& brush){return brush.id==id;});
                if(vi==map.getBrushes().end())throw std::runtime_error("Query object missing");
                SpatialWorld visual({*vi},{},{});auto hit=world->raycast(a,b),wide=world->raycast(a,b,.12f),vh=visual.raycast(a,b);
                auto feetA=a,feetB=b;feetA.y=feetB.y=.015f;
                J item=query;item["hit_id"]=J(double(hit.objectId));item["hit_position"]=vector(hit.position);item["hit_normal"]=vector(hit.normal);item["visual_hit"]=J(vh.hit);item["visual_position"]=vector(vh.position);item["wide_hit_id"]=J(double(wide.objectId));item["start_walkable"]=J(world->isWalkable(feetA,.34f));item["end_walkable"]=J(world->isWalkable(feetB,.34f));item["walk_through"]=J(world->canTraverse(feetA,feetB,.34f));discovery["queries"].push_back(item);
            }
            std::ofstream(output+"/query-report.json")<<discovery.toString()<<'\n';std::cout<<"DISCOVERY_ONLY "<<cases["queries"].size()<<" queries, no acceptance claims\n";return 0;
        }
        for(const auto& issue:map.getSpatialDiagnostics()){report["diagnostics"].push_back(J(issue));std::cout<<"DIAGNOSTIC "<<issue<<'\n';}
        check("map_diagnostics_empty",map.getSpatialDiagnostics().empty());check("navigation_connected",world->navigationConnected());
        check("player_spawn_clear",world->isWalkable(arena->playerStart,.34f));
        for(size_t i=0;i<arena->rifts.size();++i)check("rift_"+std::to_string(i)+"_clear",world->isWalkable(arena->rifts[i],.65f));
        for(size_t i=0;i<arena->sentryPositions.size();++i)check("sentry_"+std::to_string(i)+"_clear",world->isWalkable(arena->sentryPositions[i],.65f));
        std::set<std::string> kinds;
        for(size_t n=0;n<cases["structures"].size();++n) {
            const auto& spec=cases["structures"][n];const auto id=uint32_t(spec.getNumber("id"));const auto prefix="structure_"+std::to_string(id)+"_";
            auto model=std::find_if(map.getModels().begin(),map.getModels().end(),[&](const auto& m){return m.id==id;});
            auto ci=std::find_if(map.getCollisionBrushes().begin(),map.getCollisionBrushes().end(),[&](const auto& b){return b.id==id;});
            auto vi=std::find_if(map.getBrushes().begin(),map.getBrushes().end(),[&](const auto& b){return b.id==id;});
            check(prefix+"explicit_visual_collision",model!=map.getModels().end()&&ci!=map.getCollisionBrushes().end()&&vi!=map.getBrushes().end()&&!model->collisionAsset.empty());
            if(ci==map.getCollisionBrushes().end()||vi==map.getBrushes().end())continue;
            const auto kind=spec.getString("kind");kinds.insert(kind);SpatialWorld proxy({*ci},{},{}),visual({*vi},{},{});
            auto box=enemyGeometry::bounds(*vi);J item=J::object();item["id"]=J(double(id));item["kind"]=J(kind);item["minimum"]=vector(box.min);item["maximum"]=vector(box.max);item["collision_triangles"]=J(double(ci->faces.size()/3));report["structures"].push_back(item);
            // Scale is reported, not prescribed by a brittle numeric threshold.
            check(prefix+"authored_solid_cases",spec["solidRays"].size()>0);
            for(size_t r=0;r<spec["solidRays"].size();++r){const auto& ray=spec["solidRays"][r];auto a=point(ray["start"]),b=point(ray["end"]);const auto label=prefix+"solid_"+std::to_string(r)+"_";
                auto ph=proxy.raycast(a,b),vh=visual.raycast(a,b),wh=world->raycast(a,b);J evidence=J::object();evidence["label"]=J(label);evidence["start"]=vector(a);evidence["end"]=vector(b);evidence["proxy_hit"]=J(ph.hit);evidence["visual_hit"]=J(vh.hit);evidence["world_first_id"]=J(double(wh.objectId));evidence["proxy_fraction"]=J(double(ph.fraction));evidence["visual_fraction"]=J(double(vh.fraction));report["rays"].push_back(evidence);
                check(label+"visual_and_proxy",vh.hit&&ph.hit);
                check(label+"actual_wall_is_first",world->raycast(a,b).objectId==id);
                check(label+"both_los_blocked",!enemyGeometry::clear(a,b,&map)&&!enemyGeometry::clear(b,a,&map));
                check(label+"both_projectiles_blocked",!projectile(map,a,b)&&!projectile(map,b,a));
            }
            for(size_t r=0;r<spec["openingRays"].size();++r){const auto& ray=spec["openingRays"][r];auto a=point(ray["start"]),b=point(ray["end"]);const auto label=prefix+"opening_"+std::to_string(r)+"_";
                J evidence=J::object();evidence["label"]=J(label);evidence["start"]=vector(a);evidence["end"]=vector(b);evidence["world_first_id"]=J(double(world->raycast(a,b,.12f).objectId));report["rays"].push_back(evidence);
                check(label+"visual_proxy_clear",!visual.raycast(a,b,.12f).hit&&!proxy.raycast(a,b,.12f).hit);
                check(label+"both_los_clear",enemyGeometry::clear(a,b,&map,.12f)&&enemyGeometry::clear(b,a,&map,.12f));
                check(label+"both_projectiles_pass",projectile(map,a,b)&&projectile(map,b,a));
            }
        }
        check("ship_bone_crystal_present",kinds.count("ship")&&kinds.count("skeleton")&&kinds.count("crystal"));
        for(size_t n=0;n<cases["movements"].size();++n) {
            const auto& spec=cases["movements"][n];const auto name=spec.getString("name"),type=spec.getString("type");std::vector<glm::vec3> points;
            for(size_t i=0;i<spec["points"].size();++i)points.push_back(point(spec["points"][i]));if(points.size()<2)throw std::runtime_error("Movement needs >=2 points");
            check(name+"_start_clear",world->isWalkable(points.front(),.34f));const bool approach=type=="approach";
            if(approach)check(name+"_wall_blocks_direct_capsule",!world->canTraverse(points.front(),points.back(),.34f));
            auto movement=travel(map,points,false,!approach);check(name+"_ordinary_input_nonpenetrating",movement.valid);
            if(!approach)check(name+"_ordinary_input_reaches",movement.reached);
            else check(name+"_solid_endpoint_not_entered",glm::length(movement.end-points.back())>.25f);
            check(name+"_actual_motion",movement.distance>.25f);
            if(spec.getBool("requiresDetour",false))check(name+"_direct_path_requires_detour",!world->canTraverse(points.front(),points.back(),.34f));
            J entry=J::object();entry["name"]=J(name);entry["end"]=vector(movement.end);entry["distance"]=J(double(movement.distance));
            if(spec.getBool("slide",false)){auto slid=travel(map,points,true,!approach);check(name+"_slide_entered",slid.slid);check(name+"_slide_nonpenetrating",slid.valid);if(!approach)check(name+"_slide_reaches",slid.reached);else check(name+"_slide_solid_endpoint_not_entered",glm::length(slid.end-points.back())>.25f);entry["slide_end"]=vector(slid.end);}
            if(spec.getBool("enemy",false)){Enemy enemy(points.front());enemy.setArenaDefinition(arena);int frames=0;for(;frames<4000&&glm::length(enemy.getPosition()-points.back())>.8f;++frames)enemy.update(.01f,points.back(),&map);check(name+"_chaser_reaches",glm::length(enemy.getPosition()-points.back())<=.8f);entry["enemy_end"]=vector(enemy.getPosition());entry["enemy_seconds"]=J(frames*.01);}
            report["movements"].push_back(entry);
        }
    }catch(const std::exception& e){report["error"]=J(e.what());std::cerr<<e.what()<<'\n';++failures;}
    report["failures"]=J(double(failures));std::ofstream(output+"/mega-report.json")<<report.toString()<<'\n';std::cout<<"MEGA_FAILURES "<<failures<<std::endl;return failures?1:0;
}
