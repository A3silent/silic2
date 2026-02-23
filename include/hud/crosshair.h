#pragma once

#include <glad/glad.h>
#include <memory>

namespace silic2 {

class Shader;

class Crosshair {
public:
    Crosshair();
    ~Crosshair();

    void init();
    void render(int screenWidth, int screenHeight);

private:
    GLuint vao, vbo;
    std::unique_ptr<Shader> shader;

    void buildMesh();
};

} // namespace silic2
