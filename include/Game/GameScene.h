// GameScene.h - Fixed for ENet NetworkManager

#pragma once

#include <GLFW/glfw3.h>
#include "Network/NetworkManager.h"
#include "Engine/Renderer.h"
#include "Game/LocalPlayer.h"
#include "Game/RemotePlayer.h"
#include "PCD/PCD.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

namespace Game {

class GameScene {
private:
    GLFWwindow* window;
    Network::NetworkManager* netManager;
    
    std::unique_ptr<Rendering::Renderer> renderer;
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
    GameScene(GLFWwindow* win, Network::NetworkManager* net)
        : window(win), netManager(net), isRunning(false)
        , cursorCaptured(false), frameTime(0), fps(0)
        , fpsTimer(0), frameCount(0)
    {
        renderer = std::make_unique<Rendering::Renderer>();
        
        // Setup network callbacks
        netManager->SetMessageCallback([this](const std::string& type, const std::vector<std::string>& args) {
            this->HandleNetworkMessage(type, args);
        });
        
        netManager->SetPlayerJoinCallback([this](uint32_t id, const std::string& name) {
            this->OnPlayerJoin(id, name);
        });
        
        netManager->SetPlayerLeaveCallback([this](uint32_t id) {
            this->OnPlayerLeave(id);
        });
    }
    
    void Start(const std::string& mapName) {
        std::cout << "[GAME] Starting game with map: " << mapName << "\n";
        
        // Load map
        if (netManager->IsHost()) {
            if (!netManager->LoadMap(mapName)) {
                std::cerr << "[GAME] Failed to load map\n";
                return;
            }
        }
        
        currentMap = netManager->GetMap();
        
        if (currentMap.brushes.empty()) {
            std::cerr << "[GAME] Map has no geometry\n";
            return;
        }
        
        std::cout << "[GAME] Map loaded: " << currentMap.brushes.size() << " brushes\n";
        
        // Initialize renderer
        renderer->SetMap(currentMap);
        
        // Find spawn point
        glm::vec3 spawnPos(0.0f, 5.0f, 0.0f);
        for (const auto& entity : currentMap.entities) {
            if (entity.classname == "info_player_start") {
                auto it = entity.properties.find("origin");
                if (it != entity.properties.end()) {
                    // Parse "x y z"
                    std::stringstream ss(it->second);
                    ss >> spawnPos.x >> spawnPos.y >> spawnPos.z;
                    break;
                }
            }
        }
        
        // Create local player
        localPlayer = std::make_unique<Player::LocalPlayer>(
            window, 
            netManager,
            spawnPos
        );
        
        std::cout << "[GAME] Spawning at: " 
                  << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << "\n";
        
        // Create remote players for existing clients
        for (const auto& [id, client] : netManager->GetClients()) {
            if (id != netManager->GetLocalPlayerId()) {
                OnPlayerJoin(id, client.name);
            }
        }
        
        // Capture cursor
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        cursorCaptured = true;
        
        isRunning = true;
        std::cout << "[GAME] Game started!\n";
    }
    
    void Stop() {
        isRunning = false;
        
        if (cursorCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cursorCaptured = false;
        }
        
        localPlayer.reset();
        remotePlayers.clear();
        
        std::cout << "[GAME] Game stopped\n";
    }
    
    void Update(float deltaTime) {
        if (!isRunning) return;
        
        frameTime = deltaTime;
        
        // Update FPS counter
        frameCount++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.0f) {
            fps = frameCount;
            frameCount = 0;
            fpsTimer = 0.0f;
        }
        
        // Update network
        netManager->Update(deltaTime);
        
        // Update local player
        if (localPlayer) {
            localPlayer->Update(deltaTime);
        }
        
        // Update remote players
        for (auto& [id, player] : remotePlayers) {
            player->Update(deltaTime);
        }
        
        // Toggle cursor with ESC
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            static bool escPressed = false;
            if (!escPressed) {
                cursorCaptured = !cursorCaptured;
                glfwSetInputMode(window, GLFW_CURSOR, 
                    cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                escPressed = true;
            }
        } else {
            static bool escPressed = false;
            escPressed = false;
        }
    }
    
    void Render() {
        if (!isRunning || !localPlayer) return;
        
        // Clear
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Setup camera from local player
        glm::vec3 camPos = localPlayer->GetPosition();
        camPos.y += 1.6f; // Eye height
        
        glm::vec3 camDir = localPlayer->GetViewDirection();
        
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / height;
        
        renderer->SetCamera(camPos, camDir, aspect);
        
        // Render map
        renderer->RenderMap();
        
        // Render players
        if (localPlayer) {
            renderer->RenderPlayer(
                localPlayer->GetPosition(),
                localPlayer->GetYaw(),
                glm::vec3(0.2f, 0.5f, 1.0f) // Blue for local
            );
        }
        
        for (const auto& [id, player] : remotePlayers) {
            renderer->RenderPlayer(
                player->GetPosition(),
                player->GetYaw(),
                glm::vec3(1.0f, 0.2f, 0.2f) // Red for remote
            );
        }
        
        // Render HUD
        RenderHUD();
    }
    
    bool IsRunning() const { return isRunning; }

private:
    void HandleNetworkMessage(const std::string& msgType, const std::vector<std::string>& args) {
        // Handle PLAYER_STATE messages
        if (msgType == "PLAYER_STATE" && args.size() >= 8) {
            uint32_t playerId = std::stoul(args[0]);
            
            // Skip our own updates
            if (playerId == netManager->GetLocalPlayerId()) {
                return;
            }
            
            // Update remote player
            auto it = remotePlayers.find(playerId);
            if (it != remotePlayers.end()) {
                float x = std::stof(args[1]);
                float y = std::stof(args[2]);
                float z = std::stof(args[3]);
                float yaw = std::stof(args[4]);
                float pitch = std::stof(args[5]);
                
                it->second->SetPosition(glm::vec3(x, y, z));
                it->second->SetRotation(yaw, pitch);
            }
        }
        // Handle CHAT_MESSAGE
        else if (msgType == "CHAT_MESSAGE" && args.size() >= 2) {
            uint32_t senderId = std::stoul(args[0]);
            std::string message = args[1];
            
            // Find sender name
            std::string senderName = "Unknown";
            const auto& clients = netManager->GetClients();
            auto it = clients.find(senderId);
            if (it != clients.end()) {
                senderName = it->second.name;
            }
            
            std::cout << "[CHAT] " << senderName << ": " << message << "\n";
        }
        // Handle GAME_START
        else if (msgType == "GAME_START" && args.size() >= 1) {
            std::string mapName = args[0];
            std::cout << "[GAME] Host started game with map: " << mapName << "\n";
        }
    }
    
    void OnPlayerJoin(uint32_t playerId, const std::string& name) {
        std::cout << "[GAME] Player joined: " << name << " (ID: " << playerId << ")\n";
        
        // Create remote player
        auto remotePlayer = std::make_unique<Player::RemotePlayer>(
            playerId,
            name,
            glm::vec3(0.0f, 5.0f, 0.0f) // Default spawn
        );
        
        remotePlayers[playerId] = std::move(remotePlayer);
    }
    
    void OnPlayerLeave(uint32_t playerId) {
        auto it = remotePlayers.find(playerId);
        if (it != remotePlayers.end()) {
            std::cout << "[GAME] Player left: " << it->second->GetName() << "\n";
            remotePlayers.erase(it);
        }
    }
    
    void RenderHUD() {
        // Simple text overlay using OpenGL (no ImGui in game)
        
        // Save state
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glOrtho(0, width, height, 0, -1, 1);
        
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        
        // Draw player count
        char buffer[256];
        snprintf(buffer, sizeof(buffer), 
                "Players: %d | FPS: %d | Ping: %d ms",
                (int)netManager->GetPlayerCount(),
                fps,
                0); // TODO: Implement ping
        
        // Draw background
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
        glBegin(GL_QUADS);
        glVertex2f(10, 10);
        glVertex2f(400, 10);
        glVertex2f(400, 40);
        glVertex2f(10, 40);
        glEnd();
        
        // Restore state
        glEnable(GL_DEPTH_TEST);
        
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
};

} // namespace Game
