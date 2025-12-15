// GameScene.cpp - Implementation

#include "Game/GameScene.h"
#include "Game/LocalPlayer.h"
#include "Game/RemotePlayer.h"
#include <iostream>

namespace Game {

GameScene::GameScene(GLFWwindow* win, Network::NetworkManager* net)
    : window(win), netManager(net), isRunning(false)
    , cursorCaptured(false), frameTime(0), fps(0)
    , fpsTimer(0), frameCount(0)
{
    renderer = std::make_unique<Renderer>();
    
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

void GameScene::Start(const std::string& mapName) {
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
    // renderer->SetMap(currentMap); // This method doesn't exist yet, will render manually
    
    // Find spawn point
    glm::vec3 spawnPos(0.0f, 5.0f, 0.0f);
    for (const auto& entity : currentMap.entities) {
        if (entity.type == PCD::ENT_INFO_PLAYER_START) {
            spawnPos = glm::vec3(entity.position.x, entity.position.y, entity.position.z);
            break;
        }
    }
    
    // Create local player
    localPlayer = std::make_unique<Player::LocalPlayer>(
        netManager->GetLocalPlayerId(),
        "LocalPlayer",
        window, 
        netManager
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

void GameScene::Stop() {
    isRunning = false;
    
    if (cursorCaptured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        cursorCaptured = false;
    }
    
    localPlayer.reset();
    remotePlayers.clear();
    
    std::cout << "[GAME] Game stopped\n";
}

void GameScene::Update(float deltaTime) {
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
    static bool escWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!escWasPressed) {
            cursorCaptured = !cursorCaptured;
            glfwSetInputMode(window, GLFW_CURSOR, 
                cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            escWasPressed = true;
        }
    } else {
        escWasPressed = false;
    }
}

void GameScene::Render() {
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
    
    // Set up view and projection matrices
    float view[16], proj[16];
    
    // Create view matrix (simple lookAt)
    glm::vec3 target = camPos + camDir;
    glm::vec3 up(0, 1, 0);
    glm::vec3 zaxis = glm::normalize(camPos - target);
    glm::vec3 xaxis = glm::normalize(glm::cross(up, zaxis));
    glm::vec3 yaxis = glm::cross(zaxis, xaxis);
    
    view[0] = xaxis.x; view[4] = xaxis.y; view[8] = xaxis.z;   view[12] = -glm::dot(xaxis, camPos);
    view[1] = yaxis.x; view[5] = yaxis.y; view[9] = yaxis.z;   view[13] = -glm::dot(yaxis, camPos);
    view[2] = zaxis.x; view[6] = zaxis.y; view[10] = zaxis.z;  view[14] = -glm::dot(zaxis, camPos);
    view[3] = 0;       view[7] = 0;       view[11] = 0;         view[15] = 1;
    
    // Create projection matrix
    float fov = 75.0f * 3.14159f / 180.0f;
    float f = 1.0f / tan(fov / 2.0f);
    float near = 0.1f, far = 1000.0f;
    
    proj[0] = f/aspect; proj[4] = 0; proj[8] = 0;                      proj[12] = 0;
    proj[1] = 0;        proj[5] = f; proj[9] = 0;                      proj[13] = 0;
    proj[2] = 0;        proj[6] = 0; proj[10] = (far+near)/(near-far); proj[14] = (2*far*near)/(near-far);
    proj[3] = 0;        proj[7] = 0; proj[11] = -1;                    proj[15] = 0;
    
    // Render map
    renderer->RenderBrushes(currentMap.brushes, -1, view, proj);
    
    // Render HUD
    RenderHUD();
}

void GameScene::HandleNetworkMessage(const std::string& msgType, const std::vector<std::string>& args) {
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

void GameScene::OnPlayerJoin(uint32_t playerId, const std::string& name) {
    std::cout << "[GAME] Player joined: " << name << " (ID: " << playerId << ")\n";
    
    // Create remote player
    auto remotePlayer = std::make_unique<Player::RemotePlayer>(
        playerId,
        name,
        glm::vec3(0.0f, 5.0f, 0.0f) // Default spawn
    );
    
    remotePlayers[playerId] = std::move(remotePlayer);
}

void GameScene::OnPlayerLeave(uint32_t playerId) {
    auto it = remotePlayers.find(playerId);
    if (it != remotePlayers.end()) {
        std::cout << "[GAME] Player left: " << it->second->GetName() << "\n";
        remotePlayers.erase(it);
    }
}

void GameScene::RenderHUD() {
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

} // namespace Game
