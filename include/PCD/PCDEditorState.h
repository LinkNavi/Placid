#ifndef PCD_EDITOR_STATE_H
#define PCD_EDITOR_STATE_H

#include "PCDTypes.h"
#include <vector>
#include <string>

namespace PCD {

enum class EditorTool {
    SELECT, MOVE, ROTATE, SCALE,
    CREATE_BOX, CREATE_CYLINDER, CREATE_WEDGE,
    CREATE_ENTITY, VERTEX_EDIT
};

enum class GridPlane { XZ, XY, YZ };

struct EditorSettings {
    float gridSize = 1.0f;
    bool snapToGrid = true;
    bool showGrid = true;
    bool showEntityIcons = true;
    bool showBrushBounds = true;
    bool showNormals = false;
    float gridExtent = 50.0f;
    GridPlane currentPlane = GridPlane::XZ;
    float gridHeight = 0.0f;
};

struct EditorState {
    Map map;
    EditorSettings settings;
    EditorTool currentTool = EditorTool::SELECT;
    EntityType entityToPlace = ENT_INFO_PLAYER_START;
    
    // Selection
    int selectedBrushIndex = -1;
    int selectedEntityIndex = -1;
    std::vector<int> selectedBrushes;
    std::vector<int> selectedEntities;
    
    // Creation
    bool isCreating = false;
    Vec3 createStart;
    Vec3 createEnd;
    
    // Transform
    Vec3 transformOrigin;
    bool isTransforming = false;
    
    // Undo/Redo
    std::vector<Map> undoStack;
    std::vector<Map> redoStack;
    static const int MAX_UNDO = 50;
    
    // File
    std::string currentFilePath;
    bool hasUnsavedChanges = false;
    
    // Methods
    void PushUndo() {
        undoStack.push_back(map);
        if (undoStack.size() > MAX_UNDO) {
            undoStack.erase(undoStack.begin());
        }
        redoStack.clear();
    }
    
    void Undo() {
        if (!undoStack.empty()) {
            redoStack.push_back(map);
            map = undoStack.back();
            undoStack.pop_back();
            hasUnsavedChanges = true;
            DeselectAll();
        }
    }
    
    void Redo() {
        if (!redoStack.empty()) {
            undoStack.push_back(map);
            map = redoStack.back();
            redoStack.pop_back();
            hasUnsavedChanges = true;
            DeselectAll();
        }
    }
    
    void DeselectAll() {
        selectedBrushIndex = -1;
        selectedEntityIndex = -1;
        selectedBrushes.clear();
        selectedEntities.clear();
    }
    
    void SelectAll() {
        selectedBrushes.clear();
        selectedEntities.clear();
        for (size_t i = 0; i < map.brushes.size(); i++) {
            selectedBrushes.push_back(static_cast<int>(i));
        }
        for (size_t i = 0; i < map.entities.size(); i++) {
            selectedEntities.push_back(static_cast<int>(i));
        }
    }
    
    void DeleteSelected() {
        if (selectedBrushIndex >= 0 && selectedBrushIndex < static_cast<int>(map.brushes.size())) {
            PushUndo();
            map.brushes.erase(map.brushes.begin() + selectedBrushIndex);
            selectedBrushIndex = -1;
            hasUnsavedChanges = true;
        }
        if (selectedEntityIndex >= 0 && selectedEntityIndex < static_cast<int>(map.entities.size())) {
            PushUndo();
            map.entities.erase(map.entities.begin() + selectedEntityIndex);
            selectedEntityIndex = -1;
            hasUnsavedChanges = true;
        }
    }
    
    void DuplicateSelected() {
        if (selectedBrushIndex >= 0 && selectedBrushIndex < static_cast<int>(map.brushes.size())) {
            PushUndo();
            Brush copy = map.brushes[selectedBrushIndex];
            copy.id = map.nextBrushID++;
            copy.name = copy.name + "_copy";
            for (auto& v : copy.vertices) {
                v.position.x += 1.0f;
                v.position.z += 1.0f;
            }
            map.brushes.push_back(copy);
            selectedBrushIndex = static_cast<int>(map.brushes.size()) - 1;
            hasUnsavedChanges = true;
        }
        if (selectedEntityIndex >= 0 && selectedEntityIndex < static_cast<int>(map.entities.size())) {
            PushUndo();
            Entity copy = map.entities[selectedEntityIndex];
            copy.id = map.nextEntityID++;
            copy.name = copy.name + "_copy";
            copy.position.x += 1.0f;
            copy.position.z += 1.0f;
            map.entities.push_back(copy);
            selectedEntityIndex = static_cast<int>(map.entities.size()) - 1;
            hasUnsavedChanges = true;
        }
    }
    
    void NewMap() {
        PushUndo();
        map.Clear();
        map.name = "Untitled";
        map.author = "Unknown";
        currentFilePath.clear();
        hasUnsavedChanges = false;
        DeselectAll();
    }
    
    Vec3 SnapToGrid(Vec3 pos) const {
        if (settings.snapToGrid) {
            pos.x = std::round(pos.x / settings.gridSize) * settings.gridSize;
            pos.y = std::round(pos.y / settings.gridSize) * settings.gridSize;
            pos.z = std::round(pos.z / settings.gridSize) * settings.gridSize;
        }
        return pos;
    }
};

} // namespace PCD

#endif // PCD_EDITOR_STATE_H
