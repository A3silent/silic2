// Actual-map functional validation. No frame timings, rendering, or benchmarks.
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
using namespace silic2;
namespace {
using J=SimpleJson;
J vector(glm::vec3 v){J out=J::array();for(int i=0;i<3;++i)out.push_back(J(double(v[i])));return out;}
struct Travel { bool reached=false,clear=true,slid=false; float seconds=0; glm::vec3 end{0}; };
Travel travel(const Map& map,glm::vec3 start,glm::vec3 goal,bool sprint,bool slide,float seconds=45) {
    Player player;Camera camera;player.respawn(start);Travel result;
    for(int f=0;f<int(seconds*100);++f) {
        auto delta=goal-player.getPosition();delta.y=0;
        if(glm::length(delta)<.16f){result.reached=true;break;}
        camera.setDirection(delta);PlayerInput keys;keys.forward=true;keys.sprint=sprint&&f==0;keys.crouch=slide&&f>=10;
        player.processInput(keys,&camera,.01f);player.update(.01f,&map);result.seconds+=.01f;
        auto p=player.getPosition();result.clear&=std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
        if(f>5)result.clear&=map.getSpatialWorld()->isWalkable(p,.30f,player.getState()==PlayerState::SLIDING?.8f:1.8f);
        result.slid|=player.getState()==PlayerState::SLIDING;
    }
    result.end=player.getPosition();return result;
}
}
int main(int argc,char** argv) {
    if(argc!=3){std::cerr<<"Usage: forest-arena-probe MAP OUTPUT\n";return 2;}
    std::filesystem::create_directories(argv[2]);J report=J::object();report["checks"]=J::array();report["boundaries"]=J::array();report["route"]=J::array();int failures=0;
    auto check=[&](const std::string& name,bool passed){J entry=J::object();entry["name"]=J(name);entry["passed"]=J(passed);report["checks"].push_back(entry);std::cout<<name<<": "<<(passed?"PASS":"FAIL")<<std::endl;if(!passed)++failures;};
    try {
        Map map;if(!map.loadFromFile(argv[1]))throw std::runtime_error(map.getLastError());auto world=map.getSpatialWorld();if(!world)throw std::runtime_error("Spatial world required");
        auto arena=std::make_shared<ArenaDefinition>(ArenaDefinition::fromMap(map));
        check("forest_identity",map.getEnvironmentPreset()==EnvironmentPreset::ForestCrystal);
        check("far_plane_covers_spire_and_sky",map.getWorldSettings().farPlane>=400);
        check("map_diagnostics_empty",map.getSpatialDiagnostics().empty());for(const auto& d:map.getSpatialDiagnostics())std::cerr<<d<<'\n';
        check("connected_navigation",arena->hasConnectedWaypoints(.34f));
        const auto extent=arena->maxXZ-arena->minXZ;check("stable_floor_minimap_extent",map.getArenaLayout().hasFloor&&extent.x>=200&&extent.x<=240&&extent.y>=200&&extent.y<=240);
        report["floor_min"]=vector({arena->minXZ.x,arena->floorY,arena->minXZ.y});report["floor_max"]=vector({arena->maxXZ.x,arena->floorY,arena->maxXZ.y});
        // Check authored spawn access without waiting for minutes of remote pursuit.
        std::vector<glm::vec3> starts{arena->playerStart};starts.insert(starts.end(),arena->rifts.begin(),arena->rifts.end());starts.insert(starts.end(),arena->sentryPositions.begin(),arena->sentryPositions.end());
        for(size_t i=0;i<starts.size();++i) {
            bool linked=false;for(const auto& node:map.getSpatialSettings().nodes)if(world->canTraverse(starts[i],node.position,.65f)){linked=true;break;}
            check("spawn_"+std::to_string(i)+"_clear_and_connected",world->isWalkable(starts[i],.65f)&&linked);
        }
        SpatialWorld visible(map.getBrushes(),{},{});
        // Independent eyes-height radial samples must hit visible thicket as well
        // as physics. The same sampled ray need not have identical hit distance.
        for(int i=0;i<48;++i) {
            const float angle=float(i)*6.28318530718f/48;glm::vec3 direction(std::cos(angle),0,std::sin(angle));
            auto start=direction*90.f+glm::vec3(0,1.1f,0),end=direction*130.f+glm::vec3(0,1.1f,0);
            auto hit=world->raycast(start,end),vh=visible.raycast(start,end);J entry=J::object();entry["angle_degrees"]=J(i*7.5);entry["physics_hit"]=J(hit.hit);entry["visual_hit"]=J(vh.hit);entry["physics_position"]=vector(hit.position);entry["visual_position"]=vector(vh.position);entry["object_id"]=J(double(hit.objectId));report["boundaries"].push_back(entry);
            check("woodland_boundary_"+std::to_string(i),hit.hit&&vh.hit&&glm::length(glm::vec2(hit.position.x,hit.position.z))>=95);
        }
        auto centerHit=world->raycast({-20,1.2f,0},{20,1.2f,0});
        check("central_spire_visible_solid",centerHit.hit&&visible.raycast({-20,1.2f,0},{20,1.2f,0}).hit);
        check("central_spire_blocks_both_LOS",!enemyGeometry::clear({-20,1.2f,0},{20,1.2f,0},&map)&&!enemyGeometry::clear({20,1.2f,0},{-20,1.2f,0},&map));
        for(float direction:{-1.f,1.f}) {
            EnemyProjectileSystem shots;const glm::vec3 start(18*direction,1.2f,0),target(-18*direction,1.2f,0);
            shots.spawn(start,target-start,1);bool targetHit=false;
            for(int f=0;f<390;++f)targetHit|=!shots.update(.01f,&map,AABB(target-glm::vec3(.2f),target+glm::vec3(.2f))).empty();
            check(direction<0?"spire_blocks_projectile_east":"spire_blocks_projectile_west",!targetHit);
        }
        // 190m clear chord offset from the central obstruction: current10m/s
        // sprint should take about19s, without changing movement values.
        const auto chord=travel(map,{-95,.015f,15},{95,.015f,15},true,false,25);
        check("near_diameter_sprint_clear",chord.clear&&chord.reached);check("near_diameter_about_twenty_seconds",chord.seconds>=18&&chord.seconds<=22);
        report["sprint_seconds"]=J(double(chord.seconds));report["sprint_end"]=vector(chord.end);
        auto previous=arena->playerStart;
        for(size_t i=0;i<map.getSpatialSettings().route.size();++i){auto goal=map.getSpatialSettings().route[i];const auto move=travel(map,previous,goal,false,false);check("ordinary_route_"+std::to_string(i),move.clear&&move.reached);J entry=J::object();entry["index"]=J(double(i));entry["end"]=vector(move.end);entry["seconds"]=J(double(move.seconds));report["route"].push_back(entry);previous=move.end;if(!move.reached)break;}
        // Real ground enemy movement around the actual central blocker.
        Enemy chaser({-20,.015f,0});chaser.setArenaDefinition(arena);int frame=0;
        for(;frame<3000&&glm::length(chaser.getPosition()-glm::vec3(20,.015f,0))>.8f;++frame)chaser.update(.01f,{20,.015f,0},&map);
        check("chaser_detours_central_spire",glm::length(chaser.getPosition()-glm::vec3(20,.015f,0))<=.8f);report["chaser_end"]=vector(chaser.getPosition());
        // Jump near the visible edge must not bypass the wooded boundary.
        report["jump_boundaries"]=J::array();
        for(int i=0;i<8;++i) {
            const float angle=i*6.28318530718f/8;const glm::vec3 direction(std::cos(angle),0,std::sin(angle));
            Player player;Camera camera;const auto start=direction*95.f+glm::vec3(0,.015f,0);player.respawn(start);camera.setDirection(direction);float maximumHeight=0;
            for(int f=0;f<400;++f){PlayerInput input;input.forward=true;input.sprint=f==0;input.jump=f>=15&&f<25;player.processInput(input,&camera,.01f);player.update(.01f,&map);maximumHeight=std::max(maximumHeight,player.getPosition().y);}
            const auto p=player.getPosition();J jumpEvidence=J::object();jumpEvidence["angle_degrees"]=J(double(i*45));jumpEvidence["start_clear"]=J(world->isWalkable(start,.3f));jumpEvidence["start"]=vector(start);jumpEvidence["end"]=vector(p);jumpEvidence["highest_feet"]=J(double(maximumHeight));report["jump_boundaries"].push_back(jumpEvidence);check("sprint_jump_kept_inside_"+std::to_string(i),std::isfinite(p.y)&&p.y>-.1f&&glm::length(glm::vec2(p.x,p.z))<110);
        }
        // Authored inner-edge trees are tested individually from the clearing.
        // A thicket blocking the approach is recorded, never counted as tree contact.
        report["tree_approaches"]=J::array();size_t trees=0,contacts=0;
        for(const auto& model:map.getModels()) {
            if(model.name.find("Tripo pine_")!=0 || model.collisionAsset.empty() || glm::length(glm::vec2(model.position.x,model.position.z))>=104)continue;
            ++trees;auto brush=std::find_if(map.getCollisionBrushes().begin(),map.getCollisionBrushes().end(),[&](const auto& b){return b.id==model.id;});
            if(brush==map.getCollisionBrushes().end())throw std::runtime_error("Tree proxy missing");
            SpatialWorld tree({*brush},{},{});const auto direction=glm::normalize(glm::vec3(model.position.x,0,model.position.z));const glm::vec3 tangent(-direction.z,0,direction.x);
            float offset=0;bool unobstructed=false;
            for(float sample:{0.f,-1.5f,1.5f,-3.f,3.f}) {
                const auto a=direction*90.f+tangent*sample+glm::vec3(0,1.1f,0),b=model.position+direction*5.f+tangent*sample+glm::vec3(0,1.1f,0);
                if(world->raycast(a,b).objectId==model.id){offset=sample;unobstructed=true;break;}
            }
            const auto start=direction*90.f+tangent*offset+glm::vec3(0,.015f,0);bool crossed=false,contact=false,rootSupport=false,jumped=false,valid=world->isWalkable(start,.3f);float highest=0;glm::vec3 end=start;
            for(bool jump:{false,true}) {
                Player actor;Camera view;actor.respawn(start);view.setDirection(direction);bool requestedJump=false;
                for(int f=0;f<500;++f) {
                    auto before=actor.getPosition()+glm::vec3(0,.9f,0);PlayerInput input;input.forward=true;
                    const auto wall=world->raycast(actor.getPosition()+glm::vec3(0,.8f,0),actor.getPosition()+direction*1.6f+glm::vec3(0,.8f,0));
                    if(jump&&!requestedJump&&f>10&&(wall.hit||f==150)){input.jump=true;requestedJump=true;}
                    actor.processInput(input,&view,.01f);actor.update(.01f,&map);auto p=actor.getPosition();auto center=p+glm::vec3(0,.9f,0);
                    if(glm::length(center-before)>1e-6f)crossed|=tree.raycast(before,center).hit;
                    contact|=tree.raycast(center,center+direction*.45f).hit;
                    const auto support=world->support(p,.06f,.08f,.3f);rootSupport|=support.hit&&support.objectId==model.id&&actor.isOnGround();
                    contact|=rootSupport;highest=std::max(highest,p.y);jumped|=jump&&p.y>start.y+.4f;
                    valid&=std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&p.y>-.1f&&glm::length(glm::vec2(p.x,p.z))<110;end=p;
                }
            }
            contacts+=contact;check("inner_tree_"+std::to_string(model.id)+"_approach_jump_no_body_crossing",valid&&jumped&&!crossed);
            J evidence=J::object();evidence["id"]=J(double(model.id));evidence["species"]=J(model.name);evidence["direct_eye_ray_tree_first"]=J(unobstructed);evidence["actual_tree_contact"]=J(contact);evidence["stood_on_tree_root"]=J(rootSupport);evidence["body_center_crossed_tree_surface"]=J(crossed);evidence["highest_feet"]=J(double(highest));evidence["end"]=vector(end);report["tree_approaches"].push_back(evidence);
        }
        check("twelve_inner_edge_trees_tested",trees==12);check("actual_inner_tree_contacts_observed",contacts>0);report["trees_contacted"]=J(double(contacts));
        // Short approach uses normal sprint/slide input; must stop at woodland.
        const auto edge=travel(map,{90,.015f,0},{120,.015f,0},true,true,4);
        check("slide_contacts_visible_boundary",edge.clear&&edge.slid&&!edge.reached&&edge.end.x<110);report["boundary_slide_end"]=vector(edge.end);
    }catch(const std::exception& e){report["error"]=J(e.what());std::cerr<<e.what()<<'\n';++failures;}
    report["failures"]=J(double(failures));std::ofstream(std::string(argv[2])+"/forest-report.json")<<report.toString()<<'\n';return failures?1:0;
}
