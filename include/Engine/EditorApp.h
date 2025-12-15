#ifndef EDITOR_APP_H
#define EDITOR_APP_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "MapEditor.h"
#include "Renderer.h"

namespace Game {
    class GameMode;
}

namespace Editor {

enum class EditorMode {
    EDIT,
    PLAY
};

class EditorApp {
private:
    GLFWwindow* window;
    MapEditor* mapEditor;
    Renderer* renderer;
    Game::GameMode* gameMode;
    EditorMode currentMode;
    
    // Camera state
    float editorCamDist;
    float editorCamYaw;
    float editorCamPitch;
    Vec3 editorCamTarget;  // Using Camera.h Vec3
     // Drag state tracking
    float dragAccumX;
    float dragAccumZ;
    bool isLeftDragging;
    // Mouse state
    double lastX, lastY;
    bool firstMouse;
    
    // Timing
    float lastFrame;
    
public:
    EditorApp();
    ~EditorApp();
    
    bool Initialize(int width, int height, const char* title);
    void Run();
    void Shutdown();
    
    void EnterPlayMode();
    void ExitPlayMode();
    
private:
    void ProcessInput(float dt);
    void Render();
    void GetEditorViewMatrix(float* mat);
    
    // GLFW callbacks
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    
    // Helper to get instance from window
    static EditorApp* GetInstance(GLFWwindow* window);
};

} // namespace Editor

#endif // EDITOR_APP_H
