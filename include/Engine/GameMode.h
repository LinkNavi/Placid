#ifndef GAME_MODE_H
#define GAME_MODE_H

#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Renderer.h"
#include "PCD/PCD.h"
#include "Game/KeybindManager.h"

namespace Game {

class GameMode {
private:
    Camera camera;
    KeybindManager keybinds;
    Renderer* renderer;
    const PCD::Map* map;
    
    // Player state
    Vec3 velocity;
    bool isGrounded;
    float groundY;
    
    // Physics constants
    const float GRAVITY = -20.0f;
    const float JUMP_FORCE = 8.0f;
    const float MOVE_SPEED = 5.0f;
    const float SPRINT_SPEED = 10.0f;
    const float GROUND_DRAG = 10.0f;
    const float AIR_DRAG = 1.0f;
    
public:
    GameMode(Renderer* r, const PCD::Map* m);
    ~GameMode();
    
    void Initialize();
    void ProcessInput(GLFWwindow* window, float dt);
    void Update(float dt);
    void Render(float* projection);
    
    Camera& GetCamera() { return camera; }
    KeybindManager& GetKeybinds() { return keybinds; }
    
private:
    void UpdatePhysics(float dt);
    void CheckGroundCollision();
    Vec3 FindPlayerSpawn();
};

} // namespace Game

#endif // GAME_MODE_H
