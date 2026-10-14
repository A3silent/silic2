#include "engine/map.h"
#include "engine/camera.h"
#include "engine/game_config.h"
#include "player/player.h"
#include "enemy/enemy.h"
#include "enemy/enemy_geometry.h"
#include "enemy/enemy_projectile_system.h"
#include "game/arena_definition.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
int main(int argc,char** argv) {
 using namespace silic2;const std::string output=argc>1?argv[1]:"artifacts/acceptance/crystal-combat/cpu";std::filesystem::create_directories(output);
 Map map;if(!map.loadFromFile("res/maps/starry_crystal_arena.json")){std::cerr<<map.getLastError()<<'\n';return 1;}
 auto world=map.getSpatialWorld();if(!world)return 2;int failures=0;
 auto check=[&](const std::string& name,bool passed){std::cout<<"CHECK "<<name<<" "<<(passed?"PASS":"FAIL")<<std::endl;if(!passed)++failures;};
 for(const auto& issue:map.getSpatialDiagnostics())std::cout<<"DIAGNOSTIC "<<issue<<'\n';check("map_diagnostics",map.getSpatialDiagnostics().empty());
 Player player;Camera camera;player.respawn(map.getArenaLayout().playerStart);size_t node=0;
 for(auto goal:map.getSpatialSettings().route){
  bool reached=false;for(int frame=0;frame<2000;++frame){auto delta=goal-player.getPosition();delta.y=0;if(glm::length(delta)<.13f && std::abs(goal.y-player.getPosition().y)<.25f){reached=true;break;}camera.setDirection(delta);PlayerInput keys;keys.forward=true;player.processInput(keys,&camera,.01f);player.update(.01f,&map);}
  auto p=player.getPosition();std::cout<<"ROUTE "<<node<<" "<<p.x<<" "<<p.y<<" "<<p.z<<std::endl;check("WASD_route_"+std::to_string(node++),reached);if(!reached)break;
 }
 check("vault_hole_ray",!world->raycast({0,1,-26},{0,1,-20}).hit);
 check("vault_solid_ray",world->raycast({0,7,-23},{0,3,-23}).hit);
 check("curved_outer_wall_ray",world->raycast({20,1,0},{35,1,0}).hit);
 check("shared_LOS_through_vault",enemyGeometry::clear({0,1,-26},{0,1,-20},&map,.12f));
 EnemyProjectileSystem shots;shots.spawn({0,1,-26},{0,0,1},1);bool projectileHit=false;for(int i=0;i<80;++i)projectileHit|=!shots.update(.01f,&map,AABB({-.5f,.1f,-21},{.5f,1.8f,-20})).empty();check("projectile_through_vault",projectileHit);
 auto definition=std::make_shared<ArenaDefinition>(ArenaDefinition::fromMap(map));
 std::vector<Enemy> enemies;for(int i=0;i<18;++i){enemies.emplace_back(map.getArenaLayout().rifts[i%4]);enemies.back().setArenaDefinition(definition);enemies.back().setId(i+1);}
 std::ofstream csv(output+"/enemy18-frames.csv");csv<<"frame,cpu_ms\n";std::vector<double> times;
 for(int frame=0;frame<180;++frame){auto start=std::chrono::steady_clock::now();for(auto& e:enemies)e.update(1.f/60.f,{0,6.015f,-15},&map);auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();times.push_back(elapsed);csv<<frame<<','<<elapsed<<'\n';}
 std::sort(times.begin(),times.end());std::cout<<"CPU18_DIAGNOSTIC median="<<times[times.size()/2]<<" p95="<<times[size_t((times.size()-1)*.95)]<<" max="<<times.back()<<" nodes="<<map.getSpatialSettings().nodes.size()<<" triangles="<<world->triangles().size()<<std::endl;
 // Actual sprint + jump directed outward from the elevated ring; static shell must retain the actor.
 player.respawn({0,6.015f,-15});camera.setDirection({0,0,-1});PlayerInput keys;keys.forward=true;keys.sprint=true;
 for(int frame=0;frame<350;++frame){keys.sprint=frame==0;keys.jump=frame>=5&&frame<12;player.processInput(keys,&camera,.01f);player.update(.01f,&map);}
 auto end=player.getPosition();check("upper_sprint_jump_inside_visible_shell",glm::length(glm::vec2(end.x,end.z))<29.5f && end.y>-.2f);
 std::cout<<"PROBE_FAILURES "<<failures<<std::endl;return failures?3:0;
}
