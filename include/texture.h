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
    
    // 从文件加载纹理
    bool loadFromFile(const std::string& filePath);
    
    // 绑定纹理到指定纹理单元
    void bind(unsigned int unit = 0) const;
    void unbind() const;
    
    // 获取纹理ID
    GLuint getID() const { return textureID; }
    
    // 获取纹理尺寸
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
private:
    GLuint textureID;
    int width;
    int height;
    int channels;
};

// 纹理管理器 - 单例模式
class TextureManager {
public:
    static TextureManager& getInstance();
    
    // 加载纹理（如果已加载则返回现有纹理）
    std::shared_ptr<Texture> loadTexture(const std::string& filePath);
    
    // 清理所有纹理
    void clear();
    
private:
    TextureManager() = default;
    ~TextureManager() = default;
    
    // 禁止拷贝
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    
    std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;
};

} // namespace silic2