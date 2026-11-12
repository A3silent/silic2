// Short, frozen-scene diagnosis. Uses the actual renderer and records GL requests,
// not driver VRAM residency. glFinish timings include CPU submission + GPU completion.
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "engine/map.h"
#include "engine/map_renderer.h"
#include "engine/pixel_renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <set>
#include <tuple>
using namespace silic2;
namespace {
using Clock=std::chrono::steady_clock;
auto realBufferData=glad_glBufferData;auto realDeleteBuffers=glad_glDeleteBuffers;
auto realDraw=glad_glDrawElements;auto realInstances=glad_glDrawElementsInstanced;
std::unordered_map<GLuint,size_t> bytes;size_t draws=0,indices=0;
void APIENTRY bufferData(GLenum target,GLsizeiptr size,const void* data,GLenum usage){GLint id=0;glGetIntegerv(target==GL_ELEMENT_ARRAY_BUFFER?GL_ELEMENT_ARRAY_BUFFER_BINDING:GL_ARRAY_BUFFER_BINDING,&id);bytes[id]=size;realBufferData(target,size,data,usage);}
void APIENTRY deleteBuffers(GLsizei n,const GLuint* ids){for(int i=0;i<n;++i)bytes.erase(ids[i]);realDeleteBuffers(n,ids);}
void APIENTRY draw(GLenum m,GLsizei n,GLenum t,const void* p){++draws;indices+=n;realDraw(m,n,t,p);}
void APIENTRY instances(GLenum m,GLsizei n,GLenum t,const void* p,GLsizei count){++draws;indices+=size_t(n)*count;realInstances(m,n,t,p,count);}
size_t allocated(){size_t n=0;for(auto x:bytes)n+=x.second;return n;}
double ms(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::milli>(b-a).count();}
void save(const std::string& path,int w,int h){std::vector<unsigned char> p(w*h*3);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,p.data());std::ofstream f(path,std::ios::binary);f<<"P6\n"<<w<<' '<<h<<"\n255\n";for(int y=h-1;y>=0;--y)f.write((char*)p.data()+y*w*3,w*3);}
}
int main(int argc,char** argv){if(argc<3)return 2;const bool verify=argc>3 && std::string(argv[3])=="--verify";std::string out=argv[1],path=argv[2];std::filesystem::create_directories(out);if(!glfwInit())return 2;glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);glfwWindowHint(GLFW_VISIBLE,GL_FALSE);auto* window=glfwCreateWindow(640,400,"Forest short render diagnosis",nullptr,nullptr);if(!window)return 2;glfwMakeContextCurrent(window);if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))return 2;glfwSwapInterval(0);
realBufferData=glad_glBufferData;realDeleteBuffers=glad_glDeleteBuffers;realDraw=glad_glDrawElements;realInstances=glad_glDrawElementsInstanced;glad_glBufferData=bufferData;glad_glDeleteBuffers=deleteBuffers;glad_glDrawElements=draw;glad_glDrawElementsInstanced=instances;
int failed=0;std::ofstream checks(out+"/checks.jsonl");
auto check=[&](const std::string& name,bool ok){checks<<"{\"check\":\""<<name<<"\",\"passed\":"<<(ok?"true":"false")<<"}\n";if(!ok)++failed;};
std::ofstream hardware(out+"/hardware.txt");hardware<<"GL vendor: "<<glGetString(GL_VENDOR)<<"\nGL renderer: "<<glGetString(GL_RENDERER)<<"\nGL version: "<<glGetString(GL_VERSION)<<"\nVSync off, hidden serial context; map rendering only, no actor/audio/HUD.\n";
{
 Map map;if(!map.loadFromFile(path))return 2;MapRenderer renderer;renderer.loadMap(map);renderer.setEnvironmentTime(0);
 size_t pineCount=0;std::set<std::tuple<std::string,std::string,float,float,float>> groups;
 for(const auto& model:map.getModels())if(model.material=="forest_pine") {
     ++pineCount;groups.emplace(model.asset,model.texture,model.color.x,model.color.y,model.color.z);
 }
 const size_t otherBrushes=map.getBrushes().size()-pineCount;
 // The real map is intentionally used: all 264 placements pass through loadMap.
 // One opaque draw per non-pine brush/group, plus the extra translucent pass.
 const size_t drawBudget=otherBrushes+groups.size()+1;
 const size_t bufferBudget=otherBrushes*2+groups.size()*3+8;
 std::ofstream summary(out+"/measurements.jsonl"),csv(out+"/frames.csv");csv<<"width,view,frame,submit_ms,completed_ms\n";
 for(int w:{320,640}){int h=w*5/8;PixelRenderer target;if(!target.init(w,h))return 2;
 struct View{const char* name;glm::vec3 eye,at;};for(auto v:{View{"arrival",{0,1.6,60},{0,1.6,0}},View{"tree_edge",{82,1.6,0},{120,8,0}},View{"eye_spire",{35,1.6,65},{0,55,0}}}){auto view=glm::lookAt(v.eye,v.at,glm::vec3(0,1,0)),projection=glm::perspective(glm::radians(65.f),1.6f,.1f,600.f);std::vector<double> completed,submit;size_t calls=0,tris=0;
 for(int i=verify?0:-8;i<(verify?1:40);++i){glFinish();auto start=Clock::now();target.beginPixelRender();glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glDepthFunc(GL_LESS);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);draws=indices=0;renderer.render(view,projection);renderer.renderTranslucent(view,projection);auto sent=Clock::now();glFinish();auto done=Clock::now();if(i>=0){submit.push_back(ms(start,sent));completed.push_back(ms(start,done));if(!verify)csv<<w<<','<<v.name<<','<<i<<','<<submit.back()<<','<<completed.back()<<'\n';}calls=draws;tris=indices/3;}
 if(verify){check(std::string(v.name)+"_"+std::to_string(w)+"_shared_mesh_draw_budget",calls<=drawBudget);check(std::string(v.name)+"_"+std::to_string(w)+"_shared_buffer_budget",bytes.size()<=bufferBudget);}
 save(out+"/"+v.name+"-"+std::to_string(w)+".ppm",w,h);if(verify)continue;std::sort(completed.begin(),completed.end());std::sort(submit.begin(),submit.end());summary<<"{\"width\":"<<w<<",\"height\":"<<h<<",\"view\":\""<<v.name<<"\",\"frames\":40,\"submit_p50_ms\":"<<submit[20]<<",\"completed_p50_ms\":"<<completed[20]<<",\"completed_p95_ms\":"<<completed[38]<<",\"indexed_draw_calls\":"<<calls<<",\"submitted_triangles\":"<<tris<<",\"live_buffer_bytes\":"<<allocated()<<",\"live_buffers\":"<<bytes.size()<<"}\n";summary.flush();}
 }check("no_GL_errors",glGetError()==GL_NO_ERROR);
 }TextureManager::getInstance().clear();check("all_GL_buffers_released",bytes.empty());glfwDestroyWindow(window);glfwTerminate();return failed;}
