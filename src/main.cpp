#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "app.h"

int main(int argc, char* argv[]) {
    try {
        silic2::App app;
        
        // Load map with debug output
        std::cout << "Starting map loading process..." << std::endl;
        
        // Load map if specified as command line argument
        if (argc > 1) {
            std::string mapFile = argv[1];
            std::cout << "Loading map from command line: " << mapFile << std::endl;
            if (!app.loadMap(mapFile)) {
                std::cerr << "Failed to load map: " << mapFile << std::endl;
                std::cout << "Continuing with empty scene..." << std::endl;
            }
        } else {
            // Try to load default map
            std::cout << "Loading default map..." << std::endl;
            if (!app.loadMap("res/maps/test_room.json")) {
                std::cout << "No default map found, starting with empty scene." << std::endl;
            }
        }
        
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}