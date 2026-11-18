// Current procedural rifle and missing-config fallback: ADS/depth/state regression.
// Run from repo root. argv[1] is an existing output directory for PPM evidence.
#include "character/character_presentation.h"
#include "character/weapon_asset.h"
#include "character/procedural_rifle.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace silic2;
static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
namespace silic2 {
struct CharacterGeometryProbe {
    static void cpu(CharacterPresentation& p) {
        const auto rifle=buildProceduralRifle();
        require(rifle.proceduralMaterial&&rifle.localBloom,"new pure procedural material");
        require(rifle.manifestPath.empty(),"no external model manifest");
        for(const auto& part:rifle.model.parts)require(part.texture.empty(),"no external texture");
        FirstPersonView fp;fp.aimBlend=1;p.update(0,fp);
        require(p.playerState.aimBlend==1&&p.time==0,"zero-delta ADS state sync");
        p.reset();require(p.playerState.aimBlend==0,"reset clears sighted state");
    }
    static void recoil(CharacterPresentation& p){p.recoil=1;p.slideBlend=1;}
};
}
int main(int argc,char** argv) {
    const std::filesystem::path out=argc>1?argv[1]:"/tmp/silic2-ads-art";
    std::filesystem::create_directories(out);
    const auto lens=glm::perspective(glm::radians(70.f),1.6f,.05f,100.f);
    for(int i=0;i<100;++i) {
        const auto root=firstPersonWeaponRoot(i*.3f,10,true,(i%11)/10.f,(i%9)/8.f,1);
        const auto dot=lens*root*glm::vec4(0,.095f,.114f,1);
        require(std::abs(dot.x/dot.w)<1e-6f&&std::abs(dot.y/dot.w)<1e-6f,
                "ADS dot remains on camera forward through bob/slide/recoil");
    }
    require(glfwInit(),"GLFW");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
    auto* window=glfwCreateWindow(480,300,"KINETIC crystal-dot acceptance",nullptr,nullptr);require(window,"window");
    glfwMakeContextCurrent(window);require(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress),"GLAD");
    GLuint fbo,color,depth;
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,480,300,0,GL_RGB,GL_UNSIGNED_BYTE,nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
    glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,480,300);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
    require(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"world FBO");glViewport(0,0,480,300);
    for(int fallback=0;fallback<2;++fallback) {
        CharacterPresentation p;CharacterGeometryProbe::cpu(p);
        require(p.init(fallback?"/missing/handless-fallback.json":"res/config/weapon_presentation.json"),"presentation init");
        for(int frame=0;frame<4;++frame) {
            p.reset();FirstPersonView fp;fp.aimBlend=frame==0?0:frame==1?.5f:1;
            fp.speed=frame==3?10:0;fp.sliding=frame==3;p.update(.1f,fp);
            if(frame==3)CharacterGeometryProbe::recoil(p);
            glDepthMask(GL_TRUE);glClearDepth(.63);glClearColor(.055f,.075f,.10f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            // World fixture: ground checker remains visible through the optic.
            glEnable(GL_SCISSOR_TEST);
            for(int y=0;y<300;y+=30)for(int x=0;x<480;x+=30)if((x/30+y/30)%2){
                glScissor(x,y,30,30);glClearColor(.075f,.105f,.13f,1);glClear(GL_COLOR_BUFFER_BIT);
            }
            glScissor(0,0,480,300);
            glDisable(GL_DEPTH_TEST);glEnable(GL_BLEND);glEnable(GL_CULL_FACE);glDepthMask(GL_FALSE);glDepthFunc(GL_GEQUAL);
            glBlendFuncSeparate(GL_ONE,GL_ZERO,GL_ZERO,GL_ONE);glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT,GL_FUNC_ADD);
            glActiveTexture(GL_TEXTURE3);
            std::vector<float> before(480*300),after(480*300);glReadPixels(0,0,480,300,GL_DEPTH_COMPONENT,GL_FLOAT,before.data());
            p.renderFirstPerson(glm::perspective(glm::radians(32.f),1.6f,.05f,100.f));
            glReadPixels(0,0,480,300,GL_DEPTH_COMPONENT,GL_FLOAT,after.data());require(before==after,"world depth untouched");
            require(!glIsEnabled(GL_DEPTH_TEST)&&glIsEnabled(GL_BLEND)&&glIsEnabled(GL_CULL_FACE)&&glIsEnabled(GL_SCISSOR_TEST),"enable state restored");
            GLint state;glGetIntegerv(GL_ACTIVE_TEXTURE,&state);require(state==GL_TEXTURE3,"texture state restored");
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&state);require(GLuint(state)==fbo,"FBO restored");
            glGetIntegerv(GL_BLEND_EQUATION_RGB,&state);require(state==GL_FUNC_REVERSE_SUBTRACT,"blend state restored");
            std::vector<unsigned char> image(480*300*3);glReadPixels(0,0,480,300,GL_RGB,GL_UNSIGNED_BYTE,image.data());
            if(frame>=2) {
                int blue=0;double sumX=0,sumY=0;
                for(int y=146;y<154;++y)for(int x=236;x<244;++x){const auto n=(y*480+x)*3;
                    if(image[n+2]>240&&image[n+1]>160&&image[n]<130){++blue;sumX+=x+.5;sumY+=y+.5;}}
                require(blue>=2&&blue<=16,"visible compact emissive crystal-blue dot");
                require(std::abs(sumX/blue-240)<.01&&std::abs(sumY/blue-150)<.01,"rendered crystal-blue dot centered exactly");
                std::cout<<(fallback?"fallback":"actual")<<" frame="<<frame<<" blue="<<blue<<" center="<<sumX/blue<<","<<sumY/blue<<"\n";
            }
            std::ofstream f(out/((fallback?"fallback-":"actual-")+std::to_string(frame)+".ppm"),std::ios::binary);
            f<<"P6\n480 300\n255\n";for(int y=299;y>=0;--y)f.write((char*)image.data()+y*480*3,480*3);
            require(glGetError()==GL_NO_ERROR,"no GL errors");
        }
        p.shutdown();require(glGetError()==GL_NO_ERROR,"clean shutdown");
    }
    glDeleteRenderbuffers(1,&depth);glDeleteTextures(1,&color);glDeleteFramebuffers(1,&fbo);glfwDestroyWindow(window);glfwTerminate();
    std::cout<<"ADS art: 100 pose alignments, actual/fallback hip-transition-ADS-slide/recoil, state/depth, zero-delta/reset passed.\n";
}
