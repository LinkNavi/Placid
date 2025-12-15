#include "Engine/GameMode.h"
#include <iostream>
#include <algorithm>

namespace Game {

GameMode::GameMode(Renderer* r, const PCD::Map* m) 
    : renderer(r), map(m) {
}

GameMode::~GameMode() {
}

void GameMode::Initialize() {
    PCD::Vec3 spawn = FindPlayerSpawn();
    controller.position = spawn;
    controller.velocity = PCD::Vec3(0, 0, 0);
    controller.yaw = 0;
    controller.pitch = 0;
    
    std::cout << "Play mode: Spawned at (" << spawn.x << ", " << spawn.y << ", " << spawn.z << ")\n";
}

PCD::Vec3 GameMode::FindPlayerSpawn() {
    for (const auto& ent : map->entities) {
        if (ent.type == PCD::ENT_INFO_PLAYER_START || 
            ent.type == PCD::ENT_INFO_PLAYER_DEATHMATCH) {
            return PCD::Vec3(ent.position.x, ent.position.y, ent.position.z);
        }
    }
    return PCD::Vec3(0, 0, 0);
}

void GameMode::ProcessInput(GLFWwindow* window, float dt) {
    controller.ProcessInput(window, dt);
}

void GameMode::Update(float dt) {
    controller.Update(dt);
    CheckGroundCollision();
}

void GameMode::CheckGroundCollision() {
    float highest = -1000.0f;
    bool found = false;
    
    for (const auto& brush : map->brushes) {
        if (!(brush.flags & PCD::BRUSH_SOLID) || brush.vertices.empty()) continue;
        
        float minX = brush.vertices[0].position.x, maxX = minX;
        float minZ = brush.vertices[0].position.z, maxZ = minZ;
        float minY = brush.vertices[0].position.y, maxY = minY;
        
        for (const auto& v : brush.vertices) {
            minX = std::min(minX, v.position.x); maxX = std::max(maxX, v.position.x);
            minY = std::min(minY, v.position.y); maxY = std::max(maxY, v.position.y);
            minZ = std::min(minZ, v.position.z); maxZ = std::max(maxZ, v.position.z);
        }
        
        if (controller.position.x >= minX && controller.position.x <= maxX &&
            controller.position.z >= minZ && controller.position.z <= maxZ &&
            maxY > highest && maxY <= controller.position.y) {
            highest = maxY;
            found = true;
        }
    }
    
    if (found && controller.position.y - highest <= 0.1f) {
        controller.position.y = highest;
        if (controller.velocity.y < 0) controller.velocity.y = 0;
        controller.isGrounded = true;
        controller.groundY = highest;
    } else if (controller.position.y > 0.1f) {
        controller.isGrounded = false;
    }
    
    if (controller.position.y < -10.0f) {
        controller.position = FindPlayerSpawn();
        controller.velocity = PCD::Vec3(0, 0, 0);
    }
}

void GameMode::Render(float* projection) {
    PCD::Vec3 eye = controller.GetEyePosition();
    PCD::Vec3 fwd = controller.GetForward();
    PCD::Vec3 target = eye + fwd;
    PCD::Vec3 up(0, 1, 0);
    
    PCD::Vec3 f = (target - eye).Normalized();
    
    // Cross product: (f.y*up.z - f.z*up.y, f.z*up.x - f.x*up.z, f.x*up.y - f.y*up.x)
    PCD::Vec3 r(f.y*up.z - f.z*up.y, f.z*up.x - f.x*up.z, f.x*up.y - f.y*up.x);
    r = r.Normalized();
    
    // Cross product: r x f
    PCD::Vec3 u(r.y*f.z - r.z*f.y, r.z*f.x - r.x*f.z, r.x*f.y - r.y*f.x);
    
    float view[16];
    view[0] = r.x;  view[4] = r.y;  view[8] = r.z;   view[12] = -(r.x*eye.x + r.y*eye.y + r.z*eye.z);
    view[1] = u.x;  view[5] = u.y;  view[9] = u.z;   view[13] = -(u.x*eye.x + u.y*eye.y + u.z*eye.z);
    view[2] = -f.x; view[6] = -f.y; view[10] = -f.z; view[14] = f.x*eye.x + f.y*eye.y + f.z*eye.z;
    view[3] = 0;    view[7] = 0;    view[11] = 0;    view[15] = 1;
    
    renderer->RenderBrushes(map->brushes, -1, view, projection);
}

} // namespace Game
