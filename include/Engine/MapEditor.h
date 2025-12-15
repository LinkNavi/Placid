#ifndef MAP_EDITOR_H
#define MAP_EDITOR_H

#include "PCD/PCD.h"
#include "Engine/Renderer.h"
#include <cmath>

namespace Editor {

enum class GizmoAxis { NONE, X, Y, Z, XY, XZ, YZ, XYZ };

class MapEditor {
private:
    PCD::EditorState state;
    PCD::EditorUI* ui;
    
    // Manipulation state
    bool isManipulating;
    bool isDragging;
    GizmoAxis activeAxis;
    PCD::Vec3 manipulationStart;
    PCD::Vec3 objectStartPos;
    PCD::Vec3 objectStartScale;
    PCD::Vec3 objectStartRot;
    
    // Mouse tracking for smooth dragging
    float startMouseX, startMouseY;
    float lastMouseX, lastMouseY;
    
    // Store brush bounds for scaling
    struct BrushBounds {
        PCD::Vec3 min, max, center;
    };

public:
    explicit MapEditor() : isManipulating(false), isDragging(false), activeAxis(GizmoAxis::NONE),
                          startMouseX(0), startMouseY(0), lastMouseX(0), lastMouseY(0) {
        ui = new PCD::EditorUI(state);
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
    void DeselectAll() { 
        state.DeselectAll(); 
        isManipulating = false;
        isDragging = false;
    }
    
    void SetTool(PCD::EditorTool tool) { 
        state.currentTool = tool; 
        isManipulating = false;
        isDragging = false;
    }
    
    PCD::Vec3 GetSelectedObjectPosition() const {
        if (state.selectedBrushIndex >= 0 && state.selectedBrushIndex < (int)state.map.brushes.size()) {
            auto& brush = state.map.brushes[state.selectedBrushIndex];
            if (!brush.vertices.empty()) {
                PCD::Vec3 center(0, 0, 0);
                for (auto& v : brush.vertices) {
                    center.x += v.position.x;
                    center.y += v.position.y;
                    center.z += v.position.z;
                }
                center.x /= brush.vertices.size();
                center.y /= brush.vertices.size();
                center.z /= brush.vertices.size();
                return center;
            }
        }
        if (state.selectedEntityIndex >= 0 && state.selectedEntityIndex < (int)state.map.entities.size()) {
            return state.map.entities[state.selectedEntityIndex].position;
        }
        return PCD::Vec3(0, 0, 0);
    }
    
    BrushBounds GetBrushBounds(const PCD::Brush& brush) const {
        BrushBounds bounds;
        if (brush.vertices.empty()) return bounds;
        
        bounds.min = brush.vertices[0].position;
        bounds.max = brush.vertices[0].position;
        
        for (const auto& v : brush.vertices) {
            bounds.min.x = std::min(bounds.min.x, v.position.x);
            bounds.min.y = std::min(bounds.min.y, v.position.y);
            bounds.min.z = std::min(bounds.min.z, v.position.z);
            bounds.max.x = std::max(bounds.max.x, v.position.x);
            bounds.max.y = std::max(bounds.max.y, v.position.y);
            bounds.max.z = std::max(bounds.max.z, v.position.z);
        }
        
        bounds.center.x = (bounds.min.x + bounds.max.x) * 0.5f;
        bounds.center.y = (bounds.min.y + bounds.max.y) * 0.5f;
        bounds.center.z = (bounds.min.z + bounds.max.z) * 0.5f;
        
        return bounds;
    }
    
    void OnMouseClick(float worldX, float worldY, float worldZ, bool shift) {
        PCD::Vec3 clickPos(worldX, worldY, worldZ);
        
        if (state.settings.snapToGrid) {
            clickPos = state.SnapToGrid(clickPos);
        }
        
        // Check if we're using a manipulation tool and have something selected
        if ((state.currentTool == PCD::EditorTool::MOVE || 
             state.currentTool == PCD::EditorTool::ROTATE || 
             state.currentTool == PCD::EditorTool::SCALE) &&
            (state.selectedBrushIndex >= 0 || state.selectedEntityIndex >= 0)) {
            
            // Start manipulation
            isManipulating = true;
            isDragging = false; // Will be set to true on first drag
            activeAxis = shift ? GizmoAxis::XZ : GizmoAxis::XYZ; // Default behavior
            manipulationStart = clickPos;
            objectStartPos = GetSelectedObjectPosition();
            
            // Store initial mouse position
            startMouseX = worldX;
            startMouseY = worldY;
            lastMouseX = worldX;
            lastMouseY = worldY;
            
            // Store initial state
            if (state.selectedEntityIndex >= 0 && state.selectedEntityIndex < (int)state.map.entities.size()) {
                auto& ent = state.map.entities[state.selectedEntityIndex];
                objectStartScale = ent.scale;
                objectStartRot = ent.rotation;
            } else if (state.selectedBrushIndex >= 0 && state.selectedBrushIndex < (int)state.map.brushes.size()) {
                // Store brush bounds for scaling
                auto& brush = state.map.brushes[state.selectedBrushIndex];
                BrushBounds bounds = GetBrushBounds(brush);
                objectStartPos = bounds.center;
            }
            
            state.PushUndo();
            return;
        }
        
        // Tool-specific behavior
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
            case PCD::EditorTool::MOVE:
            case PCD::EditorTool::ROTATE:
            case PCD::EditorTool::SCALE:
                if (!shift) {
                    state.DeselectAll();
                }
                
                // Check entities first (smaller, easier to click)
                for (size_t i = 0; i < state.map.entities.size(); i++) {
                    auto& ent = state.map.entities[i];
                    float dx = ent.position.x - clickPos.x;
                    float dy = ent.position.y - clickPos.y;
                    float dz = ent.position.z - clickPos.z;
                    float dist = sqrt(dx*dx + dy*dy + dz*dz);
                    
                    if (dist < 2.0f) {
                        state.selectedEntityIndex = i;
                        state.selectedBrushIndex = -1;
                        return;
                    }
                }
                
                // Check brushes
                for (size_t i = 0; i < state.map.brushes.size(); i++) {
                    auto& brush = state.map.brushes[i];
                    
                    if (!brush.vertices.empty()) {
                        BrushBounds bounds = GetBrushBounds(brush);
                        
                        if (clickPos.x >= bounds.min.x && clickPos.x <= bounds.max.x &&
                            clickPos.z >= bounds.min.z && clickPos.z <= bounds.max.z) {
                            state.selectedBrushIndex = i;
                            state.selectedEntityIndex = -1;
                            return;
                        }
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    void OnMouseDrag(float worldX, float worldY, float worldZ, float screenDX, float screenDY, bool constrainAxis) {
        PCD::Vec3 dragPos(worldX, worldY, worldZ);
        
        // Handle manipulation (move/rotate/scale)
        if (isManipulating && (state.currentTool == PCD::EditorTool::MOVE ||
                               state.currentTool == PCD::EditorTool::ROTATE ||
                               state.currentTool == PCD::EditorTool::SCALE)) {
            
            isDragging = true;
            
            // Use screen-space delta for more predictable movement
            float mouseDeltaX = worldX - lastMouseX;
            float mouseDeltaY = worldY - lastMouseY;
            float mouseDeltaZ = worldZ - lastMouseZ;
            
            lastMouseX = worldX;
            lastMouseY = worldY;
            lastMouseZ = worldZ;
            
            PCD::Vec3 delta(mouseDeltaX, mouseDeltaY, mouseDeltaZ);
            
            // Apply axis constraints
            if (constrainAxis) {
                // When shift is held, constrain to XZ plane (no Y movement)
                delta.y = 0;
            }
            
            // Apply grid snapping to delta
            if (state.settings.snapToGrid) {
                float threshold = state.settings.gridSize * 0.5f;
                if (fabs(delta.x) > threshold) {
                    delta.x = std::round(delta.x / state.settings.gridSize) * state.settings.gridSize;
                } else {
                    delta.x = 0;
                }
                if (fabs(delta.y) > threshold) {
                    delta.y = std::round(delta.y / state.settings.gridSize) * state.settings.gridSize;
                } else {
                    delta.y = 0;
                }
                if (fabs(delta.z) > threshold) {
                    delta.z = std::round(delta.z / state.settings.gridSize) * state.settings.gridSize;
                } else {
                    delta.z = 0;
                }
            }
            
            switch (state.currentTool) {
                case PCD::EditorTool::MOVE:
                    if (delta.x != 0 || delta.y != 0 || delta.z != 0) {
                        ApplyMove(delta);
                        state.hasUnsavedChanges = true;
                    }
                    break;
                    
                case PCD::EditorTool::ROTATE:
                    ApplyRotation(screenDX, screenDY);
                    state.hasUnsavedChanges = true;
                    break;
                    
                case PCD::EditorTool::SCALE:
                    ApplyScale(screenDX, screenDY);
                    state.hasUnsavedChanges = true;
                    break;
                    
                default:
                    break;
            }
            
            return;
        }
        
        // Creation drag
        if (state.isCreating) {
            if (state.settings.snapToGrid) {
                dragPos = state.SnapToGrid(dragPos);
            }
            state.createEnd = dragPos;
        }
    }
    
    void OnMouseRelease() {
        if (isManipulating) {
            isManipulating = false;
            isDragging = false;
            activeAxis = GizmoAxis::NONE;
            return;
        }
        
        if (!state.isCreating) return;
        
        state.isCreating = false;
        
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
    
    PCD::Brush CreateBox(const PCD::Vec3& min, const PCD::Vec3& max) {
        return PCD::BrushFactory::CreateBox(state.map, min, max);
    }
    
    void RenderUI() {
        ui->Render();
    }
    
    void RenderGizmo(Renderer* renderer, float* view, float* proj) {
        if (state.selectedBrushIndex < 0 && state.selectedEntityIndex < 0) return;
        
        if (state.currentTool != PCD::EditorTool::MOVE &&
            state.currentTool != PCD::EditorTool::ROTATE &&
            state.currentTool != PCD::EditorTool::SCALE) return;
        
        PCD::Vec3 pos = GetSelectedObjectPosition();
        int axisInt = static_cast<int>(activeAxis);
        renderer->RenderGizmo(pos, state.currentTool, axisInt, view, proj);
    }

private:
    float lastMouseZ = 0; // Add this member
    
    void ApplyMove(const PCD::Vec3& delta) {
        // Move entity
        if (state.selectedEntityIndex >= 0 && state.selectedEntityIndex < (int)state.map.entities.size()) {
            auto& ent = state.map.entities[state.selectedEntityIndex];
            ent.position = ent.position + delta;
        }
        
        // Move brush - move all vertices by delta
        if (state.selectedBrushIndex >= 0 && state.selectedBrushIndex < (int)state.map.brushes.size()) {
            auto& brush = state.map.brushes[state.selectedBrushIndex];
            
            for (auto& v : brush.vertices) {
                v.position = v.position + delta;
            }
        }
    }
    
    void ApplyRotation(float screenDX, float screenDY) {
        if (state.selectedEntityIndex >= 0 && state.selectedEntityIndex < (int)state.map.entities.size()) {
            auto& ent = state.map.entities[state.selectedEntityIndex];
            
            float rotSpeed = 0.5f;
            
            // Rotate based on mouse movement
            ent.rotation.y += screenDX * rotSpeed;
            ent.rotation.x += screenDY * rotSpeed;
        }
    }
    
    void ApplyScale(float screenDX, float screenDY) {
        // Calculate scale factor from mouse movement
        float scaleDelta = (screenDX + screenDY) * 0.01f;
        float scaleFactor = 1.0f + scaleDelta;
        
        // Clamp scale factor to reasonable range
        scaleFactor = std::max(0.5f, std::min(2.0f, scaleFactor));
        
        // Scale entity
        if (state.selectedEntityIndex >= 0 && state.selectedEntityIndex < (int)state.map.entities.size()) {
            auto& ent = state.map.entities[state.selectedEntityIndex];
            
            ent.scale.x = std::max(0.1f, ent.scale.x * scaleFactor);
            ent.scale.y = std::max(0.1f, ent.scale.y * scaleFactor);
            ent.scale.z = std::max(0.1f, ent.scale.z * scaleFactor);
        }
        
        // Scale brush by moving vertices relative to center
        if (state.selectedBrushIndex >= 0 && state.selectedBrushIndex < (int)state.map.brushes.size()) {
            auto& brush = state.map.brushes[state.selectedBrushIndex];
            
            // Get current center
            BrushBounds bounds = GetBrushBounds(brush);
            PCD::Vec3 center = bounds.center;
            
            // Scale each vertex relative to center
            for (auto& v : brush.vertices) {
                // Get offset from center
                PCD::Vec3 offset = v.position - center;
                
                // Scale offset
                offset = offset * scaleFactor;
                
                // Apply back
                v.position = center + offset;
            }
        }
    }
};

} // namespace Editor

#endif // MAP_EDITOR_H
