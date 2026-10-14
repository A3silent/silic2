#include "editor_document.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <chrono>
#include <algorithm>
#include <stdexcept>

namespace silic2::editor {
namespace {
void check(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}
bool near(glm::vec3 a,glm::vec3 b){return glm::length(a-b)<.001f;}
}
int measureEditorEdits(const std::string& fixture,const std::string& directory) {
    try {
        std::filesystem::create_directories(directory);EditorDocument d;check(d.load(fixture),d.error());
        check(d.map().saveToFile(directory+"/input-map.json"),"cannot snapshot measured map");
        {std::ofstream identity(directory+"/identity.txt");identity<<"build="<<SILIC2_EDITOR_BUILD_TYPE<<"\nsource="<<std::filesystem::absolute(fixture).string()<<"\nnodes="<<d.map().getSpatialSettings().nodes.size()<<"\nmodels="<<d.map().getModels().size()<<"\n";}
        check(!d.map().getModels().empty(),"measurement requires actual model map");d.select({ObjectKind::Model,0});
        const auto original=d.map().toJson().toString();const auto p=d.selectedPosition(),size=d.selectedSize();
        std::ofstream csv(directory+"/edit-cost.csv");csv<<"operation,sample,milliseconds\n";std::vector<double> times;
        for(int i=0;i<12;++i){auto start=std::chrono::steady_clock::now();
            if(i<6)d.transform(p+glm::vec3(.05f*(i+1),0,0),size,0,100);
            else d.editSelected("Measured rotation",[&](SimpleJson& j){j["rotation"]=SimpleJson::parse("[0,"+std::to_string(i-5)+",0]");},101);
            check(d.error().empty(),d.error());double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();times.push_back(ms);csv<<(i<6?"position":"rotation")<<','<<i<<','<<ms<<'\n';
        }
        check(d.undo() && d.undo() && d.map().toJson().toString()==original,"coalesced drag history must produce two logical operations");
        std::sort(times.begin(),times.end());std::cout<<"EDITOR CPU EDIT COST nodes="<<d.map().getSpatialSettings().nodes.size()<<" models="<<d.map().getModels().size()<<" median_ms="<<times[times.size()/2]<<" max_ms="<<times.back()<<" samples="<<times.size()<<" (CPU transaction cost, not GUI frame time)\n";return 0;
    }catch(const std::exception& e){std::cerr<<"EDITOR MEASURE FAIL: "<<e.what()<<'\n';return 1;}
}
int runHybridSelfTests(const std::string& directory) {
    try {
        namespace fs=std::filesystem;fs::create_directories(directory+"/assets");fs::create_directories(directory+"/external/maps");
        const auto asset=fs::absolute(directory+"/assets/tetra.obj").string();
        {std::ofstream out(asset);out<<"v 0 0 0\nv 2 0 0\nv 0 0 2\nv 0 2 0\nf 1 2 3\nf 1 4 2\nf 2 4 3\nf 3 4 1\n";}
        EditorDocument d;d.createNew();
        d.addPrimitive("ramp",{4,1.5f,0},{6,3,12});check(d.selectedJson().getString("shape")=="ramp","ramp creation rejected: "+d.error());
        check(d.editSelected("Rotate ramp",[](SimpleJson& j){j["rotation"]=SimpleJson::parse("[0,30,0]");}),"ramp rotation failed");
        check(near(d.selectedSize(),{6,3,12}),"rotation changed local dimensions");
        auto bounds=d.selectedBounds();check(!near(bounds.max-bounds.min,d.selectedSize()),"rotated world bounds not derived from visual mesh");
        auto oldPosition=d.selectedPosition();d.transform(oldPosition+glm::vec3(1,0,0),d.selectedSize());check(near(d.selectedSize(),{6,3,12}),"nudge resized rotated ramp");
        check(d.addModel(asset,asset,{-4,0,-4}),"model import failed: "+d.error());
        check(d.selection().kind==ObjectKind::Model,"import did not select model");
        check(d.editSelected("Rotate model",[](SimpleJson& j){j["rotation"]=SimpleJson::parse("[10,35,0]");}),"model rotation failed");
        d.transform({-3.8f,1.2f,-4},{1.3f,.8f,1.6f},1);
        check(near(d.selectedPosition(),{-4,1,-4}) && near(d.selectedSize(),{1.3f,.8f,1.6f}),"position/scale snapping incorrect");
        const auto modelJson=d.selectedJson().toString();const auto modelBounds=d.selectedBounds();const auto count=d.map().toJson()["geometry"]["models"].size();
        d.duplicate(1);check(d.map().toJson()["geometry"]["models"].size()==count+1,"model duplicate rejected");
        check(d.undo() && d.selectedJson().toString()==modelJson,"model undo lost transform or selection");
        check(d.redo() && d.map().toJson()["geometry"]["models"].size()==count+1,"model redo lost clone");check(d.undo(),"restore model");
        const auto valid=d.map().toJson().toString();const auto revision=d.revision();
        check(!d.addModel(asset+".missing",asset,{0,0,0}),"missing resource accepted");
        check(d.map().toJson().toString()==valid && d.revision()==revision,"failed import damaged document/history");
        check(!d.editSelected("Invalid scale",[](SimpleJson& j){j["scale"]=SimpleJson::parse("[0,1,1]");}),"zero scale accepted");
        const auto savePath=fs::absolute(directory+"/external/maps/edited.json").string();check(d.save(savePath),"external save failed: "+d.error());
        EditorDocument reopened;check(reopened.load(savePath),"external reopen failed: "+reopened.error());reopened.select({ObjectKind::Model,0});
        check(near(reopened.selectedBounds().min,modelBounds.min) && near(reopened.selectedBounds().max,modelBounds.max),"external save transformed model");
        check(!fs::path(reopened.selectedJson().getString("asset")).is_absolute(),"shared save did not make asset relative to destination");
        reopened.transform(reopened.selectedPosition()+glm::vec3(1,0,0),reopened.selectedSize());check(reopened.dirty(),"edit after external reopen lost asset base");
        check(reopened.undo() && !reopened.dirty() && reopened.redo(),"external loaded model history failed");
        reopened.addNavNode({-2,0,0});const auto first=reopened.selectedJson().getNumber("id");reopened.addNavNode({0,0,2});const auto second=reopened.selectedJson().getNumber("id");
        check(reopened.connectNavNodes(int(first),int(second)),"nav connection rejected: "+reopened.error());
        check(reopened.map().toJson()["spatial"]["navigation"]["links"].size()==1,"nav link missing");
        reopened.erase();check(reopened.map().toJson()["spatial"]["navigation"]["links"].size()==0,"node deletion retained dangling links");
        check(reopened.undo() && reopened.map().toJson()["spatial"]["navigation"]["links"].size()==1,"node undo lost links");
        check(reopened.save(directory+"/external/second-save.json"),reopened.error());Map shared;check(shared.loadFromFile(directory+"/external/second-save.json"),"second external save lost resources");
        check(shared.getSchemaVersion()==3 && !shared.getCollisionBrushes().empty(),"hybrid data lost after shared reload");
        std::cout<<"EDITOR HYBRID SELF-TEST PASS: ramp/local transform/world bounds, OBJ import/TRS/clone, invalid asset/scale transaction, external Save As/reopen/edit, node/link/history\n";return 0;
    }catch(const std::exception& e){std::cerr<<"EDITOR HYBRID SELF-TEST FAIL: "<<e.what()<<'\n';return 1;}
}
int runEditorSelfTests(const std::string& fixture,const std::string& directory) {
    try {
        namespace fs=std::filesystem;
        fs::create_directories(directory);
        EditorDocument d;check(d.load(fixture),d.error());
        const std::string original=d.map().toJson().toString();
        check(!d.dirty(),"freshly loaded file dirty");
        check(!d.load(directory+"/missing-input.json"),"missing load accepted");
        check(d.map().toJson().toString()==original,"failed load lost current map");
        if(d.map().getGameMode()==GameMode::Exploration) {
            const auto originalSettings=d.map().toJson()["exploration"].toString();
            check(d.applyJson("Observation duration",[](SimpleJson& root){root["exploration"]["holdSeconds"]=SimpleJson(3.0);}),"exploration edit failed");
            check(d.undo() && d.map().toJson().toString()==original,"exploration undo failed");
            check(d.redo() && d.map().getExploration().holdSeconds==3,"exploration redo failed");
            check(d.undo(),"restore exploration before save");
            check(!d.applyJson("Invalid observation",[](SimpleJson& root){root["exploration"]["holdSeconds"]=SimpleJson(-1.0);}),"invalid exploration accepted");
            check(d.save(directory+"/exploration-roundtrip.json"),d.error()); Map shared;
            check(shared.loadFromFile(directory+"/exploration-roundtrip.json"),"shared exploration reload");
            check(shared.toJson()["exploration"].toString()==originalSettings,"exploration settings lost during editor roundtrip");
            check(shared.getGameMode()==GameMode::Exploration,"editor downgraded exploration");
            std::cout<<"EDITOR EXPLORATION SELF-TEST PASS: settings edit/undo/redo, invalid transaction, complete roundtrip\n";
            return 0;
        }
        if(d.map().getSpatialSettings().enabled && !d.map().getModels().empty()) {
            d.select({ObjectKind::Model,0});const auto position=d.selectedPosition(),scale=d.selectedSize();
            d.transform(position+glm::vec3(.25f,0,0),scale);check(d.dirty(),"full-map model edit failed: "+d.error());
            check(d.undo() && !d.dirty() && d.map().toJson().toString()==original,"full-map undo changed original");
            check(d.redo(),"full-map redo failed");
            check(d.save(directory+"/full-crystal.json"),d.error());Map shared;check(shared.loadFromFile(directory+"/full-crystal.json"),shared.getLastError());
            auto sameMeshes=[&](const auto& a,const auto& b){check(a.size()==b.size(),"compiled mesh count changed");for(size_t i=0;i<a.size();++i)check(a[i].id==b[i].id && a[i].vertices==b[i].vertices && a[i].faces==b[i].faces,"compiled mesh changed after external full-map save");};
            sameMeshes(d.map().getBrushes(),shared.getBrushes());sameMeshes(d.map().getCollisionBrushes(),shared.getCollisionBrushes());
            check(shared.getSpatialSettings().nodes.size()==d.map().getSpatialSettings().nodes.size() && shared.getSpatialSettings().links.size()==d.map().getSpatialSettings().links.size(),"full-map navigation changed");
            check(shared.getGameMode()==d.map().getGameMode(),"full-map mode changed");
            std::cout<<"EDITOR FULL CRYSTAL PASS: "<<shared.getModels().size()<<" models, "<<shared.getSpatialSettings().nodes.size()<<" nodes; transform/history/external save preserves every visual and collision vertex\n";return 0;
        }
        if(d.map().getGameMode()==GameMode::ArenaSurvival){
            const auto preset=d.map().getEnvironmentPreset();
            const char* other=preset==EnvironmentPreset::HollowOrbit?"starry_crystal":"hollow_orbit";
            check(d.applyJson("Switch environment",[&](SimpleJson& root){root["environmentPreset"]=SimpleJson(other);}),"editor environment switch failed");
            check(d.map().getEnvironmentPreset()!=preset,"environment did not switch");
            check(d.undo() && d.map().getEnvironmentPreset()==preset && !d.dirty(),"environment undo lost original preset/savepoint");
            check(d.redo() && d.map().getEnvironmentPreset()!=preset,"environment redo lost selected preset");
            check(d.undo() && d.map().toJson().toString()==original,"environment history changed document");
            check(!d.map().getObjects().empty(),"arena has no authored v2 objects");
            size_t cover=0;bool found=false;
            for(size_t i=0;i<d.map().getObjects().size();++i)if(d.map().getObjects()[i].name.find("cover")!=std::string::npos || d.map().getObjects()[i].name.find("Cover")!=std::string::npos){cover=i;found=true;break;}
            check(found,"arena cover fixture missing");d.select({ObjectKind::Object,cover});
            auto b=d.selectedBounds();const auto center=(b.min+b.max)*.5f;const auto size=b.max-b.min;
            d.transform(center+glm::vec3(.5f,0,0),size);check(d.dirty(),"v2 cover move rejected: "+d.error());
            check(near(d.map().getObjects()[cover].position,center+glm::vec3(.5f,0,0)),"v2 position not updated");
            d.transform(center+glm::vec3(.5f,0,0),size+glm::vec3(.5f,0,0));check(near(d.map().getObjects()[cover].size,size+glm::vec3(.5f,0,0)),"v2 resize not applied");
            check(d.undo() && d.redo(),"v2 transform history failed");
            d.duplicate(0);check(d.map().getObjects().size()>1 && d.selection().index!=cover,"v2 clone failed");
            std::set<uint32_t> ids;for(const auto& object:d.map().getBrushes())check(ids.insert(object.id).second,"v2 clone ID collision");
            check(d.save(directory+"/arena-edited.json"),d.error());Map shared;check(shared.loadFromFile(directory+"/arena-edited.json"),"game shared loader rejected arena edit");
            check(shared.getEnvironmentPreset()==preset,"editor save/reload downgraded environment preset");
            const auto valid=shared.toJson().toString();
            for(size_t i=0;i<d.map().getEntities().size();++i)if(d.map().getEntities()[i].type==EntityType::PLAYER_START){d.select({ObjectKind::Entity,i});break;}
            d.erase();check(!d.error().empty() && !d.dirty() && d.map().toJson().toString()==valid,"required arena marker deletion was not atomic");
            check(d.createArena(fixture) && d.dirty() && d.path().empty(),"arena template not ready for Save As");
            std::cout<<"EDITOR ARENA SELF-TEST PASS: v2 cover move/resize/clone, unique IDs, undo/redo, save/reload, required marker constraint, template Save As\n";
            return 0;
        }
        size_t wall=0;for(size_t i=0;i<d.map().getLegacyBrushes().size();++i)if(d.map().getLegacyBrushes()[i].surfaceType==SurfaceType::WALL){wall=i;break;}
        check(!d.map().getLegacyBrushes().empty(),"test fixture requires a legacy mesh");
        d.select({ObjectKind::Brush,wall});const auto old=d.map().getLegacyBrushes()[wall];
        auto bounds=d.selectedBounds();auto center=(bounds.min+bounds.max)*.5f,size=bounds.max-bounds.min;
        const glm::vec3 delta(2,0,1);
        d.transform(center+delta,size,0,17);
        check(d.dirty(),"move did not dirty file");
        const auto moved=d.map().getLegacyBrushes()[wall];
        check(moved.vertices.size()==old.vertices.size(),"legacy vertex count changed");
        for(size_t i=0;i<old.vertices.size();++i)check(near(moved.vertices[i],old.vertices[i]+delta),"flat legacy vertex transform corrupt");
        check(moved.texCoords==old.texCoords && moved.texture==old.texture && moved.faces==old.faces,"transform changed texture or topology");
        auto resized=size;for(int a=0;a<3;++a)if(resized[a]>.0001f)resized[a]*=1.25f;
        d.transform(center+delta,resized);
        check(near(d.selectedBounds().max-d.selectedBounds().min,resized),"legacy size edit failed");
        check(d.undo(),"legacy resize undo failed");
        check(d.save(directory+"/saved-map.json"),d.error());check(!d.dirty(),"savepoint dirty");
        bounds=d.selectedBounds();center=(bounds.min+bounds.max)*.5f;
        d.transform(center+delta,size,0,17); // Deliberately same active token across savepoint.
        check(d.undo() && !d.dirty(),"coalescing erased the saved state");
        check(d.redo() && d.dirty(),"redo did not restore post-save state");
        check(d.undo(),"undo to saved state failed");
        d.duplicate(1);check(d.selected(),"duplicate unselected");
        auto clone=d.map().getLegacyBrushes().back();check(clone.id!=moved.id,"duplicate ID reused");
        for(size_t i=0;i<moved.vertices.size();++i)check(near(clone.vertices[i],moved.vertices[i]+glm::vec3(1,0,1)),"legacy clone transformed wrong");
        const auto count=d.map().getLegacyBrushes().size();check(d.undo() && d.map().getLegacyBrushes().size()==count-1,"duplicate undo failed");
        check(d.redo() && d.map().getLegacyBrushes().size()==count,"duplicate redo failed");
        d.erase();check(!d.selected(),"delete left dangling selection");check(d.undo() && d.selected(),"delete undo lost selection");
        // Extension metadata survives transforms, history, save and shared reload.
        check(d.applyJson("Extension fixture",[](SimpleJson& root){
            root["customRoot"]=SimpleJson("preserved");root["worldSettings"]["customWorld"]=SimpleJson("preserved");
            root["geometry"]["customGeometry"]=SimpleJson("preserved");root["geometry"]["brushes"][0]["customBrush"]=SimpleJson("preserved");
        }),"metadata fixture rejected");
        check(d.save(directory+"/roundtrip.json"),d.error());Map reloaded;
        check(reloaded.loadFromFile(directory+"/roundtrip.json"),"shared loader rejected output");
        auto json=reloaded.toJson();check(json.getString("customRoot")=="preserved" && json["worldSettings"].getString("customWorld")=="preserved" && json["geometry"].getString("customGeometry")=="preserved" && json["geometry"]["brushes"][0].getString("customBrush")=="preserved","metadata dropped");
        const auto saved=d.map().toJson().toString();check(!d.save(directory+"/nonexistent/sub/map.json"),"unwritable save succeeded");check(d.map().toJson().toString()==saved && !d.dirty(),"failed save changed document/savepoint");
        d.createNew();check(d.dirty() && d.path().empty(),"new map not unsaved");
        d.addBox({2,2,2},{2,4,6});check(d.selected() && d.selection().kind==ObjectKind::Object,"box not authored object");
        auto box=d.map().getBrushes().back();check(box.vertices.size()==8 && box.faces.size()==36,"box is not closed volume");
        d.select(d.pick({2,2,10},{0,0,-1}));check(d.selection().kind==ObjectKind::Object,"3D pick did not select object");
        bounds=d.selectedBounds();d.transform({2.4f,2,2},{4,6,8},1);check(near((d.selectedBounds().min+d.selectedBounds().max)*.5f,{2,2,2}),"snap failed");
        d.duplicate();std::set<uint32_t> ids;for(const auto& b:d.map().getBrushes())check(ids.insert(b.id).second,"geometry IDs collided");
        check(d.save(directory+"/new-v2.json"),d.error());check(reloaded.loadFromFile(directory+"/new-v2.json"),"new schema shared reload failed");
        std::cout<<"EDITOR SELF-TEST PASS: legacy transform/resize/clone, texture metadata, unique IDs, selection, history/savepoints, failed load/save, v2 objects, shared reload\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"EDITOR SELF-TEST FAIL: "<<e.what()<<'\n';return 1;}
}
}
