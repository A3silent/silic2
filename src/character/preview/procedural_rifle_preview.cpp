// Render actual code-generated Crimson Carapace; no game art files are loaded.
#include "character/procedural_rifle.h"
#include "character/imported_weapon.h"
#include "character/character_presentation.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace silic2;
void require(bool b,const char* what){if(!b)throw std::runtime_error(what);}
int main(int argc,char** argv) {
    const std::filesystem::path folder=argc>1?argv[1]:"artifacts/acceptance/procedural-rifle/preview";
    std::filesystem::create_directories(folder);
    require(glfwInit(),"GLFW");glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
    auto* window=glfwCreateWindow(640,400,"Crimson Carapace actual GL art review",nullptr,nullptr);require(window,"window");glfwMakeContextCurrent(window);
    require(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress),"GLAD");
    GLuint fbo,color,depth;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,640,400,0,GL_RGB,GL_UNSIGNED_BYTE,nullptr);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
    glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,640,400);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
    require(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"FBO");glViewport(0,0,640,400);
    {
        const auto asset=buildProceduralRifle();ImportedWeapon renderer;std::string status;require(renderer.init(asset,status),status.c_str());
        size_t triangles=0;for(const auto& p:asset.model.parts)triangles+=p.mesh.faces.size()/3;
        std::cout<<"procedural triangles="<<triangles<<" parts="<<asset.model.parts.size()<<" emissions="<<asset.emissiveParts.size()<<"\n";
        for(int frame=0;frame<6;++frame) {
            const bool fpFrame=frame<3||frame==5;const int w=fpFrame?480:640,h=fpFrame?300:400;glViewport(0,0,w,h);
            glDepthMask(GL_TRUE);glClearDepth(.63);glDisable(GL_SCISSOR_TEST);glClearColor(.045f,.052f,.065f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            glEnable(GL_SCISSOR_TEST);
            for(int y=0;y<h;y+=30)for(int x=0;x<w;x+=30)if((x/30+y/30)%2){glScissor(x,y,30,30);glClearColor(.06f,.07f,.085f,1);glClear(GL_COLOR_BUFFER_BIT);}
            glScissor(0,0,w,h);
            const char* name=frame==0?"hip":frame==1?"ads":frame==2?"ads-recoil":frame==3?"side":frame==4?"three-quarter":"firing";
            auto projection=glm::perspective(glm::radians(fpFrame?70.f:45.f),1.6f,.05f,10.f);
            auto root=firstPersonWeaponRoot(frame*.4f,frame==2?10:0,true,frame==2?1:0,frame==2?1:0,frame==0?0:1);
            if(frame>=3&&frame<5){root=glm::translate(glm::mat4(1),glm::vec3(.03f,.035f,-.88f));root=glm::rotate(root,glm::radians(frame==3?90.f:56.f),glm::vec3(0,1,0));if(frame==4)root=glm::rotate(root,glm::radians(-12.f),glm::vec3(1,0,0));}
            glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glEnable(GL_CULL_FACE);glDepthFunc(GL_GEQUAL);glActiveTexture(GL_TEXTURE3);
            glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT,GL_FUNC_ADD);glBlendFuncSeparate(GL_ONE,GL_ZERO,GL_ZERO,GL_ONE);
            std::vector<float> before(w*h),after(w*h);glReadPixels(0,0,w,h,GL_DEPTH_COMPONENT,GL_FLOAT,before.data());
            if(frame==5) {
                CharacterPresentation presentation;require(presentation.init(),"firing presentation init");
                FirstPersonView fp;fp.aimBlend=1;presentation.update(.1f,fp);presentation.onPrimaryShot();
                presentation.renderFirstPerson(glm::perspective(glm::radians(32.f),1.6f,.05f,100.f));
                presentation.shutdown();
            } else require(renderer.render(root,projection),"render");glReadPixels(0,0,w,h,GL_DEPTH_COMPONENT,GL_FLOAT,after.data());require(before==after,"world depth preserved");
            GLint st;glGetIntegerv(GL_ACTIVE_TEXTURE,&st);require(st==GL_TEXTURE3,"active texture restored");glGetIntegerv(GL_BLEND_EQUATION_RGB,&st);require(st==GL_FUNC_REVERSE_SUBTRACT,"blend equation restored");
            require(glIsEnabled(GL_BLEND)&&glIsEnabled(GL_CULL_FACE)&&glIsEnabled(GL_SCISSOR_TEST)&&!glIsEnabled(GL_DEPTH_TEST),"GL enables restored");
            std::vector<unsigned char> pixels(w*h*3);glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
            if(frame==1||frame==2){int count=0;double sx=0,sy=0;for(int y=146;y<154;++y)for(int x=236;x<244;++x){size_t n=(y*w+x)*3;if(pixels[n]>240&&pixels[n+1]<25&&pixels[n+2]<15){++count;sx+=x+.5;sy+=y+.5;}}
                require(count==4&&sx/count==240&&sy/count==150,"tiny reticle remains centered without bloom");std::cout<<name<<" redDot="<<count<<" at "<<sx/count<<","<<sy/count<<"\n";}
            std::ofstream image(folder/(std::string(name)+".ppm"),std::ios::binary);image<<"P6\n"<<w<<" "<<h<<"\n255\n";for(int y=h-1;y>=0;--y)image.write((char*)pixels.data()+y*w*3,w*3);
            require(glGetError()==GL_NO_ERROR,"no GL errors");
        }
        for(int w:{160,480}) {
            const int h=w*5/8;glViewport(0,0,w,h);
            std::vector<float> before(w*h),after(w*h);glReadPixels(0,0,w,h,GL_DEPTH_COMPONENT,GL_FLOAT,before.data());
            require(renderer.render(firstPersonWeaponRoot(0,0,true,0,0,1),glm::perspective(glm::radians(70.f),1.6f,.05f,100.f)),"resized procedural target");
            glReadPixels(0,0,w,h,GL_DEPTH_COMPONENT,GL_FLOAT,after.data());require(before==after,"resize preserves world depth");
            GLint viewport[4],bound;glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&bound);
            require(viewport[2]==w&&viewport[3]==h&&GLuint(bound)==fbo,"resize restores caller viewport/FBO");
            require(glGetError()==GL_NO_ERROR,"resize GL error");std::cout<<"procedural target resize "<<w<<"x"<<h<<" passed\n";
        }
    }
    // Public presentation seam must use the same geometry with a missing config.
    {CharacterPresentation p;require(p.init("/missing/crimson-config.json"),"fallback init");p.shutdown();require(glGetError()==GL_NO_ERROR,"fallback resource lifecycle");}
    glDeleteRenderbuffers(1,&depth);glDeleteTextures(1,&color);glDeleteFramebuffers(1,&fbo);glfwDestroyWindow(window);glfwTerminate();
    std::cout<<"Crimson preview: hip/ADS/recoil/side/three-quarter, no external art, private-depth/state and missing-config init passed.\n";
}
