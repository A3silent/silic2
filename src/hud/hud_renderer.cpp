#include "hud/hud_renderer.h"
#include "engine/shader.h"
#include <cstddef>

namespace silic2 {
HudRenderer::HudRenderer() = default;
HudRenderer::~HudRenderer() {
    if(vao) glDeleteVertexArrays(1,&vao);
    if(vbo) glDeleteBuffers(1,&vbo);
}
bool HudRenderer::init() {
    if(shader) return true;
    shader=std::make_unique<Shader>("res/shaders/hud_batch.vert","res/shaders/hud_batch.frag");
    GLint linked=GL_FALSE;
    glGetProgramiv(shader->ID,GL_LINK_STATUS,&linked);
    if(linked!=GL_TRUE) { shader.reset();return false; }
    GLint previousVAO=0,previousBuffer=0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&previousVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&previousBuffer);
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(HudVertex),reinterpret_cast<void*>(offsetof(HudVertex,position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(HudVertex),reinterpret_cast<void*>(offsetof(HudVertex,color)));
    glBindVertexArray(previousVAO);glBindBuffer(GL_ARRAY_BUFFER,previousBuffer);
    batch.vertices.reserve(48000);
    return true;
}
void HudRenderer::render(const HudViewModel& model,int logicalWidth,int logicalHeight,int framebufferWidth,int framebufferHeight) {
    if(!shader || logicalWidth<=0 || logicalHeight<=0 || framebufferWidth<=0 || framebufferHeight<=0) return;
    buildHudGeometry(batch,model,logicalWidth,logicalHeight);
    if(batch.vertices.empty()) return;
    GLint viewport[4],program,previousVAO,previousBuffer,srcRGB,dstRGB,srcAlpha,dstAlpha,equationRGB,equationAlpha;
    glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_CURRENT_PROGRAM,&program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&previousVAO);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&previousBuffer);
    glGetIntegerv(GL_BLEND_SRC_RGB,&srcRGB);glGetIntegerv(GL_BLEND_DST_RGB,&dstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,&srcAlpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&dstAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB,&equationRGB);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&equationAlpha);
    const GLboolean depth=glIsEnabled(GL_DEPTH_TEST),blend=glIsEnabled(GL_BLEND),cull=glIsEnabled(GL_CULL_FACE),scissor=glIsEnabled(GL_SCISSOR_TEST);
    glViewport(0,0,framebufferWidth,framebufferHeight);
    glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_SCISSOR_TEST);glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    shader->use();shader->setVec2("logicalSize",static_cast<float>(logicalWidth),static_cast<float>(logicalHeight));
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,batch.vertices.size()*sizeof(HudVertex),batch.vertices.data(),GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(batch.vertices.size()));
    glBindVertexArray(previousVAO);glBindBuffer(GL_ARRAY_BUFFER,previousBuffer);glUseProgram(program);
    glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
    glBlendFuncSeparate(srcRGB,dstRGB,srcAlpha,dstAlpha);glBlendEquationSeparate(equationRGB,equationAlpha);
    if(depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if(blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if(cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if(scissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
}
} // namespace silic2
