#include "character/imported_weapon.h"
#include "engine/shader.h"
#include "stb/stb_image.h"
#include <cstddef>
#include <algorithm>
#include <stdexcept>
#include <vector>
namespace silic2 {
namespace {
struct Vertex {glm::vec3 p,n;glm::vec2 uv;};
// Guard every state touched by upload, offscreen drawing and compositing.
struct GlState {
    GLint program,vao,buffer,drawFbo,readFbo,renderbuffer,viewport[4],active,texture0,unpack;
    GLint depthFunc,blendSrcRgb,blendDstRgb,blendSrcAlpha,blendDstAlpha,blendEqRgb,blendEqAlpha;
    GLboolean depth,blend,cull,scissor,depthMask,colorMask[4];
    GLfloat clearColor[4]; GLdouble clearDepth;
    GlState() {
        glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&drawFbo);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&readFbo);glGetIntegerv(GL_RENDERBUFFER_BINDING,&renderbuffer);
        glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_ACTIVE_TEXTURE,&active);
        glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture0);
        glGetIntegerv(GL_UNPACK_ALIGNMENT,&unpack);glGetIntegerv(GL_DEPTH_FUNC,&depthFunc);
        glGetIntegerv(GL_BLEND_SRC_RGB,&blendSrcRgb);glGetIntegerv(GL_BLEND_DST_RGB,&blendDstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&blendSrcAlpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&blendDstAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB,&blendEqRgb);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&blendEqAlpha);
        depth=glIsEnabled(GL_DEPTH_TEST);blend=glIsEnabled(GL_BLEND);cull=glIsEnabled(GL_CULL_FACE);scissor=glIsEnabled(GL_SCISSOR_TEST);
        glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMask);glGetBooleanv(GL_COLOR_WRITEMASK,colorMask);
        glGetFloatv(GL_COLOR_CLEAR_VALUE,clearColor);glGetDoublev(GL_DEPTH_CLEAR_VALUE,&clearDepth);
    }
    ~GlState(){
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,drawFbo);glBindFramebuffer(GL_READ_FRAMEBUFFER,readFbo);
        glBindRenderbuffer(GL_RENDERBUFFER,renderbuffer);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
        glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,buffer);glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture0);glActiveTexture(active);glPixelStorei(GL_UNPACK_ALIGNMENT,unpack);
        glDepthFunc(depthFunc);glDepthMask(depthMask);glColorMask(colorMask[0],colorMask[1],colorMask[2],colorMask[3]);
        glClearColor(clearColor[0],clearColor[1],clearColor[2],clearColor[3]);glClearDepth(clearDepth);
        glBlendFuncSeparate(blendSrcRgb,blendDstRgb,blendSrcAlpha,blendDstAlpha);glBlendEquationSeparate(blendEqRgb,blendEqAlpha);
        if(depth)glEnable(GL_DEPTH_TEST);else glDisable(GL_DEPTH_TEST);
        if(blend)glEnable(GL_BLEND);else glDisable(GL_BLEND);
        if(cull)glEnable(GL_CULL_FACE);else glDisable(GL_CULL_FACE);
        if(scissor)glEnable(GL_SCISSOR_TEST);else glDisable(GL_SCISSOR_TEST);
    }
};
bool linked(const Shader& s){GLint ok=0;glGetProgramiv(s.ID,GL_LINK_STATUS,&ok);return ok==GL_TRUE;}
}
struct ImportedWeapon::Impl {
    struct Part {GLuint vao=0,vbo=0,texture=0;GLsizei count=0;glm::vec4 color{1};bool emissive=false,bloomSource=false;};
    std::vector<Part> parts;
    std::unique_ptr<Shader> shader,overlay;
    GLuint fbo=0,color=0,depth=0,overlayVao=0,white=0;int width=0,height=0;
    glm::mat4 fit{1};glm::vec3 muzzle{0};
    bool localBloom=false,proceduralMaterial=false;
    ~Impl(){
        for(auto& p:parts){if(p.vao)glDeleteVertexArrays(1,&p.vao);if(p.vbo)glDeleteBuffers(1,&p.vbo);if(p.texture)glDeleteTextures(1,&p.texture);}
        if(white)glDeleteTextures(1,&white);
        if(fbo)glDeleteFramebuffers(1,&fbo);if(color)glDeleteTextures(1,&color);
        if(depth)glDeleteRenderbuffers(1,&depth);if(overlayVao)glDeleteVertexArrays(1,&overlayVao);
    }
    bool target(int w,int h){
        if(w<=0||h<=0||w>8192||h>8192)return false;
        glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        if(width==w&&height==h)return true;
        glBindTexture(GL_TEXTURE_2D,color);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
        glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,w,h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)return false;
        width=w;height=h;return true;
    }
};
ImportedWeapon::ImportedWeapon()=default;
ImportedWeapon::~ImportedWeapon()=default;
bool ImportedWeapon::init(const WeaponAsset& asset,std::string& status){
    GlState state;
    try {
        auto candidate=std::make_unique<Impl>();candidate->fit=asset.fit;candidate->muzzle=asset.muzzle;
        candidate->localBloom=asset.localBloom;candidate->proceduralMaterial=asset.proceduralMaterial;
        candidate->shader=std::make_unique<Shader>("res/shaders/character_imported.vert","res/shaders/character_imported.frag");
        candidate->overlay=std::make_unique<Shader>("res/shaders/character_overlay.vert","res/shaders/character_overlay.frag");
        if(!linked(*candidate->shader)||!linked(*candidate->overlay))throw std::runtime_error("imported weapon shader link failed");
        // Always bind a complete texture, including untextured glove/material
        // draws: some drivers validate samplers even behind a uniform branch.
        glGenTextures(1,&candidate->white);glBindTexture(GL_TEXTURE_2D,candidate->white);
        const unsigned char white[]={255,255,255,255};
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        size_t textureBytes=0;
        for(size_t sourceIndex=0;sourceIndex<asset.model.parts.size();++sourceIndex){
            const auto& source=asset.model.parts[sourceIndex];
            candidate->parts.emplace_back();auto& p=candidate->parts.back();p.color=source.baseColorFactor;
            p.bloomSource=std::find(asset.emissiveParts.begin(),asset.emissiveParts.end(),sourceIndex)!=asset.emissiveParts.end();
            p.emissive=p.bloomSource||sourceIndex==asset.reticlePart;
            std::vector<Vertex> vertices;vertices.reserve(source.mesh.faces.size());
            for(auto i:source.mesh.faces)vertices.push_back({source.mesh.vertices.at(i),source.mesh.normals.at(i),source.mesh.texCoords.at(i)});
            p.count=GLsizei(vertices.size());glGenVertexArrays(1,&p.vao);glGenBuffers(1,&p.vbo);
            glBindVertexArray(p.vao);glBindBuffer(GL_ARRAY_BUFFER,p.vbo);
            glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STATIC_DRAW);
            const size_t offsets[]={offsetof(Vertex,p),offsetof(Vertex,n),offsetof(Vertex,uv)};
            for(GLuint i=0;i<3;++i){glEnableVertexAttribArray(i);glVertexAttribPointer(i,i==2?2:3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsets[i]));}
            if(!source.texture.empty()){
                int w=0,h=0,c=0;
                if(!stbi_info(source.texture.c_str(),&w,&h,&c)||w<=0||h<=0||w>4096||h>4096)throw std::runtime_error("weapon texture invalid or exceeds 4096px");
                textureBytes+=size_t(w)*size_t(h)*4;
                if(textureBytes>64*1024*1024)throw std::runtime_error("weapon textures exceed 64MiB base-level budget");
                stbi_set_flip_vertically_on_load(true);
                auto* pixels=stbi_load(source.texture.c_str(),&w,&h,&c,4);
                if(!pixels)throw std::runtime_error("weapon texture decoding failed");
                glGenTextures(1,&p.texture);glBindTexture(GL_TEXTURE_2D,p.texture);glPixelStorei(GL_UNPACK_ALIGNMENT,1);
                glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);stbi_image_free(pixels);
                glGenerateMipmap(GL_TEXTURE_2D);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
            }
        }
        glGenFramebuffers(1,&candidate->fbo);glGenTextures(1,&candidate->color);glGenRenderbuffers(1,&candidate->depth);
        glGenVertexArrays(1,&candidate->overlayVao);
        if(!candidate->target(state.viewport[2],state.viewport[3]))throw std::runtime_error("weapon depth target unavailable");
        impl=std::move(candidate);status="weapon mesh loaded (static GPU cache)";return true;
    }catch(const std::exception& e){status=e.what();return false;}
}
bool ImportedWeapon::render(const glm::mat4& root,const glm::mat4& projection){
    if(!impl)return false;
    GlState state;
    if(!impl->target(state.viewport[2],state.viewport[3]))return false;
    glViewport(0,0,impl->width,impl->height);glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDepthMask(GL_TRUE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glClearColor(0,0,0,0);glClearDepth(1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    impl->shader->use();impl->shader->setMat4("projection",projection);impl->shader->setMat4("model",root*impl->fit);
    impl->shader->setInt("baseTexture",0);
    impl->shader->setBool("proceduralMaterial",impl->proceduralMaterial);
    for(const auto& p:impl->parts){
        impl->shader->setVec4("baseColor",p.color);impl->shader->setBool("textured",p.texture!=0);
        impl->shader->setBool("emissive",p.emissive);
        impl->shader->setBool("bloomSource",p.bloomSource);
        glBindTexture(GL_TEXTURE_2D,p.texture?p.texture:impl->white);glBindVertexArray(p.vao);glDrawArrays(GL_TRIANGLES,0,p.count);
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,state.drawFbo);glViewport(state.viewport[0],state.viewport[1],state.viewport[2],state.viewport[3]);
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD,GL_FUNC_ADD);glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    glColorMask(state.colorMask[0],state.colorMask[1],state.colorMask[2],state.colorMask[3]);
    if(state.scissor)glEnable(GL_SCISSOR_TEST);
    impl->overlay->use();impl->overlay->setInt("layer",0);glBindTexture(GL_TEXTURE_2D,impl->color);
    impl->overlay->setBool("localBloom",impl->localBloom);
    impl->overlay->setVec2("texelSize",1.0f/impl->width,1.0f/impl->height);
    glBindVertexArray(impl->overlayVao);glDrawArrays(GL_TRIANGLES,0,3);return true;
}
glm::vec3 ImportedWeapon::muzzlePosition(const glm::mat4& root)const{return glm::vec3(root*impl->fit*glm::vec4(impl->muzzle,1));}
glm::vec3 ImportedWeapon::muzzleDirection(const glm::mat4& root)const{return glm::normalize(glm::vec3(root*impl->fit*glm::vec4(0,0,-1,0)));}
}
