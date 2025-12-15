// LocalPlayer.h - Player controlled by local input

#pragma once

#include "Player.h"
#include "Network/NetworkManager.h"
#include <GLFW/glfw3.h>
#include <cmath>

namespace Game {

class LocalPlayer : public Player {
private:
    // Input
    GLFWwindow* window;
    bool cursorLocked;
    double lastMouseX, lastMouseY;
    
    // Movement parameters
    float moveSpeed;
    float sprintSpeed;
    float jumpForce;
    float mouseSensitivity;
    float gravity;
    
    // Network
    Network::NetworkManager* netManager;
    float networkUpdateTimer;
    float networkUpdateInterval;
    
    // Last sent state (for delta compression)
    PCD::Vec3 lastSentPosition;
    float lastSentYaw;
    float lastSentPitch;

public:
    LocalPlayer(uint32_t id, const std::string& name, GLFWwindow* win, Network::NetworkManager* net)
        : Player(id, name)
        , window(win)
        , cursorLocked(true)
        , lastMouseX(0)
        , lastMouseY(0)
        , moveSpeed(5.0f)          // 5 units/sec
        , sprintSpeed(8.0f)        // 8 units/sec when sprinting
        , jumpForce(7.0f)          // Jump velocity
        , mouseSensitivity(0.002f) // Mouse sensitivity
        , gravity(-20.0f)          // Gravity acceleration
        , netManager(net)
        , networkUpdateTimer(0.0f)
        , networkUpdateInterval(0.05f) // Send updates at 20Hz
        , lastSentPosition(0, 0, 0)
        , lastSentYaw(0)
        , lastSentPitch(0)
    {
        // Lock cursor initially
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        // Get initial mouse position
        glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
    }
    
    void Update(float deltaTime) override {
        if (!isAlive) return;
        
        // Handle input
        ProcessMouseInput(deltaTime);
        ProcessKeyboardInput(deltaTime);
        
        // Apply physics
        ApplyPhysics(deltaTime);
        
        // Send network updates
        networkUpdateTimer += deltaTime;
        if (networkUpdateTimer >= networkUpdateInterval) {
            SendNetworkUpdate();
            networkUpdateTimer = 0.0f;
        }
    }
    
    void Render() override {
        // Local player doesn't render itself (first-person view)
        // But we could render arms/weapon here later
    }
    
    // Lock/unlock cursor (for menu access)
    void SetCursorLocked(bool locked) {
        cursorLocked = locked;
        if (locked) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    
private:
    void ProcessMouseInput(float deltaTime) {
        if (!cursorLocked) return;
        
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        
        double deltaX = mouseX - lastMouseX;
        double deltaY = mouseY - lastMouseY;
        
        lastMouseX = mouseX;
        lastMouseY = mouseY;
        
        // Update rotation
        yaw -= deltaX * mouseSensitivity;
        pitch -= deltaY * mouseSensitivity;
        
        // Clamp pitch to prevent flipping
        const float maxPitch = 1.5f; // ~85 degrees
        if (pitch > maxPitch) pitch = maxPitch;
        if (pitch < -maxPitch) pitch = -maxPitch;
    }
    
    void ProcessKeyboardInput(float deltaTime) {
        // Get input
        bool wPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        bool sPressed = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        bool aPressed = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        bool dPressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        
        // Calculate movement direction
        PCD::Vec3 moveDir(0, 0, 0);
        
        if (wPressed) {
            PCD::Vec3 forward = GetForward();
            forward.y = 0; // Don't fly when looking up/down
            forward = forward.Normalized();
            moveDir = moveDir + forward;
        }
        if (sPressed) {
            PCD::Vec3 forward = GetForward();
            forward.y = 0;
            forward = forward.Normalized();
            moveDir = moveDir - forward;
        }
        if (aPressed) {
            PCD::Vec3 right = GetRight();
            moveDir = moveDir - right;
        }
        if (dPressed) {
            PCD::Vec3 right = GetRight();
            moveDir = moveDir + right;
        }
        
        // Normalize diagonal movement
        if (moveDir.Length() > 0) {
            moveDir = moveDir.Normalized();
        }
        
        // Apply speed
        float currentSpeed = shiftPressed ? sprintSpeed : moveSpeed;
        moveDir = moveDir * currentSpeed;
        
        // Set horizontal velocity
        velocity.x = moveDir.x;
        velocity.z = moveDir.z;
        
        // Jump
        if (spacePressed && isGrounded) {
            velocity.y = jumpForce;
            isGrounded = false;
        }
    }
    
    void ApplyPhysics(float deltaTime) {
        // Apply gravity
        if (!isGrounded) {
            velocity.y += gravity * deltaTime;
        }
        
        // Update position
        position = position + velocity * deltaTime;
        
        // Simple ground check (Y = 0 for now)
        if (position.y <= 0) {
            position.y = 0;
            velocity.y = 0;
            isGrounded = true;
        } else {
            isGrounded = false;
        }
    }
    
    void SendNetworkUpdate() {
        if (!netManager) return;
        
        // Only send if position/rotation changed significantly
        float positionDelta = (position - lastSentPosition).Length();
        float rotationDelta = std::abs(yaw - lastSentYaw) + std::abs(pitch - lastSentPitch);
        
        if (positionDelta > 0.01f || rotationDelta > 0.01f) {
            netManager->SendPlayerState(
                position.x, position.y, position.z,
                yaw, pitch,
                health, 0 // weapon = 0 for now
            );
            
            lastSentPosition = position;
            lastSentYaw = yaw;
            lastSentPitch = pitch;
        }
    }
};

} // namespace Game
