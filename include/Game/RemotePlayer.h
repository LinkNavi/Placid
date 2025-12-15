// RemotePlayer.h - Remote player representation

#pragma once

#include <glm/glm.hpp>
#include <string>

namespace Player {

class RemotePlayer {
private:
    uint32_t playerId;
    std::string name;
    
    glm::vec3 position;
    glm::vec3 velocity;
    
    float yaw;
    float pitch;
    
    int health;
    int weapon;
    
    // Interpolation for smooth movement
    glm::vec3 targetPosition;
    float interpolationSpeed;

public:
    RemotePlayer(uint32_t id, const std::string& playerName, const glm::vec3& spawnPos)
        : playerId(id), name(playerName), position(spawnPos)
        , velocity(0.0f), yaw(0.0f), pitch(0.0f)
        , health(100), weapon(0), targetPosition(spawnPos)
        , interpolationSpeed(10.0f)
    {
    }
    
    void Update(float deltaTime) {
        // Smooth interpolation to target position
        position = glm::mix(position, targetPosition, interpolationSpeed * deltaTime);
        
        // Clamp to prevent floating point drift
        if (glm::distance(position, targetPosition) < 0.01f) {
            position = targetPosition;
        }
    }
    
    void SetPosition(const glm::vec3& pos) {
        targetPosition = pos;
    }
    
    void SetRotation(float newYaw, float newPitch) {
        yaw = newYaw;
        pitch = newPitch;
    }
    
    void SetHealth(int hp) { health = hp; }
    void SetWeapon(int wpn) { weapon = wpn; }
    
    uint32_t GetId() const { return playerId; }
    const std::string& GetName() const { return name; }
    const glm::vec3& GetPosition() const { return position; }
    const glm::vec3& GetVelocity() const { return velocity; }
    float GetYaw() const { return yaw; }
    float GetPitch() const { return pitch; }
    int GetHealth() const { return health; }
    int GetWeapon() const { return weapon; }
};

} // namespace Player
