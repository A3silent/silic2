// Review a real imported bundle from four angles, without changing the shipping
// first-person configuration. argv: bundle.import.json output-prefix.
#include "character/imported_weapon.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace silic2;
void require(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
int main(int argc,char** argv){
    require(argc==3,"usage: weapon-turntable bundle.import.json output-prefix");
    WeaponAsset asset;asset.model=loadStaticModelBundle(argv[1]);
    require(glfwInit(),"GLFW");glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
    auto* window=glfwCreateWindow(640,400,"Actual imported weapon turntable",nullptr,nullptr);require(window,"window");glfwMakeContextCurrent(window);
    require(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress),"GLAD");
    int w=0,h=0;glfwGetFramebufferSize(window,&w,&h);glViewport(0,0,w,h);
    {
        ImportedWeapon renderer;std::string status;require(renderer.init(asset,status),status.c_str());
        for(int frame=0;frame<4;++frame){
            glDepthMask(GL_TRUE);glClearColor(.035f,.045f,.055f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            auto root=glm::translate(glm::mat4(1),glm::vec3(0,0,-1.1f));
            root=glm::rotate(root,glm::radians(-10.f),glm::vec3(1,0,0));
            root=glm::rotate(root,glm::radians(45.f+90.f*frame),glm::vec3(0,1,0));
            require(renderer.render(root,glm::perspective(glm::radians(50.f),float(w)/h,.05f,10.f)),"render");
            std::vector<unsigned char> pixels(w*h*3);glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
            std::ofstream out(std::string(argv[2])+std::to_string(frame)+".ppm",std::ios::binary);out<<"P6\n"<<w<<' '<<h<<"\n255\n";
            for(int y=h-1;y>=0;--y)out.write((char*)pixels.data()+y*w*3,w*3);
            require(glGetError()==GL_NO_ERROR,"GL errors");
        }
    }
    glfwDestroyWindow(window);glfwTerminate();std::cout<<"Actual bundle four-angle turntable completed.\n";
}
