#include "editor_document.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

namespace silic2::editor {
namespace {
void check(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}
bool near(glm::vec3 a,glm::vec3 b){return glm::length(a-b)<.001f;}
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
        if(d.map().getGameMode()==GameMode::ArenaSurvival){
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
