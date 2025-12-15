// GameScene.h - Fixed for ENet NetworkManager

#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Network/NetworkManager.h"
#include "Engine/Renderer.h"
#include "PCD/PCD.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

// Forward declarations
namespace Player {
    class LocalPlayer;
    class RemotePlayer;
}

namespace Game {

class GameScene {
private:
    GLFWwindow* window;
    Network::NetworkManager* netManager;
    
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Player::LocalPlayer> localPlayer;
    std::unordered_map<uint32_t, std::unique_ptr<Player::RemotePlayer>> remotePlayers;
    
    PCD::Map currentMap;
    bool isRunning;
    bool cursorCaptured;
    
    // Stats
    float frameTime;
    int fps;
    float fpsTimer;
    int frameCount;

public:
    GameScene(GLFWwindow* win, Network::NetworkManager* net);
    
    void Start(const std::string& mapName);
    void Stop();
    void Update(float deltaTime);
    void Render();
    
    bool IsRunning() const { return isRunning; }

private:
    void HandleNetworkMessage(const std::string& msgType, const std::vector<std::string>& args);
    void OnPlayerJoin(uint32_t playerId, const std::string& name);
    void OnPlayerLeave(uint32_t playerId);
    void RenderHUD();
};

} // namespace Game
