// EditorApp.cpp - FIXED: Arrow key controls for object manipulation

#include "Engine/EditorApp.h"
#include "Engine/GameMode.h"
#include "Engine/TextureLoader.h"
#include <cmath>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <fstream>
#include <sstream>

namespace Editor {

EditorApp::EditorApp()
    : window(nullptr), mapEditor(nullptr), renderer(nullptr), gameMode(nullptr),
      currentMode(EditorMode::EDIT), cameraMode(CameraMode::FREE),
      cameraPosition(0, 10, 20), cameraFocusPoint(0, 0, 0),
      cameraDistance(20.0f), cameraYaw(0.0f), cameraPitch(0.4f),
      cameraFOV(60.0f), cameraMoveSpeed(10.0f), cameraRotateSpeed(0.005f),
      cameraZoomSpeed(2.0f), shiftPressed(false), dragAccumX(0), dragAccumZ(0),
      isLeftDragging(false), isRightDragging(false), isMiddleDragging(false),
      isAltPressed(false), lastX(640), lastY(360), firstMouse(true),
      lastFrame(0), deltaTime(0) {}

EditorApp::~EditorApp() { Shutdown(); }

bool EditorApp::Initialize(int width, int height, const char *title) {
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
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.12f, 0.95f);
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    renderer = new Renderer();
    if (!renderer->Initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }

    mapEditor = new MapEditor();

    // Create default floor
    if (mapEditor->GetMap().brushes.empty()) {
        PCD::Brush floor = mapEditor->CreateBox(PCD::Vec3(-20, -1, -20), PCD::Vec3(20, 0, 20));
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

    TextureLoader::LoadMapTextures(mapEditor->GetMap());

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
    if (mapEditor) {
        TextureLoader::FreeMapTextures(mapEditor->GetMap());
    }

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

    shiftPressed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
                   (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    isAltPressed = (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) ||
                   (glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);

    ImGuiIO &io = ImGui::GetIO();
    
    // Arrow key object manipulation
    if (!io.WantCaptureKeyboard && (mapEditor->GetSelectedBrushIndex() >= 0 || 
                                     mapEditor->GetSelectedEntityIndex() >= 0)) {
        
        float moveSpeed = shiftPressed ? 1.0f : 0.1f; // Shift = faster movement
        float scaleSpeed = shiftPressed ? 0.1f : 0.01f; // Shift = faster scaling
        
        PCD::EditorTool currentTool = mapEditor->GetCurrentTool();
        
        // MOVE tool - Arrow keys for XZ, Q/E for Y
        if (currentTool == PCD::EditorTool::MOVE) {
            PCD::Vec3 delta(0, 0, 0);
            
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                delta.z -= moveSpeed * dt * 10.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                delta.z += moveSpeed * dt * 10.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                delta.x -= moveSpeed * dt * 10.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                delta.x += moveSpeed * dt * 10.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
                delta.y -= moveSpeed * dt * 10.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                delta.y += moveSpeed * dt * 10.0f;
            }
            
            if (delta.x != 0 || delta.y != 0 || delta.z != 0) {
                // Apply grid snapping if enabled
                if (mapEditor->GetSettings().snapToGrid) {
                    float gridSize = mapEditor->GetSettings().gridSize;
                    delta.x = std::round(delta.x / gridSize) * gridSize;
                    delta.y = std::round(delta.y / gridSize) * gridSize;
                    delta.z = std::round(delta.z / gridSize) * gridSize;
                }
                
                // Move selected object
                if (mapEditor->GetSelectedBrushIndex() >= 0) {
                    auto& brush = mapEditor->GetMap().brushes[mapEditor->GetSelectedBrushIndex()];
                    for (auto& v : brush.vertices) {
                        v.position = v.position + delta;
                    }
                    mapEditor->SetUnsavedChanges(true);
                }
                if (mapEditor->GetSelectedEntityIndex() >= 0) {
                    auto& ent = mapEditor->GetMap().entities[mapEditor->GetSelectedEntityIndex()];
                    ent.position = ent.position + delta;
                    mapEditor->SetUnsavedChanges(true);
                }
            }
        }
        
        // SCALE tool - Arrow keys for XZ scale, Q/E for Y scale
        else if (currentTool == PCD::EditorTool::SCALE) {
            PCD::Vec3 scaleDelta(1.0f, 1.0f, 1.0f);
            bool scaleChanged = false;
            
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                scaleDelta.z += scaleSpeed * dt * 10.0f;
                scaleChanged = true;
            }
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                scaleDelta.z -= scaleSpeed * dt * 10.0f;
                scaleChanged = true;
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                scaleDelta.x -= scaleSpeed * dt * 10.0f;
                scaleChanged = true;
            }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                scaleDelta.x += scaleSpeed * dt * 10.0f;
                scaleChanged = true;
            }
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
                scaleDelta.y -= scaleSpeed * dt * 10.0f;
                scaleChanged = true;
            }
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                scaleDelta.y += scaleSpeed * dt * 10.0f;
                scaleChanged = true;
            }
            
            if (scaleChanged) {
                // Scale entity
                if (mapEditor->GetSelectedEntityIndex() >= 0) {
                    auto& ent = mapEditor->GetMap().entities[mapEditor->GetSelectedEntityIndex()];
                    ent.scale.x = std::max(0.1f, ent.scale.x * scaleDelta.x);
                    ent.scale.y = std::max(0.1f, ent.scale.y * scaleDelta.y);
                    ent.scale.z = std::max(0.1f, ent.scale.z * scaleDelta.z);
                    mapEditor->SetUnsavedChanges(true);
                }
                
                // Scale brush from center
                if (mapEditor->GetSelectedBrushIndex() >= 0) {
                    auto& brush = mapEditor->GetMap().brushes[mapEditor->GetSelectedBrushIndex()];
                    
                    // Calculate brush center
                    PCD::Vec3 center(0, 0, 0);
                    for (auto& v : brush.vertices) {
                        center.x += v.position.x;
                        center.y += v.position.y;
                        center.z += v.position.z;
                    }
                    center.x /= brush.vertices.size();
                    center.y /= brush.vertices.size();
                    center.z /= brush.vertices.size();
                    
                    // Scale from center
                    for (auto& v : brush.vertices) {
                        PCD::Vec3 offset = v.position - center;
                        offset.x *= scaleDelta.x;
                        offset.y *= scaleDelta.y;
                        offset.z *= scaleDelta.z;
                        v.position = center + offset;
                    }
                    mapEditor->SetUnsavedChanges(true);
                }
            }
        }
        
        // ROTATE tool - Arrow keys for Y rotation, Q/E for Z rotation
        else if (currentTool == PCD::EditorTool::ROTATE) {
            float rotSpeed = shiftPressed ? 2.0f : 0.5f; // degrees per frame
            bool rotChanged = false;
            PCD::Vec3 rotation(0, 0, 0);
            
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                rotation.y += rotSpeed * dt * 60.0f;
                rotChanged = true;
            }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                rotation.y -= rotSpeed * dt * 60.0f;
                rotChanged = true;
            }
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
                rotation.z += rotSpeed * dt * 60.0f;
                rotChanged = true;
            }
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                rotation.z -= rotSpeed * dt * 60.0f;
                rotChanged = true;
            }
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                rotation.x += rotSpeed * dt * 60.0f;
                rotChanged = true;
            }
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                rotation.x -= rotSpeed * dt * 60.0f;
                rotChanged = true;
            }
            
            if (rotChanged) {
                // Rotate entity
                if (mapEditor->GetSelectedEntityIndex() >= 0) {
                    auto& ent = mapEditor->GetMap().entities[mapEditor->GetSelectedEntityIndex()];
                    ent.rotation.x += rotation.x;
                    ent.rotation.y += rotation.y;
                    ent.rotation.z += rotation.z;
                    mapEditor->SetUnsavedChanges(true);
                }
                
                // Rotate brush around center
                if (mapEditor->GetSelectedBrushIndex() >= 0) {
                    auto& brush = mapEditor->GetMap().brushes[mapEditor->GetSelectedBrushIndex()];
                    
                    // Calculate center
                    PCD::Vec3 center(0, 0, 0);
                    for (auto& v : brush.vertices) {
                        center.x += v.position.x;
                        center.y += v.position.y;
                        center.z += v.position.z;
                    }
                    center.x /= brush.vertices.size();
                    center.y /= brush.vertices.size();
                    center.z /= brush.vertices.size();
                    
                    // Rotate around Y axis (most common)
                    if (rotation.y != 0) {
                        float angle = rotation.y * 3.14159f / 180.0f;
                        float cosA = std::cos(angle);
                        float sinA = std::sin(angle);
                        
                        for (auto& v : brush.vertices) {
                            float relX = v.position.x - center.x;
                            float relZ = v.position.z - center.z;
                            
                            v.position.x = center.x + relX * cosA - relZ * sinA;
                            v.position.z = center.z + relX * sinA + relZ * cosA;
                            
                            float nrelX = v.normal.x;
                            float nrelZ = v.normal.z;
                            v.normal.x = nrelX * cosA - nrelZ * sinA;
                            v.normal.z = nrelX * sinA + nrelZ * cosA;
                        }
                    }
                    
                    mapEditor->SetUnsavedChanges(true);
                }
            }
        }
    }
    
    // Camera controls (WASD when no object selected or when Alt is held)
    if (!io.WantCaptureMouse && !io.WantCaptureKeyboard && cameraMode == CameraMode::FREE) {
        float speed = cameraMoveSpeed * (shiftPressed ? 2.5f : 1.0f);

        Vec3 forward = GetCameraForward();
        Vec3 right = GetCameraRight();

        forward.y = 0;
        forward = forward.normalized();
        right.y = 0;
        right = right.normalized();

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cameraFocusPoint = cameraFocusPoint + forward * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cameraFocusPoint = cameraFocusPoint - forward * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cameraFocusPoint = cameraFocusPoint - right * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cameraFocusPoint = cameraFocusPoint + right * speed * dt;
        }
    }
}

void EditorApp::UpdateCamera(float dt) {
    cameraPosition.x = cameraFocusPoint.x + cameraDistance * sin(cameraYaw) * cos(cameraPitch);
    cameraPosition.y = cameraFocusPoint.y + cameraDistance * sin(cameraPitch);
    cameraPosition.z = cameraFocusPoint.z + cameraDistance * cos(cameraYaw) * cos(cameraPitch);
}

void EditorApp::FocusOnSelection() {
    if (mapEditor->GetSelectedBrushIndex() >= 0 || mapEditor->GetSelectedEntityIndex() >= 0) {
        PCD::Vec3 pos = mapEditor->GetSelectedObjectPosition();
        cameraFocusPoint = Vec3(pos.x, pos.y, pos.z);
        cameraDistance = 15.0f;
    }
}

void EditorApp::OrbitCamera(float deltaX, float deltaY) {
    cameraYaw -= deltaX * cameraRotateSpeed;
    cameraPitch += deltaY * cameraRotateSpeed;

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
    return Vec3(sin(cameraYaw) * cos(cameraPitch), sin(cameraPitch),
                cos(cameraYaw) * cos(cameraPitch));
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
        float fov = 75.0f * 3.14159f / 180.0f;
        float f = 1.0f / tan(fov / 2.0f);
        float near = 0.1f, far = 1000.0f;

        proj[0] = f / aspect; proj[4] = 0; proj[8] = 0; proj[12] = 0;
        proj[1] = 0; proj[5] = f; proj[9] = 0; proj[13] = 0;
        proj[2] = 0; proj[6] = 0; proj[10] = (far + near) / (near - far); proj[14] = (2 * far * near) / (near - far);
        proj[3] = 0; proj[7] = 0; proj[11] = -1; proj[15] = 0;

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
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float view[16], proj[16];
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float aspect = (float)width / height;

    GetEditorViewMatrix(view);

    float f = 1.0f / tan(cameraFOV * 3.14159f / 360.0f);
    float near = 0.1f, far = 1000.0f;

    proj[0] = f / aspect; proj[4] = 0; proj[8] = 0; proj[12] = 0;
    proj[1] = 0; proj[5] = f; proj[9] = 0; proj[13] = 0;
    proj[2] = 0; proj[6] = 0; proj[10] = (far + near) / (near - far); proj[14] = (2 * far * near) / (near - far);
    proj[3] = 0; proj[7] = 0; proj[11] = -1; proj[15] = 0;

    renderer->RenderGrid(mapEditor->GetSettings(),
                         PCD::Vec3(cameraFocusPoint.x, cameraFocusPoint.y, cameraFocusPoint.z),
                         view, proj);
    renderer->RenderBrushes(mapEditor->GetMap().brushes,
                            mapEditor->GetSelectedBrushIndex(), view, proj);
    renderer->RenderEntities(mapEditor->GetMap().entities,
                             mapEditor->GetSelectedEntityIndex(),
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
    RenderStatsPanel();
    RenderToolsPanel();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorApp::RenderStatsPanel() {
    ImGui::SetNextWindowPos(ImVec2(10, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(180, 150), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Map Statistics")) {
        const auto& stats = mapEditor->GetStats();
        ImGui::Text("Brushes: %d", stats.totalBrushes);
        ImGui::Text("Entities: %d", stats.totalEntities);
        ImGui::Text("Vertices: %d", stats.totalVertices);
        ImGui::Text("Triangles: %d", stats.totalTriangles);
        ImGui::Text("Textures: %d", stats.totalTextures);
        ImGui::Separator();
        ImGui::Text("Bounds:");
        ImGui::Text("  Min: %.1f, %.1f, %.1f", 
                    stats.mapBoundsMin.x, stats.mapBoundsMin.y, stats.mapBoundsMin.z);
        ImGui::Text("  Max: %.1f, %.1f, %.1f", 
                    stats.mapBoundsMax.x, stats.mapBoundsMax.y, stats.mapBoundsMax.z);
    }
    ImGui::End();
}

void EditorApp::RenderToolsPanel() {
    ImGuiIO& io = ImGui::GetIO();
    
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 220, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(210, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Advanced Tools")) {
        if (ImGui::CollapsingHeader("Keyboard Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Arrow Keys:");
            ImGui::Text("  Up/Down - Z axis");
            ImGui::Text("  Left/Right - X axis");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Q/E Keys:");
            ImGui::Text("  Q - Down (Y-)");
            ImGui::Text("  E - Up (Y+)");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Hold Shift:");
            ImGui::Text("  10x faster");
        }
        
        if (ImGui::CollapsingHeader("Brush Operations")) {
            if (ImGui::Button("Hollow Brush", ImVec2(190, 0))) {
                mapEditor->HollowBrush(0.25f);
            }
            
            ImGui::Separator();
            ImGui::Text("Flip:");
            ImGui::SameLine();
            if (ImGui::Button("X##flip")) mapEditor->FlipBrushX();
            ImGui::SameLine();
            if (ImGui::Button("Y##flip")) mapEditor->FlipBrushY();
            ImGui::SameLine();
            if (ImGui::Button("Z##flip")) mapEditor->FlipBrushZ();
            
            ImGui::Text("Rotate 90:");
            ImGui::SameLine();
            if (ImGui::Button("X##rot")) mapEditor->RotateBrush90(0);
            ImGui::SameLine();
            if (ImGui::Button("Y##rot")) mapEditor->RotateBrush90(1);
            ImGui::SameLine();
            if (ImGui::Button("Z##rot")) mapEditor->RotateBrush90(2);
        }
        
        if (ImGui::CollapsingHeader("Alignment")) {
            if (ImGui::Button("Align to Grid", ImVec2(190, 0))) {
                mapEditor->AlignToGrid();
            }
            
            ImGui::Text("Align Selection:");
            if (ImGui::Button("Align X", ImVec2(60, 0))) mapEditor->AlignSelectedToX();
            ImGui::SameLine();
            if (ImGui::Button("Align Y", ImVec2(60, 0))) mapEditor->AlignSelectedToY();
            ImGui::SameLine();
            if (ImGui::Button("Align Z", ImVec2(60, 0))) mapEditor->AlignSelectedToZ();
        }
    }
    ImGui::End();
}

void EditorApp::GetEditorViewMatrix(float *mat) {
    Vec3 eye = cameraPosition;
    Vec3 target = cameraFocusPoint;
    Vec3 up(0, 1, 0);

    Vec3 f = (target - eye).normalized();
    Vec3 r = f.cross(up).normalized();
    Vec3 u = r.cross(f);

    mat[0] = r.x;  mat[4] = r.y;  mat[8] = r.z;   mat[12] = -r.x * eye.x - r.y * eye.y - r.z * eye.z;
    mat[1] = u.x;  mat[5] = u.y;  mat[9] = u.z;   mat[13] = -u.x * eye.x - u.y * eye.y - u.z * eye.z;
    mat[2] = -f.x; mat[6] = -f.y; mat[10] = -f.z; mat[14] = f.x * eye.x + f.y * eye.y + f.z * eye.z;
    mat[3] = 0;    mat[7] = 0;    mat[11] = 0;    mat[15] = 1;
}

void EditorApp::EnterPlayMode() {
    std::cout << "Entering play mode...\n";
    TextureLoader::LoadMapTextures(mapEditor->GetMap());

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

EditorApp *EditorApp::GetInstance(GLFWwindow *window) {
    return static_cast<EditorApp *>(glfwGetWindowUserPointer(window));
}

void EditorApp::KeyCallback(GLFWwindow *window, int key, int scancode,
                            int action, int mods) {
    EditorApp *app = GetInstance(window);
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

        if (app->currentMode == EditorMode::EDIT && app->mapEditor) {
            ImGuiIO &io = ImGui::GetIO();
            if (io.WantCaptureKeyboard) return;

            if (key == GLFW_KEY_F) {
                app->FocusOnSelection();
            }
            
            if (key == GLFW_KEY_TAB && !(mods & GLFW_MOD_CONTROL)) {
                app->mapEditor->CycleGizmoMode();
            }

            if (mods & GLFW_MOD_CONTROL) {
                switch (key) {
                    case GLFW_KEY_N: app->mapEditor->NewMap(); break;
                    case GLFW_KEY_S: app->mapEditor->SaveMap(); break;
                    case GLFW_KEY_Z: app->mapEditor->Undo(); break;
                    case GLFW_KEY_Y: app->mapEditor->Redo(); break;
                    case GLFW_KEY_D: app->mapEditor->DuplicateSelected(); break;
                    case GLFW_KEY_A: app->mapEditor->SelectAll(); break;
                    case GLFW_KEY_C: app->mapEditor->Copy(); break;
                    case GLFW_KEY_X: app->mapEditor->Cut(); break;
                    case GLFW_KEY_V: app->mapEditor->Paste(); break;
                    case GLFW_KEY_G: app->mapEditor->AlignToGrid(); break;
                    case GLFW_KEY_H: app->mapEditor->HollowBrush(); break;
                }
            } else {
                switch (key) {
                    case GLFW_KEY_1: app->mapEditor->SetTool(PCD::EditorTool::SELECT); break;
                    case GLFW_KEY_2: app->mapEditor->SetTool(PCD::EditorTool::MOVE); break;
                    case GLFW_KEY_3: app->mapEditor->SetTool(PCD::EditorTool::ROTATE); break;
                    case GLFW_KEY_4: app->mapEditor->SetTool(PCD::EditorTool::SCALE); break;
                    case GLFW_KEY_5: app->mapEditor->SetTool(PCD::EditorTool::CREATE_BOX); break;
                    case GLFW_KEY_B: app->mapEditor->SetTool(PCD::EditorTool::CREATE_BOX); break;
                    case GLFW_KEY_C: app->mapEditor->SetTool(PCD::EditorTool::CREATE_CYLINDER); break;
                    case GLFW_KEY_DELETE: app->mapEditor->DeleteSelected(); break;
                    case GLFW_KEY_ESCAPE: app->mapEditor->DeselectAll(); break;
                    case GLFW_KEY_G:
                        app->mapEditor->GetSettings().snapToGrid = !app->mapEditor->GetSettings().snapToGrid;
                        break;
                    case GLFW_KEY_H:
                        app->mapEditor->GetSettings().showGrid = !app->mapEditor->GetSettings().showGrid;
                        break;
                }
            }
        }
    }
}

void EditorApp::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    EditorApp *app = GetInstance(window);
    if (!app) return;

    if (app->currentMode == EditorMode::PLAY) return;
    if (!app->mapEditor) return;

    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (app->isAltPressed) {
                app->cameraMode = CameraMode::ORBIT;
                app->isLeftDragging = true;
            } else {
                app->isLeftDragging = true;

                int width, height;
                glfwGetFramebufferSize(window, &width, &height);

                double mx, my;
                glfwGetCursorPos(window, &mx, &my);

                float view[16], proj[16];
                app->GetEditorViewMatrix(view);

                float f = 1.0f / tan(app->cameraFOV * 3.14159f / 360.0f);
                float aspect = (float)width / height;
                float near = 0.1f, far = 1000.0f;

                proj[0] = f / aspect; proj[4] = 0; proj[8] = 0; proj[12] = 0;
                proj[1] = 0; proj[5] = f; proj[9] = 0; proj[13] = 0;
                proj[2] = 0; proj[6] = 0; proj[10] = (far + near) / (near - far); proj[14] = (2 * far * near) / (near - far);
                proj[3] = 0; proj[7] = 0; proj[11] = -1; proj[15] = 0;

                bool shift = mods & GLFW_MOD_SHIFT;
                app->mapEditor->OnMouseClickWithRay(mx, my, width, height, view, proj, shift);
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

void EditorApp::ScrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    EditorApp *app = GetInstance(window);
    if (!app) return;

    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (app->currentMode == EditorMode::EDIT) {
        app->ZoomCamera(yoffset);
    }
}

void EditorApp::MouseCallback(GLFWwindow *window, double xpos, double ypos) {
    EditorApp *app = GetInstance(window);
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
        app->gameMode->GetController().ProcessMouseInput(dx, dy);
        return;
    }

    if (app->currentMode == EditorMode::EDIT) {
        ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureMouse) return;

        if (app->isRightDragging) {
            app->OrbitCamera(dx, dy);
        }

        if (app->isMiddleDragging || app->cameraMode == CameraMode::PAN) {
            app->PanCamera(dx, dy);
        }

        if (app->cameraMode == CameraMode::ORBIT && app->isLeftDragging) {
            app->OrbitCamera(dx, dy);
        }
    }
}

} // namespace Editor
