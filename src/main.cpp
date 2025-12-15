// main.cpp - Placid multiplayer game entry point

#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Network/NetworkManager.h"
#include "Game/MainMenu.h"
#include "Game/Lobby.h"
#include "Game/GameScene.h"

#include <iostream>
#include <memory>
#include <chrono>

enum class GameState {
    MAIN_MENU,
    LOBBY,
    PLAYING
};

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    
    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Placid Arena", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // OpenGL setup
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    
    // Game state
    GameState currentState = GameState::MAIN_MENU;
    Network::NetworkManager netManager;
    
    std::unique_ptr<Game::MainMenu> mainMenu;
    std::unique_ptr<Game::Lobby> lobby;
    std::unique_ptr<Game::GameScene> gameScene;
    
    // Create main menu
    mainMenu = std::make_unique<Game::MainMenu>();
    
    // Time tracking
    auto lastTime = std::chrono::steady_clock::now();
    
    // Main loop
    std::cout << "=== Placid Arena ===\n";
    std::cout << "Starting game...\n\n";
    
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Cap delta time to prevent huge jumps
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        // Poll events
        glfwPollEvents();
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Handle state-specific logic
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
                // Update network
                netManager.Update(deltaTime);
                
                // Render lobby UI
                Game::LobbyAction action = lobby->Render();
                
                if (action == Game::LobbyAction::START_GAME) {
                    std::string mapName = lobby->GetSelectedMap();
                    
                    // Send game start to all clients
                    netManager.SendGameStart(mapName);
                    
                    // Start game locally
                    gameScene = std::make_unique<Game::GameScene>(window, &netManager);
                    gameScene->Start(mapName);
                    currentState = GameState::PLAYING;
                    
                    // Destroy lobby UI
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
                // Check for game start message (clients only)
                if (gameScene && !gameScene->IsActive()) {
                    // Still in lobby, check for game start
                    // (This is handled by network callback)
                }
                
                // Update game
                if (gameScene) {
                    gameScene->Update(deltaTime);
                    gameScene->Render();
                }
                
                // Check for quit (ESC key)
                if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                    // TODO: Show pause menu
                    // For now, return to main menu
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
        
        // Render ImGui
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
        if (currentState != GameState::PLAYING) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Swap buffers
        glfwSwapBuffers(window);
    }
    
    // Cleanup
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
