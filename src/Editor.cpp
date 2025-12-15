// src/Editor.cpp - Standalone Map Editor

#include "Engine/EditorApp.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║     PLACID ARENA - MAP EDITOR         ║\n";
    std::cout << "╚════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    Editor::EditorApp editor;
    
    if (!editor.Initialize(1280, 720, "Placid Map Editor")) {
        std::cerr << "Failed to initialize editor\n";
        return -1;
    }
    
    // Load map from command line if provided
    if (argc > 1) {
        std::cout << "Loading map: " << argv[1] << "\n";
        // TODO: Add map loading here
    }
    
    std::cout << "\n=== EDITOR CONTROLS ===\n";
    std::cout << "  WASD/QE        - Move camera\n";
    std::cout << "  Right-Click    - Rotate camera (hold + drag)\n";
    std::cout << "  Middle-Click   - Pan camera (hold + drag)\n";
    std::cout << "  Alt+Left-Click - Orbit around selection\n";
    std::cout << "  Scroll Wheel   - Zoom in/out\n";
    std::cout << "  F              - Frame selected object\n";
    std::cout << "  Shift          - Increase movement speed\n";
    std::cout << "\n=== TOOLS ===\n";
    std::cout << "  1 - Select     4 - Scale\n";
    std::cout << "  2 - Move       5 - Create Box\n";
    std::cout << "  3 - Rotate     B - Box, C - Cylinder\n";
    std::cout << "\n=== ACTIONS ===\n";
    std::cout << "  Ctrl+N    - New map\n";
    std::cout << "  Ctrl+S    - Save map\n";
    std::cout << "  Ctrl+Z    - Undo\n";
    std::cout << "  Ctrl+Y    - Redo\n";
    std::cout << "  Ctrl+D    - Duplicate\n";
    std::cout << "  Delete    - Delete selected\n";
    std::cout << "  F5        - Test in play mode\n";
    std::cout << "========================\n\n";
    
    editor.Run();
    editor.Shutdown();
    
    std::cout << "Editor closed.\n";
    return 0;
}
