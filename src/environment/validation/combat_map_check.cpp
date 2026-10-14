#include "engine/map.h"
#include "engine/spatial_world.h"
#include "environment/environment_scene.h"
#include <iostream>
#include <chrono>
#include <cassert>
int main(){
 auto start=std::chrono::steady_clock::now();
 silic2::Map map;
 if(!map.loadFromFile("res/maps/starry_crystal_arena.json")){std::cerr<<map.getLastError()<<"\n";return 1;}
 for(const auto& d:map.getSpatialDiagnostics())std::cout<<"DIAGNOSTIC "<<d<<"\n";
 const auto& spatial=map.getSpatialSettings();const auto world=map.getSpatialWorld();
 if(!world)return 2;
 size_t bad=0;
 for(size_t i=1;i<spatial.route.size();++i){
  if(!world->canTraverse(spatial.route[i-1],spatial.route[i],.65f)){
   const auto a=spatial.route[i-1],b=spatial.route[i];
   std::cout<<"ROUTE_FAIL "<<i<<" ("<<a.x<<","<<a.y<<","<<a.z<<") -> ("<<b.x<<","<<b.y<<","<<b.z<<")\n";++bad;
  }
 }
 auto scene=silic2::deriveEnvironmentScene(map);assert(scene.combatWorld&&!scene.observatory);
 assert(silic2::environmentPhase(scene.settings,0)==0 && silic2::environmentPhase(scene.settings,49)==1);
 std::cout<<"Models="<<map.getModels().size()<<" triangles="<<world->triangles().size()<<" nav="<<spatial.nodes.size()<<" routeFailures="<<bad<<" load/checkSeconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"\n";
 return !map.getSpatialDiagnostics().empty() || bad?3:0;
}
