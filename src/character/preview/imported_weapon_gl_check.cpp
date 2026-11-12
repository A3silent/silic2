// Synthetic fixture only. Does not activate or overwrite the shipping config.
#include "character/imported_weapon.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace silic2;
void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(int argc,char** argv){
    require(glfwInit(),"GLFW init");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
    auto* window=glfwCreateWindow(320,200,"KINETIC synthetic mesh verification",nullptr,nullptr);require(window,"window");glfwMakeContextCurrent(window);
    require(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress),"GLAD");
    const std::string prefix=argc>1?argv[1]:"/tmp/kinetic-imported-synthetic-";
    const auto texturePath=std::filesystem::temp_directory_path()/"silic2-weapon-synthetic.ppm";
    {std::ofstream f(texturePath,std::ios::binary);f<<"P6\n2 2\n255\n";const unsigned char bytes[]={210,170,70,60,150,190,60,150,190,210,170,70};f.write((const char*)bytes,12);}
    GLuint fbo,color,depth;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,320,200,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
    glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,320,200);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);require(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"world framebuffer");
    glViewport(0,0,320,200);
    WeaponAsset asset;asset.fit=glm::mat4(1);asset.muzzle={0,0,-.3f};
    StaticModelPart p;p.texture=texturePath.string();p.baseColorFactor={1,1,1,1};
    // Two overlapping triangles at different depths, deliberately submitted near
    // first. A textured green/gold plane in front must hide the red rear plane.
    p.mesh.vertices={{-.15f,-.1f,0},{.15f,-.1f,0},{0,.12f,0}};
    p.mesh.normals={{0,0,1},{0,0,1},{0,0,1}};p.mesh.texCoords={{0,0},{1,0},{.5f,1}};p.mesh.faces={0,1,2};asset.model.parts.push_back(p);
    p.texture="";p.baseColorFactor={1,0,0,1};for(auto& v:p.mesh.vertices){v.x*=.3f;v.y*=.3f;v.z=-.1f;}asset.model.parts.push_back(p);
    glActiveTexture(GL_TEXTURE3);glDisable(GL_DEPTH_TEST);glEnable(GL_BLEND);glEnable(GL_CULL_FACE);glDepthMask(GL_FALSE);glDepthFunc(GL_GEQUAL);
    glBlendFuncSeparate(GL_ONE,GL_ZERO,GL_ZERO,GL_ONE);glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT,GL_FUNC_ADD);
    glEnable(GL_SCISSOR_TEST);glScissor(0,0,320,200);
    {
        ImportedWeapon renderer;std::string status;require(renderer.init(asset,status),status.c_str());
        const auto projection=glm::perspective(glm::radians(70.f),1.6f,.05f,100.f);
        for(int pose=0;pose<4;++pose){
            glDepthMask(GL_TRUE);glClearColor(.025f,.035f,.075f,1);glClearDepth(.63);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glDepthMask(GL_FALSE);
            std::vector<float> before(64000),after(64000);glReadPixels(0,0,320,200,GL_DEPTH_COMPONENT,GL_FLOAT,before.data());
            const auto root=firstPersonWeaponRoot(pose*.7f,pose?8:0,true,pose==2?1.f:0.f,pose==3?1.f:0.f);
            require(renderer.render(root,projection),"imported draw");
            glReadPixels(0,0,320,200,GL_DEPTH_COMPONENT,GL_FLOAT,after.data());require(before==after,"world depth preserved");
            require(!glIsEnabled(GL_DEPTH_TEST)&&glIsEnabled(GL_BLEND)&&glIsEnabled(GL_CULL_FACE)&&glIsEnabled(GL_SCISSOR_TEST),"enable state restored");
            GLint v;glGetIntegerv(GL_ACTIVE_TEXTURE,&v);require(v==GL_TEXTURE3,"active texture restored");glGetIntegerv(GL_DEPTH_FUNC,&v);require(v==GL_GEQUAL,"depth func restored");
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&v);require(GLuint(v)==fbo,"draw target restored");glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&v);require(GLuint(v)==fbo,"read target restored");
            glGetIntegerv(GL_BLEND_EQUATION_RGB,&v);require(v==GL_FUNC_REVERSE_SUBTRACT,"blend equation restored");glGetIntegerv(GL_BLEND_DST_ALPHA,&v);require(v==GL_ONE,"blend factors restored");
            GLboolean mask;glGetBooleanv(GL_DEPTH_WRITEMASK,&mask);require(!mask,"world write mask restored");
            std::vector<unsigned char> image(64000*3);glReadPixels(0,0,320,200,GL_RGB,GL_UNSIGNED_BYTE,image.data());
            size_t gold=0,cyan=0,red=0;for(size_t i=0;i<image.size();i+=3){if(image[i]>60&&image[i+1]>50&&image[i+2]<70)++gold;if(image[i+2]>70&&image[i+1]>60&&image[i]<60)++cyan;if(image[i]>90&&image[i+1]<20&&image[i+2]<20)++red;}
            require(gold>50&&cyan>50,"actual texture sampled");

            std::ofstream f(prefix+std::to_string(pose)+".ppm",std::ios::binary);f<<"P6\n320 200\n255\n";for(int y=199;y>=0;--y)f.write((const char*)image.data()+y*320*3,320*3);
            f.close();
            if(red)std::cerr<<"pose "<<pose<<" red="<<red<<"\n";
            require(red==0,"rear triangle rejected by private depth");
            require(glGetError()==GL_NO_ERROR,"no GL errors");
        }
        auto invalid=asset;invalid.model.parts[0].texture="/missing/synthetic-weapon-texture.png";
        require(!renderer.init(invalid,status),"bad texture rejected transactionally");
        glViewport(0,0,160,100);require(renderer.render(glm::mat4(1),projection),"private target resizes");glViewport(0,0,320,200);
    }
    for(int i=0;i<10;++i){ImportedWeapon renderer;std::string status;require(renderer.init(asset,status),"repeated GPU init");}
    require(glGetError()==GL_NO_ERROR,"clean lifecycle");
    glDeleteRenderbuffers(1,&depth);glDeleteTextures(1,&color);glDeleteFramebuffers(1,&fbo);glfwDestroyWindow(window);glfwTerminate();std::filesystem::remove(texturePath);
    std::cout<<"Synthetic textured mesh GL: four poses, actual sampled colors, world-depth and state preservation, resize, ten init/shutdown cycles passed. NOT generated rifle acceptance.\n";
}
