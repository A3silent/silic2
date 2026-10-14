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
 const std::string out=argc>1?argv[1]:"artifacts/acceptance/crystal-combat/env";
 assert(glfwInit());glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
 auto*w=glfwCreateWindow(640,360,"Crystal combat environment evidence",nullptr,nullptr);assert(w);glfwMakeContextCurrent(w);assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
 {
 silic2::Map map;assert(map.loadFromFile("res/maps/starry_crystal_arena.json"));assert(map.getSpatialDiagnostics().empty());const auto document=map.toJson().toString();
 silic2::MapRenderer renderer;assert(renderer.loadMap(map));silic2::PixelRenderer pixel;assert(pixel.init(320,200));int width,height;glfwGetFramebufferSize(w,&width,&height);
 auto render=[&](glm::vec3 eye,glm::vec3 target,float seconds,const std::string& name,float fov=45.f){
  renderer.setEnvironmentTime(seconds);pixel.beginPixelRender();
  renderer.render(glm::lookAt(eye,target,glm::vec3(0,1,0)),glm::perspective(glm::radians(fov),1.6f,.1f,180.f));glFinish();
  if(!name.empty())save(out+"/"+name+"-320.ppm",320,200);
  pixel.endPixelRender(width,height);glFinish();assert(glGetError()==GL_NO_ERROR);if(!name.empty())save(out+"/"+name+".ppm",width,height);
 };
 const auto& e=map.getEnvironment();auto start=map.getArenaLayout().playerStart+glm::vec3(0,1.6,0);
 render(start,{e.landmarkPosition.x,start.y,e.landmarkPosition.z},0,"00-arrival-horizontal");
 render(start,e.landmarkPosition,0,"01-ring-court");auto initial=pixels(width,height);
 render(start,e.landmarkPosition,47,"01-ring-court-mid");assert(initial!=pixels(width,height));
 render(start,e.landmarkPosition,49,"01-ring-court-after");auto transformed=pixels(width,height);assert(initial!=transformed);
 render(start,e.landmarkPosition,49,"");assert(transformed==pixels(width,height));
 renderer.resetEnvironment();render(start,e.landmarkPosition,0,"");assert(initial==pixels(width,height));
 renderer.setObservationState(1,true);render(start,e.landmarkPosition,0,"");assert(initial==pixels(width,height));
 // Camera positions are evidence viewpoints, never player movement or gameplay.
 render({0,1.6,-24},{0,3,-10},0,"02-lower-ravine");
 render({0,1.6,-8},{0,2,-24},0,"03-bridge-under");
 render({0,7.6,-15},{12,6,-7},0,"04-upper-curve");
 render({15,1.6,16},{15,6,0},0,"05-wide-ramp");
 render({-21,1.6,10},{-23,7,-5},0,"06-leaning-court");
 render({23,1.6,7},{12,5,-8},0,"07-sky-passage");
 render(start,e.landmarkPosition,0,"08-player-fov",45);
 render({38,49,45},{0,0,0},0,"09-layout",65);
 renderer.setRiftWarnings({{-1,-1,.65f,-1}});render({0,7.6,-9},{0,6.5,-15},0,"10-upper-rift-warning");renderer.setRiftWarnings({{-1,-1,-1,-1}});
 assert(map.toJson().toString()==document);
 silic2::Map hollow;assert(hollow.loadFromFile("res/maps/hollow_orbit_arena.json"));assert(renderer.loadMap(hollow));
 for(float t:{0.f,47.f,49.f}){
  renderer.setEnvironmentTime(t);pixel.beginPixelRender();renderer.render(glm::lookAt(glm::vec3(0,1.6,13),glm::vec3(0,3,0),glm::vec3(0,1,0)),glm::perspective(glm::radians(65.f),1.6f,.1f,150.f));pixel.endPixelRender(width,height);glFinish();save(out+(t==0?"/hollow-before.ppm":t==47?"/hollow-mid.ppm":"/hollow-after.ppm"),width,height);
 }
 assert(renderer.loadMap(map));render(start,e.landmarkPosition,0,"");assert(initial==pixels(width,height));
 auto scene=silic2::deriveEnvironmentScene(map);assert(scene.combatWorld&&!scene.observatory);
 auto geometry=silic2::buildEnvironmentGeometry(scene);assert(geometry==silic2::buildEnvironmentGeometry(scene));
 std::cout<<"PASS actual GL combat regions, timed transformation, frozen/reset pixels, retired observation no-op, cross-theme return, immutable Map. models="<<map.getModels().size()<<" brushes="<<map.getBrushes().size()<<" detailVertices="<<geometry.size()/7<<" detailVBO="<<geometry.size()*sizeof(float)<<" internal=320x200 output="<<width<<"x"<<height<<"\n";
 }
 silic2::TextureManager::getInstance().clear();
 glfwDestroyWindow(w);glfwTerminate();
}
