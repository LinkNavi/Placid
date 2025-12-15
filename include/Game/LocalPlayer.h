// LocalPlayer.h - Updated with Q3 movement

#pragma once

#include "Player.h"
#include "PlayerController.h"
#include "Network/NetworkManager.h"
#include <GLFW/glfw3.h>

namespace Game {

class LocalPlayer : public Player {
private:
    GLFWwindow* window;
    PlayerController controller;
    bool cursorLocked;
    double lastMouseX, lastMouseY;
    
    Network::NetworkManager* netManager;
    float networkUpdateTimer;
    float networkUpdateInterval;
    
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
        , netManager(net)
        , networkUpdateTimer(0.0f)
        , networkUpdateInterval(0.05f)
        , lastSentPosition(0, 0, 0)
        , lastSentYaw(0)
        , lastSentPitch(0)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
    }
    
    void Update(float deltaTime) override {
        if (!isAlive) return;
        
        ProcessMouseInput(deltaTime);
        controller.ProcessInput(window, deltaTime);
        controller.Update(deltaTime);
        
        // Sync with Player base class
        position = controller.position;
        velocity = controller.velocity;
        yaw = controller.yaw;
        pitch = controller.pitch;
        isGrounded = controller.isGrounded;
        
        // Send network updates
        networkUpdateTimer += deltaTime;
        if (networkUpdateTimer >= networkUpdateInterval) {
            SendNetworkUpdate();
            networkUpdateTimer = 0.0f;
        }
    }
    
    void Render() override {
        // Local player doesn't render itself (first-person)
    }
    
    void SetCursorLocked(bool locked) {
        cursorLocked = locked;
        glfwSetInputMode(window, GLFW_CURSOR, 
                        locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
    
    PCD::Vec3 GetEyePosition() const {
        return controller.GetEyePosition();
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
        
        controller.ProcessMouseInput(deltaX, deltaY);
    }
    
    void SendNetworkUpdate() {
        if (!netManager) return;
        
        float positionDelta = (position - lastSentPosition).Length();
        float rotationDelta = std::abs(yaw - lastSentYaw) + std::abs(pitch - lastSentPitch);
        
        if (positionDelta > 0.01f || rotationDelta > 0.01f) {
            netManager->SendPlayerState(
                position.x, position.y, position.z,
                yaw, pitch,
                health, 0
            );
            
            lastSentPosition = position;
            lastSentYaw = yaw;
            lastSentPitch = pitch;
        }
    }
};

} // namespace Game
