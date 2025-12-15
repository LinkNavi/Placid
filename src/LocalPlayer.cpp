// LocalPlayer.cpp - Implementation

#include "Game/LocalPlayer.h"
#include "Network/NetworkManager.h"
#include <cmath>
#include <iostream>

namespace Player {

LocalPlayer::LocalPlayer(uint32_t id, const std::string& name, GLFWwindow* win, Network::NetworkManager* net)
    : playerId(id)
    , playerName(name)
    , window(win)
    , netManager(net)
    , position(0.0f, 2.0f, 0.0f)
    , velocity(0.0f)
    , yaw(0.0f)
    , pitch(0.0f)
    , moveSpeed(5.0f)
    , jumpForce(8.0f)
    , gravity(20.0f)
    , isGrounded(false)
    , cursorLocked(true)
    , lastMouseX(0)
    , lastMouseY(0)
    , mouseSensitivity(0.002f)
    , networkUpdateTimer(0.0f)
    , networkUpdateInterval(0.05f)
    , lastSentPosition(0.0f)
    , lastSentYaw(0.0f)
    , lastSentPitch(0.0f)
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
}

void LocalPlayer::Update(float deltaTime) {
    ProcessMouseInput(deltaTime);
    ProcessInput(deltaTime);
    UpdatePhysics(deltaTime);
    
    // Send network updates
    networkUpdateTimer += deltaTime;
    if (networkUpdateTimer >= networkUpdateInterval) {
        SendNetworkUpdate();
        networkUpdateTimer = 0.0f;
    }
}

void LocalPlayer::ProcessMouseInput(float deltaTime) {
    if (!cursorLocked) return;
    
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    
    double deltaX = mouseX - lastMouseX;
    double deltaY = mouseY - lastMouseY;
    
    lastMouseX = mouseX;
    lastMouseY = mouseY;
    
    yaw -= deltaX * mouseSensitivity;
    pitch -= deltaY * mouseSensitivity;
    
    // Clamp pitch
    const float maxPitch = 1.5f;
    if (pitch > maxPitch) pitch = maxPitch;
    if (pitch < -maxPitch) pitch = -maxPitch;
}

void LocalPlayer::ProcessInput(float deltaTime) {
    glm::vec3 forward = GetViewDirection();
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    
    // Ground movement only
    forward.y = 0;
    forward = glm::normalize(forward);
    right.y = 0;
    right = glm::normalize(right);
    
    glm::vec3 moveDir(0.0f);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= forward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += right;
    
    if (glm::length(moveDir) > 0.0f) {
        moveDir = glm::normalize(moveDir);
    }
    
    float speed = moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        speed *= 2.0f; // Sprint
    }
    
    if (isGrounded) {
        velocity.x = moveDir.x * speed;
        velocity.z = moveDir.z * speed;
    } else {
        // Air control
        velocity.x += moveDir.x * speed * 0.1f * deltaTime;
        velocity.z += moveDir.z * speed * 0.1f * deltaTime;
    }
    
    // Jump
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) {
        velocity.y = jumpForce;
        isGrounded = false;
    }
}

void LocalPlayer::UpdatePhysics(float deltaTime) {
    // Apply gravity
    if (!isGrounded) {
        velocity.y -= gravity * deltaTime;
    }
    
    // Apply drag
    float drag = isGrounded ? 10.0f : 1.0f;
    velocity.x *= (1.0f - drag * deltaTime);
    velocity.z *= (1.0f - drag * deltaTime);
    
    // Update position
    position += velocity * deltaTime;
    
    // Simple ground collision
    if (position.y <= 0.0f) {
        position.y = 0.0f;
        if (velocity.y < 0) {
            velocity.y = 0.0f;
        }
        isGrounded = true;
    } else if (position.y > 0.1f) {
        isGrounded = false;
    }
}

void LocalPlayer::SendNetworkUpdate() {
    if (!netManager) return;
    
    float positionDelta = glm::distance(position, lastSentPosition);
    float rotationDelta = std::abs(yaw - lastSentYaw) + std::abs(pitch - lastSentPitch);
    
    if (positionDelta > 0.01f || rotationDelta > 0.01f) {
        netManager->SendPlayerState(
            position.x, position.y, position.z,
            yaw, pitch,
            100, 0
        );
        
        lastSentPosition = position;
        lastSentYaw = yaw;
        lastSentPitch = pitch;
    }
}

glm::vec3 LocalPlayer::GetViewDirection() const {
    return glm::vec3(
        std::cos(pitch) * std::sin(yaw),
        std::sin(pitch),
        std::cos(pitch) * std::cos(yaw)
    );
}

void LocalPlayer::SetCursorLocked(bool locked) {
    cursorLocked = locked;
    glfwSetInputMode(window, GLFW_CURSOR, 
                    locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

} // namespace Player
