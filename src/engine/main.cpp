#include "engine/app.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
uint64_t unsignedValue(const std::string& s) {
    if(s.empty() || !std::all_of(s.begin(),s.end(),[](unsigned char c){return std::isdigit(c);}))
        throw std::invalid_argument("Expected unsigned integer: "+s);
    return std::stoull(s);
}
}
int main(int argc,char** argv) {
    try {
        silic2::AppOptions options;
        bool mapProvided=false,seedProvided=false,outputProvided=false;
        for(int i=1;i<argc;++i) {
            std::string arg=argv[i];
            auto value=[&](){if(++i>=argc) throw std::invalid_argument("Missing value for "+arg); return std::string(argv[i]);};
            if(arg=="--help") {
                std::cout << "Silic2 [map.json] [--seed N] [--vsync on|off] [--width N] [--height N]\n"
                    "  --acceptance restarts|performance|stress|screenshots|flow|exploration|spatial-combat --output DIRECTORY [--restart-cycles 50..1000]\n"
                    "Acceptance modes use explicit synthetic input; ordinary gameplay is unchanged.\n";
                return 0;
            } else if(arg=="--seed") {
                if(seedProvided) throw std::invalid_argument("Duplicate --seed");
                options.seed=unsignedValue(value()); seedProvided=true;
            } else if(arg=="--vsync") {
                auto v=value(); if(v!="on"&&v!="off") throw std::invalid_argument("--vsync expects on|off"); options.vsync=v=="on";
            } else if(arg=="--acceptance") {
                options.acceptance=value();
                if(options.acceptance!="restarts"&&options.acceptance!="performance"&&options.acceptance!="stress"&&options.acceptance!="screenshots"&&options.acceptance!="flow"&&options.acceptance!="exploration"&&options.acceptance!="spatial-combat")
                    throw std::invalid_argument("Unknown acceptance mode");
            } else if(arg=="--restart-cycles") {
                auto n=unsignedValue(value());if(n<50||n>1000)throw std::invalid_argument("restart cycles must be 50..1000");options.restartCycles=static_cast<int>(n);
            } else if(arg=="--output") {options.outputDirectory=value();outputProvided=true;}
            else if(arg=="--width"||arg=="--height") {
                auto n=unsignedValue(value()); if(n<320||n>8192) throw std::invalid_argument("Window size out of range");
                (arg=="--width"?options.width:options.height)=static_cast<int>(n);
            } else if(arg.rfind("--",0)==0) throw std::invalid_argument("Unknown option: "+arg);
            else {if(mapProvided) throw std::invalid_argument("Only one map path allowed");options.mapFile=arg;mapProvided=true;}
        }
        if(!options.acceptance.empty()&&!outputProvided) options.outputDirectory="artifacts/acceptance/"+options.acceptance;
        silic2::App app(options);
        if(!app.loadMap(options.mapFile)) return 1;
        app.run();
    } catch(const std::exception& e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
}
