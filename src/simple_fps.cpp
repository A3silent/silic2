#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include "shader.h"

class SimpleFPS {
private:
    GLFWwindow* window;
    std::unique_ptr<silic2::Shader> shader;
    GLuint VAO, VBO;
    
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;  
    glm::vec3 cameraUp;
    float yaw, pitch;
    
    float deltaTime;
    float lastFrame;
    float lastX, lastY;
    bool firstMouse;
    
    static constexpr int SCREEN_WIDTH = 1280;
    static constexpr int SCREEN_HEIGHT = 720;
    static constexpr float MOVEMENT_SPEED = 5.0f;
    static constexpr float MOUSE_SENSITIVITY = 0.1f;

public:
    SimpleFPS() : window(nullptr), VAO(0), VBO(0),
                  cameraPos(0.0f, 1.0f, 3.0f),
                  cameraFront(0.0f, 0.0f, -1.0f),
                  cameraUp(0.0f, 1.0f, 0.0f),
                  yaw(-90.0f), pitch(0.0f),
                  deltaTime(0.0f), lastFrame(0.0f),
                  lastX(SCREEN_WIDTH / 2.0f), lastY(SCREEN_HEIGHT / 2.0f),
                  firstMouse(true) {}
                  
    ~SimpleFPS() {
        cleanup();
    }
    
    bool initWindow() {
        std::cout << "Initializing GLFW..." << std::endl;
        
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        std::cout << "Creating window..." << std::endl;
        
        window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Simple FPS", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window);
        
        // Set callbacks
        glfwSetWindowUserPointer(window, this);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        std::cout << "Window created successfully" << std::endl;
        return true;
    }
    
    bool initOpenGL() {
        std::cout << "Loading OpenGL functions..." << std::endl;
        
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return false;
        }
        
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
        
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glEnable(GL_DEPTH_TEST);
        
        return true;
    }
    
    bool initShaders() {
        std::cout << "Loading shaders from files..." << std::endl;
        
        try {
            shader = std::make_unique<silic2::Shader>("res/shaders/simple.vert", "res/shaders/simple.frag");
            std::cout << "Shaders loaded successfully" << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load shaders: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool initGeometry() {
        std::cout << "Setting up geometry..." << std::endl;
        
        // Cube vertices
        float vertices[] = {
            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,

             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,

            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f
        };
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        std::cout << "Geometry set up successfully" << std::endl;
        return true;
    }
    
    void processInput() {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
            
        float velocity = MOVEMENT_SPEED * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += velocity * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= velocity * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
        // Vertical movement
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            cameraPos += velocity * cameraUp;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            cameraPos -= velocity * cameraUp;
    }
    
    void processMouseMovement(float xoffset, float yoffset) {
        xoffset *= MOUSE_SENSITIVITY;
        yoffset *= MOUSE_SENSITIVITY;
        
        yaw += xoffset;
        pitch += yoffset;
        
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
    
    void render() {
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        shader->use();
        
        // Create matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                               (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 
                                               0.1f, 100.0f);
        
        // Set uniforms
        shader->setMat4("view", view);
        shader->setMat4("projection", projection);
        
        // Draw floor
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(20.0f, 0.1f, 20.0f));
        shader->setMat4("model", model);
        shader->setVec3("color", 0.3f, 0.3f, 0.3f);
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        // Draw cubes
        for (int i = -5; i <= 5; i += 2) {
            for (int j = -5; j <= 5; j += 2) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(i, 0.0f, j));
                shader->setMat4("model", model);
                shader->setVec3("color", 0.8f, 0.3f, 0.3f);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
    }
    
    void run() {
        if (!initWindow() || !initOpenGL() || !initShaders() || !initGeometry()) {
            return;
        }
        
        std::cout << "Starting main loop..." << std::endl;
        
        while (!glfwWindowShouldClose(window)) {
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            
            processInput();
            render();
            
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        
        std::cout << "Main loop ended" << std::endl;
    }
    
    void cleanup() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        shader.reset(); // Smart pointer will handle cleanup
        
        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }
    
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
        SimpleFPS* fps = static_cast<SimpleFPS*>(glfwGetWindowUserPointer(window));
        
        if (fps->firstMouse) {
            fps->lastX = xpos;
            fps->lastY = ypos;
            fps->firstMouse = false;
        }
        
        float xoffset = xpos - fps->lastX;
        float yoffset = fps->lastY - ypos;
        fps->lastX = xpos;
        fps->lastY = ypos;
        
        fps->processMouseMovement(xoffset, yoffset);
    }
};

int main() {
    SimpleFPS fps;
    fps.run();
    return 0;
}