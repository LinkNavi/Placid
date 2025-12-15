// GameScene.cpp - Fixed with proper rendering and HUD

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
    
    // Get map from network manager
    currentMap = netManager->GetMap();
    
    if (currentMap.brushes.empty()) {
        std::cerr << "[GAME] WARNING: Map has no geometry! Creating test floor...\n";
        
        // Create a simple test floor so we can at least see something
        PCD::Brush floor;
        floor.id = 1;
        floor.flags = PCD::BRUSH_SOLID;
        floor.name = "Test Floor";
        
        // Simple quad floor
        floor.vertices = {
            {{-20, 0, -20}, {0, 1, 0}, {0, 0}},
            {{ 20, 0, -20}, {0, 1, 0}, {1, 0}},
            {{ 20, 0,  20}, {0, 1, 0}, {1, 1}},
            {{-20, 0,  20}, {0, 1, 0}, {0, 1}}
        };
        floor.indices = {0, 1, 2, 0, 2, 3};
        floor.color = PCD::Vec3(0.5f, 0.5f, 0.5f);
        currentMap.brushes.push_back(floor);
    }
    
    std::cout << "[GAME] Map geometry: " << currentMap.brushes.size() << " brushes, "
              << currentMap.entities.size() << " entities\n";
    
    // Initialize renderer
    if (!renderer->Initialize()) {
        std::cerr << "[GAME] Failed to initialize renderer\n";
        return;
    }
    
    std::cout << "[GAME] Renderer initialized\n";
    
    // Find spawn point
    glm::vec3 spawnPos(0.0f, 2.0f, 0.0f);
    for (const auto& entity : currentMap.entities) {
        if (entity.type == PCD::ENT_INFO_PLAYER_START || 
            entity.type == PCD::ENT_INFO_PLAYER_DEATHMATCH) {
            spawnPos = glm::vec3(entity.position.x, entity.position.y + 1.6f, entity.position.z);
            std::cout << "[GAME] Found spawn point at: " << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << "\n";
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
    
    std::cout << "[GAME] Local player created (ID: " << netManager->GetLocalPlayerId() << ")\n";
    std::cout << "[GAME] Spawned at: " << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << "\n";
    
    // Create remote players for existing clients
    for (const auto& [id, client] : netManager->GetClients()) {
        if (id != netManager->GetLocalPlayerId()) {
            OnPlayerJoin(id, client.name);
        }
    }
    
    // Set clear color to sky blue
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    
    // Capture cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    cursorCaptured = true;
    
    isRunning = true;
    std::cout << "[GAME] Game started successfully!\n";
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
    
    // Render remote players
    RenderRemotePlayers(view, proj);
    
    // Render HUD
    RenderHUD();
}

void GameScene::RenderRemotePlayers(float* view, float* proj) {
    // Render each remote player as a simple colored cube
    for (const auto& [id, player] : remotePlayers) {
        glm::vec3 pos = player->GetPosition();
        
        // Create a simple player representation (box)
        std::vector<PCD::Brush> playerBrushes;
        PCD::Brush playerBox;
        playerBox.id = id;
        playerBox.flags = PCD::BRUSH_SOLID;
        
        // Player dimensions (2 units tall, 0.8 units wide)
        float halfWidth = 0.4f;
        float height = 2.0f;
        
        // Create box vertices
        playerBox.vertices = {
            // Front face
            {{pos.x - halfWidth, pos.y, pos.z + halfWidth}, {0, 0, 1}, {0, 0}},
            {{pos.x + halfWidth, pos.y, pos.z + halfWidth}, {0, 0, 1}, {1, 0}},
            {{pos.x + halfWidth, pos.y + height, pos.z + halfWidth}, {0, 0, 1}, {1, 1}},
            {{pos.x - halfWidth, pos.y + height, pos.z + halfWidth}, {0, 0, 1}, {0, 1}},
            
            // Back face
            {{pos.x + halfWidth, pos.y, pos.z - halfWidth}, {0, 0, -1}, {0, 0}},
            {{pos.x - halfWidth, pos.y, pos.z - halfWidth}, {0, 0, -1}, {1, 0}},
            {{pos.x - halfWidth, pos.y + height, pos.z - halfWidth}, {0, 0, -1}, {1, 1}},
            {{pos.x + halfWidth, pos.y + height, pos.z - halfWidth}, {0, 0, -1}, {0, 1}},
            
            // Top face
            {{pos.x - halfWidth, pos.y + height, pos.z + halfWidth}, {0, 1, 0}, {0, 0}},
            {{pos.x + halfWidth, pos.y + height, pos.z + halfWidth}, {0, 1, 0}, {1, 0}},
            {{pos.x + halfWidth, pos.y + height, pos.z - halfWidth}, {0, 1, 0}, {1, 1}},
            {{pos.x - halfWidth, pos.y + height, pos.z - halfWidth}, {0, 1, 0}, {0, 1}},
            
            // Bottom face
            {{pos.x - halfWidth, pos.y, pos.z - halfWidth}, {0, -1, 0}, {0, 0}},
            {{pos.x + halfWidth, pos.y, pos.z - halfWidth}, {0, -1, 0}, {1, 0}},
            {{pos.x + halfWidth, pos.y, pos.z + halfWidth}, {0, -1, 0}, {1, 1}},
            {{pos.x - halfWidth, pos.y, pos.z + halfWidth}, {0, -1, 0}, {0, 1}},
            
            // Right face
            {{pos.x + halfWidth, pos.y, pos.z + halfWidth}, {1, 0, 0}, {0, 0}},
            {{pos.x + halfWidth, pos.y, pos.z - halfWidth}, {1, 0, 0}, {1, 0}},
            {{pos.x + halfWidth, pos.y + height, pos.z - halfWidth}, {1, 0, 0}, {1, 1}},
            {{pos.x + halfWidth, pos.y + height, pos.z + halfWidth}, {1, 0, 0}, {0, 1}},
            
            // Left face
            {{pos.x - halfWidth, pos.y, pos.z - halfWidth}, {-1, 0, 0}, {0, 0}},
            {{pos.x - halfWidth, pos.y, pos.z + halfWidth}, {-1, 0, 0}, {1, 0}},
            {{pos.x - halfWidth, pos.y + height, pos.z + halfWidth}, {-1, 0, 0}, {1, 1}},
            {{pos.x - halfWidth, pos.y + height, pos.z - halfWidth}, {-1, 0, 0}, {0, 1}},
        };
        
        // Indices for 6 faces
        for (int face = 0; face < 6; face++) {
            uint32_t base = face * 4;
            playerBox.indices.push_back(base + 0);
            playerBox.indices.push_back(base + 1);
            playerBox.indices.push_back(base + 2);
            playerBox.indices.push_back(base + 0);
            playerBox.indices.push_back(base + 2);
            playerBox.indices.push_back(base + 3);
        }
        
        // Color based on player ID
        switch (id % 8) {
            case 0: playerBox.color = PCD::Vec3(1.0f, 0.2f, 0.2f); break; // Red
            case 1: playerBox.color = PCD::Vec3(0.2f, 0.5f, 1.0f); break; // Blue
            case 2: playerBox.color = PCD::Vec3(0.3f, 1.0f, 0.3f); break; // Green
            case 3: playerBox.color = PCD::Vec3(1.0f, 1.0f, 0.2f); break; // Yellow
            case 4: playerBox.color = PCD::Vec3(1.0f, 0.5f, 0.2f); break; // Orange
            case 5: playerBox.color = PCD::Vec3(0.8f, 0.2f, 1.0f); break; // Purple
            case 6: playerBox.color = PCD::Vec3(0.2f, 1.0f, 1.0f); break; // Cyan
            case 7: playerBox.color = PCD::Vec3(1.0f, 0.8f, 0.8f); break; // Pink
            default: playerBox.color = PCD::Vec3(0.5f, 0.5f, 0.5f); break;
        }
        
        playerBrushes.push_back(playerBox);
        
        // Render the player box
        renderer->RenderBrushes(playerBrushes, -1, view, proj);
    }
}

void GameScene::RenderHUD() {
    // Simple HUD overlay using ImGui
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowBgAlpha(0.7f);
    ImGui::Begin("Game HUD", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | 
                 ImGuiWindowFlags_NoResize | 
                 ImGuiWindowFlags_AlwaysAutoResize | 
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoInputs);
    
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "PLAYING");
    ImGui::Separator();
    ImGui::Text("FPS: %d (%.1f ms)", fps, frameTime * 1000.0f);
    ImGui::Text("Players Online: %d", static_cast<int>(remotePlayers.size()) + 1);
    
    if (localPlayer) {
        glm::vec3 pos = localPlayer->GetPosition();
        ImGui::Separator();
        ImGui::Text("Position: %.1f, %.1f, %.1f", pos.x, pos.y, pos.z);
    }
    
    // Show remote players
    if (!remotePlayers.empty()) {
        ImGui::Separator();
        ImGui::Text("Other Players:");
        for (const auto& [id, player] : remotePlayers) {
            glm::vec3 pos = player->GetPosition();
            ImGui::Text("  %s (%.0f, %.0f, %.0f)", 
                       player->GetName().c_str(), pos.x, pos.y, pos.z);
        }
    }
    
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "ESC - Toggle Cursor");
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "F10 - Quit to Menu");
    
    ImGui::End();
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
        glm::vec3(0.0f, 2.0f, 0.0f) // Default spawn
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

} // namespace Game
