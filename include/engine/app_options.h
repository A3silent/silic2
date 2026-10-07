#pragma once
#include <cstdint>
#include <string>
namespace silic2 {
struct AppOptions {
    uint64_t seed = 42;
    std::string mapFile = "res/maps/starry_crystal_arena.json";
    bool vsync = true;
    std::string acceptance; // empty, restarts, performance, stress, screenshots
    std::string outputDirectory = "artifacts/runs";
    int width = 1280, height = 720;
    int restartCycles = 50;
};
}
