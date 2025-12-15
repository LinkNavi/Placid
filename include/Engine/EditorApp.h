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

enum class CameraMode {
    FREE,      // Normal WASD movement
    ORBIT,     // Alt+Left - orbit around focus point
    PAN        // Middle mouse - pan view
};

class EditorApp {
private:
    GLFWwindow* window;
    MapEditor* mapEditor;
    Renderer* renderer;
    Game::GameMode* gameMode;
    EditorMode currentMode;
    
    // Unity-like camera
    CameraMode cameraMode;
    Vec3 cameraPosition;
    Vec3 cameraFocusPoint;
    float cameraDistance;
    float cameraYaw;
    float cameraPitch;
    float cameraFOV;
    
    // Camera movement
    float cameraMoveSpeed;
    float cameraRotateSpeed;
    float cameraZoomSpeed;
    bool shiftPressed;
    
    // Drag state
    float dragAccumX;
    float dragAccumZ;
    bool isLeftDragging;
    bool isRightDragging;
    bool isMiddleDragging;
    bool isAltPressed;
    
    // Mouse state
    double lastX, lastY;
    bool firstMouse;
    
    // Timing
    float lastFrame;
    float deltaTime;
    
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
    void UpdateCamera(float dt);
    void Render();
    
    // Camera functions
    void FocusOnSelection();
    void OrbitCamera(float deltaX, float deltaY);
    void PanCamera(float deltaX, float deltaY);
    void ZoomCamera(float delta);
    Vec3 GetCameraForward() const;
    Vec3 GetCameraRight() const;
    Vec3 GetCameraUp() const;
    void GetEditorViewMatrix(float* mat);
    
    // GLFW callbacks
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    
    // Helper
    static EditorApp* GetInstance(GLFWwindow* window);
};

} // namespace Editor

#endif // EDITOR_APP_H
