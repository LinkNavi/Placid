// Q3PlayerController.h - Quake 3 style movement

#pragma once

#include "PCD/PCDTypes.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm>

class PlayerController {
public:
    // Position and velocity
    PCD::Vec3 position;
    PCD::Vec3 velocity;
    
    // Camera
    float yaw;
    float pitch;
    float eyeHeight;
    
    // Physics state
    bool isGrounded;
    bool wasGrounded;
    float groundY;
    
    // Movement parameters (Quake 3 values)
    float moveSpeed;
    float groundAccel;
    float airAccel;
    float friction;
    float stopSpeed;
    float jumpForce;
    float gravity;
    float maxSpeed;
    float airMaxSpeed;
    float mouseSensitivity;
    
    // Strafe jumping
    float wishSpeed;
    PCD::Vec3 wishDir;
    
    PlayerController() 
        : position(0, 0, 0)
        , velocity(0, 0, 0)
        , yaw(0), pitch(0)
        , eyeHeight(1.7f)
        , isGrounded(false)
        , wasGrounded(false)
        , groundY(0)
        , moveSpeed(320.0f)      // Units per second
        , groundAccel(10.0f)     // Ground acceleration
        , airAccel(1.0f)         // Air acceleration (lower for air control)
        , friction(6.0f)         // Ground friction
        , stopSpeed(100.0f)      // Speed below which friction stops you
        , jumpForce(270.0f)      // Jump velocity
        , gravity(800.0f)        // Gravity acceleration
        , maxSpeed(320.0f)       // Max ground speed
        , airMaxSpeed(30.0f)     // Additional air speed from strafe
        , mouseSensitivity(0.002f)
        , wishSpeed(0)
        , wishDir(0, 0, 0)
    {}
    
    void ProcessMouseInput(float dx, float dy) {
        yaw -= dx * mouseSensitivity;
        pitch -= dy * mouseSensitivity;
        
        // Clamp pitch
        const float maxPitch = 1.5f;
        if (pitch > maxPitch) pitch = maxPitch;
        if (pitch < -maxPitch) pitch = -maxPitch;
    }
    
    void ProcessInput(GLFWwindow* window, float dt) {
        // Get input
        bool wPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        bool sPressed = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        bool aPressed = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        bool dPressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        
        // Calculate wish direction
        PCD::Vec3 forward = GetForward();
        PCD::Vec3 right = GetRight();
        
        // Ground movement only (no flying)
        forward.y = 0;
        forward = forward.Normalized();
        right.y = 0;
        right = right.Normalized();
        
        wishDir = PCD::Vec3(0, 0, 0);
        
        if (wPressed) wishDir = wishDir + forward;
        if (sPressed) wishDir = wishDir - forward;
        if (aPressed) wishDir = wishDir - right;
        if (dPressed) wishDir = wishDir + right;
        
        if (wishDir.Length() > 0) {
            wishDir = wishDir.Normalized();
        }
        
        wishSpeed = moveSpeed;
        
        // Apply movement
        if (isGrounded) {
            ApplyFriction(dt);
            GroundMove(dt);
        } else {
            AirMove(dt);
        }
        
        // Jump
        if (spacePressed) {
            if (isGrounded) {
                // Jump
                velocity.y = jumpForce;
                isGrounded = false;
            } else {
                // Auto-bhop: if holding jump and we just hit ground, jump again
                // This is handled in the physics update
            }
        }
    }
    
    void Update(float dt) {
        wasGrounded = isGrounded;
        
        // Apply gravity
        if (!isGrounded) {
            velocity.y -= gravity * dt;
        }
        
        // Update position
        position = position + velocity * dt;
        
        // Simple ground collision (will be replaced with map collision)
        CheckGroundCollision();
        
        // Auto bunny hop on landing
        if (isGrounded && !wasGrounded && glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_SPACE) == GLFW_PRESS) {
            velocity.y = jumpForce;
            isGrounded = false;
        }
    }
    
    PCD::Vec3 GetEyePosition() const {
        return PCD::Vec3(position.x, position.y + eyeHeight, position.z);
    }
    
    PCD::Vec3 GetForward() const {
        return PCD::Vec3(
            std::cos(pitch) * std::sin(yaw),
            std::sin(pitch),
            std::cos(pitch) * std::cos(yaw)
        );
    }
    
    PCD::Vec3 GetRight() const {
        return PCD::Vec3(
            std::sin(yaw - 3.14159f / 2.0f),
            0,
            std::cos(yaw - 3.14159f / 2.0f)
        );
    }
    
private:
    void ApplyFriction(float dt) {
        float speed = velocity.Length();
        if (speed < 0.1f) {
            velocity.x = 0;
            velocity.z = 0;
            return;
        }
        
        // Apply friction
        float drop = 0;
        
        if (isGrounded) {
            float control = speed < stopSpeed ? stopSpeed : speed;
            drop = control * friction * dt;
        }
        
        float newSpeed = speed - drop;
        if (newSpeed < 0) newSpeed = 0;
        if (newSpeed > 0) {
            newSpeed /= speed;
        }
        
        velocity.x *= newSpeed;
        velocity.z *= newSpeed;
    }
    
    void GroundMove(float dt) {
        // Don't apply wish direction if no input
        if (wishDir.Length() < 0.1f) {
            return;
        }
        
        // Q3 acceleration
        Accelerate(wishDir, wishSpeed, groundAccel, dt);
        
        // Clamp to max speed
        float speed = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
        if (speed > maxSpeed) {
            float scale = maxSpeed / speed;
            velocity.x *= scale;
            velocity.z *= scale;
        }
    }
    
    void AirMove(float dt) {
        // Air movement uses lower acceleration
        if (wishDir.Length() < 0.1f) {
            return;
        }
        
        // Air acceleration
        float accel = DotProduct(velocity, wishDir) > 0 ? airAccel : groundAccel;
        Accelerate(wishDir, wishSpeed, accel, dt);
        
        // Allow higher speeds in air (strafe jumping)
        float speed = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
        if (speed > maxSpeed + airMaxSpeed) {
            float scale = (maxSpeed + airMaxSpeed) / speed;
            velocity.x *= scale;
            velocity.z *= scale;
        }
    }
    
    void Accelerate(const PCD::Vec3& wishdir, float wishspeed, float accel, float dt) {
        float currentspeed = DotProduct(velocity, wishdir);
        float addspeed = wishspeed - currentspeed;
        
        if (addspeed <= 0) {
            return;
        }
        
        float accelspeed = accel * dt * wishspeed;
        if (accelspeed > addspeed) {
            accelspeed = addspeed;
        }
        
        velocity.x += accelspeed * wishdir.x;
        velocity.z += accelspeed * wishdir.z;
    }
    
    float DotProduct(const PCD::Vec3& a, const PCD::Vec3& b) const {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    
    void CheckGroundCollision() {
        // Simple ground check at Y=0
        // This will be replaced with proper map collision
        if (position.y <= 0) {
            position.y = 0;
            
            if (velocity.y < 0) {
                velocity.y = 0;
            }
            
            isGrounded = true;
            groundY = 0;
        } else if (position.y > 0.1f) {
            isGrounded = false;
        }
    }
};
