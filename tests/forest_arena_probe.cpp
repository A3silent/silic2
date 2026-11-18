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
        check("open_crystal_identity",map.getEnvironmentPreset()==EnvironmentPreset::ForestCrystal &&
              map.getGameMode()==GameMode::ArenaSurvival);
        check("spire_is_only_model",!map.getModels().empty() &&
              std::all_of(map.getModels().begin(),map.getModels().end(),[](const auto& m) {
                  return m.material=="forest_spire";
              }));
        const auto isRim=[](const Brush& b){return b.material=="forest_rim";};
        const auto knownSurface=[&](const Brush& b) {
            return isRim(b)||b.material=="forest_ground"||b.material=="forest_spire";
        };
        const auto& brushes=map.getBrushes();
        const auto& collision=map.getCollisionBrushes();
        check("only_mineral_render_and_collision_surfaces",
              std::all_of(brushes.begin(),brushes.end(),knownSurface) &&
              std::all_of(collision.begin(),collision.end(),knownSurface));
        check("complete_visible_rim",
              std::count_if(brushes.begin(),brushes.end(),isRim)==12 &&
              std::count_if(collision.begin(),collision.end(),isRim)==12);
        const auto rimId=[&](uint32_t id) {
            return std::any_of(brushes.begin(),brushes.end(),[&](const auto& b) {
                return b.id==id && isRim(b);
            });
        };
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
        // At player height the visible mineral lip is the physical boundary.
        // Above it the horizon stays open in every direction.
        bool openHorizon=true;
        for(int i=0;i<48;++i) {
            const float angle=float(i)*6.28318530718f/48;
            const glm::vec3 direction(std::cos(angle),0,std::sin(angle));
            const auto start=direction*90.f+glm::vec3(0,1.1f,0);
            const auto end=direction*130.f+glm::vec3(0,1.1f,0);
            const auto hit=world->raycast(start,end),vh=visible.raycast(start,end);
            const auto highStart=direction*90.f+glm::vec3(0,3.5f,0);
            const auto highEnd=direction*225.f+glm::vec3(0,3.5f,0);
            const bool highClear=!world->raycast(highStart,highEnd).hit &&
                                 !visible.raycast(highStart,highEnd).hit;
            openHorizon &= highClear;
            J entry=J::object();
            entry["angle_degrees"]=J(i*7.5);
            entry["physics_hit"]=J(hit.hit);
            entry["visual_hit"]=J(vh.hit);
            entry["high_view_clear"]=J(highClear);
            entry["physics_position"]=vector(hit.position);
            entry["visual_position"]=vector(vh.position);
            entry["object_id"]=J(double(hit.objectId));
            report["boundaries"].push_back(entry);
            const float radius=glm::length(glm::vec2(hit.position.x,hit.position.z));
            check("visible_mineral_rim_"+std::to_string(i),
                  hit.hit && vh.hit && rimId(hit.objectId) && rimId(vh.objectId) &&
                  glm::length(hit.position-vh.position)<.002f &&
                  glm::dot(hit.normal,direction)<-.9f && radius>98 && radius<104);
        }
        check("horizon_clear_above_rim",openHorizon);
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
        // Check real player sprint-jumps at both sector seams and face interiors.
        report["jump_boundaries"]=J::array();
        for(int i=0;i<16;++i) {
            const float angle=i*6.28318530718f/16;
            const glm::vec3 direction(std::cos(angle),0,std::sin(angle));
            Player player;Camera camera;
            const auto start=direction*95.f+glm::vec3(0,.015f,0);
            player.respawn(start);camera.setDirection(direction);
            float maximumHeight=0,maximumRadius=0;
            for(int f=0;f<400;++f) {
                PlayerInput input;
                input.forward=true;input.sprint=f==0;input.jump=f>=15&&f<25;
                player.processInput(input,&camera,.01f);player.update(.01f,&map);
                const auto p=player.getPosition();
                maximumHeight=std::max(maximumHeight,p.y);
                maximumRadius=std::max(maximumRadius,glm::length(glm::vec2(p.x,p.z)));
            }
            const auto p=player.getPosition();
            const bool startClear=world->isWalkable(start,.3f);
            J evidence=J::object();
            evidence["angle_degrees"]=J(i*22.5);
            evidence["start_clear"]=J(startClear);
            evidence["start"]=vector(start);
            evidence["end"]=vector(p);
            evidence["highest_feet"]=J(double(maximumHeight));
            evidence["maximum_radius"]=J(double(maximumRadius));
            report["jump_boundaries"].push_back(evidence);
            check("sprint_jump_kept_inside_"+std::to_string(i),startClear &&
                  maximumHeight>.5f && maximumRadius<104 && std::isfinite(p.y) &&
                  p.y>-.1f && player.isOnGround());
        }
        // Begin near enough for normal slide input to contact the mineral face.
        const auto edge=travel(map,{97,.015f,0},{120,.015f,0},true,true,4);
        check("slide_contacts_visible_boundary",
              edge.clear && edge.slid && !edge.reached && edge.end.x>98 && edge.end.x<104);
        report["boundary_slide_end"]=vector(edge.end);
    }catch(const std::exception& e){report["error"]=J(e.what());std::cerr<<e.what()<<'\n';++failures;}
    report["failures"]=J(double(failures));std::ofstream(std::string(argv[2])+"/forest-report.json")<<report.toString()<<'\n';return failures?1:0;
}
