// Synthetic GPU diagnostic, not generated artwork or a full scene acceptance.
// argv[1]: saved pre-change Crystal fragment shader for legacy pixel comparison.
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
void require(bool pass,const char* message){if(!pass)throw std::runtime_error(message);}
std::string read(const char* path){std::ifstream f(path);require(bool(f),"shader file");return {std::istreambuf_iterator<char>(f),{}};}
GLuint shader(GLenum type,const std::string& source){GLuint id=glCreateShader(type);const char* p=source.c_str();glShaderSource(id,1,&p,nullptr);glCompileShader(id);GLint ok;glGetShaderiv(id,GL_COMPILE_STATUS,&ok);if(!ok){char log[4096];glGetShaderInfoLog(id,sizeof(log),nullptr,log);throw std::runtime_error(log);}return id;}
GLuint program(const std::string& fragment){
 const char* vertex=R"(#version 330 core
 out vec3 FragPos; out vec3 Normal; out vec2 TexCoord; uniform vec2 testUV;
 void main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);FragPos=vec3(.4,.8,.2);Normal=vec3(0,1,0);TexCoord=testUV;})";
 GLuint v=shader(GL_VERTEX_SHADER,vertex),f=shader(GL_FRAGMENT_SHADER,fragment),p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);glDeleteShader(v);glDeleteShader(f);require(ok,"shader link");return p;
}
std::array<unsigned char,4> draw(GLuint p,int kind,bool texture,float uv,float phase){
 glUseProgram(p);
 auto i=[&](const char*n,int v){glUniform1i(glGetUniformLocation(p,n),v);};
 auto f=[&](const char*n,float v){glUniform1f(glGetUniformLocation(p,n),v);};
 auto v3=[&](const char*n,float x,float y,float z){glUniform3f(glGetUniformLocation(p,n),x,y,z);};
 i("combatWorld",1);i("materialKind",kind);i("useTexture",texture);i("texture1",0);i("lightingEnabled",0);i("regionCount",0);i("numLights",0);
 v3("objectColor",.5f,.5f,.5f);v3("ambientLight",.3f,.3f,.3f);v3("textureMean",.5f,.5f,.5f);f("textureWorldScale",1);f("textureStrength",.6f);f("phase",phase);
 glUniform2f(glGetUniformLocation(p,"testUV"),uv,uv);
 glDrawArrays(GL_TRIANGLES,0,3);glFinish();std::array<unsigned char,4> pixel{};glReadPixels(4,4,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());require(glGetError()==GL_NO_ERROR,"GL error");return pixel;
}
}
int main(int argc,char**argv){
 if(argc!=2){std::cerr<<"usage: imported_static_shader_check baseline.frag\n";return 2;}
 require(glfwInit(),"GLFW");glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);glfwWindowHint(GLFW_VISIBLE,GL_FALSE);
 auto*w=glfwCreateWindow(64,64,"Imported UV diagnostic",nullptr,nullptr);require(w,"window");glfwMakeContextCurrent(w);require(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress),"GL loader");
 int result=0;try {
 GLuint current=program(read("res/shaders/starry_crystal_map.frag")),baseline=program(read(argv[1]));
 GLuint vao,tex;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenTextures(1,&tex);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,tex);
 // Bottom-left red, top-right green: moving only authored UV must change color.
 const unsigned char rgba[]={255,0,0,255, 0,0,255,255, 0,0,255,255, 0,255,0,255};
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glViewport(0,0,8,8);
 auto red=draw(current,15,true,.25f,0),green=draw(current,15,true,.75f,0);
 require(red[0]>30&&red[1]==0&&red[2]==0,"authored UV red quadrant");require(green[1]>30&&green[0]==0&&green[2]==0,"authored UV green quadrant");
 auto fallback=draw(current,15,false,.25f,0);require(fallback==draw(current,15,false,.75f,0),"untextured fallback must not depend on UV");require(fallback[0]>30&&fallback[1]>30&&fallback[2]>30,"fallback color");
 require(red!=draw(current,15,true,.25f,1),"imported material phase response");
 int comparisons=0;for(int kind=10;kind<=14;++kind)for(bool textured:{false,true})for(float phase:{0.f,.5f,1.f}){require(draw(current,kind,textured,.25f,phase)==draw(baseline,kind,textured,.25f,phase),"legacy combat material changed");++comparisons;}
 glDeleteTextures(1,&tex);glDeleteVertexArrays(1,&vao);glDeleteProgram(current);glDeleteProgram(baseline);
 std::cout<<"PASS real GL compile/link; authored UV red/green; opaque no-texture fallback; imported phase response; "<<comparisons<<" legacy material/texture/phase comparisons pixel-exact. Synthetic diagnostic only.\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<"\n";result=1;}
 glfwDestroyWindow(w);glfwTerminate();return result;
}
