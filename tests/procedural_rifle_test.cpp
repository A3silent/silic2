// CPU checks of the actual procedural generator; no GL context or model files.
#include "character/procedural_rifle.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <limits>
#include <stdexcept>
using namespace silic2;
static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static bool finite(glm::vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
int main(){try{
    const WeaponAsset a=buildProceduralRifle(),b=buildProceduralRifle();
    require(a.proceduralMaterial && a.localBloom,"programmatic material and local emission enabled");
    require(a.manifestPath.empty(),"no model manifest dependency");
    require(!a.model.parts.empty()&&a.model.parts.size()<=16,"bounded material groups");
    require(a.model.parts.size()==b.model.parts.size(),"deterministic groups");
    require(a.reticlePart<a.model.parts.size(),"real reticle mesh part");
    require(!a.emissiveParts.empty(),"energy seams have emissive geometry");
    require(a.emissiveParts==b.emissiveParts && a.reticlePart==b.reticlePart,"deterministic emission selection");
    for(auto i:a.emissiveParts)require(i<a.model.parts.size()&&i!=a.reticlePart,"reticle excluded from seam bloom");
    glm::vec3 minimum(std::numeric_limits<float>::max()),maximum(-std::numeric_limits<float>::max());
    size_t triangles=0,vertices=0;
    for(size_t part=0;part<a.model.parts.size();++part){
        const auto& p=a.model.parts[part];const auto& q=b.model.parts[part];const auto& m=p.mesh;
        require(p.asset.empty()&&p.texture.empty(),"no OBJ or texture dependency");
        require(p.baseColorFactor==q.baseColorFactor,"deterministic palette");
        require(m.vertices==q.mesh.vertices && m.normals==q.mesh.normals && m.texCoords==q.mesh.texCoords && m.faces==q.mesh.faces,"deterministic geometry");
        require(!m.vertices.empty()&&!m.faces.empty()&&m.faces.size()%3==0,"nonempty indexed triangle mesh");
        require(m.normals.size()==m.vertices.size()&&m.texCoords.size()==m.vertices.size(),"complete per-vertex attributes");
        for(size_t i=0;i<m.vertices.size();++i){
            require(finite(m.vertices[i])&&finite(m.normals[i]),"finite geometry");
            require(std::isfinite(m.texCoords[i].x)&&std::isfinite(m.texCoords[i].y),"finite UV");
            require(std::abs(glm::length(m.normals[i])-1.f)<.002f,"unit normals");
            minimum=glm::min(minimum,m.vertices[i]);maximum=glm::max(maximum,m.vertices[i]);
        }
        for(size_t f=0;f<m.faces.size();f+=3){
            auto i=m.faces[f],j=m.faces[f+1],k=m.faces[f+2];
            require(i<m.vertices.size()&&j<m.vertices.size()&&k<m.vertices.size(),"indices in range");
            auto normal=glm::cross(m.vertices[j]-m.vertices[i],m.vertices[k]-m.vertices[i]);
            require(glm::length(normal)>1e-9f,"no degenerate triangles");normal=glm::normalize(normal);
            require(glm::dot(normal,m.normals[i])>0 && glm::dot(normal,m.normals[j])>0 && glm::dot(normal,m.normals[k])>0,"normals agree with winding");
        }
        triangles+=m.faces.size()/3;vertices+=m.vertices.size();
    }
    require(triangles>100&&triangles<=2500,"procedural rifle triangle budget");
    require(glm::length(minimum-a.model.boundsMin)<1e-5f&&glm::length(maximum-a.model.boundsMax)<1e-5f,"declared bounds contain exact geometry");
    require(a.model.boundsMin==b.model.boundsMin&&a.model.boundsMax==b.model.boundsMax,"deterministic bounds");
    const auto size=maximum-minimum;require(size.z>.4f&&size.z<2.f&&size.x>0&&size.x<1.f&&size.y>0&&size.y<1.f,"canonical meter-scale rifle envelope");
    require(a.fit==glm::mat4(1),"canonical geometry needs no external fitting transform");
    require(a.sightCenter==glm::vec3(0,.095f,.08f),"shared ADS sight anchor");
    const auto projection=glm::perspective(glm::radians(70.f),1.6f,.05f,100.f);
    for(int i=0;i<100;++i){
        const auto root=firstPersonWeaponRoot(i*.3f,10,true,(i%11)/10.f,(i%9)/8.f,1,a.sightCenter);
        const auto sight=projection*root*glm::vec4(a.sightCenter,1);
        require(std::abs(sight.x/sight.w)<1e-6f&&std::abs(sight.y/sight.w)<1e-6f,"sight anchor stays centered through movement/recoil");
    }
    // Exercise the same source-selection function as CharacterPresentation.
    // Fixtures are isolated in temp; never rename or remove user model files.
    namespace fs=std::filesystem;
    const auto folder=fs::temp_directory_path()/("silic2-procedural-rifle-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(folder);
    struct Cleanup {fs::path path;~Cleanup(){std::error_code ec;fs::remove_all(path,ec);}} cleanup{folder};
    auto write=[&](const char* name,const std::string& value){std::ofstream out(folder/name);out<<value;require(bool(out),"fixture write");};
    auto checkProcedural=[&](const fs::path& path){
        std::string status;const auto selected=resolveWeaponPresentation(path.string(),status);
        require(selected.proceduralMaterial&&selected.manifestPath.empty(),"default/fallback selects procedural source");
        require(selected.model.parts.size()==a.model.parts.size(),"fallback preserves full rifle");
        for(size_t i=0;i<a.model.parts.size();++i){
            const auto& part=selected.model.parts[i];
            require(part.asset.empty()&&part.texture.empty(),"resolved default/fallback has no model or texture reads");
            require(part.mesh.vertices==a.model.parts[i].mesh.vertices&&part.mesh.faces==a.model.parts[i].mesh.faces,"fallback is same generated rifle");
        }
    };
    checkProcedural("res/config/weapon_presentation.json");
    checkProcedural(folder/"missing.json");
    write("invalid.json","{invalid");checkProcedural(folder/"invalid.json");
    write("explicit-missing.json",R"({"source":"imported","schemaVersion":1,"enabled":true,"manifest":"absent.json","forward":"-Z","up":"+Y","position":[0,0,0],"rotationDegrees":[0,0,0],"scale":[1,1,1],"muzzle":[0,0,-0.425]})");
    checkProcedural(folder/"explicit-missing.json");
    write("disabled.json",R"({"source":"imported","schemaVersion":1,"enabled":false})");checkProcedural(folder/"disabled.json");
    write("part.obj","v -.1 0 -.425\nv .1 0 -.425\nv 0 .1 .425\nf 1 2 3\n");
    write("model.import.json",R"({"schemaVersion":1,"format":"silic2-static-model","bounds":{"min":[-0.1,0,-0.425],"max":[0.1,0.1,0.425]},"parts":[{"obj":"part.obj","baseColorFactor":[1,1,1,1]}]})");
    write("legacy.json",R"({"source":"imported","schemaVersion":1,"enabled":true,"manifest":"model.import.json","forward":"-Z","up":"+Y","position":[0,0,0],"rotationDegrees":[0,0,0],"scale":[0.55,0.55,0.55],"muzzle":[0,0,-0.425]})");
    std::string status;const auto legacy=resolveWeaponPresentation((folder/"legacy.json").string(),status);
    require(!legacy.proceduralMaterial&&!legacy.manifestPath.empty(),"explicit legacy import remains supported");
    require(!legacy.model.parts.empty()&&legacy.model.parts.front().mesh.faces.size()==3,"real synthetic OBJ retained by legacy selector");
    require(legacy.reticlePart<legacy.model.parts.size(),"legacy compatibility includes optic");
    std::cout<<"Procedural rifle CPU passed: parts="<<a.model.parts.size()<<" triangles="<<triangles<<" vertices="<<vertices<<"; deterministic, finite normals/winding, bounds, no model/texture dependencies, ADS anchor, default/missing/invalid/disabled selection and explicit legacy OBJ.\n";
}catch(const std::exception& e){std::cerr<<"Procedural rifle CPU FAILED: "<<e.what()<<'\n';return 1;}}
