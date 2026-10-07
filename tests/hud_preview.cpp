// Standalone GL fixture, excluded from the ordinary tests unless explicitly built.
// Compile with SILIC2_HUD_PREVIEW_MAIN and link HUD, shader.cpp, glad.c and GLFW.
#ifdef SILIC2_HUD_PREVIEW_MAIN
#include "hud/hud_renderer.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace silic2;

int main(int argc,char** argv) {
    const std::string output=argc>1?argv[1]:"/tmp/silic2-hud";
    if(!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    GLFWwindow* window=glfwCreateWindow(1280,720,"HUD FIXTURE - NOT PLAYTEST",nullptr,nullptr);
    if(!window) { glfwTerminate();return 2; }
    glfwMakeContextCurrent(window);
    if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) return 3;
    int result=0;
    try {
        HudRenderer hud;
        if(!hud.init()) throw std::runtime_error("HUD shader link failed");
        HudViewModel model;
        model.hp=65;model.combatSeconds=91.25f;model.cleanup=true;model.livingEnemies=3;
        model.actualHpLoss=35;model.kills=29;model.shotsFired=60;model.shotsHit=45;
        model.terrain={{{-3,-3},{3,3}},{{-11,2},{-7,4}},{{7,-4},{11,-2}}};
        model.rifts={{{0,-16},true},{{16,0},false},{{0,16},false},{{-16,0},false}};
        model.playerXZ={0,11};model.playerForwardXZ={0,-1};
        HudEnemy enemy;enemy.worldXZ={5,-7};enemy.healthAnchor={0.4f,0.4f};
        enemy.hp=3;enemy.maxHp=6;enemy.inView=true;enemy.unobstructed=true;enemy.secondsSinceDamage=0.5f;
        model.enemies={enemy};model.hitAge=0.05f;model.damageAge=0.1f;model.damageDirectionRadians=1.1f;
        const HudPhase phases[]={HudPhase::Ready,HudPhase::Playing,HudPhase::Paused,HudPhase::Won,HudPhase::Dead};
        const char* names[]={"ready","playing","paused","won","dead"};
        for(auto size:{glm::ivec2(1280,720),glm::ivec2(800,600)}) {
            glfwSetWindowSize(window,size.x,size.y);glfwPollEvents();
            int lw,lh,fw,fh;glfwGetWindowSize(window,&lw,&lh);glfwGetFramebufferSize(window,&fw,&fh);
            std::cout<<"logical "<<lw<<'x'<<lh<<" framebuffer "<<fw<<'x'<<fh<<'\n';
            for(int i=0;i<5;++i) {
                model.phase=phases[i];model.audioStatus=i==4?HudAudioStatus::Unavailable:HudAudioStatus::Available;model.fatalSource="ENEMY_PROJECTILE";
                model.resultLockRemaining=i==4?0.6f:0;
                glViewport(0,0,fw,fh);glClearColor(0.12f,0.16f,0.25f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                glViewport(3,4,111,112);glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
                glEnable(GL_SCISSOR_TEST);glScissor(0,0,7,7);glDisable(GL_BLEND);
                hud.render(model,lw,lh,fw,fh);
                GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);
                if(viewport[0]!=3 || viewport[1]!=4 || viewport[2]!=111 || viewport[3]!=112 ||
                   !glIsEnabled(GL_DEPTH_TEST) || !glIsEnabled(GL_CULL_FACE) ||
                   !glIsEnabled(GL_SCISSOR_TEST) || glIsEnabled(GL_BLEND))
                    throw std::runtime_error("HUD did not restore GL state");
                glDisable(GL_SCISSOR_TEST);glFinish();
                if(glGetError()!=GL_NO_ERROR) throw std::runtime_error("OpenGL HUD error");
                std::vector<unsigned char> pixels(static_cast<size_t>(fw)*fh*3);
                glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,fw,fh,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
                std::ofstream image(output+"-"+names[i]+"-"+std::to_string(lw)+".ppm",std::ios::binary);
                image<<"P6\n"<<fw<<' '<<fh<<"\n255\n";
                for(int y=fh-1;y>=0;--y) image.write(reinterpret_cast<const char*>(pixels.data()+static_cast<size_t>(y)*fw*3),fw*3);
            }
        }
        // GL resources destroyed while this context is still live.
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';result=4; }
    glfwDestroyWindow(window);glfwTerminate();return result;
}
#endif
