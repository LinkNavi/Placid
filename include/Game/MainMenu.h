// MainMenu.h - Main menu UI for hosting/joining games

#pragma once

#include "imgui.h"
#include <string>
#include <cstring>

namespace Game {

enum class MenuAction {
    NONE,
    HOST_GAME,
    JOIN_GAME,
    QUIT
};

class MainMenu {
private:
    char playerNameBuffer[32];
    char hostIPBuffer[64];
    char portBuffer[16];
    
    bool showError;
    std::string errorMessage;

public:
    MainMenu() 
        : showError(false)
    {
        // Default values
        strcpy(playerNameBuffer, "Player");
        strcpy(hostIPBuffer, "127.0.0.1");
        strcpy(portBuffer, "7777");
    }
    
    MenuAction Render() {
        MenuAction action = MenuAction::NONE;
        
        // Center window
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), 
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 300));
        
        ImGui::Begin("Placid Arena", nullptr, 
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        
        ImGui::Text("Welcome to Placid Arena!");
        ImGui::Separator();
        ImGui::Spacing();
        
        // Player name input
        ImGui::Text("Your Name:");
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##playername", playerNameBuffer, sizeof(playerNameBuffer));
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        // Host Game button
        if (ImGui::Button("Host Game", ImVec2(300, 40))) {
            if (strlen(playerNameBuffer) > 0) {
                action = MenuAction::HOST_GAME;
            } else {
                ShowError("Please enter your name!");
            }
        }
        
        ImGui::Spacing();
        
        // Join Game section
        ImGui::Text("Join Game:");
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("IP Address", hostIPBuffer, sizeof(hostIPBuffer));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputText("Port", portBuffer, sizeof(portBuffer));
        
        if (ImGui::Button("Join Game", ImVec2(300, 40))) {
            if (strlen(playerNameBuffer) > 0 && strlen(hostIPBuffer) > 0) {
                action = MenuAction::JOIN_GAME;
            } else {
                ShowError("Please enter your name and host IP!");
            }
        }
        
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        
        // Quit button
        if (ImGui::Button("Quit", ImVec2(300, 30))) {
            action = MenuAction::QUIT;
        }
        
        // Error popup
        if (showError) {
            ImGui::OpenPopup("Error");
            showError = false;
        }
        
        if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", errorMessage.c_str());
            ImGui::Spacing();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        ImGui::End();
        
        return action;
    }
    
    std::string GetPlayerName() const {
        return std::string(playerNameBuffer);
    }
    
    std::string GetHostIP() const {
        return std::string(hostIPBuffer);
    }
    
    uint16_t GetPort() const {
        return static_cast<uint16_t>(std::stoi(portBuffer));
    }
    
    void ShowError(const std::string& message) {
        errorMessage = message;
        showError = true;
    }
};

} // namespace Game
