#include "game_config.h"
#include "simple_json.h"
#include <iostream>
#include <fstream>

namespace silic2 {

GameConfig& GameConfig::getInstance() {
    static GameConfig instance;
    return instance;
}

bool GameConfig::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open config file: " << filename << std::endl;
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    try {
        SimpleJson json = SimpleJson::parse(content);
        if (json.isNull()) {
            std::cerr << "Failed to parse config JSON" << std::endl;
            return false;
        }
        
        // Parse window config
        if (json.hasKey("window")) {
            auto windowObj = json["window"];
            window.width = (int)windowObj.getNumber("width", window.width);
            window.height = (int)windowObj.getNumber("height", window.height);
            window.title = windowObj.getString("title", window.title);
            window.fullscreen = windowObj.getBool("fullscreen", window.fullscreen);
        }
        
        // Parse render config
        if (json.hasKey("render")) {
            auto renderObj = json["render"];
            render.pixelWidth = (int)renderObj.getNumber("pixelWidth", render.pixelWidth);
            render.pixelHeight = (int)renderObj.getNumber("pixelHeight", render.pixelHeight);
            render.nearPlane = (float)renderObj.getNumber("nearPlane", render.nearPlane);
            render.farPlane = (float)renderObj.getNumber("farPlane", render.farPlane);
            render.enableVSync = renderObj.getBool("enableVSync", render.enableVSync);
            render.enableDepthTest = renderObj.getBool("enableDepthTest", render.enableDepthTest);
        }
        
        // Parse player config
        if (json.hasKey("player")) {
            auto playerObj = json["player"];
            player.moveSpeed = (float)playerObj.getNumber("moveSpeed", player.moveSpeed);
            player.sprintSpeed = (float)playerObj.getNumber("sprintSpeed", player.sprintSpeed);
            player.jumpVelocity = (float)playerObj.getNumber("jumpVelocity", player.jumpVelocity);
            player.gravity = (float)playerObj.getNumber("gravity", player.gravity);
            player.maxFallSpeed = (float)playerObj.getNumber("maxFallSpeed", player.maxFallSpeed);
            player.height = (float)playerObj.getNumber("height", player.height);
            player.radius = (float)playerObj.getNumber("radius", player.radius);
            player.eyeHeight = (float)playerObj.getNumber("eyeHeight", player.eyeHeight);
            player.stepHeight = (float)playerObj.getNumber("stepHeight", player.stepHeight);
            player.groundCheckDistance = (float)playerObj.getNumber("groundCheckDistance", player.groundCheckDistance);
            player.mouseSensitivity = (float)playerObj.getNumber("mouseSensitivity", player.mouseSensitivity);
            player.normalFov = (float)playerObj.getNumber("normalFov", player.normalFov);
            player.sprintFov = (float)playerObj.getNumber("sprintFov", player.sprintFov);
            player.fovTransitionSpeed = (float)playerObj.getNumber("fovTransitionSpeed", player.fovTransitionSpeed);
            player.godModeSpeed = (float)playerObj.getNumber("godModeSpeed", player.godModeSpeed);
            player.enableGodMode = playerObj.getBool("enableGodMode", player.enableGodMode);
        }
        
        // Parse camera config
        if (json.hasKey("camera")) {
            auto cameraObj = json["camera"];
            camera.yaw = (float)cameraObj.getNumber("yaw", camera.yaw);
            camera.pitch = (float)cameraObj.getNumber("pitch", camera.pitch);
            camera.minPitch = (float)cameraObj.getNumber("minPitch", camera.minPitch);
            camera.maxPitch = (float)cameraObj.getNumber("maxPitch", camera.maxPitch);
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return false;
    }
}

bool GameConfig::saveToFile(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Could not create config file: " << filename << std::endl;
            return false;
        }
        
        file << "{\n";
        file << "  \"window\": {\n";
        file << "    \"width\": " << window.width << ",\n";
        file << "    \"height\": " << window.height << ",\n";
        file << "    \"title\": \"" << window.title << "\",\n";
        file << "    \"fullscreen\": " << (window.fullscreen ? "true" : "false") << "\n";
        file << "  },\n";
        
        file << "  \"render\": {\n";
        file << "    \"pixelWidth\": " << render.pixelWidth << ",\n";
        file << "    \"pixelHeight\": " << render.pixelHeight << ",\n";
        file << "    \"nearPlane\": " << render.nearPlane << ",\n";
        file << "    \"farPlane\": " << render.farPlane << ",\n";
        file << "    \"enableVSync\": " << (render.enableVSync ? "true" : "false") << ",\n";
        file << "    \"enableDepthTest\": " << (render.enableDepthTest ? "true" : "false") << "\n";
        file << "  },\n";
        
        file << "  \"player\": {\n";
        file << "    \"moveSpeed\": " << player.moveSpeed << ",\n";
        file << "    \"sprintSpeed\": " << player.sprintSpeed << ",\n";
        file << "    \"jumpVelocity\": " << player.jumpVelocity << ",\n";
        file << "    \"gravity\": " << player.gravity << ",\n";
        file << "    \"maxFallSpeed\": " << player.maxFallSpeed << ",\n";
        file << "    \"height\": " << player.height << ",\n";
        file << "    \"radius\": " << player.radius << ",\n";
        file << "    \"eyeHeight\": " << player.eyeHeight << ",\n";
        file << "    \"stepHeight\": " << player.stepHeight << ",\n";
        file << "    \"groundCheckDistance\": " << player.groundCheckDistance << ",\n";
        file << "    \"mouseSensitivity\": " << player.mouseSensitivity << ",\n";
        file << "    \"normalFov\": " << player.normalFov << ",\n";
        file << "    \"sprintFov\": " << player.sprintFov << ",\n";
        file << "    \"fovTransitionSpeed\": " << player.fovTransitionSpeed << ",\n";
        file << "    \"godModeSpeed\": " << player.godModeSpeed << ",\n";
        file << "    \"enableGodMode\": " << (player.enableGodMode ? "true" : "false") << "\n";
        file << "  },\n";
        
        file << "  \"camera\": {\n";
        file << "    \"yaw\": " << camera.yaw << ",\n";
        file << "    \"pitch\": " << camera.pitch << ",\n";
        file << "    \"minPitch\": " << camera.minPitch << ",\n";
        file << "    \"maxPitch\": " << camera.maxPitch << "\n";
        file << "  }\n";
        file << "}\n";
        
        file.close();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        return false;
    }
}

void GameConfig::resetToDefaults() {
    window = WindowConfig{};
    render = RenderConfig{};
    player = PlayerConfig{};
    camera = CameraConfig{};
}

} // namespace silic2