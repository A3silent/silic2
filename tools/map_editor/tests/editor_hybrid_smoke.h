// Included after MapEditor so these checks drive the real UI without a second editor implementation.
// All pointer and shortcut events are synthetic. This is not a human usability test.
struct HybridSmoke {
    std::string asset,output,canonical;
    HybridSmoke(std::string modelPath,std::string outputPath):asset(std::move(modelPath)),output(std::move(outputPath)){}
    size_t initialModels=0,initialObjects=0;
    bool initialized=false;
    uint64_t dragRevision=0;
    std::string beforeDrag,afterPosition;
    ImVec2 dragOrigin{0,0};
    static void require(bool ok,const std::string& message){if(!ok)throw std::runtime_error("HYBRID UI: "+message);}
    void input(int frame,ImGuiIO& io,MapEditor& editor) {
        if(!initialized){initialModels=editor.document.map().getModels().size();initialObjects=editor.document.map().getObjects().size();fs::create_directories(output+"/external");
            require(editor.document.map().saveToFile(output+"/input-map.json"),"cannot snapshot GUI input map");
            {std::ofstream identity(output+"/identity.txt");identity<<"build="<<SILIC2_EDITOR_BUILD_TYPE<<"\nsource="<<editor.document.path()<<"\nnodes="<<editor.document.map().getSpatialSettings().nodes.size()<<"\nmodels="<<initialModels<<"\n";}initialized=true;}
        auto key=[&](ImGuiKey k,bool down,bool shift=false){io.AddKeyEvent(k==ImGuiKey_A && io.ConfigMacOSXBehaviors?ImGuiMod_Super:ImGuiMod_Ctrl,down);io.AddKeyEvent(ImGuiMod_Shift,down && shift);io.AddKeyEvent(k,down);};
        auto click=[&](const char* name,bool down){require(editor.controls.count(name),std::string("missing visible widget ")+name);auto p=editor.controls.at(name);require(p.x>=0 && p.y>=0 && p.x<io.DisplaySize.x && p.y<io.DisplaySize.y,std::string("widget outside window ")+name);io.AddMousePosEvent(p.x,p.y);io.AddMouseButtonEvent(0,down);};
        if(frame==2 || frame==3)key(ImGuiKey_I,frame==2);
        if(frame==5 || frame==6)click("file-path",frame==5);
        if(frame==7 || frame==8)key(ImGuiKey_A,frame==7);
        if(frame==9)io.AddInputCharactersUTF8(asset.c_str());
        if(frame==11 || frame==12)click("file-submit",frame==11);
        if(frame==15 || frame==16)click("rotate",frame==15);
        if(frame==19 || frame==20)click("scale",frame==19);
        if(frame==23 || frame==24)key(ImGuiKey_D,frame==23);
        if(frame==27 || frame==28)key(ImGuiKey_Z,frame==27);
        if(frame==31 || frame==32)key(ImGuiKey_Y,frame==31);
        if(frame==35 || frame==36)click("add-object",frame==35);
        if(frame==38 || frame==39)click("add-ramp",frame==38);
        if(frame==43 || frame==44)key(ImGuiKey_Z,frame==43);
        if(frame==47 || frame==48)key(ImGuiKey_Y,frame==47);
        if(frame==51 || frame==52)key(ImGuiKey_S,frame==51,true);
        if(frame==54 || frame==55)click("file-path",frame==54);
        if(frame==56 || frame==57)key(ImGuiKey_A,frame==56);
        if(frame==58)io.AddInputCharactersUTF8((output+"/external/edited.json").c_str());
        if(frame==60 || frame==61)click("file-submit",frame==60);
        if(frame==65 || frame==66)key(ImGuiKey_O,frame==65);
        if(frame==68 || frame==69)click("file-path",frame==68);
        if(frame==70 || frame==71)key(ImGuiKey_A,frame==70);
        if(frame==72)io.AddInputCharactersUTF8((output+"/external/edited.json").c_str());
        if(frame==74 || frame==75)click("file-submit",frame==74);
        if(frame==96){editor.select({ObjectKind::Model,0});editor.focusSelection();beforeDrag=editor.document.map().toJson().toString();dragRevision=editor.document.revision();}
        if(frame==98){click("position-drag",true);dragOrigin=editor.controls.at("position-drag");}
        if(frame>=100 && frame<=110)io.AddMousePosEvent(dragOrigin.x+(frame-99)*3,dragOrigin.y);
        if(frame==112)io.AddMouseButtonEvent(0,false);
        if(frame==118){click("rotation-drag",true);dragOrigin=editor.controls.at("rotation-drag");dragRevision=editor.document.revision();}
        if(frame>=120 && frame<=130)io.AddMousePosEvent(dragOrigin.x+(frame-119)*3,dragOrigin.y);
        if(frame==132)io.AddMouseButtonEvent(0,false);
        if(frame==136 || frame==137)key(ImGuiKey_Z,frame==136);
        if(frame==140 || frame==141)key(ImGuiKey_Z,frame==140);
    }
    void verify(int frame,MapEditor& editor) {
        const auto& d=editor.document;
        if(frame==14){require(d.map().getModels().size()==initialModels+1,"import button failed: "+d.error());require(d.selection().kind==ObjectKind::Model,"import selection missing");}
        if(frame==18)require(std::abs(d.selectedJson()["rotation"][1].getNumber()-editor.angleStep)<.01,"rotation button failed: "+d.error());
        if(frame==22)require(std::abs(d.selectedSize().x-1.1f)<.001f,"scale button failed: "+d.error());
        if(frame==26 || frame==34)require(d.map().getModels().size()==initialModels+2,"duplicate/redo failed: "+d.error());
        if(frame==30)require(d.map().getModels().size()==initialModels+1,"duplicate undo failed");
        if(frame==41 || frame==50)require(d.map().getObjects().size()==initialObjects+1 && d.selectedJson().getString("shape")=="ramp","ramp add/redo failed: "+d.error());
        if(frame==46)require(d.map().getObjects().size()==initialObjects,"ramp undo failed");
        if(frame==63){
            require(!d.dirty() && fs::exists(output+"/external/edited.json"),"Save As UI failed: "+d.error());
            Map shared;require(shared.loadFromFile(output+"/external/edited.json"),"shared external load failed: "+shared.getLastError());canonical=shared.toJson().toString();
        }
        if(frame==78){require(!d.dirty() && d.map().toJson().toString()==canonical,"Open UI did not reload exact saved map: "+d.error());require(d.map().getModels().size()==initialModels+2,"external models missing");editor.focusAll();std::cout<<"HYBRID UI PASS: OBJ import, rotation, scale, clone/undo/redo, ramp add/undo/redo, external Save As and Open through real ImGui controls\n";}
        if(frame==106 || frame==126)require(d.revision()==dragRevision,"drag rebuilt the shared document before release");
        if(frame==114){require(d.revision()==dragRevision+1,"position drag did not commit exactly once: "+d.error());afterPosition=d.map().toJson().toString();require(afterPosition!=beforeDrag,"position drag made no change");}
        if(frame==134)require(d.revision()==dragRevision+1,"rotation drag did not commit exactly once: "+d.error());
        if(frame==138)require(d.map().toJson().toString()==afterPosition,"one undo did not revert exactly one rotation drag");
        if(frame==144){require(d.map().toJson().toString()==beforeDrag,"two undos did not revert two drags");std::cout<<"HYBRID DRAG PASS: full map nodes "<<d.map().getSpatialSettings().nodes.size()<<", live visual-only position/rotation previews, one commit and one undo per drag\n";}
        if(frame==80)editor.geometryView=1;
        if(frame==84)editor.geometryView=2;
        if(frame==88){editor.geometryView=0;}
        if(frame==14 || frame==18 || frame==22 || frame==26 || frame==30 || frame==34 || frame==41 || frame==46 || frame==50 || frame==63 || frame==78)
            std::cout<<"HYBRID FRAME "<<frame<<" models "<<d.map().getModels().size()<<" objects "<<d.map().getObjects().size()<<" dirty "<<d.dirty()<<" error "<<d.error()<<'\n';
    }
    static bool capture(int frame){return frame==14 || frame==22 || frame==41 || frame==63 || frame==79 || frame==82 || frame==86 || frame==92 || frame==106 || frame==126 || frame==144;}
};

struct LayoutSmoke {
    std::string original;
    ImVec2 pointer{0,0};
    uint64_t revision=0;
    void input(int frame,ImGuiIO& io,MapEditor& editor,GLFWwindow* window) {
        if(frame==2){editor.select({ObjectKind::Model,0});editor.focusSelection();original=editor.document.map().toJson().toString();revision=editor.document.revision();}
        if(frame==4 || frame==20){pointer=editor.controls.at("rotation-drag");glfwSetCursorPos(window,pointer.x,pointer.y);io.AddMousePosEvent(pointer.x,pointer.y);io.AddMouseButtonEvent(0,true);revision=editor.document.revision();}
        if((frame>=5 && frame<=11) || (frame>=21 && frame<=27)){pointer.x+=3;glfwSetCursorPos(window,pointer.x,pointer.y);io.AddMousePosEvent(pointer.x,pointer.y);}
        if(frame==13 || frame==29)io.AddMouseButtonEvent(0,false);
        if(frame==15 || frame==16 || frame==34 || frame==35){bool down=frame==15 || frame==34;io.AddKeyEvent(ImGuiMod_Ctrl,down);io.AddKeyEvent(ImGuiKey_Z,down);}
    }
    void verify(int frame,MapEditor& editor) {
        if(frame==12 || frame==28){
            HybridSmoke::require(editor.document.revision()==revision,"layout drag committed before release");
            const auto screen=ImGui::GetIO().DisplaySize;const float panel=std::clamp(screen.x*.25f,220.0f,320.0f);
            for(const char* name:{"position","size","rotation-field"}){auto bounds=editor.controlBounds.at(name);HybridSmoke::require(bounds.first.x>=screen.x-panel+4 && bounds.second.x<=screen.x-4 && bounds.second.x-bounds.first.x>=180,std::string("TRS field outside inspector: ")+name);}
            std::cout<<"TRS LAYOUT PASS width "<<screen.x<<" all three-component fields within Inspector; drag preview active\n";
        }
        if(frame==14 || frame==32)HybridSmoke::require(editor.document.revision()>revision,"layout drag must commit on release");
        if(frame==18 || frame==37)HybridSmoke::require(editor.document.map().toJson().toString()==original,"layout drag undo failed");
    }
    static bool capture(int frame){return frame==12 || frame==28 || frame==32;}
};
