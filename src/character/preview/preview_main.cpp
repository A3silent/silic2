#include "character/character_presentation.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <iostream>
#include <cassert>
using namespace silic2;
int main() {
    assert(glfwInit());
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
    auto* w=glfwCreateWindow(320,200,"KINETIC preview",nullptr,nullptr);
    assert(w); glfwMakeContextCurrent(w);
    assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));
    GLuint fbo, color, depth;
    glGenFramebuffers(1,&fbo); glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glGenTextures(1,&color); glBindTexture(GL_TEXTURE_2D,color);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,320,200,0,GL_RGB,GL_UNSIGNED_BYTE,nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
    glGenRenderbuffers(1,&depth); glBindRenderbuffer(GL_RENDERBUFFER,depth);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,320,200);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    CharacterPresentation p; assert(p.init());
    FirstPersonView player; player.speed=6; player.sliding=false;
    const auto projection=glm::perspective(glm::radians(45.0f),1.6f,.05f,100.f);
    const auto view=glm::lookAt(glm::vec3(0,1.3f,5),glm::vec3(0,.9f,0),glm::vec3(0,1,0));
    std::vector<CharacterView> chars(2);
    chars[0].position={-.7f,0,0}; chars[0].movement=1;
    chars[1].type=EnemyType::Sentry; chars[1].position={.7f,0,0}; chars[1].dimensions={1.2f,1,1.2f};
    std::vector<unsigned char> pixels(320*200*3);
    for(int frame=0;frame<6;++frame) {
        p.reset(); p.update(.1f,player);
        chars[1].charging=frame==1||frame==2||frame==3;
        chars[1].chargeProgress=frame==1?.45f:.85f;
        chars[1].aimLocked=frame>=2;
        chars[0].hitFlash=chars[1].hitFlash=frame==3?1.f:0.f;
        if(frame==4) { p.emit(CharacterEffect::Death,{-0.7f,.9f,0}); p.update(.13f,player); }
        if(frame==5) {player.sliding=true; p.update(.12f,player);p.onPrimaryShot();}
        glViewport(0,0,320,200); glClearColor(.025f,.035f,.075f,1);
        glDepthMask(GL_TRUE); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE); glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glDepthMask(GL_FALSE);
        glDepthFunc(GL_GEQUAL);
        p.renderWorld(view,projection,chars);
        assert(glIsEnabled(GL_CULL_FACE));assert(!glIsEnabled(GL_DEPTH_TEST));assert(glIsEnabled(GL_BLEND));
        GLint func; glGetIntegerv(GL_DEPTH_FUNC,&func);assert(func==GL_GEQUAL);
        GLboolean mask; glGetBooleanv(GL_DEPTH_WRITEMASK,&mask);assert(!mask);
        std::vector<float> depthBefore(320*200), depthAfter(320*200);
        glReadPixels(0,0,320,200,GL_DEPTH_COMPONENT,GL_FLOAT,depthBefore.data());
        p.renderFirstPerson(projection);
        glReadPixels(0,0,320,200,GL_DEPTH_COMPONENT,GL_FLOAT,depthAfter.data());
        assert(depthBefore==depthAfter);
        assert(glIsEnabled(GL_CULL_FACE));assert(!glIsEnabled(GL_DEPTH_TEST));assert(glIsEnabled(GL_BLEND));
        glGetBooleanv(GL_DEPTH_WRITEMASK,&mask);assert(!mask);
        glReadPixels(0,0,320,200,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
        std::ofstream out("/tmp/kinetic-preview-"+std::to_string(frame)+".ppm",std::ios::binary);
        out<<"P6\n320 200\n255\n";
        for(int y=199;y>=0;--y) out.write((char*)pixels.data()+y*320*3,320*3);
        assert(glGetError()==GL_NO_ERROR);
    }
    for(int i=0;i<100;++i) p.emit(CharacterEffect::Death,{0,0,0});
    assert(p.activeEffectCount()==CharacterPresentation::EffectCapacity);
    auto count=p.activeEffectCount();p.update(0,player);assert(p.activeEffectCount()==count);
    p.update(1,player);player.sliding=false;p.update(1,player);assert(p.activeEffectCount()==0);
    for(int i=0;i<50;++i){p.emit(CharacterEffect::Death,{0,0,0});p.reset();assert(p.activeEffectCount()==0);}
    p.shutdown();assert(glGetError()==GL_NO_ERROR);
    glDeleteRenderbuffers(1,&depth);glDeleteTextures(1,&color);glDeleteFramebuffers(1,&fbo);
    glfwDestroyWindow(w);glfwTerminate();
    std::cout<<"GL preview, state restoration, bounded effects, pause, expiry and 50 presentation resets passed\n";
}
