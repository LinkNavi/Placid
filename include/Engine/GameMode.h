#ifndef GAME_MODE_H
#define GAME_MODE_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "Game/PlayerController.h"
#include "Renderer.h"
#include "PCD/PCD.h"

namespace Game {

class GameMode {
private:
    PlayerController controller;
    Renderer* renderer;
    const PCD::Map* map;
    
public:
    GameMode(Renderer* r, const PCD::Map* m);
    ~GameMode();
    
    void Initialize();
    void ProcessInput(GLFWwindow* window, float dt);
    void Update(float dt);
    void Render(float* projection);
    
    PlayerController& GetController() { return controller; }
    
private:
    PCD::Vec3 FindPlayerSpawn();
    void CheckGroundCollision();
};

} // namespace Game

#endif // GAME_MODE_H
