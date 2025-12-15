// LocalPlayer.h - Fixed with proper includes

#pragma once
// LocalPlayer.h - Fixed with proper includes

#pragma once

#ifndef GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#endif
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "PCD/PCDTypes.h"
#include <cstdint>
#include <string>

// Forward declarations
namespace Network {
    class NetworkManager;
}

namespace Player {

class LocalPlayer {
private:
    uint32_t playerId;
    std::string playerName;
    
    GLFWwindow* window;
    Network::NetworkManager* netManager;
    
    // Transform
    glm::vec3 position;
    glm::vec3 velocity;
    float yaw;
    float pitch;
    
    // Movement
    float moveSpeed;
    float jumpForce;
    float gravity;
    bool isGrounded;
    
    // Mouse
    bool cursorLocked;
    double lastMouseX, lastMouseY;
    float mouseSensitivity;
    
    // Network
    float networkUpdateTimer;
    float networkUpdateInterval;
    glm::vec3 lastSentPosition;
    float lastSentYaw;
    float lastSentPitch;

public:
    LocalPlayer(uint32_t id, const std::string& name, GLFWwindow* win, Network::NetworkManager* net);
    
    void Update(float deltaTime);
    void SetCursorLocked(bool locked);
    
    glm::vec3 GetPosition() const { return position; }
    glm::vec3 GetViewDirection() const;
    float GetYaw() const { return yaw; }
    float GetPitch() const { return pitch; }

private:
    void ProcessInput(float deltaTime);
    void ProcessMouseInput(float deltaTime);
    void UpdatePhysics(float deltaTime);
    void SendNetworkUpdate();
};

} // namespace Player
