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
static std::vector<unsigned char> pixels(int w,int h){
    std::vector<unsigned char> p(w*h*3);glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,p.data());return p;
}
static void save(const std::string& path,int w,int h){
    auto p=pixels(w,h);std::ofstream file(path,std::ios::binary);
    file<<"P6\n"<<w<<" "<<h<<"\n255\n";
    for(int y=h-1;y>=0;--y)file.write(reinterpret_cast<char*>(p.data())+y*w*3,w*3);
}
int main(int argc,char** argv){
    const std::string output=argc>1?argv[1]:"artifacts/acceptance/horror/env";
    const std::string mapPath=argc>2?argv[2]:"res/maps/hollow_orbit_arena.json";
    assert(glfwInit());glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE,GL_FALSE);auto* window=glfwCreateWindow(640,360,"Environment art validation",nullptr,nullptr);
    assert(window);glfwMakeContextCurrent(window);assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));
    glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
    {
        silic2::Map map;assert(map.loadFromFile(mapPath));
        const auto document=map.toJson().toString();
        silic2::MapRenderer renderer;assert(renderer.loadMap(map));
        silic2::PixelRenderer pixel;assert(pixel.init(320,200));
        int width,height;glfwGetFramebufferSize(window,&width,&height);
        const auto view=glm::lookAt(glm::vec3(0,1.6,13),glm::vec3(0,3,0),glm::vec3(0,1,0));
        const auto projection=glm::perspective(glm::radians(65.f),1.6f,.1f,150.f);
        auto render=[&](float t,const glm::mat4& v,const glm::mat4& p){
            renderer.setEnvironmentTime(t);pixel.beginPixelRender();renderer.render(v,p);
            pixel.endPixelRender(width,height);glFinish();assert(glGetError()==GL_NO_ERROR);
        };
        const auto& settings=map.getEnvironment();
        const float mid=settings.transformationStart+settings.transformationDuration*.5f;
        const float after=settings.transformationStart+settings.transformationDuration;
        render(0,view,projection);save(output+"/phase-before.ppm",width,height);const auto before=pixels(width,height);
        render(mid,view,projection);save(output+"/phase-mid.ppm",width,height);
        render(after,view,projection);save(output+"/phase-after.ppm",width,height);
        assert(before!=pixels(width,height));
        render(after,view,projection);const auto frozen=pixels(width,height);
        render(after,view,projection);assert(frozen==pixels(width,height));
        renderer.resetEnvironment();render(0,view,projection);assert(before==pixels(width,height));
        const auto spawnView=glm::lookAt(glm::vec3(0,1.6,13),glm::vec3(0,1.6,0),glm::vec3(0,1,0));
        const auto spawnProjection=glm::perspective(glm::radians(45.f),1.6f,.1f,100.f);
        render(0,spawnView,spawnProjection);save(output+"/spawn-before.ppm",width,height);
        render(after,spawnView,spawnProjection);save(output+"/spawn-after.ppm",width,height);
        const auto overhead=glm::lookAt(glm::vec3(20,28,27),glm::vec3(0,0,0),glm::vec3(0,1,0));
        render(0,overhead,projection);save(output+"/layout.ppm",width,height);
        renderer.setRiftWarnings({{-1,.65f,-1,-1}});
        const auto riftView=glm::lookAt(glm::vec3(8,1.6,10),glm::vec3(15,1.3,0),glm::vec3(0,1,0));
        render(0,riftView,spawnProjection);save(output+"/rift-warning.ppm",width,height);
        assert(map.toJson().toString()==document);
        silic2::Map legacy;assert(legacy.loadFromFile("res/maps/test_room.json"));assert(renderer.loadMap(legacy));assert(!renderer.hasEnvironment());
        render(0,view,projection);
        assert(renderer.loadMap(map));assert(renderer.hasEnvironment());renderer.resetEnvironment();assert(renderer.getEnvironmentTime()==0);
        render(0,view,projection);assert(before==pixels(width,height));
        silic2::Map other;
        const char* otherPath=map.getEnvironmentPreset()==silic2::EnvironmentPreset::HollowOrbit?
            "res/maps/starry_crystal_arena.json":"res/maps/hollow_orbit_arena.json";
        assert(other.loadFromFile(otherPath));assert(renderer.loadMap(other));
        assert(renderer.getEnvironmentTime()==0);
        assert(renderer.getEnvironmentParticleCount()==other.getEnvironment().particleCount);
        render(0,view,projection);assert(before!=pixels(width,height));
        assert(renderer.loadMap(map));render(0,view,projection);
        assert(before==pixels(width,height)); // Cross-theme return cannot retain stale state.
        assert(map.toJson().toString()==document);
        const auto geometry=silic2::buildEnvironmentGeometry(silic2::deriveEnvironmentScene(map));
        std::cout<<"PASS actual GL, frozen/reset pixels, unchanged map, legacy and cross-theme reload. Environment vertices="<<geometry.size()/7
                 <<" triangles="<<geometry.size()/21<<" internal=320x200 output="<<width<<"x"<<height<<" phase times=0,"<<mid<<","<<after<<"\n";
    }
    glfwDestroyWindow(window);glfwTerminate();
}
