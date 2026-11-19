// Hidden-context specimen sheet. Each cell renders the actual world-character path;
// the additional close view exercises the Censer interrupt progress at identical poses.
#include "character/character_presentation.h"
#include "enemy/crystal_combat.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace silic2;
namespace {
constexpr int W=960,H=600;
void require(bool ok,const char* reason) { if(!ok) throw std::runtime_error(reason); }
void writeImage(const std::string& path,int width,int height,const std::vector<unsigned char>& rgb) {
    std::ofstream file(path,std::ios::binary);
    file<<"P6\n"<<width<<" "<<height<<"\n255\n";
    for(int y=height-1;y>=0;--y)
        file.write(reinterpret_cast<const char*>(rgb.data()+y*width*3),width*3);
    require(file.good(),"write specimen");
}

void checkMinions(CharacterPresentation& models,const std::string& dir) {
    const auto projection=glm::perspective(glm::radians(32.f),float(W)/H,.1f,100.f);
    const auto view=glm::lookAt(glm::vec3(.95f,1.43f,3.28f),glm::vec3(0,.9f,0),glm::vec3(0,1,0));
    const auto lowView=glm::lookAt(glm::vec3(.53f,1.12f,2.43f),glm::vec3(0,.48f,0),glm::vec3(0,1,0));
    const auto crowdView=glm::lookAt(glm::vec3(0,3.6f,7.8f),glm::vec3(0,.7f,-2.4f),glm::vec3(0,1,0));
    GLuint query=0;
    glGenQueries(1,&query);
    std::vector<unsigned char> rgb(W*H*3);
    auto capture=[&](const auto& camera,const std::vector<CharacterView>& characters,const std::string& file) {
        glDepthMask(GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glBeginQuery(GL_PRIMITIVES_GENERATED,query);
        models.renderWorld(camera,projection,characters);
        glEndQuery(GL_PRIMITIVES_GENERATED);
        GLuint triangles=0;glGetQueryObjectuiv(query,GL_QUERY_RESULT,&triangles);
        glReadPixels(0,0,W,H,GL_RGB,GL_UNSIGNED_BYTE,rgb.data());
        writeImage(dir+"/"+file+".ppm",W,H,rgb);
        require(glGetError()==GL_NO_ERROR,"minion draw/readback");
        return triangles;
    };
    auto amber=[&] {
        int pixels=0;
        for(size_t i=0;i<rgb.size();i+=3)
            pixels+=rgb[i]>150 && rgb[i]>rgb[i+1]*1.48f && rgb[i+1]>rgb[i+2]*1.8f;
        return pixels;
    };
    CharacterView chaser;chaser.crystalMinion=true;
    chaser.movement=1;chaser.phase=1.57f;
    const auto chaserTriangles=capture(view,{chaser},"chaser-stride");
    const auto firstStep=rgb;
    chaser.phase=-1.57f;
    capture(view,{chaser},"chaser-counterstep");
    require(firstStep!=rgb,"opposed crystal chaser footfalls");
    chaser.crystalMinion=false;
    capture(view,{chaser},"classic-chaser");
    require(amber()==0 && firstStep!=rgb,"classic chaser retains its separate presentation");
    chaser.crystalMinion=true;
    CharacterView sentry;sentry.type=EnemyType::Sentry;sentry.crystalMinion=true;
    const auto sentryTriangles=capture(lowView,{sentry},"sentry-idle");
    const int idle=amber();
    sentry.charging=true;sentry.chargeProgress=.54f;
    capture(lowView,{sentry},"sentry-tracking");
    const int tracking=amber();
    sentry.chargeProgress=.96f;sentry.aimLocked=true;
    capture(lowView,{sentry},"sentry-locked");
    const int locked=amber();
    sentry.hitFlash=1;
    capture(lowView,{sentry},"sentry-hit-locked");
    const int hitLocked=amber();
    require(idle<tracking && tracking<locked && hitLocked>=locked*.92f,"sentry aperture and warm tell survive hit flash");
    std::vector<CharacterView> crowd;
    for(int i=0;i<100;++i) {
        const int row=i/10,column=i%10;
        const bool isSentry=(i%10+row)%5==0;
        auto c=isSentry?sentry:chaser;
        const float scatter=std::sin(i*2.19f);
        c.position={(column-4.5f)*1.16f+scatter*.36f,0,-row*1.45f+std::cos(i*.7f)*.48f};
        c.heading=glm::normalize(glm::vec3(-c.position.x,0,8.f-c.position.z));
        c.phase=i*1.17f;c.hitFlash=0;
        c.charging=c.aimLocked=isSentry && i%2==0;
        crowd.push_back(c);
    }
    const auto crowdTriangles=capture(crowdView,crowd,"minion-crowd");
    require(chaserTriangles>50 && sentryTriangles>50 && crowdTriangles==80*chaserTriangles+20*sentryTriangles,
            "all 100 instances reach the geometry pipeline");
    std::cout<<"Crystal minion triangles (chaser/sentry/100 mixed): "<<chaserTriangles<<"/"<<sentryTriangles
             <<"/"<<crowdTriangles<<"; idle/tracking/lock/hit amber pixels: "<<idle<<"/"<<tracking<<"/"<<locked<<"/"<<hitLocked<<"\n";
    glDeleteQueries(1,&query);
}
}
int main(int argc,char** argv) {
    require(argc==2,"usage: crystal_creature_check OUTPUT_DIR");
    require(glfwInit(),"GLFW init");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
    auto w=glfwCreateWindow(W,H,"Crystal creature poses",nullptr,nullptr);
    require(w,"GLFW window"); glfwMakeContextCurrent(w);
    require(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress),"GL loader");
    GLuint fbo,texture,depth;
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,W,H,0,GL_RGB,GL_UNSIGNED_BYTE,nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
    glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,W,H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
    require(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"preview target");
    glViewport(0,0,W,H);glClearColor(.13f,.13f,.17f,1);
    CharacterPresentation models;
    require(models.init(),"model init");
    const std::array<EnemyType,3> types{{EnemyType::ForkHound,EnemyType::VeilRay,EnemyType::HangingCenser}};
    const std::array<AttackPhase,4> phases{{AttackPhase::Approach,AttackPhase::Windup,AttackPhase::Attack,AttackPhase::Recovery}};
    std::vector<unsigned char> pixels(W*H*3),first;
    std::array<int,3> marks{};
    const auto projection=glm::perspective(glm::radians(35.f),float(W)/H,.1f,90.f);
    for(int row=0;row<3;++row) for(int frame=0;frame<(row==2?7:4);++frame) {
        CharacterView c;
        c.type=types[row]; c.heading={0,0,1};c.position={0,0,0};
        c.attackPhase=phases[std::min(3,frame)];c.attackProgress=.56f;c.phase=.24f;c.movement=.85f;
        if(row==2 && frame>=4) {c.attackPhase=AttackPhase::Windup;c.interruptHits=frame-4;c.hitFlash=1;}
        if(row==2 && frame==2) {c.attackPhase=AttackPhase::Recovery;c.attackProgress=.10f;}
        if(row==2 && frame==3) {c.attackPhase=AttackPhase::Interrupted;c.interruptHits=crystalCombat::GlandHitLimit;}
        c.attackStart={-14,-30,7};c.attackEnd={-1,-30,7};
        const glm::vec3 focus(0,row==2?1.12f:row==0?.66f:.33f,0);
        const glm::vec3 eyeOffset=row==0?glm::vec3(3.9f,.72f,2.3f):
                                  row==1?glm::vec3(.95f,-2.4f,5.f):glm::vec3(1.2f,-1.5f,4.1f);
        const float scale=crystalCombat::bodyScale(c.type);
        const auto view=glm::lookAt((focus+eyeOffset)*scale,focus*scale,glm::vec3(0,1,0));
        glDepthMask(GL_TRUE);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);glDisable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);glDepthMask(GL_FALSE);glDepthFunc(GL_GEQUAL);
        models.renderWorld(view,projection,{c});
        require(glIsEnabled(GL_BLEND)&&!glIsEnabled(GL_DEPTH_TEST)&&glIsEnabled(GL_CULL_FACE),"world flags");
        GLboolean mask;glGetBooleanv(GL_DEPTH_WRITEMASK,&mask);require(!mask,"depth mask");
        GLint fn;glGetIntegerv(GL_DEPTH_FUNC,&fn);require(fn==GL_GEQUAL,"depth function");
        glReadPixels(0,0,W,H,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
        int specimen=0,amber=0;
        for(size_t i=0;i<pixels.size();i+=3) {
            specimen+=pixels[i]>65 || pixels[i+2]>72;
            amber+=pixels[i]>140&&pixels[i]>1.25f*pixels[i+1]&&pixels[i+1]>1.5f*pixels[i+2];
        }
        require(specimen>3000,"visible model");
        if(frame==0) first=pixels;
        if(frame==1) require(first!=pixels,"windup changes pose");
        if(row==2 && frame>=4) marks[frame-4]=amber;
        writeImage(std::string(argv[1])+"/"+std::to_string(row)+"-"+std::to_string(frame)+".ppm",W,H,pixels);
        require(glGetError()==GL_NO_ERROR,"OpenGL error");
    }
    std::cout<<"Censer warm pixels, hits 0/1/2: "<<marks[0]<<' '<<marks[1]<<' '<<marks[2]<<"\n";
    require(marks[0]>marks[1]&&marks[1]>marks[2],"three visible extinguishing seams");
    // Use one orthographic camera and the depth silhouette to measure actual
    // world-space dimensions, independent of lighting or preview framing.
    constexpr float halfWidth=11.f;
    constexpr float halfHeight=halfWidth*H/W;
    const auto comparisonProjection=glm::ortho(-halfWidth,halfWidth,-halfHeight,halfHeight,.1f,70.f);
    const auto comparisonView=glm::lookAt(glm::vec3(0,4,22),glm::vec3(0,4,0),glm::vec3(0,1,0));
    std::vector<CharacterView> lineup(3);
    for(size_t i=0;i<lineup.size();++i) lineup[i].type=types[i];
    lineup[0].position.x=-8.5f;lineup[0].heading={1,0,0};
    lineup[1].position.x=-1.8f;
    lineup[2].position.x=7.5f;
    glDepthMask(GL_TRUE);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    models.renderWorld(comparisonView,comparisonProjection,lineup);
    std::vector<float> depths(W*H);
    glReadPixels(0,0,W,H,GL_DEPTH_COMPONENT,GL_FLOAT,depths.data());
    std::array<float,3> silhouettes{};
    for(int i=0;i<3;++i) {
        const int left= i==0?0:i==1?185:637;
        const int right=i==0?185:i==1?637:W;
        int xmin=right,xmax=left,ymin=H,ymax=-1;
        for(int x=left;x<right;++x) for(int y=0;y<H;++y) if(depths[y*W+x]<1.f) {
            xmin=std::min(xmin,x);xmax=std::max(xmax,x);ymin=std::min(ymin,y);ymax=std::max(ymax,y);
        }
        require(xmin>left && xmax<right-1 && ymin>0 && ymax<H-1 && xmin<=xmax,"one complete animal per column");
        silhouettes[i]=(i==2 ? ymax-ymin+1 : xmax-xmin+1)*(2*halfWidth/W);
    }
    std::cout<<"Common camera silhouettes (meters), Hound length / Ray span / Censer height: "
             <<silhouettes[0]<<" / "<<silhouettes[1]<<" / "<<silhouettes[2]<<"\n";
    require(silhouettes[0]>2.45f && silhouettes[0]<2.9f,"unchanged hound");
    require(silhouettes[1]>8.45f && silhouettes[1]<8.95f,"scaled ray span");
    require(silhouettes[2]>7.95f && silhouettes[2]<8.65f,"scaled censer height");
    glReadPixels(0,0,W,H,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    writeImage(std::string(argv[1])+"/same-scale.ppm",W,H,pixels);
    // At the same locked coordinates the just-passed strip must go dark. Count
    // warm pixels around each actual barrier from an oblique player camera.
    CharacterView ray;ray.type=EnemyType::VeilRay;ray.position={-7,5,0};ray.heading={1,0,0};
    ray.attackStart={-7,.04f,0};ray.attackEnd={7,.04f,0};
    const auto wide=glm::lookAt(glm::vec3(0,5,14),glm::vec3(0,1,0),glm::vec3(0,1,0));
    auto warmSide=[&](int x0,int x1) {
        int count=0;
        for(int y=0;y<H/2;++y) for(int x=x0;x<x1;++x) {
            const int i=(W*y+x)*3;
            if(pixels[i]>170 && pixels[i]>pixels[i+1]*1.3f && pixels[i+1]>pixels[i+2]*1.8f) ++count;
        }
        return count;
    };
    for(int frame=0;frame<4;++frame) {
        glDepthMask(GL_TRUE);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        ray.attackPhase=frame==0?AttackPhase::Windup:frame==1?AttackPhase::Attack:AttackPhase::Recovery;
        ray.attackProgress=.52f;
        if(frame==1) ray.position={.28f,5,0};
        if(frame<3) models.renderWorld(wide,projection,{ray});
        else {
            auto targets=crystalCombat::volleyTargets({0,.04f,0});
            models.renderImpactWarnings(wide,projection,{targets.begin(),targets.end()});
        }
        glReadPixels(0,0,W,H,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
        int left=warmSide(20,W/2-90),right=warmSide(W/2+90,W-20);
        if(frame==0) require(left>100 && right>100,"full sweep forecast");
        else if(frame==1) require(left==0 && right>100,"passed section safe");
        else if(frame==2) require(left==0 && right==0,"sweep retired");
        else require(left+right>100,"detached impact warnings");
        writeImage(std::string(argv[1])+"/ground-"+std::to_string(frame)+".ppm",W,H,pixels);
    }
    require(glGetError()==GL_NO_ERROR,"ground cue GL errors");
    std::cout<<"4 ground/sweep/landing transitions passed\n";
    checkMinions(models,argv[1]);
    models.shutdown();glDeleteRenderbuffers(1,&depth);glDeleteTextures(1,&texture);glDeleteFramebuffers(1,&fbo);
    glfwDestroyWindow(w);glfwTerminate();std::cout<<"15 inspected poses with state checks\n";
}
