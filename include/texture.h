#pragma once

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace silic2 {

class Texture {
public:
    Texture();
    ~Texture();
    
    // Load texture from file
    bool loadFromFile(const std::string& filePath);
    
    // Bind texture to specified texture unit
    void bind(unsigned int unit = 0) const;
    void unbind() const;
    
    // Get texture ID
    GLuint getID() const { return textureID; }
    
    // Get texture dimensions
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
private:
    GLuint textureID;
    int width;
    int height;
    int channels;
};

// Texture manager - Singleton pattern
class TextureManager {
public:
    static TextureManager& getInstance();
    
    // Load texture (return existing texture if already loaded)
    std::shared_ptr<Texture> loadTexture(const std::string& filePath);
    
    // Clear all textures
    void clear();
    
private:
    TextureManager() = default;
    ~TextureManager() = default;
    
    // Disable copying
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    
    std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;
};

} // namespace silic2