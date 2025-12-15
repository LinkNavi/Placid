// main.cpp - Fixed with proper game start synchronization

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Network/NetworkManager.h"
#include "Game/MainMenu.h"
#include "Game/Lobby.h"
#include "Game/GameScene.h"
#include "Game/LocalPlayer.h"
#include "Game/RemotePlayer.h"

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

enum class GameState {
    MAIN_MENU,
    LOBBY,
    PLAYING
};

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4);
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Placid Arena", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    // Load OpenGL with glad
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    
    GameState currentState = GameState::MAIN_MENU;
    Network::NetworkManager netManager;
    
    std::unique_ptr<Game::MainMenu> mainMenu;
    std::unique_ptr<Game::Lobby> lobby;
    std::unique_ptr<Game::GameScene> gameScene;
    
    mainMenu = std::make_unique<Game::MainMenu>();
    
    auto lastTime = std::chrono::steady_clock::now();
    
    std::cout << "=== Placid Arena ===\n";
    std::cout << "Starting game...\n\n";
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        switch (currentState) {
            case GameState::MAIN_MENU: {
                Game::MenuAction action = mainMenu->Render();
                
                if (action == Game::MenuAction::HOST_GAME) {
                    std::string playerName = mainMenu->GetPlayerName();
                    uint16_t port = mainMenu->GetPort();
                    
                    if (netManager.HostGame(port, playerName)) {
                        std::cout << "Hosting game on port " << port << "\n";
                        lobby = std::make_unique<Game::Lobby>(&netManager, true);
                        currentState = GameState::LOBBY;
                    } else {
                        mainMenu->ShowError("Failed to host game!");
                    }
                }
                else if (action == Game::MenuAction::JOIN_GAME) {
                    std::string playerName = mainMenu->GetPlayerName();
                    std::string hostIP = mainMenu->GetHostIP();
                    uint16_t port = mainMenu->GetPort();
                    
                    if (netManager.JoinGame(hostIP, port, playerName)) {
                        std::cout << "Joined game at " << hostIP << ":" << port << "\n";
                        lobby = std::make_unique<Game::Lobby>(&netManager, false);
                        currentState = GameState::LOBBY;
                    } else {
                        mainMenu->ShowError("Failed to connect to host!");
                    }
                }
                else if (action == Game::MenuAction::QUIT) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                
                break;
            }
            
            case GameState::LOBBY: {
                netManager.Update(deltaTime);
                
                Game::LobbyAction action = lobby->Render();
                
                // Check if client should start game (received GAME_START from host)
                if (!netManager.IsHost() && lobby->ShouldStartGame()) {
                    std::cout << "[MAIN] Client starting game...\n";
                    lobby->ResetStartFlag();
                    
                    gameScene = std::make_unique<Game::GameScene>(window, &netManager);
                    
                    // Get the map name from the loaded map
                    const PCD::Map& map = netManager.GetMap();
                    std::string mapName = map.name.empty() ? "Unknown Map" : map.name;
                    
                    gameScene->Start(mapName);
                    currentState = GameState::PLAYING;
                    lobby.reset();
                }
                else if (action == Game::LobbyAction::START_GAME) {
                    std::string mapName = lobby->GetSelectedMap();
                    
                    // Host: Send game start message to all clients
                    if (netManager.IsHost()) {
                        std::cout << "[MAIN] Host starting game with map: " << mapName << "\n";
                        netManager.SendGameStart(mapName);
                        
                        // Small delay to ensure message is sent
                        netManager.Update(0.016f);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                    
                    // Both host and client: start the game scene
                    gameScene = std::make_unique<Game::GameScene>(window, &netManager);
                    gameScene->Start(mapName);
                    currentState = GameState::PLAYING;
                    
                    lobby.reset();
                }
                else if (action == Game::LobbyAction::LEAVE_LOBBY) {
                    netManager.Disconnect();
                    lobby.reset();
                    currentState = GameState::MAIN_MENU;
                }
                
                break;
            }
            
            case GameState::PLAYING: {
                if (gameScene) {
                    gameScene->Update(deltaTime);
                    gameScene->Render();
                }
                
                // Check for quit with F10 (ESC toggles cursor)
                if (glfwGetKey(window, GLFW_KEY_F10) == GLFW_PRESS) {
                    if (gameScene) {
                        gameScene->Stop();
                        gameScene.reset();
                    }
                    netManager.Disconnect();
                    currentState = GameState::MAIN_MENU;
                }
                
                break;
            }
        }
        
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
        if (currentState != GameState::PLAYING) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
        
        auto frameEnd = std::chrono::steady_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - currentTime);
        const auto targetFrameTime = std::chrono::microseconds(16667);
        
        if (frameDuration < targetFrameTime) {
            std::this_thread::sleep_for(targetFrameTime - frameDuration);
        }
    }
    
    if (netManager.IsConnected()) {
        netManager.Disconnect();
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "\nGoodbye!\n";
    return 0;
}
