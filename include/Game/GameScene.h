// GameScene.h - Main game scene with map and players

#pragma once

#include "Player.h"
#include "LocalPlayer.h"
#include "RemotePlayer.h"
#include "Network/NetworkManager.h"
#include "PCD/PCDTypes.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace Game {

class GameScene {
private:
    GLFWwindow* window;
    Network::NetworkManager* netManager;
    
    // Players
    std::unique_ptr<LocalPlayer> localPlayer;
    std::unordered_map<uint32_t, std::unique_ptr<RemotePlayer>> remotePlayers;
    
    // Map data (simple for now)
    struct MapGeometry {
        std::vector<float> vertices;
        std::vector<uint32_t> indices;
    };
    MapGeometry mapGeometry;
    std::string mapName;
    
    // Camera
    PCD::Vec3 cameraPosition;
    float cameraYaw;
    float cameraPitch;
    
    // Game state
    bool isActive;
    bool showScoreboard;

public:
    GameScene(GLFWwindow* win, Network::NetworkManager* net)
        : window(win)
        , netManager(net)
        , cameraPosition(0, 0, 0)
        , cameraYaw(0)
        , cameraPitch(0)
        , isActive(false)
        , showScoreboard(false)
    {
        // Set up network callbacks
        netManager->SetPlayerJoinCallback([this](uint32_t playerId, const std::string& name) {
            OnPlayerJoined(playerId, name);
        });
        
        netManager->SetPlayerLeaveCallback([this](uint32_t playerId) {
            OnPlayerLeft(playerId);
        });
        
        netManager->SetMessageCallback([this](const std::string& msgType,
                                             const std::vector<std::string>& args,
                                             const std::string& fromIP,
                                             uint16_t fromPort) {
            HandleNetworkMessage(msgType, args);
        });
    }
    
    void Start(const std::string& map) {
        mapName = map;
        isActive = true;
        
        // Load map
        LoadMap(map);
        
        // Create local player
        uint32_t localId = netManager->GetLocalPlayerId();
        const auto& clients = netManager->GetClients();
        auto it = clients.find(localId);
        std::string playerName = "Player";
        if (it != clients.end()) {
            playerName = it->second.name;
        }
        
        localPlayer = std::make_unique<LocalPlayer>(localId, playerName, window, netManager);
        
        // Spawn at origin for now
        localPlayer->position = PCD::Vec3(0, 2, 0);
        
        // Create remote players for everyone already in lobby
        for (const auto& [id, client] : clients) {
            if (id != localId) {
                OnPlayerJoined(id, client.name);
            }
        }
        
        std::cout << "[GAME] Started game on map: " << map << "\n";
    }
    
    void Update(float deltaTime) {
        if (!isActive) return;
        
        // Update network
        netManager->Update(deltaTime);
        
        // Update local player
        if (localPlayer) {
            localPlayer->Update(deltaTime);
            
            // Update camera from local player
            cameraPosition = localPlayer->GetEyePosition();
            cameraYaw = localPlayer->yaw;
            cameraPitch = localPlayer->pitch;
        }
        
        // Update remote players
        for (auto& [id, player] : remotePlayers) {
            player->Update(deltaTime);
        }
        
        // Check for ESC to show scoreboard
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            // TODO: Show pause menu
        }
        
        if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
            showScoreboard = true;
        } else {
            showScoreboard = false;
        }
    }
    
    void Render() {
        if (!isActive) return;
        
        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Set up camera
        SetupCamera();
        
        // Render map
        RenderMap();
        
        // Render all remote players
        for (auto& [id, player] : remotePlayers) {
            player->Render();
        }
        
        // Render HUD (in 2D)
        RenderHUD();
    }
    
    void Stop() {
        isActive = false;
        localPlayer.reset();
        remotePlayers.clear();
        std::cout << "[GAME] Stopped game\n";
    }
    
    bool IsActive() const { return isActive; }

private:
    void SetupCamera() {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        
        // Perspective projection
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;
        float fov = 90.0f;
        float near = 0.1f;
        float far = 1000.0f;
        
        float f = 1.0f / std::tan(fov * 3.14159f / 360.0f);
        float mat[16] = {
            f/aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, (far+near)/(near-far), -1,
            0, 0, (2*far*near)/(near-far), 0
        };
        glMultMatrixf(mat);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // Apply camera rotation
        glRotatef(-cameraPitch * 180.0f / 3.14159f, 1, 0, 0);
        glRotatef(-cameraYaw * 180.0f / 3.14159f, 0, 1, 0);
        
        // Apply camera position
        glTranslatef(-cameraPosition.x, -cameraPosition.y, -cameraPosition.z);
    }
    
    void RenderMap() {
        if (mapGeometry.vertices.empty()) {
            // Render simple ground plane if no map loaded
            glColor3f(0.3f, 0.3f, 0.3f);
            glBegin(GL_QUADS);
            glVertex3f(-100, 0, -100);
            glVertex3f(100, 0, -100);
            glVertex3f(100, 0, 100);
            glVertex3f(-100, 0, 100);
            glEnd();
            
            // Draw grid
            glColor3f(0.5f, 0.5f, 0.5f);
            glBegin(GL_LINES);
            for (int i = -10; i <= 10; i++) {
                glVertex3f(i * 10, 0, -100);
                glVertex3f(i * 10, 0, 100);
                glVertex3f(-100, 0, i * 10);
                glVertex3f(100, 0, i * 10);
            }
            glEnd();
        } else {
            // Render actual map geometry
            glColor3f(0.7f, 0.7f, 0.7f);
            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < mapGeometry.indices.size(); i++) {
                uint32_t idx = mapGeometry.indices[i];
                glVertex3f(
                    mapGeometry.vertices[idx * 3 + 0],
                    mapGeometry.vertices[idx * 3 + 1],
                    mapGeometry.vertices[idx * 3 + 2]
                );
            }
            glEnd();
        }
    }
    
    void RenderHUD() {
        // Switch to 2D rendering
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
        
        // Draw crosshair
        glColor3f(1, 1, 1);
        glLineWidth(2.0f);
        int centerX = width / 2;
        int centerY = height / 2;
        int crosshairSize = 10;
        
        glBegin(GL_LINES);
        glVertex2f(centerX - crosshairSize, centerY);
        glVertex2f(centerX + crosshairSize, centerY);
        glVertex2f(centerX, centerY - crosshairSize);
        glVertex2f(centerX, centerY + crosshairSize);
        glEnd();
        
        // Draw health bar (bottom left)
        if (localPlayer) {
            int barWidth = 200;
            int barHeight = 20;
            int barX = 20;
            int barY = height - 40;
            
            // Background
            glColor3f(0.2f, 0.2f, 0.2f);
            glBegin(GL_QUADS);
            glVertex2f(barX, barY);
            glVertex2f(barX + barWidth, barY);
            glVertex2f(barX + barWidth, barY + barHeight);
            glVertex2f(barX, barY + barHeight);
            glEnd();
            
            // Health
            float healthPercent = (float)localPlayer->health / (float)localPlayer->maxHealth;
            glColor3f(1.0f - healthPercent, healthPercent, 0);
            glBegin(GL_QUADS);
            glVertex2f(barX, barY);
            glVertex2f(barX + barWidth * healthPercent, barY);
            glVertex2f(barX + barWidth * healthPercent, barY + barHeight);
            glVertex2f(barX, barY + barHeight);
            glEnd();
            
            // Border
            glColor3f(1, 1, 1);
            glLineWidth(2.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(barX, barY);
            glVertex2f(barX + barWidth, barY);
            glVertex2f(barX + barWidth, barY + barHeight);
            glVertex2f(barX, barY + barHeight);
            glEnd();
        }
        
        // Restore 3D rendering
        glEnable(GL_DEPTH_TEST);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }
    
    void LoadMap(const std::string& mapPath) {
        // TODO: Actually load .pcd file
        // For now, just use empty map (ground plane will be rendered)
        
        std::cout << "[GAME] Loading map: " << mapPath << "\n";
        std::cout << "[GAME] Map loading not implemented yet - using ground plane\n";
        
        mapGeometry.vertices.clear();
        mapGeometry.indices.clear();
    }
    
    void OnPlayerJoined(uint32_t playerId, const std::string& name) {
        if (playerId == netManager->GetLocalPlayerId()) {
            return; // That's us
        }
        
        std::cout << "[GAME] Player joined: " << name << " (ID: " << playerId << ")\n";
        
        // Create remote player
        auto remotePlayer = std::make_unique<RemotePlayer>(playerId, name);
        remotePlayer->position = PCD::Vec3(0, 2, 0); // Default spawn
        remotePlayers[playerId] = std::move(remotePlayer);
    }
    
    void OnPlayerLeft(uint32_t playerId) {
        auto it = remotePlayers.find(playerId);
        if (it != remotePlayers.end()) {
            std::cout << "[GAME] Player left: " << it->second->playerName << "\n";
            remotePlayers.erase(it);
        }
    }
    
    void HandleNetworkMessage(const std::string& msgType, const std::vector<std::string>& args) {
        // Filter out system messages
        if (msgType == Network::MessageType::PING_REQUEST ||
            msgType == Network::MessageType::PING_RESPONSE ||
            msgType.empty()) {
            return;
        }
        
        if (msgType == Network::MessageType::PLAYER_STATE && args.size() >= 8) {
            uint32_t playerId = std::stoul(args[0]);
            
            // Update remote player
            auto it = remotePlayers.find(playerId);
            if (it != remotePlayers.end()) {
                float x = std::stof(args[1]);
                float y = std::stof(args[2]);
                float z = std::stof(args[3]);
                float yaw = std::stof(args[4]);
                float pitch = std::stof(args[5]);
                int health = std::stoi(args[6]);
                
                it->second->UpdateFromNetwork(x, y, z, yaw, pitch, health);
            }
        }
    }
};

} // namespace Game
