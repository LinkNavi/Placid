#include "Engine/EditorApp.h"
#include "Server.h"
#include <iostream>
#include <string>

void PrintUsage(const char* programName) {
    std::cout << "Placid - FPS Game Engine\n";
    std::cout << "Usage: " << programName << " [mode]\n\n";
    std::cout << "Modes:\n";
    std::cout << "  --editor       Start the map editor (default)\n";
    std::cout << "  --server PORT  Start dedicated server on PORT\n";
    std::cout << "  --game         Start the game client\n";
    std::cout << "  --help         Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string mode = "--editor";  // Default mode
    
    // Parse arguments
    if (argc > 1) {
        mode = argv[1];
    }
    
    if (mode == "--help" || mode == "-h") {
        PrintUsage(argv[0]);
        return 0;
    }
    
    // Start Editor
    if (mode == "--editor") {
        std::cout << "Starting Placid Map Editor...\n";
        
        Editor::EditorApp editor;
        if (!editor.Initialize(1280, 720, "Placid Map Editor")) {
            std::cerr << "Failed to initialize editor\n";
            return -1;
        }
        
        editor.Run();
        editor.Shutdown();
        return 0;
    }
    
    // Start Server
    if (mode == "--server") {
        if (argc < 3) {
            std::cerr << "Error: Server mode requires a port number\n";
            std::cerr << "Usage: " << argv[0] << " --server PORT\n";
            return -1;
        }
        
        int port = std::stoi(argv[2]);
        std::cout << "Starting Placid Dedicated Server on port " << port << "...\n";
        
        Server server(port);
        server.Start();
        
        std::cout << "Server running. Press Ctrl+C to stop.\n";
        
        // Keep running until interrupted
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        server.Stop();
        return 0;
    }
    
    // Start Game
    if (mode == "--game") {
        std::cout << "Game client mode not yet implemented\n";
        std::cout << "Use --editor to create maps\n";
        return 0;
    }
    
    // Unknown mode
    std::cerr << "Unknown mode: " << mode << "\n\n";
    PrintUsage(argv[0]);
    return -1;
}
