#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "engine/map.h"
#include "engine/map_renderer.h"
#include "engine/pixel_renderer.h"
#include "environment/environment_scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <cassert>
#include <string>
static std::vector<unsigned char> pixels(int w,int h){std::vector<unsigned char> p(w*h*3);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,p.data());return p;}
static void save(const std::string& path,int w,int h){auto p=pixels(w,h);std::ofstream f(path,std::ios::binary);f<<"P6\n"<<w<<" "<<h<<"\n255\n";for(int y=h-1;y>=0;--y)f.write(reinterpret_cast<char*>(p.data())+y*w*3,w*3);}
int main(int argc,char** argv){
 const std::string out=argc>1?argv[1]:"artifacts/acceptance/forest-arena/views";
 assert(glfwInit());glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
 auto*w=glfwCreateWindow(640,360,"Forest crystal evidence",nullptr,nullptr);assert(w);glfwMakeContextCurrent(w);assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
 {
 silic2::Map map;assert(map.loadFromFile(argc>2?argv[2]:"res/maps/starry_crystal_arena.json"));assert(map.getSpatialDiagnostics().empty());const auto document=map.toJson().toString();
 const auto scene=silic2::deriveEnvironmentScene(map);assert(scene.forestWorld&&!scene.combatWorld&&!scene.observatory);assert(scene.settings.preset==silic2::EnvironmentPreset::ForestCrystal);
 silic2::MapRenderer renderer;assert(renderer.loadMap(map));silic2::PixelRenderer pixel;assert(pixel.init(320,200));int width,height;glfwGetFramebufferSize(w,&width,&height);
 auto render=[&](glm::vec3 eye,glm::vec3 target,float seconds,const std::string& name,float fov=65.f){
  renderer.setEnvironmentTime(seconds);pixel.beginPixelRender();
  const auto view=glm::lookAt(eye,target,glm::vec3(0,1,0));
  const auto projection=glm::perspective(glm::radians(fov),1.6f,.1f,600.f);
  renderer.render(view,projection);renderer.renderTranslucent(view,projection);glFinish();
  if(!name.empty())save(out+"/"+name+"-320.ppm",320,200);
  pixel.endPixelRender(width,height);glFinish();assert(glGetError()==GL_NO_ERROR);if(!name.empty())save(out+"/"+name+".ppm",width,height);
 };
 render({0,1.6,60},{0,1.6,0},0,"01-arrival-level");auto baseline=pixels(width,height);
 render({35,1.6,65},{0,55,0},0,"02-eye-and-spire");
 render({0,1.6,38},{0,0,20},0,"03-engraved-totem");
 render({80,1.6,0},{110,12,0},0,"04-woodland-edge");
 render({0,1.6,-65},{0,35,0},0,"05-south-facing");
 render({0,1.6,60},{0,1.6,0},49,"06-phase");auto changed=pixels(width,height);assert(changed!=baseline);
 render({0,1.6,60},{0,1.6,0},49,"06-phase-repeat");assert(changed==pixels(width,height));
 renderer.resetEnvironment();render({0,1.6,60},{0,1.6,0},0,"");assert(baseline==pixels(width,height));
 render({60,36,95},{0,35,0},0,"07-concept-elevated-diagnostic");
 renderer.setRiftWarnings({{.7f,-1,-1,-1}});render({0,1.6,48},{0,0,40},0,"08-rift-warning");renderer.setRiftWarnings({{-1,-1,-1,-1}});
 assert(map.toJson().toString()==document);
 silic2::Map hollow;assert(hollow.loadFromFile("res/maps/hollow_orbit_arena.json"));assert(renderer.loadMap(hollow));render({0,1.6,13},{0,3,0},0,"hollow");
 silic2::Map legacy;assert(legacy.loadFromFile("res/maps/crystal_megastructure_arena.json"));assert(renderer.loadMap(legacy));assert(silic2::deriveEnvironmentScene(legacy).combatWorld);render({0,1.6,18},{0,3,0},0,"legacy-crystal");
 assert(renderer.loadMap(map));render({0,1.6,60},{0,1.6,0},0,"");assert(baseline==pixels(width,height));
 std::cout<<"PASS forest actual GL shader links, 320x200 views, phase/freeze/reset, legacy/Hollow switching, immutable Map; environment tree representation is batched, actual mesh stage is map metadata.\n";
 }
 silic2::TextureManager::getInstance().clear();glfwDestroyWindow(w);glfwTerminate();
}
