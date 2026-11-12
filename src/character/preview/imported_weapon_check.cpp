// CPU-only optional asset/fitting contract test. Synthetic triangle is never
// installed into res/models and is not represented as a Tripo generation.
#include "character/weapon_asset.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cmath>
namespace fs=std::filesystem;
using namespace silic2;
void require(bool value,const char* what){if(!value)throw std::runtime_error(what);}
void write(const fs::path& path,const std::string& content){std::ofstream f(path);f<<content;require(bool(f),"fixture write");}
int main(){
    const fs::path dir=fs::temp_directory_path()/"silic2-weapon-contract";
    fs::create_directories(dir);
    const auto config=dir/"weapon.json";
    const std::string fit=R"("manifest":"model.import.json","forward":"-Z","up":"+Y","position":[0,0,0],"rotationDegrees":[0,0,0],"scale":[0.55,0.55,0.55],"muzzle":[0,0,-0.425])";
    write(dir/"part.obj","v -.1 0 -.425\nv .1 0 -.425\nv 0 .1 .425\nf 1 2 3\n");
    write(dir/"model.import.json",R"({"schemaVersion":1,"format":"silic2-static-model","bounds":{"min":[-0.1,0,-0.425],"max":[0.1,0.1,0.425]},"parts":[{"obj":"part.obj","baseColorFactor":[1,1,1,1]}]})");
    WeaponAsset asset;std::string status;
    require(!loadWeaponAsset((dir/"absent.json").string(),asset,status),"missing config fallback");
    write(config,"{\"schemaVersion\":1,\"enabled\":false,"+fit+"}");
    require(!loadWeaponAsset(config.string(),asset,status),"disabled fallback");
    write(config,"{\"schemaVersion\":1,\"enabled\":true,"+fit+"}");
    require(loadWeaponAsset(config.string(),asset,status),status.c_str());
    require(asset.model.parts.size()==1,"fixture loaded");
    const auto previous=asset.manifestPath;
    auto badFit=fit;badFit.replace(badFit.find("[0,0,-0.425]"),12,"[9,0,0]");
    write(config,"{\"schemaVersion\":1,\"enabled\":true,"+badFit+"}");
    require(!loadWeaponAsset(config.string(),asset,status),"invalid muzzle rejected");
    require(status.find("muzzle")!=std::string::npos,"muzzle diagnostic");
    require(asset.manifestPath==previous,"failed load transactional");
    auto wrongAxis=fit;wrongAxis.replace(wrongAxis.find("-Z"),2,"+Z");
    write(config,"{\"schemaVersion\":1,\"enabled\":true,"+wrongAxis+"}");
    require(!loadWeaponAsset(config.string(),asset,status),"wrong axis rejected");
    require(status.find("normalized")!=std::string::npos,"axis diagnostic");
    write(config,"{\"schemaVersion\":1,\"enabled\":true,"+fit+"}");
    std::string huge="v -.1 0 -.425\nv .1 0 -.425\nv 0 .1 .425\n";
    for(int i=0;i<20001;++i)huge+="f 1 2 3\n";
    write(dir/"part.obj",huge);
    require(!loadWeaponAsset(config.string(),asset,status),"triangle budget rejected");
    require(status.find("20000")!=std::string::npos,"budget diagnostic");
    fs::remove(dir/"part.obj");
    require(!loadWeaponAsset(config.string(),asset,status),"missing mesh fallback");
    for(int pose=0;pose<100;++pose){
        const auto root=firstPersonWeaponRoot(pose*.3f,8,pose%2==0,(pose%11)/10.f,(pose%7)/6.f);
        const auto muzzle=root*asset.fit*glm::vec4(asset.muzzle,1);
        require(std::isfinite(muzzle.x)&&std::isfinite(muzzle.y)&&muzzle.z<-.1f,"finite forward visual muzzle");
    }
    const auto idle=firstPersonWeaponRoot(0,0,true,0,0);
    require(glm::length(glm::vec3(idle[3])-glm::vec3(.18f,-.162f,-.55f))<1e-6f,"existing idle transform");
    fs::remove_all(dir);
    std::cout<<"Weapon CPU contract: enabled synthetic mesh, missing/disabled/invalid fallback, transactional failure, existing transform and 100 visual poses passed. Actual Tripo fitting pending.\n";
}
