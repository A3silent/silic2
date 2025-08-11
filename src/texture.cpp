#include "texture.h"
#include <iostream>

// Need to define STB_IMAGE_IMPLEMENTATION to implement stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace silic2 {

Texture::Texture() : textureID(0), width(0), height(0), channels(0) {
}

Texture::~Texture() {
    if (textureID) {
        glDeleteTextures(1, &textureID);
    }
}

bool Texture::loadFromFile(const std::string& filePath) {
    // Flip image vertically because OpenGL texture coordinate origin is at bottom-left
    stbi_set_flip_vertically_on_load(true);
    
    // Load image data
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filePath << std::endl;
        std::cerr << "Reason: " << stbi_failure_reason() << std::endl;
        return false;
    }
    
    // Generate OpenGL texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture format
    GLenum format = GL_RGB;
    if (channels == 1)
        format = GL_RED;
    else if (channels == 3)
        format = GL_RGB;
    else if (channels == 4)
        format = GL_RGBA;
    
    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    
    // Generate mipmap
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Set texture parameters
    // Use nearest neighbor filtering to maintain pixelated effect
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Free image data
    stbi_image_free(data);
    
    std::cout << "Loaded texture: " << filePath << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
    
    return true;
}

void Texture::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

// TextureManager implementation
TextureManager& TextureManager::getInstance() {
    static TextureManager instance;
    return instance;
}

std::shared_ptr<Texture> TextureManager::loadTexture(const std::string& filePath) {
    // Check cache
    auto it = textureCache.find(filePath);
    if (it != textureCache.end()) {
        return it->second;
    }
    
    // Load new texture
    auto texture = std::make_shared<Texture>();
    if (texture->loadFromFile(filePath)) {
        textureCache[filePath] = texture;
        return texture;
    }
    
    return nullptr;
}

void TextureManager::clear() {
    textureCache.clear();
}

} // namespace silic2