// Environment texture acceptance fixture: real renderers, synthetic fixed actors.
// Texture controls below are diagnostic GPU data only, never delivered artwork.
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "engine/map.h"
#include "engine/map_renderer.h"
#include "engine/pixel_renderer.h"
#include "engine/texture.h"
#include "character/character_presentation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <array>
using namespace silic2;
namespace {
void require(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
std::vector<unsigned char> pixels(int w,int h){std::vector<unsigned char> p(w*h*3);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,p.data());return p;}
void save(const std::string& path,int w,int h){auto p=pixels(w,h);std::ofstream f(path,std::ios::binary);f<<"P6\n"<<w<<" "<<h<<"\n255\n";for(int y=h-1;y>=0;--y)f.write(reinterpret_cast<char*>(p.data())+y*w*3,w*3);}
size_t changed(const std::vector<unsigned char>& a,const std::vector<unsigned char>& b){size_t count=0;for(size_t i=0;i<a.size();i+=3)if(a[i]!=b[i]||a[i+1]!=b[i+1]||a[i+2]!=b[i+2])++count;return count;}
struct View { const char* name;glm::vec3 eye,target; };
const View views[]={
 {"metal",{0,7.6,-15},{12,6,-7}},
 {"ceramic",{-21,1.6,10},{-23,7,-5}},
 {"shell",{-21,1.6,6},{-27,4,1}},
 {"crystal",{0,1.6,-8},{0,2,-24}}
};
const char* names[]={"metal_deck","mineral_ceramic","dark_shell","crystal_cleavage"};
const std::array<unsigned char,3> means[]={{{71,77,82}},{{171,164,151}},{{66,54,42}},{{76,105,122}}};
}
int main(int argc,char**argv){
 std::string out=argc>1?argv[1]:"artifacts/acceptance/crystal-textures/checks";
 require(glfwInit(),"glfw");glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
 auto*w=glfwCreateWindow(640,360,"Texture validation",nullptr,nullptr);require(w,"window");glfwMakeContextCurrent(w);require(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress),"GL loader");glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
 try {
  Map map;require(map.loadFromFile("res/maps/starry_crystal_arena.json"),"map");auto document=map.toJson().toString();
  MapRenderer renderer;require(renderer.loadMap(map),"renderer");PixelRenderer pixel;require(pixel.init(320,200),"pixel");CharacterPresentation characters;require(characters.init(),"characters");
  FirstPersonView player;player.feet={7,0,15};player.forward={0,0,-1};characters.update(.125f,player);
  CharacterView chaser;chaser.position={6,0,10.5};chaser.heading={0,0,1};chaser.movement=.65f;
  CharacterView sentry;sentry.type=EnemyType::Sentry;sentry.position={9,0,8.5};sentry.dimensions={1.2f,1,1.2f};sentry.heading={0,0,1};sentry.charging=true;sentry.chargeProgress=.8f;sentry.aimLocked=true;
  int width,height;glfwGetFramebufferSize(w,&width,&height);
  auto render=[&](const View& v,float t,const std::string& label,bool actors=false){
   renderer.setEnvironmentTime(t);pixel.beginPixelRender();auto view=glm::lookAt(v.eye,v.target,glm::vec3(0,1,0));auto projection=glm::perspective(glm::radians(45.f),1.6f,.1f,180.f);renderer.render(view,projection);
   if(actors){characters.renderWorld(view,projection,{chaser,sentry});characters.renderFirstPerson(projection);}
   if(!label.empty())save(out+"/"+label+"-320.ppm",320,200);
   pixel.endPixelRender(width,height);glFinish();require(glGetError()==GL_NO_ERROR,"GL error");if(!label.empty())save(out+"/"+label+".ppm",width,height);
  };
  std::array<std::shared_ptr<Texture>,4> textures;
  std::ofstream evidence(out+"/sampling.jsonl");
  for(size_t i=0;i<4;++i){
   std::string path="res/texture/crystal_combat/"+std::string(names[i])+"-64.png";textures[i]=TextureManager::getInstance().loadTexture(path);require(bool(textures[i]),"texture missing");require(textures[i]->getWidth()==64&&textures[i]->getHeight()==64,"texture dimensions");textures[i]->bind();
   GLint w0=0,h0=0;glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&w0);glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&h0);require(w0==64&&h0==64,"GPU dimensions");
   std::vector<unsigned char> original(64*64*3);glGetTexImage(GL_TEXTURE_2D,0,GL_RGB,GL_UNSIGNED_BYTE,original.data());
   renderer.setCrystalTextureMask(15);render(views[i],0,std::string(names[i])+"-sampled");auto sampled=pixels(width,height);
   std::vector<unsigned char> flat(original.size());for(size_t j=0;j<flat.size();++j)flat[j]=means[i][j%3];textures[i]->bind();glTexSubImage2D(GL_TEXTURE_2D,0,0,0,64,64,GL_RGB,GL_UNSIGNED_BYTE,flat.data());glGenerateMipmap(GL_TEXTURE_2D);
   render(views[i],0,std::string(names[i])+"-gpu-flat-control");auto control=pixels(width,height);size_t count=changed(sampled,control);require(count>0,"texture loaded but not sampled");
   textures[i]->bind();glTexSubImage2D(GL_TEXTURE_2D,0,0,0,64,64,GL_RGB,GL_UNSIGNED_BYTE,original.data());glGenerateMipmap(GL_TEXTURE_2D);render(views[i],0,"");require(sampled==pixels(width,height),"GPU texture restore");
   renderer.setCrystalTextureMask(15u & ~(1u<<i));render(views[i],0,std::string(names[i])+"-disabled");require(sampled!=pixels(width,height),"profile toggle no effect");
   evidence<<"{\"material\":\""<<names[i]<<"\",\"gpuSize\":[64,64],\"changedPixelsAgainstFlatGPUData\":"<<count<<",\"totalWindowPixels\":"<<width*height<<",\"exactRestore\":true}\n";
  }
  const View actorView{"actors",{7,1.6,15},{7,1.5,8}};
  renderer.setCrystalTextureMask(0);render(actorView,12,"actors-before",true);
  renderer.setCrystalTextureMask(15);render(actorView,12,"actors-after",true);render(actorView,49,"actors-after-shift",true);
  // Actual missing files: temporarily move only runtime copies, never sources.
  renderer.setCrystalTextureMask(0);render(views[0],0,"");const auto fallback=pixels(width,height);renderer.setCrystalTextureMask(15);
  textures.fill(nullptr);TextureManager::getInstance().clear();
  std::vector<std::pair<std::filesystem::path,std::filesystem::path>> moved;
  auto restoreFiles=[&]{for(auto& pair:moved)if(std::filesystem::exists(pair.second))std::filesystem::rename(pair.second,pair.first);};
  try {
   for(auto name:names){std::filesystem::path original="res/texture/crystal_combat/"+std::string(name)+"-64.png",hidden=original.string()+".validation-hidden";std::filesystem::rename(original,hidden);moved.push_back({original,hidden});}
   require(renderer.loadMap(map),"missing texture should not reject map");render(views[0],0,"missing-files-fallback");require(fallback==pixels(width,height),"fallback pixels differ");restoreFiles();
  } catch(...){restoreFiles();throw;}
  require(renderer.loadMap(map),"reload after restore");renderer.setCrystalTextureMask(15);render(actorView,12,"");const auto frozen=pixels(width,height);render(actorView,12,"");require(frozen==pixels(width,height),"freeze");
  require(document==map.toJson().toString(),"map mutated");
  // Short paired rendering diagnostic, not the full gameplay performance gate.
  std::ofstream csv(out+"/paired-render-times.csv");csv<<"frame,texture_mask,milliseconds\n";
  for(int i=0;i<160;++i)for(unsigned mask:{0u,15u}){renderer.setCrystalTextureMask(mask);auto start=std::chrono::steady_clock::now();render(actorView,12,"",true);auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();if(i>=20)csv<<i<<","<<mask<<","<<ms<<"\n";}
  TextureManager::getInstance().clear();
  std::cout<<"PASS four GPU-data controls and exact restore, per-material toggles, real missing-file fallback, fixed production actors, freeze and unchanged Map. runtime=4x64x64 RGB+mips, no new geometry/draw calls.\n";
 } catch(const std::exception& e){std::cerr<<e.what()<<"\n";glfwDestroyWindow(w);glfwTerminate();return 1;}
 glfwDestroyWindow(w);glfwTerminate();
}
