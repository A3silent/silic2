#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// OpenGL loader - must be included before GLFW
#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "editor_document.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <chrono>
#include <stdexcept>
using namespace silic2::editor;

namespace fs = std::filesystem;
using namespace silic2;

// Simple camera class
class Camera {
public:
    glm::vec3 position = glm::vec3(10.0f, 10.0f, 10.0f);
    glm::vec3 front = glm::vec3(-0.5f, -0.5f, -0.5f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right;

    float yaw = -135.0f;
    float pitch = -30.0f;
    float fov = 45.0f;
    float moveSpeed = 5.0f;
    float mouseSensitivity = 0.3f;

    Camera() {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix(float aspect) const {
        return glm::perspective(glm::radians(fov), aspect, 0.1f, 1000.0f);
    }

    void processKeyboard(int direction, float deltaTime) {
        float velocity = moveSpeed * deltaTime;

        switch(direction) {
            case 0: // Forward (W)
                position += front * velocity;
                break;
            case 1: // Backward (S)
                position -= front * velocity;
                break;
            case 2: // Left (A)
                position -= right * velocity;
                break;
            case 3: // Right (D)
                position += right * velocity;
                break;
            case 4: // Up (Q)
                position += up * velocity;
                break;
            case 5: // Down (E)
                position -= up * velocity;
                break;
        }
    }

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (constrainPitch) {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void processMouseScroll(float yoffset) {
        fov -= yoffset * 2.0f;
        if (fov < 1.0f)
            fov = 1.0f;
        if (fov > 90.0f)
            fov = 90.0f;
    }

    void lookAt(glm::vec3 target) {
        const auto direction=glm::normalize(target-position);
        yaw=glm::degrees(std::atan2(direction.z,direction.x));pitch=glm::degrees(std::asin(direction.y));updateCameraVectors();
    }
    void reset() {
        position = glm::vec3(10.0f, 10.0f, 10.0f);
        yaw = -135.0f;
        pitch = -30.0f;
        fov = 45.0f;
        updateCameraVectors();
    }

private:
    void updateCameraVectors() {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);

        right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, front));
    }
};

class MapRenderer {
private:
    GLuint shaderProgram;
    bool selectionPass=false;
    GLuint VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        layout (location = 2) in vec3 aNormal;

        out vec3 FragPos;
        out vec3 Color;
        out vec3 Normal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Color = aColor;
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 FragPos;
        in vec3 Color;
        in vec3 Normal;

        uniform vec3 lightPos;
        uniform vec3 lightColor;
        uniform vec3 viewPos;
        uniform float alpha;
        uniform bool unlit;

        void main() {
            vec3 ambient = 0.3 * Color;

            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor * Color;

            vec3 result = unlit ? Color : ambient + diffuse;
            FragColor = vec4(result, alpha);
        }
    )";

    const char* gridVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
        }
    )";

    const char* gridFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;

        void main() {
            FragColor = vec4(color, 0.3);
        }
    )";

    GLuint gridShader;
    GLuint gridVAO, gridVBO;
    std::vector<float> gridVertices;

public:
    MapRenderer() {
        initShaders();
        initBuffers();
        initGrid();
    }

    ~MapRenderer() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteProgram(shaderProgram);

        glDeleteVertexArrays(1, &gridVAO);
        glDeleteBuffers(1, &gridVBO);
        glDeleteProgram(gridShader);
    }

    void initShaders() {
        // Create and compile vertex shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        // Create and compile fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        // Create shader program
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // Grid shader
        GLuint gridVert = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(gridVert, 1, &gridVertexShader, NULL);
        glCompileShader(gridVert);

        GLuint gridFrag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(gridFrag, 1, &gridFragmentShader, NULL);
        glCompileShader(gridFrag);

        gridShader = glCreateProgram();
        glAttachShader(gridShader, gridVert);
        glAttachShader(gridShader, gridFrag);
        glLinkProgram(gridShader);

        glDeleteShader(gridVert);
        glDeleteShader(gridFrag);
    }

    void initBuffers() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }

    void initGrid() {
        gridVertices.clear();
        float gridSize = 50.0f;
        float step = 1.0f;

        // Grid lines along X
        for (float z = -gridSize; z <= gridSize; z += step) {
            gridVertices.push_back(-gridSize); gridVertices.push_back(0); gridVertices.push_back(z);
            gridVertices.push_back(gridSize); gridVertices.push_back(0); gridVertices.push_back(z);
        }

        // Grid lines along Z
        for (float x = -gridSize; x <= gridSize; x += step) {
            gridVertices.push_back(x); gridVertices.push_back(0); gridVertices.push_back(-gridSize);
            gridVertices.push_back(x); gridVertices.push_back(0); gridVertices.push_back(gridSize);
        }

        // Add axis lines (thicker/colored)
        // X axis - red
        gridVertices.push_back(0); gridVertices.push_back(0); gridVertices.push_back(0);
        gridVertices.push_back(5); gridVertices.push_back(0); gridVertices.push_back(0);

        // Y axis - green
        gridVertices.push_back(0); gridVertices.push_back(0); gridVertices.push_back(0);
        gridVertices.push_back(0); gridVertices.push_back(5); gridVertices.push_back(0);

        // Z axis - blue
        gridVertices.push_back(0); gridVertices.push_back(0); gridVertices.push_back(0);
        gridVertices.push_back(0); gridVertices.push_back(0); gridVertices.push_back(5);

        glGenVertexArrays(1, &gridVAO);
        glGenBuffers(1, &gridVBO);

        glBindVertexArray(gridVAO);
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    void updateMapGeometry(const Map& map) {
        vertices.clear();
        indices.clear();

        for (const auto& brush : map.getBrushes()) {
            for(size_t f=0;f+2<brush.faces.size();f+=3) {
                const auto a=brush.faces[f],b=brush.faces[f+1],c=brush.faces[f+2];
                if(a>=brush.vertices.size() || b>=brush.vertices.size() || c>=brush.vertices.size()) continue;
                auto normal=glm::cross(brush.vertices[b]-brush.vertices[a],brush.vertices[c]-brush.vertices[a]);
                normal=glm::length(normal)>1e-6f?glm::normalize(normal):glm::vec3(0,1,0);
                for(auto index:{a,b,c}) {
                    auto p=brush.vertices[index];
                    vertices.insert(vertices.end(),{p.x,p.y,p.z,brush.color.x,brush.color.y,brush.color.z,normal.x,normal.y,normal.z});
                    indices.push_back(static_cast<unsigned int>(indices.size()));
                }
            }
        }

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    void renderGrid(const Camera& camera, float aspect) {
        glUseProgram(gridShader);

        glUniformMatrix4fv(glGetUniformLocation(gridShader, "view"), 1, GL_FALSE,
                          glm::value_ptr(camera.getViewMatrix()));
        glUniformMatrix4fv(glGetUniformLocation(gridShader, "projection"), 1, GL_FALSE,
                          glm::value_ptr(camera.getProjectionMatrix(aspect)));

        glBindVertexArray(gridVAO);

        // Draw grid
        glUniform3f(glGetUniformLocation(gridShader, "color"), 0.3f, 0.3f, 0.3f);
        glDrawArrays(GL_LINES, 0, gridVertices.size() / 3 - 6);

        // Draw axes with different colors
        glLineWidth(1.0f);

        // X axis - red
        glUniform3f(glGetUniformLocation(gridShader, "color"), 1.0f, 0.0f, 0.0f);
        glDrawArrays(GL_LINES, gridVertices.size() / 3 - 6, 2);

        // Y axis - green
        glUniform3f(glGetUniformLocation(gridShader, "color"), 0.0f, 1.0f, 0.0f);
        glDrawArrays(GL_LINES, gridVertices.size() / 3 - 4, 2);

        // Z axis - blue
        glUniform3f(glGetUniformLocation(gridShader, "color"), 0.0f, 0.0f, 1.0f);
        glDrawArrays(GL_LINES, gridVertices.size() / 3 - 2, 2);

        glLineWidth(1.0f);
        glBindVertexArray(0);
    }

    void render(const Camera& camera, float aspect, bool wireframe = false) {
        if (indices.empty()) return;

        if (wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        glUseProgram(shaderProgram);

        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE,
                          glm::value_ptr(camera.getViewMatrix()));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE,
                          glm::value_ptr(camera.getProjectionMatrix(aspect)));

        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 0.0f, 10.0f, 0.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(camera.position));
        glUniform1f(glGetUniformLocation(shaderProgram, "alpha"), wireframe ? 0.5f : 1.0f);
        glUniform1i(glGetUniformLocation(shaderProgram, "unlit"),0);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        if (wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    void renderEntities(const Map& map, const Camera& camera, float aspect) {
        for (const auto& entity : map.getEntities()) {
            glm::vec3 color;
            float size = 0.5f;

            switch (entity.type) {
                case EntityType::PLAYER_START:
                    color = glm::vec3(0.0f, 1.0f, 0.0f);
                    break;
                case EntityType::ENEMY_SPAWN:
                    color = glm::vec3(1.0f, 0.0f, 0.0f);
                    break;
                case EntityType::ITEM_PICKUP:
                    color = glm::vec3(0.0f, 0.0f, 1.0f);
                    size = 0.3f;
                    break;
                default:
                    color = glm::vec3(1.0f, 1.0f, 0.0f);
                    break;
            }

            renderBox(entity.position, glm::max(glm::abs(entity.scale),glm::vec3(.1f)) * size, color, camera, aspect);
        }

        for (const auto& light : map.getLights()) {
            renderBox(light.position, glm::vec3(0.3f), light.color * light.intensity, camera, aspect);
        }
    }

    void renderSelection(Bounds bounds,const Camera& camera,float aspect) {
        glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);selectionPass=true;
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
        renderBox((bounds.min+bounds.max)*.5f,glm::max(bounds.max-bounds.min,glm::vec3(.03f))*1.006f,{.2f,1.0f,.85f},camera,aspect);
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);selectionPass=false;
        glEnable(GL_DEPTH_TEST);
    }

    void renderBox(const glm::vec3& pos, const glm::vec3& size, const glm::vec3& color,
                   const Camera& camera, float aspect) {
        float vertices[] = {
            -0.5f, -0.5f, -0.5f,  color.x, color.y, color.z,  0.0f, 0.0f, -1.0f,
             0.5f, -0.5f, -0.5f,  color.x, color.y, color.z,  0.0f, 0.0f, -1.0f,
             0.5f,  0.5f, -0.5f,  color.x, color.y, color.z,  0.0f, 0.0f, -1.0f,
            -0.5f,  0.5f, -0.5f,  color.x, color.y, color.z,  0.0f, 0.0f, -1.0f,

            -0.5f, -0.5f,  0.5f,  color.x, color.y, color.z,  0.0f, 0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  color.x, color.y, color.z,  0.0f, 0.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  color.x, color.y, color.z,  0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  color.x, color.y, color.z,  0.0f, 0.0f, 1.0f,
        };

        unsigned int cubeIndices[] = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            0, 1, 5, 5, 4, 0,
            2, 3, 7, 7, 6, 2,
            0, 3, 7, 7, 4, 0,
            1, 2, 6, 6, 5, 1
        };

        GLuint tempVAO, tempVBO, tempEBO;
        glGenVertexArrays(1, &tempVAO);
        glGenBuffers(1, &tempVBO);
        glGenBuffers(1, &tempEBO);

        glBindVertexArray(tempVAO);

        glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tempEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glUseProgram(shaderProgram);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::scale(model, size);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE,
                          glm::value_ptr(camera.getViewMatrix()));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE,
                          glm::value_ptr(camera.getProjectionMatrix(aspect)));

        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 0.0f, 10.0f, 0.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(camera.position));
        glUniform1f(glGetUniformLocation(shaderProgram, "alpha"), 1.0f);
        glUniform1i(glGetUniformLocation(shaderProgram, "unlit"),selectionPass);

        glBindVertexArray(tempVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        glDeleteVertexArrays(1, &tempVAO);
        glDeleteBuffers(1, &tempVBO);
        glDeleteBuffers(1, &tempEBO);
    }
};


class MapEditor {
public:
    EditorDocument document;
    Camera camera;
    bool showGrid=true,showMarkers=true,wireframe=false,snapEnabled=true;
    float gridStep=1;
    ImVec2 cancelButtonCenter{0,0};
    bool modalButtonsReachable=true;
    std::string status="Open a map, or start from the Arena template.";
    enum class Action { None,New,Open,Arena,Quit };
private:
    MapRenderer renderer;
    GLFWwindow* window;
    uint64_t renderedRevision=~uint64_t(0),editToken=1;
    bool navigating=false,viewportHovered=false;
    double lastMouseX=0,lastMouseY=0;
    ImVec2 viewportPos{260,56},viewportSize{640,600};
    Action pending=Action::None;
    std::string pendingPath;
    bool confirmUnsaved=false,browse=false,saveDialog=false,overwrite=false;
    bool saveThenContinue=false;
    std::string browseDirectory=SILIC2_ROOT "/res/maps",overwritePath;
    char directoryInput[1024]{},fileInput[1024]{};
    std::string filter;
    char filterInput[128]{};
    static constexpr ImGuiWindowFlags fixed=ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings;
public:
    explicit MapEditor(GLFWwindow* w):window(w) { copy(directoryInput,browseDirectory); }
    static void copy(char* dst,const std::string& value,size_t capacity=1024) { std::snprintf(dst,capacity,"%s",value.c_str()); }
    bool hasPending() const { return pending!=Action::None; }
    bool unsavedPrompt() const { return confirmUnsaved; }
    void request(Action action,const std::string& path="") {
        pending=action;pendingPath=path;
        if(document.dirty()){confirmUnsaved=true;return;}
        performPending();
    }
    void cancelPending() { pending=Action::None;pendingPath.clear();confirmUnsaved=false;saveThenContinue=false; }
    void discardPending() { confirmUnsaved=false;performPending(); }
    void performPending() {
        auto action=pending;auto path=pendingPath;pending=Action::None;pendingPath.clear();
        if(action==Action::Quit){glfwSetWindowShouldClose(window,GL_TRUE);return;}
        if(action==Action::New){document.createNew();status="New map. Save As to choose its location.";focusAll();}
        if(action==Action::Arena){if(document.createArena(SILIC2_ROOT "/res/maps/starry_crystal_arena.json")){status="Arena template ready. Save As creates your own map.";focusAll();}}
        if(action==Action::Open){if(document.load(path)){status="Opened "+document.path();focusAll();}}
    }
    void openBrowser(bool saving) {
        browse=true;saveDialog=saving;overwrite=false;
        if(!document.path().empty())browseDirectory=fs::path(document.path()).parent_path().string();
        copy(directoryInput,browseDirectory);
        copy(fileInput,saving?(document.path().empty()?"untitled.json":fs::path(document.path()).filename().string()):"");
    }
    bool saveAt(const std::string& path) {
        if(!document.save(path))return false;
        status="Saved "+document.path();browse=false;overwrite=false;
        if(saveThenContinue){saveThenContinue=false;performPending();}
        return true;
    }
    void save(bool as=false) {
        if(as || document.path().empty())openBrowser(true);
        else if(!saveAt(document.path()) && saveThenContinue)confirmUnsaved=true;
    }
    void focusSelection() {
        if(!document.selected()){focusAll();return;}
        auto b=document.selectedBounds();const auto target=(b.min+b.max)*.5f;
        float distance=std::max(3.0f,glm::length(b.max-b.min)*1.4f);
        // Move past an enclosing roof/wall when framing an interior object, so
        // the next click does not silently select its occluder instead.
        for(int attempt=0;attempt<10;++attempt){
            camera.position=target-camera.front*distance;
            if(document.pick(camera.position,camera.front,showMarkers)==document.selection())break;
            distance*=.65f;
        }
        camera.lookAt(target);
    }
    void focusAll() {
        if(document.map().getBrushes().empty()){camera.reset();return;}
        auto b=EditorDocument::brushBounds(document.map().getBrushes()[0]);
        for(const auto& brush:document.map().getBrushes()){auto a=EditorDocument::brushBounds(brush);b.min=glm::min(a.min,b.min);b.max=glm::max(a.max,b.max);}
        camera.reset();camera.position=(b.min+b.max)*.5f-camera.front*std::max(8.0f,glm::length(b.max-b.min)*.9f);
    }
    void nudge(glm::vec3 delta) {
        if(!document.selected())return;auto b=document.selectedBounds();document.transform((b.min+b.max)*.5f+delta,b.max-b.min,0);
    }
    void pickAt(ImVec2 screen) {
        if(viewportSize.x<=0 || viewportSize.y<=0)return;
        const glm::vec4 nearPoint((screen.x-viewportPos.x)/viewportSize.x*2-1,1-(screen.y-viewportPos.y)/viewportSize.y*2,-1,1);
        const auto inverse=glm::inverse(camera.getProjectionMatrix(viewportSize.x/viewportSize.y)*camera.getViewMatrix());
        auto world=inverse*nearPoint;world/=world.w;
        document.select(document.pick(camera.position,glm::normalize(glm::vec3(world)-camera.position),showMarkers));
    }
    ImVec2 project(glm::vec3 position) const {
        glm::vec4 clip=camera.getProjectionMatrix(viewportSize.x/viewportSize.y)*camera.getViewMatrix()*glm::vec4(position,1);
        return {viewportPos.x+(clip.x/clip.w+1)*.5f*viewportSize.x,viewportPos.y+(1-clip.y/clip.w)*.5f*viewportSize.y};
    }
    void stopNavigation() { if(navigating){navigating=false;glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);} }
    void processInput(float dt) {
        auto& io=ImGui::GetIO();
        const bool focused=glfwGetWindowAttrib(window,GLFW_FOCUSED)!=0;
        const bool right=glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS;
        if(navigating && (!right || !focused || browse || confirmUnsaved))stopNavigation();
        if(!navigating && viewportHovered && right && focused && !io.WantTextInput && !browse && !confirmUnsaved){
            navigating=true;glfwGetCursorPos(window,&lastMouseX,&lastMouseY);glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
        }
        if(navigating) {
            double x,y;glfwGetCursorPos(window,&x,&y);camera.processMouseMovement(float(x-lastMouseX),float(lastMouseY-y));lastMouseX=x;lastMouseY=y;
            const float speed=camera.moveSpeed; if(io.KeyShift)camera.moveSpeed*=3;
            const int keys[]={GLFW_KEY_W,GLFW_KEY_S,GLFW_KEY_A,GLFW_KEY_D,GLFW_KEY_E,GLFW_KEY_Q};
            for(int i=0;i<6;++i)if(glfwGetKey(window,keys[i])==GLFW_PRESS)camera.processKeyboard(i,dt);
            camera.moveSpeed=speed;return;
        }
        if(viewportHovered && !browse && !confirmUnsaved && !io.WantTextInput && io.MouseWheel!=0)
            camera.position+=camera.front*io.MouseWheel*std::max(1.0f,camera.moveSpeed*.4f);
        const bool command=io.KeyCtrl || io.KeySuper;
        if(browse || confirmUnsaved)return;
        // Save is a document command even with a text field focused. Text editing
        // retains all ordinary keys and its own undo/redo/select-all shortcuts.
        if(command && ImGui::IsKeyPressed(ImGuiKey_S,false)){save(io.KeyShift);return;}
        if(io.WantTextInput || ImGui::IsAnyItemActive())return;
        if(command) {
            if(ImGui::IsKeyPressed(ImGuiKey_Z,false)){if(io.KeyShift)document.redo();else document.undo();}
            if(ImGui::IsKeyPressed(ImGuiKey_Y,false))document.redo();
            if(ImGui::IsKeyPressed(ImGuiKey_D,false))document.duplicate(snapEnabled?gridStep:1);
            if(ImGui::IsKeyPressed(ImGuiKey_N,false))request(Action::New);
            if(ImGui::IsKeyPressed(ImGuiKey_O,false))openBrowser(false);
        } else {
            if(ImGui::IsKeyPressed(ImGuiKey_F,false))focusSelection();
            if(ImGui::IsKeyPressed(ImGuiKey_Delete,false) || ImGui::IsKeyPressed(ImGuiKey_Backspace,false))document.erase();
            const float step=(snapEnabled?gridStep:.1f)*(io.KeyShift?10:1);
            if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow))nudge({-step,0,0});
            if(ImGui::IsKeyPressed(ImGuiKey_RightArrow))nudge({step,0,0});
            if(ImGui::IsKeyPressed(ImGuiKey_UpArrow))nudge({0,0,-step});
            if(ImGui::IsKeyPressed(ImGuiKey_DownArrow))nudge({0,0,step});
            if(ImGui::IsKeyPressed(ImGuiKey_PageUp))nudge({0,step,0});
            if(ImGui::IsKeyPressed(ImGuiKey_PageDown))nudge({0,-step,0});
        }
    }
    void editString(const char* label,const char* key,SimpleJson json) {
        char value[256];copy(value,json.getString(key),sizeof(value));
        const bool changed=ImGui::InputText(label,value,sizeof(value));if(ImGui::IsItemActivated())++editToken;
        if(changed)document.editSelected(label,[&](SimpleJson& target){target[key]=SimpleJson(value);},editToken);
    }
    void editFloat(const char* label,const char* key,SimpleJson json,float defaultValue,float min,float max) {
        float value=json.getNumber(key,defaultValue);const bool changed=ImGui::DragFloat(label,&value,.05f,min,max);
        if(ImGui::IsItemActivated())++editToken;
        if(changed)document.editSelected(label,[&](SimpleJson& target){target[key]=SimpleJson(double(value));},editToken);
    }
    void editVector(const char* label,const char* key,SimpleJson json,glm::vec3 defaults=glm::vec3(0)) {
        glm::vec3 value=defaults;if(json[key].isArray())for(int a=0;a<3;++a)value[a]=json[key][a].getNumber();
        bool changed=ImGui::DragFloat3(label,glm::value_ptr(value),.05f);if(ImGui::IsItemActivated())++editToken;
        if(changed)document.editSelected(label,[&](SimpleJson& target){target[key]=SimpleJson::array();for(int a=0;a<3;++a)target[key].push_back(SimpleJson(double(value[a])));},editToken);
    }
    void properties() {
        if(!document.selected()){ImGui::TextWrapped("Select an object in the viewport or the list.");return;}
        const auto selection=document.selection();auto j=document.selectedJson();
        auto bounds=document.selectedBounds();glm::vec3 center=(bounds.min+bounds.max)*.5f,size=bounds.max-bounds.min;
        ImGui::TextColored({.3f,.95f,.8f,1},"SELECTED OBJECT");
        if(j.hasKey("id"))ImGui::Text("ID %.0f",j.getNumber("id"));
        if(selection.kind!=ObjectKind::Brush && selection.kind!=ObjectKind::Light)editString("Name","name",j);
        bool changed=ImGui::DragFloat3("Position",glm::value_ptr(center),.05f,0,0,"%.2f");if(ImGui::IsItemActivated())++editToken;
        if(changed)document.transform(center,size,0,editToken);
        if(ImGui::IsItemDeactivatedAfterEdit() && snapEnabled)document.transform(center,size,gridStep,editToken);
        if(selection.kind!=ObjectKind::Light){
            changed=ImGui::DragFloat3("Size",glm::value_ptr(size),.05f,0,10000,"%.2f");if(ImGui::IsItemActivated())++editToken;
            if(changed)document.transform(center,size,0,editToken);
            if(ImGui::IsItemDeactivatedAfterEdit() && snapEnabled)document.transform(center,size,gridStep,editToken);
            if(selection.kind==ObjectKind::Object && j.getString("shape")=="plane")ImGui::TextWrapped("Horizontal plane: Y size stays zero.");
            if(selection.kind==ObjectKind::Brush)ImGui::TextWrapped("Legacy mesh: flat axes remain flat. Use Add Box for solid geometry.");
        }
        if(ImGui::Button("Snap to grid"))document.transform(center,size,gridStep);
        ImGui::SameLine();if(ImGui::Button("Focus [F]"))focusSelection();
        if(ImGui::Button("Duplicate"))document.duplicate(snapEnabled?gridStep:1);
        ImGui::SameLine();if(ImGui::Button("Delete"))document.erase();
        ImGui::Separator();
        if(selection.kind==ObjectKind::Object || selection.kind==ObjectKind::Brush){
            editString("Material","material",j);editString("Texture path","texture",j);
            glm::vec3 color(.8f);if(j["color"].isArray())for(int a=0;a<3;++a)color[a]=j["color"][a].getNumber();
            changed=ImGui::ColorEdit3("Color",glm::value_ptr(color));if(ImGui::IsItemActivated())++editToken;
            if(changed)document.editSelected("Change color",[&](SimpleJson& target){target["color"]=SimpleJson::parse("[]");for(int a=0;a<3;++a)target["color"].push_back(SimpleJson(double(color[a])));},editToken);
            const char* types[]={"floor","wall","ceiling"};int current=1;for(int i=0;i<3;++i)if(j.getString("type")==types[i])current=i;
            if(ImGui::Combo("Surface",&current,types,3))document.editSelected("Change surface",[&](SimpleJson& target){target["type"]=SimpleJson(types[current]);});
            if(selection.kind==ObjectKind::Object)ImGui::Text("Shape: %s",j.getString("shape").c_str());
        } else if(selection.kind==ObjectKind::Entity) {
            ImGui::Text("Type: %s",j.getString("type").c_str());
            glm::vec3 rotation(0);if(j["rotation"].isArray())for(int a=0;a<3;++a)rotation[a]=j["rotation"][a].getNumber();
            changed=ImGui::DragFloat3("Rotation",glm::value_ptr(rotation),1);if(ImGui::IsItemActivated())++editToken;
            if(changed)document.editSelected("Rotate entity",[&](SimpleJson& target){target["rotation"]=SimpleJson::parse("[]");for(int a=0;a<3;++a)target["rotation"].push_back(SimpleJson(double(rotation[a])));},editToken);
            editString("Subtype","stringProperty",j);
            editFloat("Value / HP","floatProperty1",j,0,0,10000);
            editFloat("Range / value","floatProperty2",j,0,0,10000);
            int quantity=int(j.getNumber("intProperty",0));if(ImGui::InputInt("Quantity",&quantity))document.editSelected("Entity quantity",[&](SimpleJson& target){target["intProperty"]=SimpleJson(double(quantity));});
            editVector("Vector / color","vec3Property",j);
            if(j.hasKey("slot"))ImGui::Text("Arena slot: %.0f (fixed)",j.getNumber("slot"));
            ImGui::TextWrapped("Arena requires one player, four rifts and three sentry markers. Invalid edits keep the previous map.");
        } else {
            const char* lightTypes[]={"point","directional","spot"};int lightType=0;for(int a=0;a<3;++a)if(j.getString("type")==lightTypes[a])lightType=a;
            if(ImGui::Combo("Light type",&lightType,lightTypes,3))document.editSelected("Light type",[&](SimpleJson& target){target["type"]=SimpleJson(lightTypes[lightType]);});
            editVector("Direction","direction",j,{0,-1,0});
            if(j.getString("type")=="spot"){editFloat("Inner cone","innerConeAngle",j,30,0,180);editFloat("Outer cone","outerConeAngle",j,45,0,180);}
            editFloat("Intensity","intensity",j,1,0,100);editFloat("Range","range",j,10,.01f,1000);
            glm::vec3 color(1);if(j["color"].isArray())for(int a=0;a<3;++a)color[a]=j["color"][a].getNumber();
            changed=ImGui::ColorEdit3("Light color",glm::value_ptr(color));if(ImGui::IsItemActivated())++editToken;
            if(changed)document.editSelected("Light color",[&](SimpleJson& target){target["color"]=SimpleJson::parse("[]");for(int a=0;a<3;++a)target["color"].push_back(SimpleJson(double(color[a])));},editToken);
            if(j.hasKey("anchor"))ImGui::TextWrapped("Anchored light. Moving it adjusts its anchor offset.");
        }
    }
    void objectList() {
        ImGui::InputTextWithHint("##filter","Filter objects...",filterInput,sizeof(filterInput));
        auto root=document.map().toJson();
        auto list=[&](const char* title,ObjectKind kind,SimpleJson array){
            if(!ImGui::CollapsingHeader(title,ImGuiTreeNodeFlags_DefaultOpen))return;
            for(size_t i=0;i<array.size();++i){
                const auto& object=array[i];std::string label=object.getString("name");
                if(label.empty())label=object.getString("material",object.getString("type","Object"));
                if(object.hasKey("id"))label+=" #"+std::to_string(int(object.getNumber("id")));
                else label+=" ["+std::to_string(i)+"]";
                if(std::strlen(filterInput) && label.find(filterInput)==std::string::npos)continue;
                ImGui::PushID(int(kind)*100000+int(i));
                if(ImGui::Selectable(label.c_str(),document.selection()==Selection{kind,i}))document.select({kind,i});
                if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))focusSelection();
                ImGui::PopID();
            }
        };
        list("Geometry",ObjectKind::Object,root["geometry"]["objects"]);
        list("Legacy meshes",ObjectKind::Brush,root["geometry"]["brushes"]);
        list("Entities",ObjectKind::Entity,root["entities"]);
        list("Lights",ObjectKind::Light,root["lights"]);
    }
    void worldSettings() {
        auto root=document.map().toJson();auto ws=document.map().getWorldSettings();
        ImGui::Text("Schema: %.0f",root.getNumber("schemaVersion",1));
        ImGui::TextWrapped("Mode: %s",root.getString("gameMode","legacy").c_str());
        const char* presets[]={"none","starry_crystal"};int preset=root.getString("environmentPreset")=="starry_crystal"?1:0;
        if(ImGui::Combo("Environment",&preset,presets,2))document.applyJson("Environment preset",[&](SimpleJson& json){json["environmentPreset"]=SimpleJson(presets[preset]);});
        const auto env=document.map().getEnvironment();
        float start=env.transformationStart,duration=env.transformationDuration;int particles=int(env.particleCount);
        bool ec=ImGui::DragFloat("Shift time",&start,.1f,0,3600);if(ImGui::IsItemActivated())++editToken;
        if(ec)document.applyJson("Shift start",[&](SimpleJson& json){json["environment"]["transformationStart"]=SimpleJson(double(start));},editToken);
        ec=ImGui::DragFloat("Duration",&duration,.1f,.1f,600);if(ImGui::IsItemActivated())++editToken;
        if(ec)document.applyJson("Shift duration",[&](SimpleJson& json){json["environment"]["transformationDuration"]=SimpleJson(double(duration));},editToken);
        if(ImGui::InputInt("Particles",&particles))document.applyJson("Particle count",[&](SimpleJson& json){json["environment"]["particleCount"]=SimpleJson(double(std::clamp(particles,0,2000)));});
        bool changed=ImGui::DragFloat("Gravity",&ws.gravity,.1f);if(ImGui::IsItemActivated())++editToken;
        if(ImGui::IsItemHovered())ImGui::SetTooltip("Compatibility field. Saved in the map; does not override the game player gravity.");
        if(changed)document.applyJson("World gravity",[&](SimpleJson& json){json["worldSettings"]["gravity"]=SimpleJson(double(ws.gravity));},editToken);
        changed=ImGui::ColorEdit3("Ambient",glm::value_ptr(ws.ambientLight));if(ImGui::IsItemActivated())++editToken;
        if(changed)document.applyJson("Ambient color",[&](SimpleJson& json){auto value=SimpleJson::parse("[]");for(int i=0;i<3;++i)value.push_back(SimpleJson(double(ws.ambientLight[i])));json["worldSettings"]["ambientLight"]=value;},editToken);
        changed=ImGui::ColorEdit3("Background",glm::value_ptr(ws.backgroundColor));if(ImGui::IsItemActivated())++editToken;
        if(changed)document.applyJson("Background color",[&](SimpleJson& json){auto value=SimpleJson::parse("[]");for(int i=0;i<3;++i)value.push_back(SimpleJson(double(ws.backgroundColor[i])));json["worldSettings"]["backgroundColor"]=value;},editToken);
        ImGui::TextWrapped("Preview shows geometry and markers. Game materials, lighting and effects are not simulated here.");
    }
    void dialogs() {
        if(confirmUnsaved)ImGui::OpenPopup("Unsaved changes");
        const auto screen=ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos({screen.x*.5f,screen.y*.5f},ImGuiCond_Always,{.5f,.5f});
        ImGui::SetNextWindowSize({std::min(560.0f,screen.x-32),190},ImGuiCond_Always);
        if(ImGui::BeginPopupModal("Unsaved changes",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings)){
            if(!confirmUnsaved)ImGui::CloseCurrentPopup();
            ImGui::TextWrapped("Save your changes before continuing?");
            ImGui::TextWrapped("%s",document.path().empty()?"Untitled map":document.path().c_str());
            modalButtonsReachable=true;
            auto checkButton=[&](){const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();modalButtonsReachable=modalButtonsReachable && a.x>=0 && a.y>=0 && b.x<=screen.x && b.y<=screen.y;};
            if(ImGui::Button("Save and continue")){
                confirmUnsaved=false;ImGui::CloseCurrentPopup();saveThenContinue=true;save();
            }checkButton();
            ImGui::SameLine();if(ImGui::Button("Discard changes")){ImGui::CloseCurrentPopup();discardPending();}checkButton();
            ImGui::SameLine();if(ImGui::Button("Cancel")){ImGui::CloseCurrentPopup();cancelPending();}checkButton();
            const auto cancelMin=ImGui::GetItemRectMin(),cancelMax=ImGui::GetItemRectMax();cancelButtonCenter={(cancelMin.x+cancelMax.x)*.5f,(cancelMin.y+cancelMax.y)*.5f};
            ImGui::EndPopup();
        }
        if(browse)ImGui::OpenPopup("Choose map file");
        ImGui::SetNextWindowPos({screen.x*.5f,screen.y*.5f},ImGuiCond_Always,{.5f,.5f});
        ImGui::SetNextWindowSize({std::min(660.0f,screen.x-32),std::min(480.0f,screen.y-32)},ImGuiCond_Appearing);
        if(ImGui::BeginPopupModal("Choose map file",nullptr,ImGuiWindowFlags_NoSavedSettings)){
            ImGui::TextUnformatted(saveDialog?"SAVE MAP AS":"OPEN MAP");
            if(ImGui::Button("Game maps")){browseDirectory=SILIC2_ROOT "/res/maps";copy(directoryInput,browseDirectory);}
            ImGui::SameLine();if(ImGui::Button("Parent")){browseDirectory=fs::path(browseDirectory).parent_path().string();copy(directoryInput,browseDirectory);}
            ImGui::SameLine();if(ImGui::Button("Home")){const char* home=std::getenv("HOME");if(home){browseDirectory=home;copy(directoryInput,browseDirectory);}}
            ImGui::SetNextItemWidth(-60);bool go=ImGui::InputText("##directory",directoryInput,sizeof(directoryInput),ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();if(ImGui::Button("Go") || go)browseDirectory=directoryInput;
            ImGui::BeginChild("filelist",{0,-135},true);
            std::error_code error;std::vector<fs::directory_entry> entries;
            for(fs::directory_iterator it(browseDirectory,error),end;!error && it!=end;it.increment(error))entries.push_back(*it);
            std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){std::error_code e1,e2;bool ad=a.is_directory(e1),bd=b.is_directory(e2);return ad!=bd?ad>bd:a.path().filename()<b.path().filename();});
            if(error)ImGui::TextWrapped("Cannot read folder: %s",error.message().c_str());
            for(const auto& entry:entries){
                std::error_code ec;bool folder=entry.is_directory(ec);if(!folder && entry.path().extension()!=".json")continue;
                std::string label=(folder?"[DIR] ":"")+entry.path().filename().string();
                if(ImGui::Selectable(label.c_str(),!folder && entry.path().filename().string()==fileInput,ImGuiSelectableFlags_AllowDoubleClick)){
                    if(folder){browseDirectory=entry.path().string();copy(directoryInput,browseDirectory);break;}
                    copy(fileInput,entry.path().filename().string());
                    if(!saveDialog && ImGui::IsMouseDoubleClicked(0)){browse=false;ImGui::CloseCurrentPopup();request(Action::Open,entry.path().string());break;}
                }
            }
            ImGui::EndChild();
            ImGui::InputText("File name / path",fileInput,sizeof(fileInput));
            ImGui::TextWrapped("%s",document.error().c_str());
            if(ImGui::Button(saveDialog?"Save":"Open")){
                fs::path selected=fileInput;if(selected.is_relative())selected=fs::path(browseDirectory)/selected;
                if(saveDialog && selected.extension().empty())selected+=".json";
                if(std::strlen(fileInput)){
                    if(saveDialog){
                        std::error_code ec;if(fs::exists(selected,ec) && selected.lexically_normal()!=fs::path(document.path()).lexically_normal()){overwrite=true;overwritePath=selected.string();}
                        else if(saveAt(selected.string()))ImGui::CloseCurrentPopup();
                    }else{browse=false;ImGui::CloseCurrentPopup();request(Action::Open,selected.string());}
                }
            }
            ImGui::SameLine();if(ImGui::Button("Cancel")){browse=false;overwrite=false;cancelPending();ImGui::CloseCurrentPopup();}
            if(overwrite)ImGui::OpenPopup("Replace existing file?");
            if(ImGui::BeginPopupModal("Replace existing file?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
                ImGui::TextWrapped("Replace %s?",overwritePath.c_str());
                if(ImGui::Button("Replace")){overwrite=false;if(saveAt(overwritePath)){}ImGui::CloseCurrentPopup();}
                ImGui::SameLine();if(ImGui::Button("Keep existing")){overwrite=false;ImGui::CloseCurrentPopup();}
                ImGui::EndPopup();
            }
            if(!browse)ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
    void ui() {
        const auto screen=ImGui::GetIO().DisplaySize;
        const float left=std::clamp(screen.x*.20f,180.0f,260.0f),right=std::clamp(screen.x*.25f,220.0f,320.0f);
        constexpr float top=58,bottom=90;
        viewportPos={left,top};viewportSize={std::max(1.0f,screen.x-left-right),std::max(1.0f,screen.y-top-bottom)};
        ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({screen.x,top});
        ImGui::Begin("Toolbar",nullptr,fixed|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_MenuBar);
        if(ImGui::BeginMenuBar()){
            if(ImGui::BeginMenu("File")){
                if(ImGui::MenuItem("New map","Cmd/Ctrl+N"))request(Action::New);
                if(ImGui::MenuItem("New from Arena template"))request(Action::Arena);
                if(ImGui::MenuItem("Open...","Cmd/Ctrl+O"))openBrowser(false);
                if(ImGui::MenuItem("Save","Cmd/Ctrl+S"))save();
                if(ImGui::MenuItem("Save As...","Cmd/Ctrl+Shift+S"))save(true);
                ImGui::Separator();if(ImGui::MenuItem("Quit"))request(Action::Quit);ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Edit")){
                if(ImGui::MenuItem(("Undo "+document.undoLabel()).c_str(),"Cmd/Ctrl+Z",false,document.canUndo()))document.undo();
                if(ImGui::MenuItem(("Redo "+document.redoLabel()).c_str(),"Cmd/Ctrl+Shift+Z",false,document.canRedo()))document.redo();
                if(ImGui::MenuItem("Duplicate","Cmd/Ctrl+D",false,document.selected()))document.duplicate(snapEnabled?gridStep:1);
                if(ImGui::MenuItem("Delete","Delete",false,document.selected()))document.erase();ImGui::EndMenu();
            }
            ImGui::Text("  %s%s",document.path().empty()?"UNTITLED":fs::path(document.path()).filename().string().c_str(),document.dirty()?" * UNSAVED":"");
            ImGui::EndMenuBar();
        }
        if(ImGui::Button("Open"))openBrowser(false);ImGui::SameLine();if(ImGui::Button("Save"))save();
        ImGui::SameLine();if(ImGui::Button("Arena template"))request(Action::Arena);
        ImGui::SameLine();ImGui::BeginDisabled(!document.canUndo());if(ImGui::Button("Undo"))document.undo();ImGui::EndDisabled();
        ImGui::SameLine();ImGui::BeginDisabled(!document.canRedo());if(ImGui::Button("Redo"))document.redo();ImGui::EndDisabled();
        ImGui::SameLine();ImGui::Checkbox("Snap",&snapEnabled);ImGui::SameLine();ImGui::SetNextItemWidth(64);ImGui::DragFloat("##gridstep",&gridStep,.1f,.1f,10,"%.1f");
        ImGui::End();
        ImGui::SetNextWindowPos({0,top});ImGui::SetNextWindowSize({left,screen.y-top-bottom});
        ImGui::Begin("Objects",nullptr,fixed);objectList();ImGui::Separator();
        if(ImGui::Button("+ Add object"))ImGui::OpenPopup("Add object");
        if(ImGui::BeginPopup("Add object")){
            const auto position=EditorDocument::snap(camera.position+camera.front*5.0f,snapEnabled?gridStep:0);
            if(ImGui::MenuItem("Solid box"))document.addBox(position,{2,2,2});
            if(ImGui::MenuItem("Horizontal plane")){
                document.addBox(position,{4,.1f,4});
                document.editSelected("Make plane",[](SimpleJson& object){object["shape"]=SimpleJson("plane");object["size"][1]=SimpleJson(0.0);object["type"]=SimpleJson("floor");});
            }
            ImGui::Separator();
            const char* labels[]={"Player start","Light entity","Enemy spawn","Item pickup","Trigger"};
            for(int i=0;i<5;++i)if(ImGui::MenuItem(labels[i]))document.addEntity(static_cast<EntityType>(i),position);
            ImGui::Separator();
            const char* lights[]={"Point light","Directional light","Spot light"};const char* typeNames[]={"point","directional","spot"};
            for(int i=0;i<3;++i)if(ImGui::MenuItem(lights[i])){
                document.addLight(position);document.editSelected("Light type",[&](SimpleJson& light){light["type"]=SimpleJson(typeNames[i]);});
            }
            ImGui::EndPopup();
        }
        ImGui::End();
        ImGui::SetNextWindowPos({screen.x-right,top});ImGui::SetNextWindowSize({right,screen.y-top-bottom});
        ImGui::Begin("Inspector",nullptr,fixed);ImGui::PushItemWidth(-85);
        properties();ImGui::Separator();
        if(ImGui::CollapsingHeader("Map settings"))worldSettings();
        if(ImGui::CollapsingHeader("View settings")){
            ImGui::Checkbox("Grid",&showGrid);ImGui::Checkbox("Markers",&showMarkers);ImGui::Checkbox("Wireframe",&wireframe);
            ImGui::DragFloat("Camera speed",&camera.moveSpeed,.1f,.1f,100);if(ImGui::Button("Frame whole map"))focusAll();
        }
        ImGui::PopItemWidth();ImGui::End();
        ImGui::SetNextWindowPos(viewportPos);ImGui::SetNextWindowSize(viewportSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
        ImGui::Begin("Viewport",nullptr,fixed|ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::InvisibleButton("viewport",viewportSize,ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonRight);
        viewportHovered=ImGui::IsItemHovered();
        if(viewportHovered && ImGui::IsMouseClicked(0) && !navigating)pickAt(ImGui::GetMousePos());
        auto draw=ImGui::GetWindowDrawList();draw->AddText({viewportPos.x+12,viewportPos.y+12},IM_COL32(185,209,220,255),navigating?"FLYING - RELEASE RIGHT MOUSE TO EDIT":"CLICK TO SELECT  /  HOLD RIGHT MOUSE TO FLY");
        ImGui::End();ImGui::PopStyleVar();
        ImGui::SetNextWindowPos({0,screen.y-bottom});ImGui::SetNextWindowSize({screen.x,bottom});
        ImGui::Begin("Status",nullptr,fixed|ImGuiWindowFlags_NoDecoration);
        ImGui::TextWrapped("RMB + WASD: fly | Q/E: down/up | wheel: dolly | F: focus | arrows/PgUp/PgDn: move | Shift: faster");
        if(!document.error().empty())ImGui::TextWrapped("Edit rejected: %s",document.error().c_str());else ImGui::TextWrapped("%s",status.c_str());
        ImGui::End();dialogs();
        std::string title="Silic2 Map Editor - "+(document.path().empty()?"Untitled":fs::path(document.path()).filename().string())+(document.dirty()?" *":"");glfwSetWindowTitle(window,title.c_str());
    }
    void render() {
        int fw,fh,lw,lh;glfwGetFramebufferSize(window,&fw,&fh);glfwGetWindowSize(window,&lw,&lh);if(fw<=0 || fh<=0 || lw<=0 || lh<=0)return;
        glViewport(0,0,fw,fh);glClearColor(.045f,.06f,.085f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        const float sx=float(fw)/lw,sy=float(fh)/lh;
        int x=int(viewportPos.x*sx),y=int((lh-viewportPos.y-viewportSize.y)*sy),width=int(viewportSize.x*sx),height=int(viewportSize.y*sy);
        if(width<=0 || height<=0)return;
        glViewport(x,y,width,height);glEnable(GL_SCISSOR_TEST);glScissor(x,y,width,height);
        auto color=document.map().getWorldSettings().backgroundColor;glClearColor(color.r,color.g,color.b,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
        if(document.revision()!=renderedRevision){renderer.updateMapGeometry(document.map());renderedRevision=document.revision();}
        const float aspect=viewportSize.x/viewportSize.y;
        renderer.render(camera,aspect,wireframe);
        if(showGrid)renderer.renderGrid(camera,aspect);
        if(showMarkers)renderer.renderEntities(document.map(),camera,aspect);
        if(document.selected())renderer.renderSelection(document.selectedBounds(),camera,aspect);
        glDisable(GL_SCISSOR_TEST);glViewport(0,0,fw,fh);
    }
    void screenshot(const std::string& path) {
        int w,h;glfwGetFramebufferSize(window,&w,&h);std::vector<unsigned char> pixels(size_t(w)*h*3);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
        std::ofstream out(path,std::ios::binary);out<<"P6\n"<<w<<' '<<h<<"\n255\n";for(int row=h-1;row>=0;--row)out.write(reinterpret_cast<char*>(pixels.data()+size_t(row)*w*3),w*3);
    }
};

static void glfw_error_callback(int error,const char* description){std::cerr<<"GLFW "<<error<<": "<<description<<'\n';}

int main(int argc,char** argv) {
    if(argc>1 && std::string(argv[1])=="--self-test")return runEditorSelfTests(argc>2?argv[2]:SILIC2_ROOT "/res/maps/test_room.json",argc>3?argv[3]:"/tmp/silic2-editor-self-test");
    bool smoke=argc>1 && std::string(argv[1])=="--smoke";
    const std::string fixture=smoke?(argc>2?argv[2]:SILIC2_ROOT "/res/maps/test_room.json"):(argc>1?argv[1]:"");
    const std::string output=smoke?(argc>3?argv[3]:"/tmp/silic2-editor-smoke"):"";
    glfwSetErrorCallback(glfw_error_callback);if(!glfwInit())return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    GLFWwindow* window=glfwCreateWindow(1440,900,"Silic2 Map Editor",nullptr,nullptr);if(!window){glfwTerminate();return 2;}
    glfwSetWindowSizeLimits(window,800,600,GLFW_DONT_CARE,GLFW_DONT_CARE);glfwMakeContextCurrent(window);glfwSwapInterval(smoke?0:1);
#ifndef __APPLE__
    if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))return 3;
#endif
    IMGUI_CHECKVERSION();ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;
    ImGui::StyleColorsDark();auto& style=ImGui::GetStyle();style.WindowRounding=0;style.FrameRounding=3;style.ItemSpacing={8,8};
    style.Colors[ImGuiCol_Header]={.12f,.37f,.37f,1};style.Colors[ImGuiCol_Button]={.1f,.3f,.34f,1};
    ImGui_ImplGlfw_InitForOpenGL(window,true);ImGui_ImplOpenGL3_Init("#version 330 core");
    int result=0;
    try {
        MapEditor editor(window);
        if(!fixture.empty()){editor.request(MapEditor::Action::Open,fixture);if(!editor.document.error().empty())throw std::runtime_error(editor.document.error());}
        int frame=0;double previous=glfwGetTime();Selection smokeSelection;glm::vec3 smokeInitialCenter(0);size_t smokeInitialCount=0;double smokeSelectedId=-1;
        auto objectCount=[&](){return editor.document.map().getBrushes().size()+editor.document.map().getEntities().size()+editor.document.map().getLights().size();};
        auto center=[&](){auto bounds=editor.document.selectedBounds();return (bounds.min+bounds.max)*.5f;};
        if(smoke)fs::create_directories(output);
        while(!glfwWindowShouldClose(window)){
            glfwPollEvents();
            if(glfwWindowShouldClose(window)){glfwSetWindowShouldClose(window,GL_FALSE);editor.request(MapEditor::Action::Quit);if(glfwWindowShouldClose(window))break;}
            const double now=glfwGetTime();float dt=std::min(.05f,float(now-previous));previous=now;
            ImGui_ImplOpenGL3_NewFrame();ImGui_ImplGlfw_NewFrame();
            if(smoke){
                // Events go through the same ImGui input/shortcut path as real use.
                if(frame==2){
                    const auto& meshes=editor.document.map().getLegacyBrushes();
                    if(!meshes.empty() && editor.document.map().getGameMode()!=GameMode::ArenaSurvival)editor.document.select({ObjectKind::Brush,meshes.size()>1?1u:0u});
                    else {
                        const auto& objects=editor.document.map().getObjects();size_t selected=objects.size()>1?1:0;
                        for(size_t i=0;i<objects.size();++i)if(objects[i].name.find("cover")!=std::string::npos || objects[i].name.find("Cover")!=std::string::npos){selected=i;break;}
                        editor.document.select({ObjectKind::Object,selected});
                    }
                    editor.focusSelection();auto b=editor.document.selectedBounds();auto target=(b.min+b.max)*.5f;
                    smokeSelection=editor.document.selection();smokeInitialCenter=target;smokeInitialCount=objectCount();smokeSelectedId=editor.document.selectedJson().getNumber("id",-1);
                    auto p=editor.project(target);io.AddMousePosEvent(p.x,p.y);io.AddMouseButtonEvent(0,true);
                }
                if(frame==3)io.AddMouseButtonEvent(0,false);
                if(frame==4)io.AddKeyEvent(ImGuiKey_RightArrow,true);
                if(frame==5)io.AddKeyEvent(ImGuiKey_RightArrow,false);
                if(frame==6){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_D,true);}
                if(frame==7){io.AddKeyEvent(ImGuiKey_D,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
                if(frame==8){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_Z,true);}
                if(frame==9){io.AddKeyEvent(ImGuiKey_Z,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
                if(frame==10){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_Y,true);}
                if(frame==11){io.AddKeyEvent(ImGuiKey_Y,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
                if(frame==18){auto p=editor.cancelButtonCenter;io.AddMousePosEvent(p.x,p.y);io.AddMouseButtonEvent(0,true);}
                if(frame==19){auto p=editor.cancelButtonCenter;io.AddMousePosEvent(p.x,p.y);io.AddMouseButtonEvent(0,false);}
            }
            ImGui::NewFrame();editor.ui();editor.processInput(dt);
            if(smoke){
                if(frame>=2 && frame<=13)std::cout<<"INPUT FRAME "<<frame<<" selection "<<int(editor.document.selection().kind)<<":"<<editor.document.selection().index<<" id "<<editor.document.selectedJson().getNumber("id",-1)<<" center "<<center().x<<","<<center().y<<","<<center().z<<" objects "<<objectCount()<<" dirty "<<editor.document.dirty()<<" error "<<editor.document.error()<<'\n';
                if(frame==3 && (!(editor.document.selection()==smokeSelection) || editor.document.selectedJson().getNumber("id",-1)!=smokeSelectedId))throw std::runtime_error("synthetic click selected unexpected object");
                if(frame==5 && (!editor.document.dirty() || glm::length(center()-smokeInitialCenter-glm::vec3(1,0,0))>.01f))throw std::runtime_error("synthetic arrow move not processed");
                if(frame==7 && objectCount()!=smokeInitialCount+1)throw std::runtime_error("synthetic duplicate shortcut failed");
                if(frame==9 && objectCount()!=smokeInitialCount)throw std::runtime_error("synthetic undo shortcut failed");
                if(frame==11 && objectCount()!=smokeInitialCount+1)throw std::runtime_error("synthetic redo shortcut failed");
                if(frame==12){editor.status="SYNTHETIC SMOKE: click / move / duplicate / undo / redo";editor.request(MapEditor::Action::New);if(!editor.unsavedPrompt())throw std::runtime_error("dirty guard missing");}
                if(frame>=16 && frame<=20)std::cout<<"MODAL FRAME "<<frame<<" mouse "<<io.MousePos.x<<","<<io.MousePos.y<<" cancel "<<editor.cancelButtonCenter.x<<","<<editor.cancelButtonCenter.y<<" down "<<io.MouseDown[0]<<" pending "<<editor.unsavedPrompt()<<" popup "<<ImGui::IsPopupOpen("Unsaved changes")<<'\n';
                if((frame==14 || frame==16) && !editor.modalButtonsReachable)throw std::runtime_error("modal buttons outside window after resize");
                if(frame==20){if(editor.unsavedPrompt() || ImGui::IsPopupOpen("Unsaved changes"))throw std::runtime_error("Cancel button left dirty modal open");if(!editor.saveAt(output+"/edited.json"))throw std::runtime_error(editor.document.error());Map reloaded;if(!reloaded.loadFromFile(output+"/edited.json"))throw std::runtime_error("shared game loader rejected edited map");if(editor.document.dirty() || reloaded.toJson().toString()!=editor.document.map().toJson().toString())throw std::runtime_error("saved map differs from edited state");std::cout<<"SAVE VERIFIED objects "<<objectCount()<<" selection id "<<editor.document.selectedJson().getNumber("id",-1)<<" center "<<center().x<<","<<center().y<<","<<center().z<<'\n';}
                if(frame==15)glfwSetWindowSize(window,800,600);
            }
            editor.render();ImGui::Render();ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            if(smoke && (frame==11 || frame==14 || frame==16 || frame==22)){glFinish();editor.screenshot(output+"/frame-"+std::to_string(frame)+".ppm");int lw,lh,fw,fh;glfwGetWindowSize(window,&lw,&lh);glfwGetFramebufferSize(window,&fw,&fh);std::cout<<"GL FRAME "<<frame<<" logical "<<lw<<'x'<<lh<<" framebuffer "<<fw<<'x'<<fh<<'\n';if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("GL smoke error");}
            glfwSwapBuffers(window);if(smoke && ++frame>24)break;
        }
        editor.stopNavigation();
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';result=4;}
    // MapEditor and every GL resource are already destroyed while the context is live.
    ImGui_ImplOpenGL3_Shutdown();ImGui_ImplGlfw_Shutdown();ImGui::DestroyContext();glfwDestroyWindow(window);glfwTerminate();return result;
}
