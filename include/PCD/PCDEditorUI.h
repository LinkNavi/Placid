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
  EditorState &state;

  bool showEntityList = true;
  bool showBrushList = true;
  bool showProperties = true;
  bool showToolbar = true;
  bool showTextures = true;

  char mapNameBuffer[256] = {};
  char authorBuffer[256] = {};
  char entityNameBuffer[256] = {};
  char brushNameBuffer[256] = {};
  char texturePathBuffer[512] = {};

public:
  explicit EditorUI(EditorState &s) : state(s) {
    strncpy(mapNameBuffer, state.map.name.c_str(), sizeof(mapNameBuffer) - 1);
    strncpy(authorBuffer, state.map.author.c_str(), sizeof(authorBuffer) - 1);
  }

  void Render() {
    RenderMainMenuBar();
    if (showToolbar)
      RenderToolbar();
    if (showBrushList)
      RenderBrushList();
    if (showEntityList)
      RenderEntityList();
    if (showTextures)
      RenderTexturePanel();
    if (showProperties)
      RenderProperties();
    RenderStatusBar();
  }

private:
  void RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New", "Ctrl+N"))
          state.NewMap();
        if (ImGui::MenuItem("Open...", "Ctrl+O")) {
        }
        if (ImGui::MenuItem("Save", "Ctrl+S"))
          SaveMap();
        if (ImGui::MenuItem("Save As...")) {
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Export .pcd"))
          ExportPCD();
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, !state.undoStack.empty()))
          state.Undo();
        if (ImGui::MenuItem("Redo", "Ctrl+Y", false, !state.redoStack.empty()))
          state.Redo();
        ImGui::Separator();
        if (ImGui::MenuItem("Delete", "Del"))
          state.DeleteSelected();
        if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
          state.DuplicateSelected();
        ImGui::Separator();
        if (ImGui::MenuItem("Select All", "Ctrl+A"))
          state.SelectAll();
        if (ImGui::MenuItem("Deselect", "Esc"))
          state.DeselectAll();
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Toolbar", nullptr, &showToolbar);
        ImGui::MenuItem("Brush List", nullptr, &showBrushList);
        ImGui::MenuItem("Entity List", nullptr, &showEntityList);
        ImGui::MenuItem("Textures", nullptr, &showTextures);
        ImGui::MenuItem("Properties", nullptr, &showProperties);
        ImGui::Separator();
        ImGui::MenuItem("Show Grid", nullptr, &state.settings.showGrid);
        ImGui::MenuItem("Show Entity Icons", nullptr,
                        &state.settings.showEntityIcons);
        ImGui::MenuItem("Show Brush Bounds", nullptr,
                        &state.settings.showBrushBounds);
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Create")) {
        if (ImGui::BeginMenu("Brushes")) {
          if (ImGui::MenuItem("Box"))
            state.currentTool = EditorTool::CREATE_BOX;
          if (ImGui::MenuItem("Cylinder"))
            state.currentTool = EditorTool::CREATE_CYLINDER;
          if (ImGui::MenuItem("Wedge/Ramp"))
            state.currentTool = EditorTool::CREATE_WEDGE;
          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Spawn Points")) {
          if (ImGui::MenuItem("Player Start"))
            PlaceEntity(ENT_INFO_PLAYER_START);
          if (ImGui::MenuItem("Deathmatch"))
            PlaceEntity(ENT_INFO_PLAYER_DEATHMATCH);
          if (ImGui::MenuItem("Team Red"))
            PlaceEntity(ENT_INFO_TEAM_SPAWN_RED);
          if (ImGui::MenuItem("Team Blue"))
            PlaceEntity(ENT_INFO_TEAM_SPAWN_BLUE);
          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Triggers")) {
          if (ImGui::MenuItem("Once"))
            PlaceEntity(ENT_TRIGGER_ONCE);
          if (ImGui::MenuItem("Multiple"))
            PlaceEntity(ENT_TRIGGER_MULTIPLE);
          if (ImGui::MenuItem("Hurt"))
            PlaceEntity(ENT_TRIGGER_HURT);
          if (ImGui::MenuItem("Push"))
            PlaceEntity(ENT_TRIGGER_PUSH);
          if (ImGui::MenuItem("Teleport"))
            PlaceEntity(ENT_TRIGGER_TELEPORT);
          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Lights")) {
          if (ImGui::MenuItem("Point"))
            PlaceEntity(ENT_LIGHT);
          if (ImGui::MenuItem("Spot"))
            PlaceEntity(ENT_LIGHT_SPOT);
          if (ImGui::MenuItem("Environment"))
            PlaceEntity(ENT_LIGHT_ENV);
          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Items")) {
          if (ImGui::MenuItem("Health"))
            PlaceEntity(ENT_ITEM_HEALTH);
          if (ImGui::MenuItem("Armor"))
            PlaceEntity(ENT_ITEM_ARMOR);
          if (ImGui::MenuItem("Ammo"))
            PlaceEntity(ENT_ITEM_AMMO);
          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Weapons")) {
          if (ImGui::MenuItem("Shotgun"))
            PlaceEntity(ENT_WEAPON_SHOTGUN);
          if (ImGui::MenuItem("Rocket"))
            PlaceEntity(ENT_WEAPON_ROCKET);
          if (ImGui::MenuItem("Railgun"))
            PlaceEntity(ENT_WEAPON_RAILGUN);
          if (ImGui::MenuItem("Plasma"))
            PlaceEntity(ENT_WEAPON_PLASMA);
          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Func")) {
          if (ImGui::MenuItem("Door"))
            PlaceEntity(ENT_FUNC_DOOR);
          if (ImGui::MenuItem("Button"))
            PlaceEntity(ENT_FUNC_BUTTON);
          if (ImGui::MenuItem("Platform"))
            PlaceEntity(ENT_FUNC_PLATFORM);
          if (ImGui::MenuItem("Rotating"))
            PlaceEntity(ENT_FUNC_ROTATING);
          ImGui::EndMenu();
        }

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Tools")) {
        if (ImGui::MenuItem("Select", "Q"))
          state.currentTool = EditorTool::SELECT;
        if (ImGui::MenuItem("Move", "W"))
          state.currentTool = EditorTool::MOVE;
        if (ImGui::MenuItem("Rotate", "E"))
          state.currentTool = EditorTool::ROTATE;
        if (ImGui::MenuItem("Scale", "R"))
          state.currentTool = EditorTool::SCALE;
        ImGui::Separator();
        if (ImGui::MenuItem("Vertex Edit", "V"))
          state.currentTool = EditorTool::VERTEX_EDIT;
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }
  }

  void RenderToolbar() {
    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(80, 380), ImGuiCond_FirstUseEver);

    ImGui::Begin("Tools", &showToolbar, ImGuiWindowFlags_NoResize);

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Hotkeys:");
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "1-5");
    ImGui::Separator();

    ImVec4 activeColor(0.2f, 0.5f, 0.8f, 1.0f);

    if (state.currentTool == PCD::EditorTool::SELECT) {
      ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
    }
    if (ImGui::Button("1-Select", ImVec2(60, 35)))
      state.currentTool = PCD::EditorTool::SELECT;
    if (state.currentTool == PCD::EditorTool::SELECT) {
      ImGui::PopStyleColor();
    }

    if (state.currentTool == PCD::EditorTool::MOVE) {
      ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
    }
    if (ImGui::Button("2-Move", ImVec2(60, 35)))
      state.currentTool = PCD::EditorTool::MOVE;
    if (state.currentTool == PCD::EditorTool::MOVE) {
      ImGui::PopStyleColor();
    }

    if (state.currentTool == PCD::EditorTool::ROTATE) {
      ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
    }
    if (ImGui::Button("3-Rotate", ImVec2(60, 35)))
      state.currentTool = PCD::EditorTool::ROTATE;
    if (state.currentTool == PCD::EditorTool::ROTATE) {
      ImGui::PopStyleColor();
    }

    if (state.currentTool == PCD::EditorTool::SCALE) {
      ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
    }
    if (ImGui::Button("4-Scale", ImVec2(60, 35)))
      state.currentTool = PCD::EditorTool::SCALE;
    if (state.currentTool == PCD::EditorTool::SCALE) {
      ImGui::PopStyleColor();
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Create:");

    if (state.currentTool == PCD::EditorTool::CREATE_BOX) {
      ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
    }
    if (ImGui::Button("5-Box (B)", ImVec2(60, 35)))
      state.currentTool = PCD::EditorTool::CREATE_BOX;
    if (state.currentTool == PCD::EditorTool::CREATE_BOX) {
      ImGui::PopStyleColor();
    }

    if (state.currentTool == PCD::EditorTool::CREATE_CYLINDER) {
      ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
    }
    if (ImGui::Button("Cyl (C)", ImVec2(60, 35)))
      state.currentTool = PCD::EditorTool::CREATE_CYLINDER;
    if (state.currentTool == PCD::EditorTool::CREATE_CYLINDER) {
      ImGui::PopStyleColor();
    }

    if (state.currentTool == PCD::EditorTool::CREATE_WEDGE) {
      ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
    }
    if (ImGui::Button("Wedge", ImVec2(60, 35)))
      state.currentTool = PCD::EditorTool::CREATE_WEDGE;
    if (state.currentTool == PCD::EditorTool::CREATE_WEDGE) {
      ImGui::PopStyleColor();
    }

    ImGui::Separator();

    if (state.currentTool == PCD::EditorTool::CREATE_ENTITY) {
      ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
    }
    if (ImGui::Button("Entity", ImVec2(60, 35)))
      state.currentTool = PCD::EditorTool::CREATE_ENTITY;
    if (state.currentTool == PCD::EditorTool::CREATE_ENTITY) {
      ImGui::PopStyleColor();
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Hold Shift:");
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Constrain");

    ImGui::End();
  }

  void RenderBrushList() {
    ImGui::SetNextWindowPos(ImVec2(10, 420), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);

    ImGui::Begin("Brushes", &showBrushList);
    ImGui::Text("Count: %zu", state.map.brushes.size());
    ImGui::Separator();

    for (size_t i = 0; i < state.map.brushes.size(); i++) {
      const auto &brush = state.map.brushes[i];
      std::string label = brush.name.empty()
                              ? "Brush #" + std::to_string(brush.id)
                              : brush.name;

      bool isSelected = (state.selectedBrushIndex == static_cast<int>(i));
      if (ImGui::Selectable(label.c_str(), isSelected)) {
        state.selectedBrushIndex = static_cast<int>(i);
        state.selectedEntityIndex = -1;
      }
    }
    ImGui::End();
  }

  void RenderEntityList() {
    ImGui::SetNextWindowPos(ImVec2(10, 630), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);

    ImGui::Begin("Entities", &showEntityList);
    ImGui::Text("Count: %zu", state.map.entities.size());
    ImGui::Separator();

    for (size_t i = 0; i < state.map.entities.size(); i++) {
      const auto &ent = state.map.entities[i];
      std::string label = ent.name.empty()
                              ? std::string(GetEntityTypeName(ent.type)) +
                                    " #" + std::to_string(ent.id)
                              : ent.name;

      bool isSelected = (state.selectedEntityIndex == static_cast<int>(i));
      if (ImGui::Selectable(label.c_str(), isSelected)) {
        state.selectedEntityIndex = static_cast<int>(i);
        state.selectedBrushIndex = -1;
      }
    }
    ImGui::End();
  }

  void RenderTexturePanel() {
    ImGui::SetNextWindowPos(ImVec2(220, 420), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 360), ImGuiCond_FirstUseEver);

    ImGui::Begin("Textures", &showTextures);

    ImGui::Text("Texture Library");
    ImGui::Text("Count: %zu", state.map.textures.size());
    ImGui::Separator();

    // Load texture button
    if (ImGui::Button("Load Texture...", ImVec2(230, 30))) {
      ImGui::OpenPopup("LoadTexture");
    }

    // Load texture popup
    if (ImGui::BeginPopup("LoadTexture")) {
      ImGui::Text("Enter texture path:");
      ImGui::SetNextItemWidth(400);
      if (ImGui::InputText("##texpath", texturePathBuffer,
                           sizeof(texturePathBuffer),
                           ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadTexture(texturePathBuffer);
        texturePathBuffer[0] = '\0';
        ImGui::CloseCurrentPopup();
      }

      ImGui::Separator();
      ImGui::Text("Supported formats: PNG, JPG, BMP, TGA");
      ImGui::Text("Example: textures/wall.png");

      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    ImGui::Spacing();

    // Create checkerboard button
    if (ImGui::Button("Create Checkerboard", ImVec2(230, 25))) {
      Texture checker = TextureLoader::CreateCheckerboardTexture(64);
      state.map.AddTexture(checker);
      state.hasUnsavedChanges = true;
    }

    ImGui::Separator();

    // Texture list
    ImGui::BeginChild("TextureList", ImVec2(0, 0), true);

    for (auto &[id, tex] : state.map.textures) {
      ImGui::PushID(id);

      // Load OpenGL texture if not loaded
      if (tex.glTextureID == 0 && !tex.data.empty()) {
        tex.glTextureID = TextureLoader::CreateGLTexture(tex);
      }

      // Show texture preview
      if (tex.glTextureID != 0) {
        ImGui::Image((void *)(intptr_t)tex.glTextureID, ImVec2(64, 64));
      } else {
        ImGui::Button("No Preview", ImVec2(64, 64));
      }

      ImGui::SameLine();
      ImGui::BeginGroup();

      ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s",
                         tex.name.c_str());
      ImGui::Text("ID: %u", tex.id);
      ImGui::Text("Size: %ux%u", tex.width, tex.height);

      // Apply to selected brush button
      if (state.selectedBrushIndex >= 0 &&
          state.selectedBrushIndex < (int)state.map.brushes.size()) {
        if (ImGui::SmallButton("Apply to Brush")) {
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
      ImGui::TextWrapped(
          "Click 'Load Texture...' to add textures to your map.");
    }

    ImGui::EndChild();
    ImGui::End();
  }

  void RenderProperties() {
    ImGui::SetNextWindowPos(ImVec2(1060, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(210, 750), ImGuiCond_FirstUseEver);

    ImGui::Begin("Properties", &showProperties);

    // Map properties
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

    // Grid settings
    if (ImGui::CollapsingHeader("Grid", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Checkbox("Snap to Grid", &state.settings.snapToGrid);
      ImGui::DragFloat("Grid Size", &state.settings.gridSize, 0.25f, 0.25f,
                       16.0f);
      ImGui::DragFloat("Grid Height", &state.settings.gridHeight, 0.5f, -100.0f,
                       100.0f);

      const char *planes[] = {"XZ (Floor)", "XY (Front)", "YZ (Side)"};
      int plane = static_cast<int>(state.settings.currentPlane);
      if (ImGui::Combo("Plane", &plane, planes, 3)) {
        state.settings.currentPlane = static_cast<GridPlane>(plane);
      }
    }

    // Brush properties
    if (state.selectedBrushIndex >= 0 &&
        state.selectedBrushIndex < static_cast<int>(state.map.brushes.size())) {
      RenderBrushProperties();
    }

    // Entity properties
    if (state.selectedEntityIndex >= 0 &&
        state.selectedEntityIndex <
            static_cast<int>(state.map.entities.size())) {
      RenderEntityProperties();
    }

    ImGui::End();
  }

  void RenderBrushProperties() {
    if (ImGui::CollapsingHeader("Brush", ImGuiTreeNodeFlags_DefaultOpen)) {
      auto &brush = state.map.brushes[state.selectedBrushIndex];

      strncpy(brushNameBuffer, brush.name.c_str(), sizeof(brushNameBuffer) - 1);
      if (ImGui::InputText("Name##brush", brushNameBuffer,
                           sizeof(brushNameBuffer))) {
        brush.name = brushNameBuffer;
        state.hasUnsavedChanges = true;
      }

      ImGui::Text("Vertices: %zu", brush.vertices.size());
      ImGui::Text("Triangles: %zu", brush.indices.size() / 3);
      ImGui::ColorEdit3("Color", &brush.color.x);

      ImGui::Text("Flags:");

#define FLAG_CHECKBOX(name, flag)                                              \
  {                                                                            \
    bool v = brush.flags & flag;                                               \
    if (ImGui::Checkbox(name, &v)) {                                           \
      if (v)                                                                   \
        brush.flags |= flag;                                                   \
      else                                                                     \
        brush.flags &= ~flag;                                                  \
      state.hasUnsavedChanges = true;                                          \
    }                                                                          \
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

      // Texture settings
      ImGui::Separator();
      ImGui::Text("Texture:");

      if (brush.textureID > 0) {
        auto *tex = state.map.GetTexture(brush.textureID);
        if (tex) {
          ImGui::Text("  %s", tex->name.c_str());
          ImGui::Text("  ID: %u", tex->id);

          if (ImGui::Button("Remove Texture")) {
            brush.textureID = 0;
            state.hasUnsavedChanges = true;
          }
          ImGui::Separator();
          ImGui::Text("Texture Repeat:");

          bool uvChanged = false;

          if (ImGui::DragFloat("Repeat X", &brush.uvScaleX, 0.1f, 0.1f,
                               20.0f)) {
            uvChanged = true;
          }
          if (ImGui::SameLine(); ImGui::Button("Reset##X")) {
            brush.uvScaleX = 1.0f;
            uvChanged = true;
          }

          if (ImGui::DragFloat("Repeat Y", &brush.uvScaleY, 0.1f, 0.1f,
                               20.0f)) {
            uvChanged = true;
          }
          if (ImGui::SameLine(); ImGui::Button("Reset##Y")) {
            brush.uvScaleY = 1.0f;
            uvChanged = true;
          }

          ImGui::Separator();
          ImGui::Text("Texture Offset:");

          if (ImGui::DragFloat("Offset X", &brush.uvOffsetX, 0.05f, -10.0f,
                               10.0f)) {
            uvChanged = true;
          }
          if (ImGui::DragFloat("Offset Y", &brush.uvOffsetY, 0.05f, -10.0f,
                               10.0f)) {
            uvChanged = true;
          }

          if (uvChanged) {
            state.hasUnsavedChanges = true;
          }

          if (ImGui::Button("Reset All UV", ImVec2(180, 25))) {
            brush.uvScaleX = 1.0f;
            brush.uvScaleY = 1.0f;
            brush.uvOffsetX = 0.0f;
            brush.uvOffsetY = 0.0f;
            state.hasUnsavedChanges = true;
          }

          ImGui::Separator();
          ImGui::Text("UV Transform:");
          if (ImGui::DragFloat("Scale X", &brush.uvScaleX, 0.1f, 0.1f, 10.0f)) {
            state.hasUnsavedChanges = true;
          }
          if (ImGui::DragFloat("Scale Y", &brush.uvScaleY, 0.1f, 0.1f, 10.0f)) {
            state.hasUnsavedChanges = true;
          }
          if (ImGui::DragFloat("Offset X", &brush.uvOffsetX, 0.1f, -10.0f,
                               10.0f)) {
            state.hasUnsavedChanges = true;
          }
          if (ImGui::DragFloat("Offset Y", &brush.uvOffsetY, 0.1f, -10.0f,
                               10.0f)) {
            state.hasUnsavedChanges = true;
          }
        } else {
          ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "  Invalid ID");
        }
      } else {
        ImGui::TextDisabled("  None");
        ImGui::TextWrapped("Use the Textures panel to apply a texture.");
      }
    }
  }

  void RenderEntityProperties() {
    if (ImGui::CollapsingHeader("Entity", ImGuiTreeNodeFlags_DefaultOpen)) {
      auto &ent = state.map.entities[state.selectedEntityIndex];

      strncpy(entityNameBuffer, ent.name.c_str(), sizeof(entityNameBuffer) - 1);
      if (ImGui::InputText("Name##ent", entityNameBuffer,
                           sizeof(entityNameBuffer))) {
        ent.name = entityNameBuffer;
        state.hasUnsavedChanges = true;
      }

      ImGui::Text("Type: %s", GetEntityTypeName(ent.type));

      if (ImGui::DragFloat3("Position", &ent.position.x, 0.1f))
        state.hasUnsavedChanges = true;
      if (ImGui::DragFloat3("Rotation", &ent.rotation.x, 1.0f, -180.0f, 180.0f))
        state.hasUnsavedChanges = true;
      if (ImGui::DragFloat3("Scale", &ent.scale.x, 0.1f, 0.1f, 10.0f))
        state.hasUnsavedChanges = true;

      ImGui::Separator();
      RenderEntityTypeProperties(ent);
    }
  }

  void RenderEntityTypeProperties(Entity &ent) {
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
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 25));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 25));

    ImGui::Begin("StatusBar", nullptr, flags);

    const char *toolNames[] = {
        "Select (1)",   "Move (2)",         "Rotate (3)",
        "Scale (4)",    "Create Box (5/B)", "Create Cylinder (C)",
        "Create Wedge", "Place Entity",     "Vertex Edit"};
    ImVec4 toolColor(0.4f, 0.8f, 1.0f, 1.0f);
    ImGui::TextColored(toolColor, "Tool:");
    ImGui::SameLine();
    ImGui::Text("%s", toolNames[static_cast<int>(state.currentTool)]);

    ImGui::SameLine(250);
    ImGui::Text("| Brushes: %zu | Entities: %zu | Textures: %zu",
                state.map.brushes.size(), state.map.entities.size(),
                state.map.textures.size());

    ImGui::SameLine(550);
    ImGui::Text("| Grid: %.2f", state.settings.gridSize);

    ImGui::SameLine(650);
    if (state.settings.snapToGrid) {
      ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "| Snap: ON");
    } else {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "| Snap: OFF");
    }

    ImGui::SameLine(770);
    if (state.hasUnsavedChanges) {
      ImGui::TextColored(ImVec4(1, 0.6f, 0.4f, 1), "| * UNSAVED *");
    } else {
      ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1), "| Saved");
    }

    ImGui::SameLine(viewport->Size.x - 380);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                       "F5: Test | Ctrl+S: Save | Del: Delete");

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
    std::string path =
        state.currentFilePath.empty() ? "map.pcd" : state.currentFilePath;
    if (PCDWriter::Save(state.map, path)) {
      state.hasUnsavedChanges = false;
      state.currentFilePath = path;
    }
  }

  void LoadTexture(const std::string &path) {
    Texture tex;
    if (TextureLoader::LoadImage(path, tex)) {
      state.map.AddTexture(tex);
      state.hasUnsavedChanges = true;
      std::cout << "[UI] Loaded texture: " << tex.name << "\n";
    } else {
      std::cerr << "[UI] Failed to load texture: " << path << "\n";
    }
  }
};

} // namespace PCD

#endif // PCD_EDITOR_UI_H
