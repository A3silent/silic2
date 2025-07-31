#include "texture.h"
#include <iostream>

// 需要定义STB_IMAGE_IMPLEMENTATION来实现stb_image
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
    // 翻转图像，因为OpenGL的纹理坐标原点在左下角
    stbi_set_flip_vertically_on_load(true);
    
    // 加载图像数据
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filePath << std::endl;
        std::cerr << "Reason: " << stbi_failure_reason() << std::endl;
        return false;
    }
    
    // 生成OpenGL纹理
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // 设置纹理格式
    GLenum format = GL_RGB;
    if (channels == 1)
        format = GL_RED;
    else if (channels == 3)
        format = GL_RGB;
    else if (channels == 4)
        format = GL_RGBA;
    
    // 上传纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    
    // 生成mipmap
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // 设置纹理参数
    // 使用最近邻过滤以保持像素化效果
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // 释放图像数据
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
    // 检查缓存
    auto it = textureCache.find(filePath);
    if (it != textureCache.end()) {
        return it->second;
    }
    
    // 加载新纹理
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