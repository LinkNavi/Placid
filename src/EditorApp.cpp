// Enhanced EditorApp.cpp with Unity-like camera controls

#include "Engine/EditorApp.h"
#include "Engine/GameMode.h"
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <cmath>

namespace Editor {

EditorApp::EditorApp() 
    : window(nullptr), mapEditor(nullptr), renderer(nullptr), gameMode(nullptr),
      currentMode(EditorMode::EDIT), cameraMode(CameraMode::FREE),
      cameraPosition(0, 10, 20), cameraFocusPoint(0, 0, 0),
      cameraDistance(20.0f), cameraYaw(0.0f), cameraPitch(0.4f), cameraFOV(60.0f),
      cameraMoveSpeed(10.0f), cameraRotateSpeed(0.005f), cameraZoomSpeed(2.0f),
      shiftPressed(false),
      dragAccumX(0), dragAccumZ(0),
      isLeftDragging(false), isRightDragging(false), isMiddleDragging(false),
      isAltPressed(false),
      lastX(640), lastY(360), firstMouse(true), 
      lastFrame(0), deltaTime(0) {}

EditorApp::~EditorApp() {
    Shutdown();
}

bool EditorApp::Initialize(int width, int height, const char* title) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    renderer = new Renderer();
    if (!renderer->Initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }
    
    mapEditor = new MapEditor();
    
    // Create default floor if map is empty
    if (mapEditor->GetMap().brushes.empty()) {
        PCD::Brush floor = mapEditor->CreateBox(
            PCD::Vec3(-20, -1, -20),
            PCD::Vec3(20, 0, 20)
        );
        floor.name = "Floor";
        floor.color = PCD::Vec3(0.6f, 0.6f, 0.6f);
        mapEditor->GetMap().brushes.push_back(floor);
        
        PCD::Entity spawn;
        spawn.id = mapEditor->GetMap().nextEntityID++;
        spawn.type = PCD::ENT_INFO_PLAYER_START;
        spawn.position = PCD::Vec3(0, 0.1f, 0);
        spawn.name = "PlayerSpawn";
        mapEditor->GetMap().entities.push_back(spawn);
    }
    
    return true;
}

void EditorApp::Run() {
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        ProcessInput(deltaTime);
        UpdateCamera(deltaTime);
        Render();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void EditorApp::Shutdown() {
    delete gameMode;
    delete mapEditor;
    delete renderer;
    
    if (window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

void EditorApp::ProcessInput(float dt) {
    if (currentMode == EditorMode::PLAY) {
        if (gameMode) {
            gameMode->ProcessInput(window, dt);
        }
        return;
    }
    
    // Check modifier keys
    shiftPressed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
                   (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    isAltPressed = (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) ||
                   (glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
    
    // Camera movement (only when not hovering UI)
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse && !io.WantCaptureKeyboard && cameraMode == CameraMode::FREE) {
        float speed = cameraMoveSpeed * (shiftPressed ? 2.0f : 1.0f);
        
        Vec3 forward = GetCameraForward();
        Vec3 right = GetCameraRight();
        
        // Ground-aligned movement
        forward.y = 0;
        forward = forward.normalized();
        right.y = 0;
        right = right.normalized();
        
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cameraFocusPoint = cameraFocusPoint + forward * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cameraFocusPoint = cameraFocusPoint - forward * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cameraFocusPoint = cameraFocusPoint - right * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cameraFocusPoint = cameraFocusPoint + right * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            cameraFocusPoint.y -= speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            cameraFocusPoint.y += speed * dt;
        }
    }
}

void EditorApp::UpdateCamera(float dt) {
    // Calculate camera position from focus point, distance, and rotation
    cameraPosition.x = cameraFocusPoint.x + cameraDistance * sin(cameraYaw) * cos(cameraPitch);
    cameraPosition.y = cameraFocusPoint.y + cameraDistance * sin(cameraPitch);
    cameraPosition.z = cameraFocusPoint.z + cameraDistance * cos(cameraYaw) * cos(cameraPitch);
}

void EditorApp::FocusOnSelection() {
    if (mapEditor->GetSelectedBrushIndex() >= 0) {
        PCD::Vec3 pos = mapEditor->GetSelectedObjectPosition();
        cameraFocusPoint = Vec3(pos.x, pos.y, pos.z);
        cameraDistance = 15.0f;
    } else if (mapEditor->GetSelectedEntityIndex() >= 0) {
        PCD::Vec3 pos = mapEditor->GetSelectedObjectPosition();
        cameraFocusPoint = Vec3(pos.x, pos.y, pos.z);
        cameraDistance = 10.0f;
    }
}

void EditorApp::OrbitCamera(float deltaX, float deltaY) {
    cameraYaw -= deltaX * cameraRotateSpeed;
    cameraPitch += deltaY * cameraRotateSpeed;
    
    // Clamp pitch
    const float maxPitch = 1.5f;
    if (cameraPitch > maxPitch) cameraPitch = maxPitch;
    if (cameraPitch < -maxPitch) cameraPitch = -maxPitch;
}

void EditorApp::PanCamera(float deltaX, float deltaY) {
    Vec3 right = GetCameraRight();
    Vec3 up = GetCameraUp();
    
    float panSpeed = cameraDistance * 0.002f;
    cameraFocusPoint = cameraFocusPoint - right * deltaX * panSpeed;
    cameraFocusPoint = cameraFocusPoint + up * deltaY * panSpeed;
}

void EditorApp::ZoomCamera(float delta) {
    cameraDistance -= delta * cameraZoomSpeed;
    if (cameraDistance < 1.0f) cameraDistance = 1.0f;
    if (cameraDistance > 500.0f) cameraDistance = 500.0f;
}

Vec3 EditorApp::GetCameraForward() const {
    return Vec3(
        sin(cameraYaw) * cos(cameraPitch),
        sin(cameraPitch),
        cos(cameraYaw) * cos(cameraPitch)
    );
}

Vec3 EditorApp::GetCameraRight() const {
    Vec3 forward = GetCameraForward();
    Vec3 up(0, 1, 0);
    return forward.cross(up).normalized();
}

Vec3 EditorApp::GetCameraUp() const {
    Vec3 forward = GetCameraForward();
    Vec3 right = GetCameraRight();
    return right.cross(forward).normalized();
}

void EditorApp::Render() {
    if (currentMode == EditorMode::PLAY) {
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / height;
        
        float proj[16];
        gameMode->GetCamera().GetProjectionMatrix(proj, aspect);
        gameMode->Render(proj);
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowBgAlpha(0.7f);
        ImGui::Begin("Play Mode", nullptr, 
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "PLAY MODE");
        ImGui::Separator();
        ImGui::Text("WASD - Move");
        ImGui::Text("Space - Jump");
        ImGui::Text("Shift - Sprint");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "ESC - Return to Editor");
        ImGui::End();
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        return;
    }
    
    // Editor mode
    glClearColor(0.18f, 0.18f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    float view[16], proj[16];
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float aspect = (float)width / height;
    
    GetEditorViewMatrix(view);
    
    // Projection matrix
    float f = 1.0f / tan(cameraFOV * 3.14159f / 360.0f);
    float near = 0.1f, far = 1000.0f;
    
    proj[0] = f/aspect; proj[4] = 0; proj[8] = 0;                      proj[12] = 0;
    proj[1] = 0;        proj[5] = f; proj[9] = 0;                      proj[13] = 0;
    proj[2] = 0;        proj[6] = 0; proj[10] = (far+near)/(near-far); proj[14] = (2*far*near)/(near-far);
    proj[3] = 0;        proj[7] = 0; proj[11] = -1;                    proj[15] = 0;
    
    renderer->RenderGrid(mapEditor->GetSettings(), PCD::Vec3(cameraFocusPoint.x, cameraFocusPoint.y, cameraFocusPoint.z), view, proj);
    renderer->RenderBrushes(mapEditor->GetMap().brushes, mapEditor->GetSelectedBrushIndex(), view, proj);
    renderer->RenderEntities(mapEditor->GetMap().entities, mapEditor->GetSelectedEntityIndex(), 
                            mapEditor->GetSettings().showEntityIcons, view, proj);
    
    mapEditor->RenderGizmo(renderer, view, proj);
    
    if (mapEditor->IsCreating()) {
        renderer->RenderCreationPreview(mapEditor->GetCreateStart(), mapEditor->GetCreateEnd(),
                                       mapEditor->GetSettings().gridSize, view, proj);
    }
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    mapEditor->RenderUI();
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorApp::GetEditorViewMatrix(float* mat) {
    Vec3 eye = cameraPosition;
    Vec3 target = cameraFocusPoint;
    Vec3 up(0, 1, 0);
    
    Vec3 f = (target - eye).normalized();
    Vec3 r = f.cross(up).normalized();
    Vec3 u = r.cross(f);
    
    mat[0] = r.x;  mat[4] = r.y;  mat[8] = r.z;   mat[12] = -r.x*eye.x - r.y*eye.y - r.z*eye.z;
    mat[1] = u.x;  mat[5] = u.y;  mat[9] = u.z;   mat[13] = -u.x*eye.x - u.y*eye.y - u.z*eye.z;
    mat[2] = -f.x; mat[6] = -f.y; mat[10] = -f.z; mat[14] = f.x*eye.x + f.y*eye.y + f.z*eye.z;
    mat[3] = 0;    mat[7] = 0;    mat[11] = 0;    mat[15] = 1;
}

void EditorApp::EnterPlayMode() {
    std::cout << "Entering play mode...\n";
    currentMode = EditorMode::PLAY;
    gameMode = new Game::GameMode(renderer, &mapEditor->GetMap());
    gameMode->Initialize();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    firstMouse = true;
}

void EditorApp::ExitPlayMode() {
    std::cout << "Exiting play mode...\n";
    currentMode = EditorMode::EDIT;
    delete gameMode;
    gameMode = nullptr;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    firstMouse = true;
}

EditorApp* EditorApp::GetInstance(GLFWwindow* window) {
    return static_cast<EditorApp*>(glfwGetWindowUserPointer(window));
}

void EditorApp::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    EditorApp* app = GetInstance(window);
    if (!app) return;
    
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_F5) {
            if (app->currentMode == EditorMode::EDIT) {
                app->EnterPlayMode();
            } else {
                app->ExitPlayMode();
            }
            return;
        }
        
        if (key == GLFW_KEY_ESCAPE && app->currentMode == EditorMode::PLAY) {
            app->ExitPlayMode();
            return;
        }
        
        if (app->currentMode == EditorMode::PLAY && app->gameMode) {
            app->gameMode->GetKeybinds().OnKeyEvent(key, action);
            return;
        }
        
        if (!app->mapEditor) return;
        
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard) return;
        
        // F key to frame selection
        if (key == GLFW_KEY_F) {
            app->FocusOnSelection();
        }
        
        if (mods & GLFW_MOD_CONTROL) {
            if (key == GLFW_KEY_N) app->mapEditor->NewMap();
            if (key == GLFW_KEY_S) app->mapEditor->SaveMap();
            if (key == GLFW_KEY_Z) app->mapEditor->Undo();
            if (key == GLFW_KEY_Y) app->mapEditor->Redo();
            if (key == GLFW_KEY_D) app->mapEditor->DuplicateSelected();
            if (key == GLFW_KEY_A) app->mapEditor->SelectAll();
        } else {
            if (key == GLFW_KEY_1) app->mapEditor->SetTool(PCD::EditorTool::SELECT);
            if (key == GLFW_KEY_2) app->mapEditor->SetTool(PCD::EditorTool::MOVE);
            if (key == GLFW_KEY_3) app->mapEditor->SetTool(PCD::EditorTool::ROTATE);
            if (key == GLFW_KEY_4) app->mapEditor->SetTool(PCD::EditorTool::SCALE);
            if (key == GLFW_KEY_5) app->mapEditor->SetTool(PCD::EditorTool::CREATE_BOX);
            
            if (key == GLFW_KEY_DELETE) app->mapEditor->DeleteSelected();
            if (key == GLFW_KEY_ESCAPE) app->mapEditor->DeselectAll();
            
            if (key == GLFW_KEY_B) app->mapEditor->SetTool(PCD::EditorTool::CREATE_BOX);
            if (key == GLFW_KEY_C) app->mapEditor->SetTool(PCD::EditorTool::CREATE_CYLINDER);
        }
    }
}

void EditorApp::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    EditorApp* app = GetInstance(window);
    if (!app) return;
    
    if (app->currentMode == EditorMode::PLAY) return;
    if (!app->mapEditor) return;
    
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (app->isAltPressed) {
                // Alt+Left = Orbit mode
                app->cameraMode = CameraMode::ORBIT;
                app->isLeftDragging = true;
            } else {
                // Normal left click - selection/manipulation
                app->dragAccumX = 0;
                app->dragAccumZ = 0;
                app->isLeftDragging = true;
                
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                
                double mx, my;
                glfwGetCursorPos(window, &mx, &my);
                
                float ndcX = (2.0f * mx / width) - 1.0f;
                float ndcY = 1.0f - (2.0f * my / height);
                
                auto& settings = app->mapEditor->GetSettings();
                float gridY = settings.gridHeight;
                float worldX = app->cameraFocusPoint.x + ndcX * app->cameraDistance * 0.5f;
                float worldZ = app->cameraFocusPoint.z + ndcY * app->cameraDistance * 0.5f;
                
                bool shift = mods & GLFW_MOD_SHIFT;
                app->mapEditor->OnMouseClick(worldX, gridY, worldZ, shift);
            }
        } else if (action == GLFW_RELEASE) {
            app->isLeftDragging = false;
            if (app->cameraMode == CameraMode::ORBIT) {
                app->cameraMode = CameraMode::FREE;
            } else {
                app->mapEditor->OnMouseRelease();
            }
        }
    }
    
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        app->isRightDragging = (action == GLFW_PRESS);
    }
    
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            app->cameraMode = CameraMode::PAN;
            app->isMiddleDragging = true;
        } else {
            app->isMiddleDragging = false;
            app->cameraMode = CameraMode::FREE;
        }
    }
}

void EditorApp::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    EditorApp* app = GetInstance(window);
    if (!app) return;
    
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    
    if (app->currentMode == EditorMode::EDIT) {
        app->ZoomCamera(yoffset);
    }
}

void EditorApp::MouseCallback(GLFWwindow* window, double xpos, double ypos) {
    EditorApp* app = GetInstance(window);
    if (!app) return;
    
    if (app->firstMouse) {
        app->lastX = xpos;
        app->lastY = ypos;
        app->firstMouse = false;
    }
    
    float dx = xpos - app->lastX;
    float dy = ypos - app->lastY;
    app->lastX = xpos;
    app->lastY = ypos;
    
    if (app->currentMode == EditorMode::PLAY && app->gameMode) {
        app->gameMode->GetCamera().ProcessMouse(dx, dy);
        return;
    }
    
    if (app->currentMode == EditorMode::EDIT) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) return;
        
        // Right-click rotate (orbit)
        if (app->isRightDragging) {
            app->OrbitCamera(dx, dy);
        }
        
        // Middle-click pan
        if (app->isMiddleDragging || app->cameraMode == CameraMode::PAN) {
            app->PanCamera(dx, dy);
        }
        
        // Alt+Left orbit
        if (app->cameraMode == CameraMode::ORBIT && app->isLeftDragging) {
            app->OrbitCamera(dx, dy);
        }
        
        // Drag for manipulation
        if (app->mapEditor && app->isLeftDragging && app->cameraMode == CameraMode::FREE) {
            float moveScale = app->cameraDistance * 0.005f;
            
            Vec3 right = app->GetCameraRight();
            Vec3 forward = app->GetCameraForward();
            forward.y = 0;
            forward = forward.normalized();
            
float invDX = -dx;
float invDY = -dy;
         
float worldDX = (invDX * right.x - invDY * forward.x) * moveScale;
float worldDZ = (invDX * right.z - invDY * forward.z) * moveScale;

            
            app->dragAccumX += worldDX;
            app->dragAccumZ += worldDZ;
            
            auto& settings = app->mapEditor->GetSettings();
            float gridY = settings.gridHeight;
            
            bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
                         glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
            
            app->mapEditor->OnMouseDrag(app->dragAccumX, gridY, app->dragAccumZ, 
                                       dx, dy, shift);
        }
    }
}

} // namespace Editor
