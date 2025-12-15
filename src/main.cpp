#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "Engine/Camera.h"
#include "Game/KeybindManager.h"
#include <iostream>

Camera camera;
KeybindManager keybinds;
double lastX = 640, lastY = 360;
bool firstMouse = true;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    keybinds.OnKeyEvent(key, action);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    keybinds.OnMouseButtonEvent(button, action);
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float dx = xpos - lastX;
    float dy = ypos - lastY;
    lastX = xpos;
    lastY = ypos;
    
    camera.ProcessMouse(dx, dy);
}

void processInput(float dt) {
    float forward = keybinds.GetAxis(KeybindManager::MOVE_FORWARD, KeybindManager::MOVE_BACKWARD);
    float right = keybinds.GetAxis(KeybindManager::MOVE_RIGHT, KeybindManager::MOVE_LEFT);
    
    float speed = keybinds.IsPressed(KeybindManager::SPRINT) ? 10.0f : 5.0f;
    
    if (forward != 0 || right != 0) {
        Vec3 fwd = camera.GetForward();
        Vec3 rgt = camera.GetRight();
        
        fwd.y = 0;
        fwd = fwd.normalized();
        
        camera.position = camera.position + fwd * forward * dt * speed;
        camera.position = camera.position + rgt * right * dt * speed;
    }
    
    if (keybinds.IsPressed(KeybindManager::FIRE)) {
        // TODO: Fire weapon
    }
    
    if (keybinds.IsPressed(KeybindManager::RELOAD)) {
        // TODO: Reload
    }
}

static const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 vertexColor;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

static const char* fragmentShaderSrc = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader error:\n" << log << std::endl;
    }
    return s;
}

void setIdentityMatrix(float* mat) {
    for (int i = 0; i < 16; i++) mat[i] = (i % 5 == 0) ? 1.0f : 0.0f;
}

int main() {
    if (!glfwInit()) return -1;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Arena Shooter", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "GLAD failed" << std::endl;
        return -1;
    }
    
    glEnable(GL_DEPTH_TEST);
    
    // Arena geometry
    float vertices[] = {
        // Floor
        -20, 0, -20,  0.3f, 0.3f, 0.3f,
         20, 0, -20,  0.3f, 0.3f, 0.3f,
         20, 0,  20,  0.3f, 0.3f, 0.3f,
        -20, 0,  20,  0.3f, 0.3f, 0.3f,
        
        // North wall
        -20, 0, -20,  0.2f, 0.2f, 0.8f,
         20, 0, -20,  0.2f, 0.2f, 0.8f,
         20, 5, -20,  0.2f, 0.2f, 0.8f,
        -20, 5, -20,  0.2f, 0.2f, 0.8f,
        
        // South wall
        -20, 0, 20,  0.8f, 0.2f, 0.2f,
         20, 0, 20,  0.8f, 0.2f, 0.2f,
         20, 5, 20,  0.8f, 0.2f, 0.2f,
        -20, 5, 20,  0.8f, 0.2f, 0.2f,
        
        // West wall
        -20, 0, -20,  0.2f, 0.8f, 0.2f,
        -20, 0,  20,  0.2f, 0.8f, 0.2f,
        -20, 5,  20,  0.2f, 0.8f, 0.2f,
        -20, 5, -20,  0.2f, 0.8f, 0.2f,
        
        // East wall
        20, 0, -20,  0.8f, 0.8f, 0.2f,
        20, 0,  20,  0.8f, 0.8f, 0.2f,
        20, 5,  20,  0.8f, 0.8f, 0.2f,
        20, 5, -20,  0.8f, 0.8f, 0.2f,
    };
    
    unsigned int indices[] = {
        0,1,2, 0,2,3,
        4,5,6, 4,6,7,
        8,9,10, 8,10,11,
        12,13,14, 12,14,15,
        16,17,18, 16,18,19
    };
    
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    float lastFrame = 0;
    
    std::cout << "Controls:\n";
    std::cout << "WASD - Move\n";
    std::cout << "Shift - Sprint\n";
    std::cout << "Mouse - Look\n";
    std::cout << "LMB - Fire\n";
    std::cout << "R - Reload\n";
    
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput(dt);
        
        if (keybinds.IsPressed(KeybindManager::PAUSE)) {
            glfwSetWindowShouldClose(window, true);
        }
        
        glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(program);
        
        float view[16], proj[16], model[16];
        camera.GetViewMatrix(view);
        
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / height;
        camera.GetProjectionMatrix(proj, aspect);
        
        setIdentityMatrix(model);
        
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, proj);
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, model);
        
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 30, GL_UNSIGNED_INT, 0);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
