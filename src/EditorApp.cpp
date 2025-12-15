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
      currentMode(EditorMode::EDIT),
      editorCamDist(30.0f), editorCamYaw(0.45f), editorCamPitch(0.6f),
      editorCamTarget(0, 0, 0), lastX(640), lastY(360), firstMouse(true), lastFrame(0) {}

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
    
    std::cout << "\n=== Placid Map Editor ===\n";
    std::cout << "Controls:\n";
    std::cout << "  WASD/QE     - Move camera\n";
    std::cout << "  Right Mouse - Rotate view\n";
    std::cout << "  Middle Mouse- Pan view\n";
    std::cout << "  Scroll      - Zoom\n";
    std::cout << "  Left Click  - Select/Manipulate\n";
    std::cout << "  1-5         - Tools (Select/Move/Rotate/Scale/Create)\n";
    std::cout << "  Shift+Drag  - Constrain to axis\n";
    std::cout << "  Ctrl+S      - Save map\n";
    std::cout << "  Delete      - Delete selected\n";
    std::cout << "  Ctrl+D      - Duplicate\n";
    std::cout << "  F5          - Test map\n";
    std::cout << "=========================\n\n";
    
    return true;
}

void EditorApp::Run() {
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        ProcessInput(dt);
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
    
    // Camera movement (only when not hovering UI)
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) {
        float moveSpeed = editorCamDist * 0.02f;
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            editorCamTarget.z -= moveSpeed * cos(editorCamYaw);
            editorCamTarget.x -= moveSpeed * sin(editorCamYaw);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            editorCamTarget.z += moveSpeed * cos(editorCamYaw);
            editorCamTarget.x += moveSpeed * sin(editorCamYaw);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            editorCamTarget.x -= moveSpeed * cos(editorCamYaw);
            editorCamTarget.z += moveSpeed * sin(editorCamYaw);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            editorCamTarget.x += moveSpeed * cos(editorCamYaw);
            editorCamTarget.z -= moveSpeed * sin(editorCamYaw);
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            editorCamTarget.y -= moveSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            editorCamTarget.y += moveSpeed;
        }
    }
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
    
    float fov = 45.0f;
    float f = 1.0f / tan(fov * 3.14159f / 360.0f);
    float near = 0.1f, far = 500.0f;
    
    proj[0] = f/aspect; proj[4] = 0; proj[8] = 0;                      proj[12] = 0;
    proj[1] = 0;        proj[5] = f; proj[9] = 0;                      proj[13] = 0;
    proj[2] = 0;        proj[6] = 0; proj[10] = (far+near)/(near-far); proj[14] = (2*far*near)/(near-far);
    proj[3] = 0;        proj[7] = 0; proj[11] = -1;                    proj[15] = 0;
    
    renderer->RenderGrid(mapEditor->GetSettings(), PCD::Vec3(editorCamTarget.x, editorCamTarget.y, editorCamTarget.z), view, proj);
    renderer->RenderBrushes(mapEditor->GetMap().brushes, mapEditor->GetSelectedBrushIndex(), view, proj);
    renderer->RenderEntities(mapEditor->GetMap().entities, mapEditor->GetSelectedEntityIndex(), 
                            mapEditor->GetSettings().showEntityIcons, view, proj);
    
    // Render gizmo for selected object
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
    float camX = editorCamTarget.x + editorCamDist * sin(editorCamYaw) * cos(editorCamPitch);
    float camY = editorCamTarget.y + editorCamDist * sin(editorCamPitch);
    float camZ = editorCamTarget.z + editorCamDist * cos(editorCamYaw) * cos(editorCamPitch);
    
    Vec3 eye(camX, camY, camZ);
    Vec3 target = editorCamTarget;
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
    std::cout << "Play mode active. Press ESC to return to editor.\n";
}

void EditorApp::ExitPlayMode() {
    std::cout << "Exiting play mode...\n";
    currentMode = EditorMode::EDIT;
    delete gameMode;
    gameMode = nullptr;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    firstMouse = true;
    std::cout << "Returned to editor.\n";
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
        
        // Check if UI wants keyboard
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard) return;
        
        if (mods & GLFW_MOD_CONTROL) {
            if (key == GLFW_KEY_N) app->mapEditor->NewMap();
            if (key == GLFW_KEY_S) app->mapEditor->SaveMap();
            if (key == GLFW_KEY_Z) app->mapEditor->Undo();
            if (key == GLFW_KEY_Y) app->mapEditor->Redo();
            if (key == GLFW_KEY_D) app->mapEditor->DuplicateSelected();
            if (key == GLFW_KEY_A) app->mapEditor->SelectAll();
        } else {
            // Tool hotkeys (number keys)
            if (key == GLFW_KEY_1) app->mapEditor->SetTool(PCD::EditorTool::SELECT);
            if (key == GLFW_KEY_2) app->mapEditor->SetTool(PCD::EditorTool::MOVE);
            if (key == GLFW_KEY_3) app->mapEditor->SetTool(PCD::EditorTool::ROTATE);
            if (key == GLFW_KEY_4) app->mapEditor->SetTool(PCD::EditorTool::SCALE);
            if (key == GLFW_KEY_5) app->mapEditor->SetTool(PCD::EditorTool::CREATE_BOX);
            
            if (key == GLFW_KEY_DELETE) app->mapEditor->DeleteSelected();
            if (key == GLFW_KEY_ESCAPE) app->mapEditor->DeselectAll();
            
            // B/C for quick brush creation
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
    
    // Check if UI wants mouse
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            
            float ndcX = (2.0f * mx / width) - 1.0f;
            float ndcY = 1.0f - (2.0f * my / height);
            
            auto& settings = app->mapEditor->GetSettings();
            float gridY = settings.gridHeight;
            float worldX = app->editorCamTarget.x + ndcX * app->editorCamDist * 0.5f;
            float worldZ = app->editorCamTarget.z + ndcY * app->editorCamDist * 0.5f;
            
            bool shift = mods & GLFW_MOD_SHIFT;
            app->mapEditor->OnMouseClick(worldX, gridY, worldZ, shift);
        } else if (action == GLFW_RELEASE) {
            app->mapEditor->OnMouseRelease();
        }
    }
}

void EditorApp::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    EditorApp* app = GetInstance(window);
    if (!app) return;
    
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    
    if (app->currentMode == EditorMode::EDIT) {
        app->editorCamDist -= yoffset * 2.0f;
        if (app->editorCamDist < 5.0f) app->editorCamDist = 5.0f;
        if (app->editorCamDist > 200.0f) app->editorCamDist = 200.0f;
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
        // Check if UI wants mouse
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) return;
        
        // Rotate with right mouse
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            app->editorCamYaw += dx * 0.005f;
            app->editorCamPitch += dy * 0.005f;
            if (app->editorCamPitch < 0.1f) app->editorCamPitch = 0.1f;
            if (app->editorCamPitch > 1.5f) app->editorCamPitch = 1.5f;
        }
        
        // Pan with middle mouse
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            float panSpeed = app->editorCamDist * 0.002f;
            app->editorCamTarget.x -= dx * panSpeed * cos(app->editorCamYaw);
            app->editorCamTarget.z -= dx * panSpeed * sin(app->editorCamYaw);
            app->editorCamTarget.y += dy * panSpeed;
        }
        
        // Drag for manipulation or creation
        if (app->mapEditor && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            
            float ndcX = (2.0f * xpos / width) - 1.0f;
            float ndcY = 1.0f - (2.0f * ypos / height);
            
            auto& settings = app->mapEditor->GetSettings();
            float gridY = settings.gridHeight;
            float worldX = app->editorCamTarget.x + ndcX * app->editorCamDist * 0.5f;
            float worldZ = app->editorCamTarget.z + ndcY * app->editorCamDist * 0.5f;
            
            bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
                         glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
            app->mapEditor->OnMouseDrag(worldX, gridY, worldZ, dx, dy, shift);
        }
    }
}

} // namespace Editor
