// Lobby.h - Updated with map transfer support

#pragma once

#include "imgui.h"
#include "Network/NetworkManager.h"
#include <string>
#include <vector>
#include <cstring>
#include <filesystem>

namespace Game {

enum class LobbyAction {
    NONE,
    START_GAME,
    LEAVE_LOBBY
};

class Lobby {
private:
    Network::NetworkManager* netManager;
    bool isHost;
    
    std::vector<std::string> availableMaps;
    int selectedMapIndex;
    
    char chatInputBuffer[256];
    std::vector<std::string> chatMessages;
    bool scrollToBottom;
    
    bool waitingForMap;

public:
    Lobby(Network::NetworkManager* net, bool host) 
        : netManager(net)
        , isHost(host)
        , selectedMapIndex(0)
        , scrollToBottom(false)
        , waitingForMap(false)
    {
        memset(chatInputBuffer, 0, sizeof(chatInputBuffer));
        
        availableMaps = ScanMapsDirectory("Assets/maps");
        
        if (availableMaps.empty()) {
            availableMaps.push_back("maps/dm_arena.pcd");
            availableMaps.push_back("maps/dm_facility.pcd");
            availableMaps.push_back("maps/test.pcd");
        }
        
        netManager->SetMessageCallback([this](const std::string& msgType, 
                                             const std::vector<std::string>& args,
                                             const std::string& fromIP, 
                                             uint16_t fromPort) {
            if (msgType == Network::MessageType::CHAT_MESSAGE && args.size() >= 2) {
                uint32_t senderId = std::stoul(args[0]);
                std::string message = args[1];
                
                std::string senderName = "Unknown";
                const auto& clients = netManager->GetClients();
                auto it = clients.find(senderId);
                if (it != clients.end()) {
                    senderName = it->second.name;
                }
                
                AddChatMessage(senderName + ": " + message);
            }
        });
        
        // Client: request map on join
        if (!isHost) {
            std::cout << "[LOBBY] Requesting map from host...\n";
            AddChatMessage("[System] Requesting map from host...");
            netManager->RequestMap();
            waitingForMap = true;
            
            netManager->SetMapLoadedCallback([this](const PCD::Map& map) {
                std::cout << "[LOBBY] Map loaded!\n";
                AddChatMessage("[System] Map loaded successfully!");
                waitingForMap = false;
            });
        }
    }
    
    LobbyAction Render() {
        LobbyAction action = LobbyAction::NONE;
        
        ImGuiIO& io = ImGui::GetIO();
        
        // Lobby window
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(300, io.DisplaySize.y - 20));
        ImGui::Begin("Lobby", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        
        if (isHost) {
            ImGui::Text("Lobby (You are Host)");
        } else {
            ImGui::Text("Lobby");
        }
        ImGui::Separator();
        ImGui::Spacing();
        
        // Player list
        ImGui::Text("Players (%d):", netManager->GetPlayerCount());
        ImGui::BeginChild("PlayerList", ImVec2(0, 200), true);
        
        const auto& clients = netManager->GetClients();
        for (const auto& [id, client] : clients) {
            ImGui::PushID(id);
            
            ImVec4 color = GetPlayerColor(id);
            ImGui::TextColored(color, "[%d] %s", id, client.name.c_str());
            
            if (id == netManager->GetLocalPlayerId()) {
                ImGui::SameLine();
                ImGui::TextDisabled("(You)");
            }
            
            if (id == 1) {
                ImGui::SameLine();
                ImGui::TextDisabled("(Host)");
            }
            
            // Show map status
            if (!client.hasMap) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "[No Map]");
            } else {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "[Ready]");
            }
            
            ImGui::PopID();
        }
        
        ImGui::EndChild();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Map selection (host only)
        if (isHost) {
            ImGui::Text("Select Map:");
            ImGui::SetNextItemWidth(260);
            
            if (!availableMaps.empty()) {
                if (ImGui::BeginCombo("##map", availableMaps[selectedMapIndex].c_str())) {
                    for (size_t i = 0; i < availableMaps.size(); i++) {
                        bool isSelected = (selectedMapIndex == static_cast<int>(i));
                        if (ImGui::Selectable(availableMaps[i].c_str(), isSelected)) {
                            selectedMapIndex = static_cast<int>(i);
                            
                            // Load map on host
                            if (netManager->LoadMap(availableMaps[selectedMapIndex])) {
                                AddChatMessage("[System] Map loaded: " + availableMaps[selectedMapIndex]);
                            } else {
                                AddChatMessage("[System] Failed to load map!");
                            }
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            
            ImGui::Spacing();
            
            // Check if all players have map
            bool allPlayersReady = true;
            for (const auto& [id, client] : clients) {
                if (!client.hasMap) {
                    allPlayersReady = false;
                    break;
                }
            }
            
            if (!allPlayersReady) {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Waiting for players...");
            }
            
            // Start button
            ImGui::BeginDisabled(!allPlayersReady);
            if (ImGui::Button("Start Game", ImVec2(260, 40))) {
                action = LobbyAction::START_GAME;
            }
            ImGui::EndDisabled();
        } else {
            if (waitingForMap) {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Downloading map...");
            } else {
                ImGui::Text("Waiting for host to start...");
            }
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        if (ImGui::Button("Leave Lobby", ImVec2(260, 30))) {
            action = LobbyAction::LEAVE_LOBBY;
        }
        
        ImGui::End();
        
        // Chat window
        ImGui::SetNextWindowPos(ImVec2(320, 10));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 330, io.DisplaySize.y - 20));
        ImGui::Begin("Chat", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        
        ImGui::BeginChild("ChatMessages", ImVec2(0, -30), true);
        
        for (const auto& msg : chatMessages) {
            ImGui::TextWrapped("%s", msg.c_str());
        }
        
        if (scrollToBottom) {
            ImGui::SetScrollHereY(1.0f);
            scrollToBottom = false;
        }
        
        ImGui::EndChild();
        
        ImGui::SetNextItemWidth(-80);
        bool enterPressed = ImGui::InputText("##chatinput", chatInputBuffer, 
                                            sizeof(chatInputBuffer),
                                            ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        bool sendPressed = ImGui::Button("Send", ImVec2(70, 0));
        
        if ((enterPressed || sendPressed) && strlen(chatInputBuffer) > 0) {
            SendChatMessage(chatInputBuffer);
            memset(chatInputBuffer, 0, sizeof(chatInputBuffer));
        }
        
        ImGui::End();
        
        return action;
    }
    
    std::string GetSelectedMap() const {
        if (availableMaps.empty()) {
            return "maps/test.pcd";
        }
        return availableMaps[selectedMapIndex];
    }
    
    void AddChatMessage(const std::string& message) {
        chatMessages.push_back(message);
        scrollToBottom = true;
        
        if (chatMessages.size() > 100) {
            chatMessages.erase(chatMessages.begin());
        }
    }
    
private:
    std::vector<std::string> ScanMapsDirectory(const std::string& path) {
        namespace fs = std::filesystem;
        std::vector<std::string> files;
        
        try {
            if (fs::exists(path) && fs::is_directory(path)) {
                for (const auto& entry : fs::directory_iterator(path)) {
                    if (entry.is_regular_file()) {
                        std::string filename = entry.path().string();
                        if (filename.size() >= 4 && 
                            filename.substr(filename.size() - 4) == ".pcd") {
                            files.push_back(filename);
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[LOBBY] Error scanning maps: " << e.what() << "\n";
        }
        
        return files;
    }
    
    void SendChatMessage(const char* message) {
        netManager->SendChatMessage(message);
        
        const auto& clients = netManager->GetClients();
        auto it = clients.find(netManager->GetLocalPlayerId());
        std::string senderName = "You";
        if (it != clients.end()) {
            senderName = it->second.name;
        }
        
        AddChatMessage(senderName + ": " + message);
    }
    
    ImVec4 GetPlayerColor(uint32_t playerId) {
        switch (playerId % 8) {
            case 0: return ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
            case 1: return ImVec4(0.2f, 0.5f, 1.0f, 1.0f);
            case 2: return ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
            case 3: return ImVec4(1.0f, 1.0f, 0.2f, 1.0f);
            case 4: return ImVec4(1.0f, 0.5f, 0.2f, 1.0f);
            case 5: return ImVec4(0.8f, 0.2f, 1.0f, 1.0f);
            case 6: return ImVec4(0.2f, 1.0f, 1.0f, 1.0f);
            case 7: return ImVec4(1.0f, 0.8f, 0.8f, 1.0f);
            default: return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        }
    }
};

} // namespace Game
