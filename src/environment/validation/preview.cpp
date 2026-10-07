#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "engine/map.h"
#include "engine/map_renderer.h"
#include "engine/pixel_renderer.h"
#include "environment/environment_renderer.h"
#include <cassert>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <chrono>
#include <algorithm>
static void capture(const char* path,int w,int h){
 std::vector<unsigned char> p(w*h*3);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,p.data());
 std::ofstream f(path,std::ios::binary);f<<"P6\n"<<w<<" "<<h<<"\n255\n";
 for(int y=h-1;y>=0;--y)f.write((char*)p.data()+y*w*3,w*3);
}
int main(){
 if(!glfwInit())return 1;glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
 glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
 auto* window=glfwCreateWindow(640,360,"Environment validation",nullptr,nullptr);if(!window)return 2;glfwMakeContextCurrent(window);
 if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))return 3;
 GLuint fbo,tex,depth;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
 glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,320,200,0,GL_RGB,GL_UNSIGNED_BYTE,nullptr);
 glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);
 glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,320,200);
 glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)return 4;
 glViewport(0,0,320,200);glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
 {
 silic2::Map map;if(!map.loadFromFile("res/maps/starry_crystal_arena.json"))return 5;
 silic2::MapRenderer renderer;renderer.loadMap(map);
 auto view=glm::lookAt(glm::vec3(0,1.6,13),glm::vec3(0,2.5,0),glm::vec3(0,1,0));auto proj=glm::perspective(glm::radians(80.f),1.6f,.1f,150.f);
 renderer.setEnvironmentTime(0);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);renderer.render(view,proj);capture("/tmp/silic2_env_before.ppm",320,200);
 renderer.setEnvironmentTime(47);renderer.setRiftWarnings({{-1, .7f, -1, -1}});glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);renderer.render(view,proj);capture("/tmp/silic2_env_mid.ppm",320,200);
 renderer.setEnvironmentTime(49);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);renderer.render(view,proj);capture("/tmp/silic2_env_after.ppm",320,200);
 auto overhead=glm::lookAt(glm::vec3(20,28,27),glm::vec3(0,0,0),glm::vec3(0,1,0));
 glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);renderer.render(overhead,proj);capture("/tmp/silic2_env_layout.ppm",320,200);
 renderer.setEnvironmentParticleCount(2000);std::vector<double> ms;
 for(int i=0;i<360;++i){auto start=std::chrono::steady_clock::now();renderer.setEnvironmentTime(i/60.f);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);renderer.render(view,proj);glFinish();if(i>=60)ms.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());}
 std::sort(ms.begin(),ms.end());std::cout<<"ENV_ONLY_2000_PARTICLES p95_ms="<<ms[284]<<" p99_ms="<<ms[296]<<" glError="<<glGetError()<<"\n";
 // Authoring regression: a material edit cannot disable the preset; move
 // the actual central mesh and rift, then reload the same shared Map document.
 {
 auto document=map.toJson();
 for(size_t i=0;i<document["geometry"]["brushes"].size();++i){
     auto& brush=document["geometry"]["brushes"][i];
     if(brush.getNumber("id",0)==1)brush["material"]=silic2::SimpleJson("plain_floor");
     if(brush.getNumber("id",0)==6){
         auto& vertices=brush["vertices"];
         for(size_t v=0;v<vertices.size();v+=3){
             vertices[v]=silic2::SimpleJson(vertices[v].getNumber()+2);
             vertices[v+1]=silic2::SimpleJson(vertices[v+1].getNumber()*1.1);
         }
     }
 }
 for(size_t i=0;i<document["geometry"]["objects"].size();++i){
     auto& object=document["geometry"]["objects"][i];
     if(object.getNumber("id",0)==1)object["material"]=silic2::SimpleJson("plain_floor");
 }
 for(size_t i=0;i<document["entities"].size();++i){
     auto& entity=document["entities"][i];
     if(entity.getString("type","")=="rift" && entity.getNumber("slot",-1)==1){
         entity["position"]=silic2::SimpleJson::parse("[12,0,6]");
         entity["rotation"]=silic2::SimpleJson::parse("[0,35,0]");
         entity["scale"]=silic2::SimpleJson::parse("[2,1.5,0.8]");
     }
 }
 silic2::Map edited;
 if(!edited.loadFromJson(document)){std::cerr<<edited.getLastError();return 8;}
 renderer.loadMap(edited);assert(renderer.hasEnvironment());
 renderer.setRiftWarnings({{-1,.65f,-1,-1}});
 glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);renderer.render(overhead,proj);
 capture("/tmp/silic2_env_map_edit.ppm",320,200);
 bool aligned=false;
 for(const auto& light:renderer.getCombinedLights())
     if(glm::length(light.position-glm::vec3(12,1.5,6))<.001f)aligned=true;
 assert(aligned && glGetError()==GL_NO_ERROR);
 silic2::Map skyOnly;
 assert(skyOnly.loadFromJson(silic2::SimpleJson::parse(R"({"schemaVersion":2,"gameMode":"legacy","environmentPreset":"starry_crystal"})")));
 renderer.loadMap(skyOnly);assert(renderer.hasEnvironment());
 glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);renderer.render(view,proj);
 capture("/tmp/silic2_env_sky_only.ppm",320,200);
 assert(glGetError()==GL_NO_ERROR);
 renderer.loadMap(map);
 std::cout<<"PASS: actual GL material-independent preset, moved/scaled mesh+rift+warning light, sky-only reload\n";
 }
 // Simulation pause/render determinism and reset: identical time produces identical pixels.
 auto pixels = [] { std::vector<unsigned char> p(320*200*3);glReadPixels(0,0,320,200,GL_RGB,GL_UNSIGNED_BYTE,p.data());return p; };
 renderer.resetEnvironment();renderer.setEnvironmentParticleCount(256);
 glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);renderer.render(view,proj);const auto reset=pixels();
 glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);renderer.render(view,proj);assert(reset==pixels());
 renderer.setEnvironmentTime(60);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);renderer.render(view,proj);assert(reset!=pixels());
 renderer.resetEnvironment();glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);renderer.render(view,proj);assert(reset==pixels());
 {
 silic2::EnvironmentRenderer environment(silic2::deriveEnvironmentScene(map));
 assert(environment.transformation()==0);environment.setTime(45);assert(environment.transformation()==0);
 environment.setTime(47);assert(environment.transformation()==.5f);environment.setTime(49);assert(environment.transformation()==1);
 environment.reset();glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);environment.renderSky(view,proj);auto sky=pixels();
 auto translatedView=view;translatedView[3]=glm::vec4(3,8,-21,1);
 glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);environment.renderSky(translatedView,proj);assert(sky==pixels());
 }
 std::cout<<"PASS: frozen time, reset pixels, 45/47/49 transformation, world-anchored sky\n";
 {
 silic2::PixelRenderer pixel;assert(pixel.init(320,200));int fw,fh;glfwGetFramebufferSize(window,&fw,&fh);
 const auto spawnView=glm::lookAt(glm::vec3(0,1.6,13),glm::vec3(0,1.6,0),glm::vec3(0,1,0));
 const auto spawnProj=glm::perspective(glm::radians(45.f),320.f/200.f,.1f,100.f);
 renderer.resetEnvironment();pixel.beginPixelRender();renderer.render(spawnView,spawnProj);pixel.endPixelRender(fw,fh);capture("/tmp/silic2_env_post_before.ppm",fw,fh);
 renderer.setEnvironmentTime(49);renderer.setRiftWarnings({{.5f,-1,-1,-1}});pixel.beginPixelRender();renderer.render(spawnView,spawnProj);pixel.endPixelRender(fw,fh);capture("/tmp/silic2_env_post_after.ppm",fw,fh);
 renderer.setRiftWarnings({{-1,.65f,-1,-1}});
 const auto riftView=glm::lookAt(glm::vec3(8,1.6,10),glm::vec3(15,1.3,0),glm::vec3(0,1,0));
 pixel.beginPixelRender();renderer.render(riftView,spawnProj);pixel.endPixelRender(fw,fh);capture("/tmp/silic2_env_rift_warning.ppm",fw,fh);
 std::cout<<"POSTPROCESS_FRAMEBUFFER "<<fw<<"x"<<fh<<" internal=320x200 FOV=45\n";
 }
 glBindFramebuffer(GL_FRAMEBUFFER,fbo);glViewport(0,0,320,200);
 silic2::Map legacy;if(!legacy.loadFromFile("res/maps/test_room.json"))return 6;renderer.loadMap(legacy);if(renderer.hasEnvironment())return 7;
 renderer.setEnvironmentTime(80);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);renderer.render(view,proj);capture("/tmp/silic2_env_legacy.ppm",320,200);
 renderer.loadMap(map);renderer.resetEnvironment();std::cout<<"RELOAD_RESET glError="<<glGetError()<<"\n";
 }
 glDeleteRenderbuffers(1,&depth);glDeleteTextures(1,&tex);glDeleteFramebuffers(1,&fbo);glfwDestroyWindow(window);glfwTerminate();
}
