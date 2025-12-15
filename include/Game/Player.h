// Player.h - Base player entity for both local and remote players

#pragma once

#include "PCD/PCDTypes.h"
#include <string>
#include <cstdint>

namespace Game {

class Player {
public:
    // Identity
    uint32_t playerId;
    std::string playerName;
    
    // Transform
    PCD::Vec3 position;
    PCD::Vec3 velocity;
    float yaw;          // Horizontal rotation (radians)
    float pitch;        // Vertical rotation (radians)
    
    // State
    int health;
    int maxHealth;
    bool isAlive;
    bool isGrounded;
    
    // Visual
    float bodyHeight;   // 2.0 units (player is 2 units tall)
    float bodyWidth;    // 0.8 units
    float eyeHeight;    // 1.7 units (camera offset from feet)
    
    // Color for rendering (each player gets unique color)
    float colorR, colorG, colorB;
    
public:
    Player(uint32_t id, const std::string& name)
        : playerId(id)
        , playerName(name)
        , position(0, 0, 0)
        , velocity(0, 0, 0)
        , yaw(0.0f)
        , pitch(0.0f)
        , health(100)
        , maxHealth(100)
        , isAlive(true)
        , isGrounded(false)
        , bodyHeight(2.0f)
        , bodyWidth(0.8f)
        , eyeHeight(1.7f)
        , colorR(0.5f)
        , colorG(0.5f)
        , colorB(0.5f)
    {
        // Assign color based on player ID
        AssignColor(id);
    }
    
    virtual ~Player() = default;
    
    // Update player (called every frame)
    virtual void Update(float deltaTime) = 0;
    
    // Render player
    virtual void Render() = 0;
    
    // Get eye position (for camera or ray casting)
    PCD::Vec3 GetEyePosition() const {
        return PCD::Vec3(position.x, position.y + eyeHeight, position.z);
    }
    
    // Get forward direction vector
    PCD::Vec3 GetForward() const {
        return PCD::Vec3(
            std::cos(pitch) * std::sin(yaw),
            std::sin(pitch),
            std::cos(pitch) * std::cos(yaw)
        );
    }
    
    // Get right direction vector
    PCD::Vec3 GetRight() const {
        return PCD::Vec3(
            std::sin(yaw - 3.14159f / 2.0f),
            0,
            std::cos(yaw - 3.14159f / 2.0f)
        );
    }
    
    // Take damage
    void TakeDamage(int damage) {
        if (!isAlive) return;
        
        health -= damage;
        if (health <= 0) {
            health = 0;
            isAlive = false;
        }
    }
    
    // Respawn player
    void Respawn(const PCD::Vec3& spawnPos) {
        position = spawnPos;
        velocity = PCD::Vec3(0, 0, 0);
        health = maxHealth;
        isAlive = true;
        isGrounded = false;
    }
    
    // Get bounding box for collision
    struct AABB {
        PCD::Vec3 min;
        PCD::Vec3 max;
    };
    
    AABB GetBoundingBox() const {
        float halfWidth = bodyWidth / 2.0f;
        return AABB{
            PCD::Vec3(position.x - halfWidth, position.y, position.z - halfWidth),
            PCD::Vec3(position.x + halfWidth, position.y + bodyHeight, position.z + halfWidth)
        };
    }
    
protected:
    void AssignColor(uint32_t id) {
        // Assign distinct colors based on player ID
        switch (id % 8) {
            case 0: colorR = 1.0f; colorG = 0.2f; colorB = 0.2f; break; // Red
            case 1: colorR = 0.2f; colorG = 0.5f; colorB = 1.0f; break; // Blue
            case 2: colorR = 0.3f; colorG = 1.0f; colorB = 0.3f; break; // Green
            case 3: colorR = 1.0f; colorG = 1.0f; colorB = 0.2f; break; // Yellow
            case 4: colorR = 1.0f; colorG = 0.5f; colorB = 0.2f; break; // Orange
            case 5: colorR = 0.8f; colorG = 0.2f; colorB = 1.0f; break; // Purple
            case 6: colorR = 0.2f; colorG = 1.0f; colorB = 1.0f; break; // Cyan
            case 7: colorR = 1.0f; colorG = 0.8f; colorB = 0.8f; break; // Pink
        }
    }
};

} // namespace Game
