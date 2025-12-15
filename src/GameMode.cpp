#include "Engine/GameMode.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace Game {

GameMode::GameMode(Renderer* r, const PCD::Map* m) 
    : renderer(r), map(m), velocity(0, 0, 0), isGrounded(false), groundY(0) {
}

GameMode::~GameMode() {
}

void GameMode::Initialize() {
    // Find player spawn and teleport there
    Vec3 spawnPos = FindPlayerSpawn();
    camera.position = spawnPos;
    camera.yaw = 0;
    camera.pitch = 0;
    
    velocity = Vec3(0, 0, 0);
    isGrounded = false;
    
    std::cout << "Game mode initialized. Spawned at (" 
              << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ")\n";
}

Vec3 GameMode::FindPlayerSpawn() {
    // Look for player spawn entity
    for (const auto& ent : map->entities) {
        if (ent.type == PCD::ENT_INFO_PLAYER_START || 
            ent.type == PCD::ENT_INFO_PLAYER_DEATHMATCH) {
            return Vec3(ent.position.x, ent.position.y + 1.6f, ent.position.z);
        }
    }
    
    // No spawn found, use default position above ground
    return Vec3(0, 2.0f, 0);
}

void GameMode::ProcessInput(GLFWwindow* window, float dt) {
    // Get movement input
    float forward = keybinds.GetAxis(KeybindManager::MOVE_FORWARD, KeybindManager::MOVE_BACKWARD);
    float right = keybinds.GetAxis(KeybindManager::MOVE_RIGHT, KeybindManager::MOVE_LEFT);
    
    // Get movement vectors
    Vec3 fwd = camera.GetForward();
    Vec3 rgt = camera.GetRight();
    
    // Ground movement only (no flying)
    fwd.y = 0;
    fwd = fwd.normalized();
    rgt.y = 0;
    rgt = rgt.normalized();
    
    // Calculate desired velocity
    float speed = keybinds.IsPressed(KeybindManager::SPRINT) ? SPRINT_SPEED : MOVE_SPEED;
    Vec3 wishDir = fwd * forward + rgt * right;
    
    if (wishDir.x != 0 || wishDir.z != 0) {
        wishDir = wishDir.normalized();
    }
    
    // Apply movement force
    if (isGrounded) {
        velocity.x = wishDir.x * speed;
        velocity.z = wishDir.z * speed;
    } else {
        // Air control (reduced)
        velocity.x += wishDir.x * speed * 0.1f * dt;
        velocity.z += wishDir.z * speed * 0.1f * dt;
    }
    
    // Jump
    if (keybinds.IsPressed(KeybindManager::JUMP) && isGrounded) {
        velocity.y = JUMP_FORCE;
        isGrounded = false;
    }
}

void GameMode::Update(float dt) {
    UpdatePhysics(dt);
}

void GameMode::UpdatePhysics(float dt) {
    // Apply gravity
    if (!isGrounded) {
        velocity.y += GRAVITY * dt;
    }
    
    // Apply drag
    float drag = isGrounded ? GROUND_DRAG : AIR_DRAG;
    velocity.x *= (1.0f - drag * dt);
    velocity.z *= (1.0f - drag * dt);
    
    // Clamp velocity
    float maxSpeed = 50.0f;
    float speedXZ = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
    if (speedXZ > maxSpeed) {
        float scale = maxSpeed / speedXZ;
        velocity.x *= scale;
        velocity.z *= scale;
    }
    
    // Update position
    camera.position.x += velocity.x * dt;
    camera.position.y += velocity.y * dt;
    camera.position.z += velocity.z * dt;
    
    // Simple ground collision
    CheckGroundCollision();
}

void GameMode::CheckGroundCollision() {
    // Find the highest floor brush below player
    float highestFloor = -1000.0f;
    bool foundFloor = false;
    
    for (const auto& brush : map->brushes) {
        if (!(brush.flags & PCD::BRUSH_SOLID)) continue;
        
        // Get brush bounds
        if (brush.vertices.empty()) continue;
        
        float minX = brush.vertices[0].position.x;
        float maxX = minX;
        float minZ = brush.vertices[0].position.z;
        float maxZ = minZ;
        float minY = brush.vertices[0].position.y;
        float maxY = minY;
        
        for (const auto& v : brush.vertices) {
            minX = std::min(minX, v.position.x);
            maxX = std::max(maxX, v.position.x);
            minY = std::min(minY, v.position.y);
            maxY = std::max(maxY, v.position.y);
            minZ = std::min(minZ, v.position.z);
            maxZ = std::max(maxZ, v.position.z);
        }
        
        // Check if player is above this brush
        if (camera.position.x >= minX && camera.position.x <= maxX &&
            camera.position.z >= minZ && camera.position.z <= maxZ) {
            
            // This brush is under the player
            if (maxY > highestFloor && maxY <= camera.position.y) {
                highestFloor = maxY;
                foundFloor = true;
            }
        }
    }
    
    // Check if we're on the ground
    const float GROUND_THRESHOLD = 0.1f;
    if (foundFloor && camera.position.y - highestFloor <= GROUND_THRESHOLD) {
        camera.position.y = highestFloor;
        velocity.y = 0;
        isGrounded = true;
        groundY = highestFloor;
    } else {
        isGrounded = false;
    }
    
    // Prevent falling through the world
    if (camera.position.y < -10.0f) {
        // Respawn at spawn point
        Vec3 spawn = FindPlayerSpawn();
        camera.position = spawn;
        velocity = Vec3(0, 0, 0);
    }
}

void GameMode::Render(float* projection) {
    // Get view matrix
    float view[16];
    camera.GetViewMatrix(view);
    
    // Render the map brushes (solid geometry only)
    renderer->RenderBrushes(map->brushes, -1, view, projection);
    
    // Don't render entities in play mode (or render as gameplay objects)
}

} // namespace Game
