#ifndef PCD_EDITOR_UI_H
#define PCD_EDITOR_UI_H

#include "Engine/TextureLoader.h"
#include "PCDBrushFactory.h"
#include "PCDEditorState.h"
#include "PCDFile.h"
#include <algorithm>
#include <cstring>
#include <imgui.h>

namespace PCD {

class EditorUI {
private:
    EditorState& state;

    bool showEntityList = true;
    bool showBrushList = true;
    bool showProperties = true;
    bool showToolbar = true;
    bool showTextures = true;
    bool showLayers = false;
    bool showPrefabs = false;
    bool showHelp = false;
    bool showAbout = false;
    bool showMapSettings = false;
    bool showExportOptions = false;

    char mapNameBuffer[256] = {};
    char authorBuffer[256] = {};
    char entityNameBuffer[256] = {};
    char brushNameBuffer[256] = {};
    char texturePathBuffer[512] = {};
    char searchBuffer[128] = {};
    char exportPathBuffer[512] = {};
    
    // Map settings
    float skyColorR = 0.5f, skyColorG = 0.7f, skyColorB = 1.0f;
    float ambientR = 0.3f, ambientG = 0.3f, ambientB = 0.3f;
    float fogDensity = 0.0f;
    bool fogEnabled = false;

public:
    explicit EditorUI(EditorState& s) : state(s) {
        strncpy(mapNameBuffer, state.map.name.c_str(), sizeof(mapNameBuffer) - 1);
        strncpy(authorBuffer, state.map.author.c_str(), sizeof(authorBuffer) - 1);
    }

    void Render() {
        RenderMainMenuBar();
        if (showToolbar) RenderToolbar();
        if (showBrushList) RenderBrushList();
        if (showEntityList) RenderEntityList();
        if (showTextures) RenderTexturePanel();
        if (showProperties) RenderProperties();
        if (showLayers) RenderLayersPanel();
        if (showPrefabs) RenderPrefabsPanel();
        if (showHelp) RenderHelpWindow();
        if (showAbout) RenderAboutWindow();
        if (showMapSettings) RenderMapSettingsWindow();
        if (showExportOptions) RenderExportWindow();
        RenderStatusBar();
    }

private:
    void RenderMainMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New", "Ctrl+N")) state.NewMap();
                if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                    // TODO: File dialog
                }
                if (ImGui::MenuItem("Save", "Ctrl+S")) SaveMap();
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                    // TODO: File dialog
                }
                ImGui::Separator();
                
                if (ImGui::BeginMenu("Recent Files")) {
                    ImGui::MenuItem("(No recent files)", nullptr, false, false);
                    ImGui::EndMenu();
                }
                
                ImGui::Separator();
                if (ImGui::BeginMenu("Export")) {
                    if (ImGui::MenuItem("Export .pcd")) ExportPCD();
                    if (ImGui::MenuItem("Export .obj")) {
                        showExportOptions = true;
                    }
                    if (ImGui::MenuItem("Export .map (Quake)")) {
                        // TODO: Export to Quake MAP format
                    }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("Import")) {
                    if (ImGui::MenuItem("Import .obj")) {
                        // TODO: Import OBJ
                    }
                    ImGui::EndMenu();
                }
                
                ImGui::Separator();
                if (ImGui::MenuItem("Map Settings...")) {
                    showMapSettings = true;
                }
                
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, !state.undoStack.empty())) state.Undo();
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, !state.redoStack.empty())) state.Redo();
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                    // Cut handled externally
                }
                if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                    // Copy handled externally
                }
                if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                    // Paste handled externally
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Delete", "Del")) state.DeleteSelected();
                if (ImGui::MenuItem("Duplicate", "Ctrl+D")) state.DuplicateSelected();
                ImGui::Separator();
                if (ImGui::MenuItem("Select All", "Ctrl+A")) state.SelectAll();
                if (ImGui::MenuItem("Deselect", "Esc")) state.DeselectAll();
                if (ImGui::MenuItem("Invert Selection")) {
                    // TODO: Invert selection
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Toolbar", nullptr, &showToolbar);
                ImGui::MenuItem("Brush List", nullptr, &showBrushList);
                ImGui::MenuItem("Entity List", nullptr, &showEntityList);
                ImGui::MenuItem("Textures", nullptr, &showTextures);
                ImGui::MenuItem("Properties", nullptr, &showProperties);
                ImGui::MenuItem("Layers", nullptr, &showLayers);
                ImGui::MenuItem("Prefabs", nullptr, &showPrefabs);
                ImGui::Separator();
                ImGui::MenuItem("Show Grid", "H", &state.settings.showGrid);
                ImGui::MenuItem("Show Entity Icons", nullptr, &state.settings.showEntityIcons);
                ImGui::MenuItem("Show Brush Bounds", nullptr, &state.settings.showBrushBounds);
                ImGui::MenuItem("Show Normals", nullptr, &state.settings.showNormals);
                ImGui::Separator();
                
                if (ImGui::BeginMenu("Grid Size")) {
                    if (ImGui::MenuItem("0.25")) state.settings.gridSize = 0.25f;
                    if (ImGui::MenuItem("0.5")) state.settings.gridSize = 0.5f;
                    if (ImGui::MenuItem("1.0")) state.settings.gridSize = 1.0f;
                    if (ImGui::MenuItem("2.0")) state.settings.gridSize = 2.0f;
                    if (ImGui::MenuItem("4.0")) state.settings.gridSize = 4.0f;
                    if (ImGui::MenuItem("8.0")) state.settings.gridSize = 8.0f;
                    ImGui::EndMenu();
                }
                
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Create")) {
                if (ImGui::BeginMenu("Brushes")) {
                    if (ImGui::MenuItem("Box", "B")) state.currentTool = EditorTool::CREATE_BOX;
                    if (ImGui::MenuItem("Cylinder", "C")) state.currentTool = EditorTool::CREATE_CYLINDER;
                    if (ImGui::MenuItem("Wedge/Ramp")) state.currentTool = EditorTool::CREATE_WEDGE;
                    ImGui::Separator();
                    if (ImGui::MenuItem("Stairs...")) {
                        // TODO: Stairs dialog
                    }
                    if (ImGui::MenuItem("Arch...")) {
                        // TODO: Arch dialog
                    }
                    if (ImGui::MenuItem("Sphere...")) {
                        // TODO: Sphere dialog
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Spawn Points")) {
                    if (ImGui::MenuItem("Player Start")) PlaceEntity(ENT_INFO_PLAYER_START);
                    if (ImGui::MenuItem("Deathmatch")) PlaceEntity(ENT_INFO_PLAYER_DEATHMATCH);
                    if (ImGui::MenuItem("Team Red")) PlaceEntity(ENT_INFO_TEAM_SPAWN_RED);
                    if (ImGui::MenuItem("Team Blue")) PlaceEntity(ENT_INFO_TEAM_SPAWN_BLUE);
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Triggers")) {
                    if (ImGui::MenuItem("Once")) PlaceEntity(ENT_TRIGGER_ONCE);
                    if (ImGui::MenuItem("Multiple")) PlaceEntity(ENT_TRIGGER_MULTIPLE);
                    if (ImGui::MenuItem("Hurt")) PlaceEntity(ENT_TRIGGER_HURT);
                    if (ImGui::MenuItem("Push")) PlaceEntity(ENT_TRIGGER_PUSH);
                    if (ImGui::MenuItem("Teleport")) PlaceEntity(ENT_TRIGGER_TELEPORT);
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Lights")) {
                    if (ImGui::MenuItem("Point Light")) PlaceEntity(ENT_LIGHT);
                    if (ImGui::MenuItem("Spot Light")) PlaceEntity(ENT_LIGHT_SPOT);
                    if (ImGui::MenuItem("Environment Light")) PlaceEntity(ENT_LIGHT_ENV);
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Items")) {
                    if (ImGui::MenuItem("Health")) PlaceEntity(ENT_ITEM_HEALTH);
                    if (ImGui::MenuItem("Armor")) PlaceEntity(ENT_ITEM_ARMOR);
                    if (ImGui::MenuItem("Ammo")) PlaceEntity(ENT_ITEM_AMMO);
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Weapons")) {
                    if (ImGui::MenuItem("Shotgun")) PlaceEntity(ENT_WEAPON_SHOTGUN);
                    if (ImGui::MenuItem("Rocket Launcher")) PlaceEntity(ENT_WEAPON_ROCKET);
                    if (ImGui::MenuItem("Railgun")) PlaceEntity(ENT_WEAPON_RAILGUN);
                    if (ImGui::MenuItem("Plasma Gun")) PlaceEntity(ENT_WEAPON_PLASMA);
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Func")) {
                    if (ImGui::MenuItem("Door")) PlaceEntity(ENT_FUNC_DOOR);
                    if (ImGui::MenuItem("Button")) PlaceEntity(ENT_FUNC_BUTTON);
                    if (ImGui::MenuItem("Platform")) PlaceEntity(ENT_FUNC_PLATFORM);
                    if (ImGui::MenuItem("Rotating")) PlaceEntity(ENT_FUNC_ROTATING);
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("Ambient")) {
                    if (ImGui::MenuItem("Sound")) PlaceEntity(ENT_AMBIENT_SOUND);
                    if (ImGui::MenuItem("Particles")) PlaceEntity(ENT_ENV_PARTICLE);
                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::MenuItem("Select", "1")) state.currentTool = EditorTool::SELECT;
                if (ImGui::MenuItem("Move", "2")) state.currentTool = EditorTool::MOVE;
                if (ImGui::MenuItem("Rotate", "3")) state.currentTool = EditorTool::ROTATE;
                if (ImGui::MenuItem("Scale", "4")) state.currentTool = EditorTool::SCALE;
                ImGui::Separator();
                if (ImGui::MenuItem("Vertex Edit", "V")) state.currentTool = EditorTool::VERTEX_EDIT;
                ImGui::Separator();
                if (ImGui::MenuItem("Snap to Grid", "G", state.settings.snapToGrid)) {
                    state.settings.snapToGrid = !state.settings.snapToGrid;
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Keyboard Shortcuts")) {
                    showHelp = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("About")) {
                    showAbout = true;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void RenderToolbar() {
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(85, 450), ImGuiCond_FirstUseEver);

        ImGui::Begin("Tools", &showToolbar, ImGuiWindowFlags_NoResize);

        ImVec4 activeColor(0.2f, 0.5f, 0.8f, 1.0f);
        ImVec4 normalColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);

        auto ToolButton = [&](const char* label, EditorTool tool, const char* tooltip = nullptr) {
            if (state.currentTool == tool) {
                ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
            }
            if (ImGui::Button(label, ImVec2(65, 30))) {
                state.currentTool = tool;
            }
            if (state.currentTool == tool) {
                ImGui::PopStyleColor();
            }
            if (tooltip && ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", tooltip);
            }
        };

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Tools");
        ImGui::Separator();

        ToolButton("1-Select", EditorTool::SELECT, "Select objects (1)");
        ToolButton("2-Move", EditorTool::MOVE, "Move objects (2)");
        ToolButton("3-Rotate", EditorTool::ROTATE, "Rotate objects (3)");
        ToolButton("4-Scale", EditorTool::SCALE, "Scale objects (4)");

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Create");

        ToolButton("5-Box", EditorTool::CREATE_BOX, "Create box brush (5/B)");
        ToolButton("Cylinder", EditorTool::CREATE_CYLINDER, "Create cylinder (C)");
        ToolButton("Wedge", EditorTool::CREATE_WEDGE, "Create wedge/ramp");
        ToolButton("Entity", EditorTool::CREATE_ENTITY, "Place entity");

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Edit");

        ToolButton("Vertex", EditorTool::VERTEX_EDIT, "Edit vertices (V)");

        ImGui::Separator();
        
        // Quick actions
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Grid: %.2f", state.settings.gridSize);
        
        if (ImGui::Button(state.settings.snapToGrid ? "Snap ON" : "Snap OFF", ImVec2(65, 25))) {
            state.settings.snapToGrid = !state.settings.snapToGrid;
        }

        ImGui::End();
    }

    void RenderBrushList() {
        ImGui::SetNextWindowPos(ImVec2(10, 490), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(200, 180), ImGuiCond_FirstUseEver);

        ImGui::Begin("Brushes", &showBrushList);
        
        // Search filter
        ImGui::SetNextItemWidth(180);
        ImGui::InputText("##search_brush", searchBuffer, sizeof(searchBuffer));
        ImGui::SameLine();
        if (ImGui::Button("X##clear")) searchBuffer[0] = '\0';
        
        ImGui::Text("Count: %zu", state.map.brushes.size());
        ImGui::Separator();

        ImGui::BeginChild("BrushListScroll", ImVec2(0, 0), true);
        
        for (size_t i = 0; i < state.map.brushes.size(); i++) {
            const auto& brush = state.map.brushes[i];
            std::string label = brush.name.empty() ? "Brush #" + std::to_string(brush.id) : brush.name;
            
            // Filter
            if (searchBuffer[0] != '\0') {
                std::string lowerLabel = label;
                std::string lowerSearch = searchBuffer;
                std::transform(lowerLabel.begin(), lowerLabel.end(), lowerLabel.begin(), ::tolower);
                std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
                if (lowerLabel.find(lowerSearch) == std::string::npos) continue;
            }

            bool isSelected = (state.selectedBrushIndex == static_cast<int>(i));
            
            // Color indicator for brush type
            ImVec4 color(0.5f, 0.5f, 0.5f, 1.0f);
            if (brush.flags & BRUSH_TRIGGER) color = ImVec4(0.8f, 0.2f, 0.8f, 1.0f);
            else if (brush.flags & BRUSH_WATER) color = ImVec4(0.2f, 0.4f, 0.8f, 1.0f);
            else if (brush.flags & BRUSH_LAVA) color = ImVec4(0.9f, 0.3f, 0.1f, 1.0f);
            
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                state.selectedBrushIndex = static_cast<int>(i);
                state.selectedEntityIndex = -1;
            }
            ImGui::PopStyleColor();
            
            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Select")) {
                    state.selectedBrushIndex = static_cast<int>(i);
                    state.selectedEntityIndex = -1;
                }
                if (ImGui::MenuItem("Duplicate")) {
                    state.selectedBrushIndex = static_cast<int>(i);
                    state.DuplicateSelected();
                }
                if (ImGui::MenuItem("Delete")) {
                    state.selectedBrushIndex = static_cast<int>(i);
                    state.DeleteSelected();
                }
                ImGui::EndPopup();
            }
        }
        
        ImGui::EndChild();
        ImGui::End();
    }

    void RenderEntityList() {
        ImGui::SetNextWindowPos(ImVec2(10, 680), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);

        ImGui::Begin("Entities", &showEntityList);
        ImGui::Text("Count: %zu", state.map.entities.size());
        ImGui::Separator();

        ImGui::BeginChild("EntityListScroll", ImVec2(0, 0), true);
        
        for (size_t i = 0; i < state.map.entities.size(); i++) {
            const auto& ent = state.map.entities[i];
            std::string label = ent.name.empty() ? 
                std::string(GetEntityTypeName(ent.type)) + " #" + std::to_string(ent.id) : ent.name;

            bool isSelected = (state.selectedEntityIndex == static_cast<int>(i));
            
            // Color based on entity type
            ImVec4 color(0.5f, 0.5f, 0.5f, 1.0f);
            if (ent.type >= ENT_INFO_PLAYER_START && ent.type <= ENT_INFO_TEAM_SPAWN_BLUE) {
                color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
            } else if (ent.type >= ENT_LIGHT && ent.type <= ENT_LIGHT_ENV) {
                color = ImVec4(1.0f, 1.0f, 0.5f, 1.0f);
            } else if (ent.type >= ENT_TRIGGER_ONCE && ent.type <= ENT_TRIGGER_TELEPORT) {
                color = ImVec4(0.8f, 0.4f, 0.8f, 1.0f);
            } else if (ent.type >= ENT_ITEM_HEALTH && ent.type <= ENT_ITEM_AMMO) {
                color = ImVec4(0.3f, 0.8f, 1.0f, 1.0f);
            }
            
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                state.selectedEntityIndex = static_cast<int>(i);
                state.selectedBrushIndex = -1;
            }
            ImGui::PopStyleColor();
            
            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Select")) {
                    state.selectedEntityIndex = static_cast<int>(i);
                    state.selectedBrushIndex = -1;
                }
                if (ImGui::MenuItem("Duplicate")) {
                    state.selectedEntityIndex = static_cast<int>(i);
                    state.DuplicateSelected();
                }
                if (ImGui::MenuItem("Delete")) {
                    state.selectedEntityIndex = static_cast<int>(i);
                    state.DeleteSelected();
                }
                ImGui::EndPopup();
            }
        }
        
        ImGui::EndChild();
        ImGui::End();
    }

    void RenderTexturePanel() {
        ImGui::SetNextWindowPos(ImVec2(220, 490), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(250, 340), ImGuiCond_FirstUseEver);

        ImGui::Begin("Textures", &showTextures);

        ImGui::Text("Texture Library");
        ImGui::Text("Count: %zu", state.map.textures.size());
        ImGui::Separator();

        if (ImGui::Button("Load Texture...", ImVec2(230, 25))) {
            ImGui::OpenPopup("LoadTexture");
        }

        if (ImGui::BeginPopup("LoadTexture")) {
            ImGui::Text("Enter texture path:");
            ImGui::SetNextItemWidth(400);
            if (ImGui::InputText("##texpath", texturePathBuffer, sizeof(texturePathBuffer),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                LoadTexture(texturePathBuffer);
                texturePathBuffer[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::Separator();
            ImGui::Text("Supported formats: PNG, JPG, BMP, TGA");
            if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (ImGui::Button("Create Checkerboard", ImVec2(230, 22))) {
            Texture checker = TextureLoader::CreateCheckerboardTexture(64);
            state.map.AddTexture(checker);
            state.hasUnsavedChanges = true;
        }

        ImGui::Separator();

        ImGui::BeginChild("TextureList", ImVec2(0, 0), true);

        for (auto& [id, tex] : state.map.textures) {
            ImGui::PushID(id);

            if (tex.glTextureID == 0 && !tex.data.empty()) {
                tex.glTextureID = TextureLoader::CreateGLTexture(tex);
            }

            if (tex.glTextureID != 0) {
                ImGui::Image((void*)(intptr_t)tex.glTextureID, ImVec2(64, 64));
            } else {
                ImGui::Button("No Preview", ImVec2(64, 64));
            }

            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", tex.name.c_str());
            ImGui::Text("ID: %u", tex.id);
            ImGui::Text("Size: %ux%u", tex.width, tex.height);

            if (state.selectedBrushIndex >= 0 && 
                state.selectedBrushIndex < (int)state.map.brushes.size()) {
                if (ImGui::SmallButton("Apply")) {
                    state.map.brushes[state.selectedBrushIndex].textureID = tex.id;
                    state.hasUnsavedChanges = true;
                }
            }

            ImGui::EndGroup();
            ImGui::Separator();
            ImGui::PopID();
        }

        if (state.map.textures.empty()) {
            ImGui::TextDisabled("No textures loaded");
            ImGui::TextWrapped("Click 'Load Texture...' to add textures.");
        }

        ImGui::EndChild();
        ImGui::End();
    }
    
    void RenderLayersPanel() {
        ImGui::SetNextWindowPos(ImVec2(480, 490), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
        
        ImGui::Begin("Layers", &showLayers);
        ImGui::TextDisabled("Layer support coming soon");
        ImGui::Separator();
        
        // Placeholder layers
        static bool layer1Visible = true, layer2Visible = true;
        ImGui::Checkbox("##vis1", &layer1Visible);
        ImGui::SameLine();
        ImGui::Selectable("Default Layer");
        
        ImGui::Checkbox("##vis2", &layer2Visible);
        ImGui::SameLine();
        ImGui::Selectable("Detail");
        
        ImGui::Separator();
        if (ImGui::Button("New Layer", ImVec2(180, 0))) {
            // TODO
        }
        
        ImGui::End();
    }
    
    void RenderPrefabsPanel() {
        ImGui::SetNextWindowPos(ImVec2(480, 300), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(200, 180), ImGuiCond_FirstUseEver);
        
        ImGui::Begin("Prefabs", &showPrefabs);
        ImGui::TextDisabled("Prefab system coming soon");
        ImGui::Separator();
        
        if (state.selectedBrushIndex >= 0 || state.selectedEntityIndex >= 0) {
            if (ImGui::Button("Save as Prefab", ImVec2(180, 0))) {
                // TODO: Save selection as prefab
            }
        }
        
        ImGui::Text("Available Prefabs:");
        ImGui::BeginChild("PrefabList", ImVec2(0, 0), true);
        ImGui::TextDisabled("(No prefabs)");
        ImGui::EndChild();
        
        ImGui::End();
    }
    
    void RenderHelpWindow() {
        ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Keyboard Shortcuts", &showHelp)) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Camera Controls");
            ImGui::BulletText("WASD/QE - Move camera");
            ImGui::BulletText("Right-Click + Drag - Orbit camera");
            ImGui::BulletText("Middle-Click + Drag - Pan camera");
            ImGui::BulletText("Alt + Left-Click - Orbit around focus");
            ImGui::BulletText("Scroll Wheel - Zoom");
            ImGui::BulletText("F - Focus on selection");
            
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Tools");
            ImGui::BulletText("1 - Select tool");
            ImGui::BulletText("2 - Move tool");
            ImGui::BulletText("3 - Rotate tool");
            ImGui::BulletText("4 - Scale tool");
            ImGui::BulletText("5/B - Create box");
            ImGui::BulletText("C - Create cylinder");
            ImGui::BulletText("V - Vertex edit mode");
            ImGui::BulletText("Tab - Cycle gizmo mode");
            
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Editing");
            ImGui::BulletText("Ctrl+Z - Undo");
            ImGui::BulletText("Ctrl+Y - Redo");
            ImGui::BulletText("Ctrl+C - Copy");
            ImGui::BulletText("Ctrl+X - Cut");
            ImGui::BulletText("Ctrl+V - Paste");
            ImGui::BulletText("Ctrl+D - Duplicate");
            ImGui::BulletText("Ctrl+A - Select all");
            ImGui::BulletText("Delete - Delete selection");
            ImGui::BulletText("Escape - Deselect");
            ImGui::BulletText("Ctrl+G - Align to grid");
            ImGui::BulletText("Ctrl+H - Hollow brush");
            
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "View");
            ImGui::BulletText("G - Toggle grid snap");
            ImGui::BulletText("H - Toggle grid visibility");
            ImGui::BulletText("F5 - Enter play mode");
        }
        ImGui::End();
    }
    
    void RenderAboutWindow() {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("About", &showAbout, ImGuiWindowFlags_NoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Placid Arena Map Editor");
            ImGui::Text("Version 1.0.0");
            ImGui::Separator();
            ImGui::TextWrapped("A lightweight map editor for creating game levels with brush-based geometry and entity placement.");
            ImGui::Separator();
            ImGui::Text("PCD Format Version: 2");
            ImGui::Text("OpenGL 3.3 Core Profile");
        }
        ImGui::End();
    }
    
    void RenderMapSettingsWindow() {
        ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Map Settings", &showMapSettings)) {
            if (ImGui::CollapsingHeader("Map Info", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::InputText("Map Name", mapNameBuffer, sizeof(mapNameBuffer));
                ImGui::InputText("Author", authorBuffer, sizeof(authorBuffer));
                
                if (ImGui::Button("Apply")) {
                    state.map.name = mapNameBuffer;
                    state.map.author = authorBuffer;
                    state.hasUnsavedChanges = true;
                }
            }
            
            if (ImGui::CollapsingHeader("Environment")) {
                ImGui::ColorEdit3("Sky Color", &skyColorR);
                ImGui::ColorEdit3("Ambient Light", &ambientR);
                ImGui::Checkbox("Enable Fog", &fogEnabled);
                if (fogEnabled) {
                    ImGui::SliderFloat("Fog Density", &fogDensity, 0.0f, 1.0f);
                }
            }
            
            if (ImGui::CollapsingHeader("Gameplay")) {
                static int gameMode = 0;
                const char* gameModes[] = {"Deathmatch", "Team DM", "CTF", "Custom"};
                ImGui::Combo("Game Mode", &gameMode, gameModes, 4);
                
                static int maxPlayers = 16;
                ImGui::SliderInt("Max Players", &maxPlayers, 2, 32);
                
                static float respawnTime = 5.0f;
                ImGui::SliderFloat("Respawn Time", &respawnTime, 1.0f, 30.0f);
            }
        }
        ImGui::End();
    }
    
    void RenderExportWindow() {
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Export Options", &showExportOptions)) {
            ImGui::InputText("Output Path", exportPathBuffer, sizeof(exportPathBuffer));
            
            static bool exportTextures = true;
            static bool triangulate = true;
            static float scale = 1.0f;
            
            ImGui::Checkbox("Export Textures", &exportTextures);
            ImGui::Checkbox("Triangulate Faces", &triangulate);
            ImGui::SliderFloat("Scale", &scale, 0.1f, 10.0f);
            
            ImGui::Separator();
            
            if (ImGui::Button("Export", ImVec2(120, 0))) {
                // TODO: Perform export
                showExportOptions = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                showExportOptions = false;
            }
        }
        ImGui::End();
    }

    void RenderProperties() {
        ImGui::SetNextWindowPos(ImVec2(1060, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(210, 750), ImGuiCond_FirstUseEver);

        ImGui::Begin("Properties", &showProperties);

        if (ImGui::CollapsingHeader("Map", ImGuiTreeNodeFlags_DefaultOpen)) {
            strncpy(mapNameBuffer, state.map.name.c_str(), sizeof(mapNameBuffer) - 1);
            if (ImGui::InputText("Name", mapNameBuffer, sizeof(mapNameBuffer))) {
                state.map.name = mapNameBuffer;
                state.hasUnsavedChanges = true;
            }
            strncpy(authorBuffer, state.map.author.c_str(), sizeof(authorBuffer) - 1);
            if (ImGui::InputText("Author", authorBuffer, sizeof(authorBuffer))) {
                state.map.author = authorBuffer;
                state.hasUnsavedChanges = true;
            }
        }

        if (ImGui::CollapsingHeader("Grid", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Snap to Grid", &state.settings.snapToGrid);
            ImGui::DragFloat("Grid Size", &state.settings.gridSize, 0.25f, 0.25f, 16.0f);
            ImGui::DragFloat("Grid Height", &state.settings.gridHeight, 0.5f, -100.0f, 100.0f);

            const char* planes[] = {"XZ (Floor)", "XY (Front)", "YZ (Side)"};
            int plane = static_cast<int>(state.settings.currentPlane);
            if (ImGui::Combo("Plane", &plane, planes, 3)) {
                state.settings.currentPlane = static_cast<GridPlane>(plane);
            }
        }

        if (state.selectedBrushIndex >= 0 && 
            state.selectedBrushIndex < static_cast<int>(state.map.brushes.size())) {
            RenderBrushProperties();
        }

        if (state.selectedEntityIndex >= 0 && 
            state.selectedEntityIndex < static_cast<int>(state.map.entities.size())) {
            RenderEntityProperties();
        }

        ImGui::End();
    }

    void RenderBrushProperties() {
        if (ImGui::CollapsingHeader("Brush", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& brush = state.map.brushes[state.selectedBrushIndex];

            strncpy(brushNameBuffer, brush.name.c_str(), sizeof(brushNameBuffer) - 1);
            if (ImGui::InputText("Name##brush", brushNameBuffer, sizeof(brushNameBuffer))) {
                brush.name = brushNameBuffer;
                state.hasUnsavedChanges = true;
            }

            ImGui::Text("Vertices: %zu", brush.vertices.size());
            ImGui::Text("Triangles: %zu", brush.indices.size() / 3);
            
            if (ImGui::ColorEdit3("Color", &brush.color.x)) {
                state.hasUnsavedChanges = true;
            }

            ImGui::Separator();
            ImGui::Text("Flags:");

#define FLAG_CHECKBOX(name, flag) { \
    bool v = brush.flags & flag; \
    if (ImGui::Checkbox(name, &v)) { \
        if (v) brush.flags |= flag; else brush.flags &= ~flag; \
        state.hasUnsavedChanges = true; \
    } \
}
            FLAG_CHECKBOX("Solid", BRUSH_SOLID);
            FLAG_CHECKBOX("Detail", BRUSH_DETAIL);
            FLAG_CHECKBOX("Trigger", BRUSH_TRIGGER);
            FLAG_CHECKBOX("Water", BRUSH_WATER);
            FLAG_CHECKBOX("Lava", BRUSH_LAVA);
            FLAG_CHECKBOX("Ladder", BRUSH_LADDER);
            FLAG_CHECKBOX("Clip", BRUSH_CLIP);
            FLAG_CHECKBOX("No Collide", BRUSH_NOCOLLIDE);
#undef FLAG_CHECKBOX

            ImGui::Separator();
            ImGui::Text("Texture:");

            if (brush.textureID > 0) {
                auto* tex = state.map.GetTexture(brush.textureID);
                if (tex) {
                    ImGui::Text("  %s", tex->name.c_str());
                    
                    if (ImGui::Button("Remove Texture")) {
                        brush.textureID = 0;
                        state.hasUnsavedChanges = true;
                    }
                    
                    ImGui::Separator();
                    ImGui::Text("UV Settings:");
                    
                    if (ImGui::DragFloat("Scale X", &brush.uvScaleX, 0.1f, 0.1f, 20.0f)) {
                        state.hasUnsavedChanges = true;
                    }
                    if (ImGui::DragFloat("Scale Y", &brush.uvScaleY, 0.1f, 0.1f, 20.0f)) {
                        state.hasUnsavedChanges = true;
                    }
                    if (ImGui::DragFloat("Offset X", &brush.uvOffsetX, 0.05f, -10.0f, 10.0f)) {
                        state.hasUnsavedChanges = true;
                    }
                    if (ImGui::DragFloat("Offset Y", &brush.uvOffsetY, 0.05f, -10.0f, 10.0f)) {
                        state.hasUnsavedChanges = true;
                    }
                    
                    if (ImGui::Button("Reset UV", ImVec2(180, 0))) {
                        brush.uvScaleX = brush.uvScaleY = 1.0f;
                        brush.uvOffsetX = brush.uvOffsetY = 0.0f;
                        state.hasUnsavedChanges = true;
                    }
                }
            } else {
                ImGui::TextDisabled("  None");
                ImGui::TextWrapped("Use Textures panel to apply.");
            }
        }
    }

    void RenderEntityProperties() {
        if (ImGui::CollapsingHeader("Entity", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& ent = state.map.entities[state.selectedEntityIndex];

            strncpy(entityNameBuffer, ent.name.c_str(), sizeof(entityNameBuffer) - 1);
            if (ImGui::InputText("Name##ent", entityNameBuffer, sizeof(entityNameBuffer))) {
                ent.name = entityNameBuffer;
                state.hasUnsavedChanges = true;
            }

            ImGui::Text("Type: %s", GetEntityTypeName(ent.type));

            if (ImGui::DragFloat3("Position", &ent.position.x, 0.1f)) state.hasUnsavedChanges = true;
            if (ImGui::DragFloat3("Rotation", &ent.rotation.x, 1.0f, -180.0f, 180.0f)) state.hasUnsavedChanges = true;
            if (ImGui::DragFloat3("Scale", &ent.scale.x, 0.1f, 0.1f, 10.0f)) state.hasUnsavedChanges = true;

            ImGui::Separator();
            RenderEntityTypeProperties(ent);
        }
    }

    void RenderEntityTypeProperties(Entity& ent) {
        switch (ent.type) {
        case ENT_LIGHT:
        case ENT_LIGHT_SPOT:
        case ENT_LIGHT_ENV: {
            float color[3] = {std::stof(ent.GetProperty("color_r", "1")),
                              std::stof(ent.GetProperty("color_g", "1")),
                              std::stof(ent.GetProperty("color_b", "1"))};
            if (ImGui::ColorEdit3("Light Color", color)) {
                ent.SetProperty("color_r", std::to_string(color[0]));
                ent.SetProperty("color_g", std::to_string(color[1]));
                ent.SetProperty("color_b", std::to_string(color[2]));
                state.hasUnsavedChanges = true;
            }

            float intensity = std::stof(ent.GetProperty("intensity", "1"));
            if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f)) {
                ent.SetProperty("intensity", std::to_string(intensity));
                state.hasUnsavedChanges = true;
            }

            float radius = std::stof(ent.GetProperty("radius", "10"));
            if (ImGui::DragFloat("Radius", &radius, 0.5f, 0.0f, 500.0f)) {
                ent.SetProperty("radius", std::to_string(radius));
                state.hasUnsavedChanges = true;
            }
            break;
        }

        case ENT_TRIGGER_HURT: {
            float damage = std::stof(ent.GetProperty("damage", "10"));
            if (ImGui::DragFloat("Damage", &damage, 1.0f, 0.0f, 1000.0f)) {
                ent.SetProperty("damage", std::to_string(damage));
                state.hasUnsavedChanges = true;
            }
            break;
        }

        case ENT_TRIGGER_PUSH: {
            float force[3] = {std::stof(ent.GetProperty("force_x", "0")),
                              std::stof(ent.GetProperty("force_y", "10")),
                              std::stof(ent.GetProperty("force_z", "0"))};
            if (ImGui::DragFloat3("Push Force", force, 0.5f)) {
                ent.SetProperty("force_x", std::to_string(force[0]));
                ent.SetProperty("force_y", std::to_string(force[1]));
                ent.SetProperty("force_z", std::to_string(force[2]));
                state.hasUnsavedChanges = true;
            }
            break;
        }

        case ENT_FUNC_DOOR: {
            float moveDir[3] = {std::stof(ent.GetProperty("move_x", "0")),
                                std::stof(ent.GetProperty("move_y", "3")),
                                std::stof(ent.GetProperty("move_z", "0"))};
            if (ImGui::DragFloat3("Move Distance", moveDir, 0.1f)) {
                ent.SetProperty("move_x", std::to_string(moveDir[0]));
                ent.SetProperty("move_y", std::to_string(moveDir[1]));
                ent.SetProperty("move_z", std::to_string(moveDir[2]));
                state.hasUnsavedChanges = true;
            }

            float speed = std::stof(ent.GetProperty("speed", "2"));
            if (ImGui::DragFloat("Speed", &speed, 0.1f, 0.1f, 20.0f)) {
                ent.SetProperty("speed", std::to_string(speed));
                state.hasUnsavedChanges = true;
            }
            break;
        }

        case ENT_ITEM_HEALTH:
        case ENT_ITEM_ARMOR:
        case ENT_ITEM_AMMO: {
            int amount = std::stoi(ent.GetProperty("amount", "25"));
            if (ImGui::DragInt("Amount", &amount, 1, 1, 200)) {
                ent.SetProperty("amount", std::to_string(amount));
                state.hasUnsavedChanges = true;
            }

            float respawn = std::stof(ent.GetProperty("respawn_time", "30"));
            if (ImGui::DragFloat("Respawn Time", &respawn, 1.0f, 0.0f, 300.0f)) {
                ent.SetProperty("respawn_time", std::to_string(respawn));
                state.hasUnsavedChanges = true;
            }
            break;
        }

        default:
            break;
        }
    }

    void RenderStatusBar() {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 25));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 25));

        ImGui::Begin("StatusBar", nullptr, flags);

        const char* toolNames[] = {"Select", "Move", "Rotate", "Scale", "Box", "Cylinder", "Wedge", "Entity", "Vertex"};
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Tool:");
        ImGui::SameLine();
        ImGui::Text("%s", toolNames[static_cast<int>(state.currentTool)]);

        ImGui::SameLine(200);
        ImGui::Text("| Brushes: %zu | Entities: %zu | Textures: %zu",
                    state.map.brushes.size(), state.map.entities.size(), state.map.textures.size());

        ImGui::SameLine(500);
        ImGui::Text("| Grid: %.2f", state.settings.gridSize);

        ImGui::SameLine(600);
        ImGui::TextColored(state.settings.snapToGrid ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                           "| Snap: %s", state.settings.snapToGrid ? "ON" : "OFF");

        ImGui::SameLine(720);
        ImGui::TextColored(state.hasUnsavedChanges ? ImVec4(1.0f, 0.6f, 0.4f, 1.0f) : ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                           "| %s", state.hasUnsavedChanges ? "* UNSAVED *" : "Saved");

        ImGui::SameLine(viewport->Size.x - 350);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "F5: Play | Ctrl+S: Save | F1: Help");

        ImGui::End();
    }

    void PlaceEntity(EntityType type) {
        state.entityToPlace = type;
        state.currentTool = EditorTool::CREATE_ENTITY;
    }

    void SaveMap() {
        if (state.currentFilePath.empty()) {
            state.currentFilePath = "map.pcd";
        }
        ExportPCD();
    }

    void ExportPCD() {
        std::string path = state.currentFilePath.empty() ? "map.pcd" : state.currentFilePath;
        if (PCDWriter::Save(state.map, path)) {
            state.hasUnsavedChanges = false;
            state.currentFilePath = path;
        }
    }

    void LoadTexture(const std::string& path) {
        Texture tex;
        if (TextureLoader::LoadImage(path, tex)) {
            state.map.AddTexture(tex);
            state.hasUnsavedChanges = true;
        }
    }
};

} // namespace PCD

#endif // PCD_EDITOR_UI_H
