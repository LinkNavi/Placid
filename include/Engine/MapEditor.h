#ifndef MAP_EDITOR_H
#define MAP_EDITOR_H

#include "PCD/PCD.h"

namespace Editor {

class MapEditor {
private:
    PCD::EditorState state;
    PCD::EditorUI* ui;

public:
    MapEditor() {
        ui = new PCD::EditorUI(state);
        
        // Initialize with a default map
        state.map.name = "NewMap";
        state.map.author = "Unknown";
    }
    
    ~MapEditor() {
        delete ui;
    }
    
    // Core functionality
    PCD::Map& GetMap() { return state.map; }
    const PCD::Map& GetMap() const { return state.map; }
    
    PCD::EditorSettings& GetSettings() { return state.settings; }
    const PCD::EditorSettings& GetSettings() const { return state.settings; }
    
    int GetSelectedBrushIndex() const { return state.selectedBrushIndex; }
    int GetSelectedEntityIndex() const { return state.selectedEntityIndex; }
    
    bool IsCreating() const { return state.isCreating; }
    PCD::Vec3 GetCreateStart() const { return state.createStart; }
    PCD::Vec3 GetCreateEnd() const { return state.createEnd; }
    
    // Actions
    void NewMap() { state.NewMap(); }
    void SaveMap() { 
        if (state.currentFilePath.empty()) {
            state.currentFilePath = "map.pcd";
        }
        PCD::PCDWriter::Save(state.map, state.currentFilePath);
        state.hasUnsavedChanges = false;
    }
    
    void Undo() { state.Undo(); }
    void Redo() { state.Redo(); }
    void DeleteSelected() { state.DeleteSelected(); }
    void DuplicateSelected() { state.DuplicateSelected(); }
    void SelectAll() { state.SelectAll(); }
    void DeselectAll() { state.DeselectAll(); }
    
    // Tool handling
    void OnKeyPress(int key) {
        switch (key) {
            case GLFW_KEY_Q: state.currentTool = PCD::EditorTool::SELECT; break;
            case GLFW_KEY_W: state.currentTool = PCD::EditorTool::MOVE; break;
            case GLFW_KEY_E: state.currentTool = PCD::EditorTool::ROTATE; break;
            case GLFW_KEY_R: state.currentTool = PCD::EditorTool::SCALE; break;
            case GLFW_KEY_V: state.currentTool = PCD::EditorTool::VERTEX_EDIT; break;
            case GLFW_KEY_B: state.currentTool = PCD::EditorTool::CREATE_BOX; break;
            case GLFW_KEY_C: state.currentTool = PCD::EditorTool::CREATE_CYLINDER; break;
        }
    }
    
    void OnMouseClick(float worldX, float worldY, float worldZ, bool shift) {
        PCD::Vec3 clickPos(worldX, worldY, worldZ);
        
        if (state.settings.snapToGrid) {
            clickPos = state.SnapToGrid(clickPos);
        }
        
        switch (state.currentTool) {
            case PCD::EditorTool::CREATE_BOX:
            case PCD::EditorTool::CREATE_CYLINDER:
            case PCD::EditorTool::CREATE_WEDGE:
                state.isCreating = true;
                state.createStart = clickPos;
                state.createEnd = clickPos;
                break;
                
            case PCD::EditorTool::CREATE_ENTITY:
                state.PushUndo();
                {
                    PCD::Entity ent = PCD::BrushFactory::CreateEntity(
                        state.map, 
                        state.entityToPlace, 
                        clickPos
                    );
                    state.map.entities.push_back(ent);
                    state.selectedEntityIndex = state.map.entities.size() - 1;
                    state.hasUnsavedChanges = true;
                }
                break;
                
            case PCD::EditorTool::SELECT:
                // Simple selection by proximity
                if (!shift) {
                    state.DeselectAll();
                }
                
                // Check entities first
                for (size_t i = 0; i < state.map.entities.size(); i++) {
                    auto& ent = state.map.entities[i];
                    float dx = ent.position.x - clickPos.x;
                    float dy = ent.position.y - clickPos.y;
                    float dz = ent.position.z - clickPos.z;
                    float dist = sqrt(dx*dx + dy*dy + dz*dz);
                    
                    if (dist < 2.0f) {
                        state.selectedEntityIndex = i;
                        return;
                    }
                }
                
                // Check brushes
                for (size_t i = 0; i < state.map.brushes.size(); i++) {
                    auto& brush = state.map.brushes[i];
                    
                    // Simple AABB check
                    if (!brush.vertices.empty()) {
                        float minX = brush.vertices[0].position.x;
                        float maxX = minX;
                        float minZ = brush.vertices[0].position.z;
                        float maxZ = minZ;
                        
                        for (auto& v : brush.vertices) {
                            minX = std::min(minX, v.position.x);
                            maxX = std::max(maxX, v.position.x);
                            minZ = std::min(minZ, v.position.z);
                            maxZ = std::max(maxZ, v.position.z);
                        }
                        
                        if (clickPos.x >= minX && clickPos.x <= maxX &&
                            clickPos.z >= minZ && clickPos.z <= maxZ) {
                            state.selectedBrushIndex = i;
                            return;
                        }
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    void OnMouseDrag(float worldX, float worldY, float worldZ) {
        if (!state.isCreating) return;
        
        PCD::Vec3 dragPos(worldX, worldY, worldZ);
        
        if (state.settings.snapToGrid) {
            dragPos = state.SnapToGrid(dragPos);
        }
        
        state.createEnd = dragPos;
    }
    
    void OnMouseRelease() {
        if (!state.isCreating) return;
        
        state.isCreating = false;
        
        // Create the brush
        PCD::Vec3 min(
            std::min(state.createStart.x, state.createEnd.x),
            std::min(state.createStart.y, state.createEnd.y),
            std::min(state.createStart.z, state.createEnd.z)
        );
        
        PCD::Vec3 max(
            std::max(state.createStart.x, state.createEnd.x),
            std::max(state.createStart.y, state.createEnd.y),
            std::max(state.createStart.z, state.createEnd.z)
        );
        
        // Ensure minimum size
        if (max.x - min.x < 0.1f) max.x = min.x + state.settings.gridSize;
        if (max.y - min.y < 0.1f) max.y = min.y + state.settings.gridSize * 2;
        if (max.z - min.z < 0.1f) max.z = min.z + state.settings.gridSize;
        
        state.PushUndo();
        
        PCD::Brush newBrush;
        switch (state.currentTool) {
            case PCD::EditorTool::CREATE_BOX:
                newBrush = PCD::BrushFactory::CreateBox(state.map, min, max);
                newBrush.name = "Box";
                break;
                
            case PCD::EditorTool::CREATE_CYLINDER: {
                PCD::Vec3 center(
                    (min.x + max.x) * 0.5f,
                    (min.y + max.y) * 0.5f,
                    (min.z + max.z) * 0.5f
                );
                float radius = std::max(max.x - min.x, max.z - min.z) * 0.5f;
                float height = max.y - min.y;
                newBrush = PCD::BrushFactory::CreateCylinder(state.map, center, radius, height);
                newBrush.name = "Cylinder";
                break;
            }
                
            case PCD::EditorTool::CREATE_WEDGE:
                newBrush = PCD::BrushFactory::CreateWedge(state.map, min, max);
                newBrush.name = "Wedge";
                break;
                
            default:
                return;
        }
        
        state.map.brushes.push_back(newBrush);
        state.selectedBrushIndex = state.map.brushes.size() - 1;
        state.hasUnsavedChanges = true;
    }
    
    // Brush creation helpers
    PCD::Brush CreateBox(const PCD::Vec3& min, const PCD::Vec3& max) {
        return PCD::BrushFactory::CreateBox(state.map, min, max);
    }
    
    // UI rendering
    void RenderUI() {
        ui->Render();
    }
};

} // namespace Editor

#endif // MAP_EDITOR_H
