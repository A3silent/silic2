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
 const std::string out=argc>1?argv[1]:"artifacts/acceptance/crystal-world/env";
 assert(glfwInit());glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
 auto*w=glfwCreateWindow(640,360,"Crystal Observatory evidence",nullptr,nullptr);assert(w);glfwMakeContextCurrent(w);assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
 {
 silic2::Map map;assert(map.loadFromFile("res/maps/starry_crystal_arena.json"));const auto document=map.toJson().toString();
 silic2::MapRenderer renderer;assert(renderer.loadMap(map));silic2::PixelRenderer pixel;assert(pixel.init(320,200));int width,height;glfwGetFramebufferSize(w,&width,&height);
 float fieldOfView=65.f;
 const auto& e=map.getExploration();const auto scene=silic2::deriveEnvironmentScene(map);
 auto render=[&](glm::vec3 eye,glm::vec3 target,float progress,bool complete,const std::string& name){
  renderer.setEnvironmentTime(12);renderer.setObservationState(progress,complete);pixel.beginPixelRender();
  renderer.render(glm::lookAt(eye,target,glm::vec3(0,1,0)),glm::perspective(glm::radians(fieldOfView),1.6f,.1f,180.f));glFinish();
  if(!name.empty())save(out+"/"+name+"-320.ppm",320,200);
  pixel.endPixelRender(width,height);glFinish();assert(glGetError()==GL_NO_ERROR);if(!name.empty())save(out+"/"+name+".ppm",width,height);
 };
 const auto& route=e.guidePoints;const glm::vec3 eyeHeight(0,e.observerEyeHeight,0);
 render(route[0]+eyeHeight,route[1]+eyeHeight,0,false,"01-entry");
 render(route[2]+eyeHeight,route[3]+glm::vec3(0,2.0,0),0,false,"02-court");
 render(route[4]+eyeHeight,route[5]+glm::vec3(0,2.0,0),0,false,"03-gallery");
 for(const auto& z:e.zones)if(z.id=="court"){
   auto center=(z.minimum+z.maximum)*.5f;
   render({center.x,e.observerEyeHeight,center.z+5.f},{center.x,5.f,z.minimum.z},0,false,"02-court-skyline");
 }
 render(e.observerPosition+glm::vec3(0,e.observerEyeHeight,-7),e.observerPosition+glm::vec3(0,.3,0),0,false,"04-approach");
 render(scene.observerEye,e.target,0,false,"04-aligned");const auto initial=pixels(width,height);
 render(scene.observerEye+glm::vec3(0,0,3),e.target,0,false,"04-offset");assert(initial!=pixels(width,height));
 render(scene.observerEye,e.target,.55f,false,"04-progress");assert(initial!=pixels(width,height));
 render(scene.observerEye,e.target,1,true,"04-calibrated");const auto completed=pixels(width,height);assert(initial!=completed);
 render(scene.observerEye,e.target,1,true,"");assert(completed==pixels(width,height));
 renderer.resetEnvironment();render(scene.observerEye,e.target,0,false,"");assert(initial==pixels(width,height));
 render(e.landmarkPosition+glm::vec3(6,e.observerEyeHeight-e.landmarkPosition.y,2),e.landmarkPosition,1,true,"05-well");
 render(scene.exitCenter+glm::vec3(0,e.observerEyeHeight-scene.exitCenter.y,-8),route.back()+eyeHeight,1,true,"06-exit");
 render(route.back()+eyeHeight,route.front()+eyeHeight,1,true,"07-return-view");
 render({35,45,40},{0,0,0},0,false,"08-layout");
 fieldOfView=45.f;
 render(scene.observerEye,e.target,0,false,"09-player-fov-aligned");
 render(route[0]+eyeHeight,route[1]+eyeHeight,0,false,"09-player-fov-entry");
 fieldOfView=65.f;
 assert(document==map.toJson().toString());
 silic2::Map hollow;assert(hollow.loadFromFile("res/maps/hollow_orbit_arena.json"));assert(renderer.loadMap(hollow));renderer.setObservationState(1,true);
 renderer.setEnvironmentTime(0);pixel.beginPixelRender();renderer.render(glm::lookAt(glm::vec3(0,1.6,13),glm::vec3(0,3,0),glm::vec3(0,1,0)),glm::perspective(glm::radians(65.f),1.6f,.1f,150.f));pixel.endPixelRender(width,height);glFinish();save(out+"/hollow-baseline.ppm",width,height);
 const auto hollowInitial=pixels(width,height);
 renderer.resetEnvironment();renderer.setObservationState(.5f,true);renderer.setEnvironmentTime(0);pixel.beginPixelRender();renderer.render(glm::lookAt(glm::vec3(0,1.6,13),glm::vec3(0,3,0),glm::vec3(0,1,0)),glm::perspective(glm::radians(65.f),1.6f,.1f,150.f));pixel.endPixelRender(width,height);glFinish();assert(hollowInitial==pixels(width,height));
 for(float t:{47.f,49.f}){renderer.setEnvironmentTime(t);pixel.beginPixelRender();renderer.render(glm::lookAt(glm::vec3(0,1.6,13),glm::vec3(0,3,0),glm::vec3(0,1,0)),glm::perspective(glm::radians(65.f),1.6f,.1f,150.f));pixel.endPixelRender(width,height);glFinish();save(out+(t==47.f?"/hollow-mid.ppm":"/hollow-after.ppm"),width,height);}
 assert(renderer.loadMap(map));render(scene.observerEye,e.target,0,false,"");assert(initial==pixels(width,height));
 auto geometry=silic2::buildEnvironmentGeometry(scene);assert(geometry==silic2::buildEnvironmentGeometry(scene));
 // Reproject each physical arc to the focal plane. Every vertex must belong to
 // the same inner/outer circle; an offset observer must produce parallax.
 auto forward=glm::normalize(scene.focalPoint-scene.observerEye);float distance=glm::length(scene.focalPoint-scene.observerEye);int counts[3]={0,0,0};float error=0;
 for(size_t i=0;i<geometry.size();i+=7){float tag=geometry[i+6];if(tag<10||tag>=13)continue;int piece=int(tag)-10;++counts[piece];glm::vec3 p(geometry[i],geometry[i+1],geometry[i+2]);assert(p.y>3.f);auto ray=p-scene.observerEye;auto projected=scene.observerEye+ray*(distance/glm::dot(ray,forward));float radius=glm::length(projected-scene.focalPoint);error=std::max(error,std::min(std::abs(radius-scene.ringRadius),std::abs(radius-scene.ringRadius-.22f)));}
 assert(error<.0001f && counts[0]>0&&counts[1]>0&&counts[2]>0);
 std::cout<<"PASS actual GL nodes, actual arc projection maximum error="<<error<<", frozen/reset/cross-theme pixels, serialized map unchanged. vertices="<<geometry.size()/7<<" triangles="<<geometry.size()/21<<" VBO bytes="<<geometry.size()*sizeof(float)<<" internal320x200 output="<<width<<"x"<<height<<"\n";
 }
 glfwDestroyWindow(w);glfwTerminate();
}
