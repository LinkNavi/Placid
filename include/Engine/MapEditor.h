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
    GizmoAxis activeAxis;
    PCD::Vec3 manipulationStart;
    PCD::Vec3 objectStartPos;
    PCD::Vec3 objectStartScale;
    PCD::Vec3 objectStartRot;
    float manipulationStartDist;

public:
    explicit MapEditor() : isManipulating(false), activeAxis(GizmoAxis::NONE), manipulationStartDist(0) {
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
    void DeselectAll() { state.DeselectAll(); isManipulating = false; }
    
    void SetTool(PCD::EditorTool tool) { 
        state.currentTool = tool; 
        isManipulating = false;
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
    
    void OnMouseClick(float worldX, float worldY, float worldZ, bool shift) {
        PCD::Vec3 clickPos(worldX, worldY, worldZ);
        
        if (state.settings.snapToGrid) {
            clickPos = state.SnapToGrid(clickPos);
        }
        
        // Check if clicking on gizmo
        if (state.currentTool == PCD::EditorTool::MOVE || 
            state.currentTool == PCD::EditorTool::ROTATE || 
            state.currentTool == PCD::EditorTool::SCALE) {
            
            if (state.selectedBrushIndex >= 0 || state.selectedEntityIndex >= 0) {
                PCD::Vec3 objPos = GetSelectedObjectPosition();
                GizmoAxis axis = HitTestGizmo(clickPos, objPos);
                
                if (axis != GizmoAxis::NONE) {
                    isManipulating = true;
                    activeAxis = axis;
                    manipulationStart = clickPos;
                    objectStartPos = objPos;
                    
                    // Store initial state for manipulation
                    if (state.selectedEntityIndex >= 0) {
                        auto& ent = state.map.entities[state.selectedEntityIndex];
                        objectStartScale = ent.scale;
                        objectStartRot = ent.rotation;
                    }
                    
                    state.PushUndo();
                    return;
                }
            }
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
                        return;
                    }
                }
                
                // Check brushes
                for (size_t i = 0; i < state.map.brushes.size(); i++) {
                    auto& brush = state.map.brushes[i];
                    
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
    
    void OnMouseDrag(float worldX, float worldY, float worldZ, float dx, float dy, bool constrainAxis) {
        PCD::Vec3 dragPos(worldX, worldY, worldZ);
        
        if (isManipulating) {
            PCD::Vec3 delta = dragPos - manipulationStart;
            
            // Constrain to axis
            if (constrainAxis || activeAxis != GizmoAxis::XYZ) {
                switch (activeAxis) {
                    case GizmoAxis::X:
                        delta.y = 0; delta.z = 0;
                        break;
                    case GizmoAxis::Y:
                        delta.x = 0; delta.z = 0;
                        break;
                    case GizmoAxis::Z:
                        delta.x = 0; delta.y = 0;
                        break;
                    case GizmoAxis::XY:
                        delta.z = 0;
                        break;
                    case GizmoAxis::XZ:
                        delta.y = 0;
                        break;
                    case GizmoAxis::YZ:
                        delta.x = 0;
                        break;
                    default:
                        break;
                }
            }
            
            if (state.settings.snapToGrid) {
                delta.x = std::round(delta.x / state.settings.gridSize) * state.settings.gridSize;
                delta.y = std::round(delta.y / state.settings.gridSize) * state.settings.gridSize;
                delta.z = std::round(delta.z / state.settings.gridSize) * state.settings.gridSize;
            }
            
            switch (state.currentTool) {
                case PCD::EditorTool::MOVE:
                    ApplyMove(delta);
                    break;
                    
                case PCD::EditorTool::ROTATE:
                    ApplyRotation(dx, dy);
                    break;
                    
                case PCD::EditorTool::SCALE:
                    ApplyScale(delta);
                    break;
                    
                default:
                    break;
            }
            
            state.hasUnsavedChanges = true;
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
    GizmoAxis HitTestGizmo(const PCD::Vec3& clickPos, const PCD::Vec3& gizmoPos) {
        float threshold = 0.5f;
        PCD::Vec3 delta = clickPos - gizmoPos;
        
        // Test axes
        if (fabs(delta.y) < threshold && fabs(delta.z) < threshold && delta.x > 0 && delta.x < 3.0f)
            return GizmoAxis::X;
        if (fabs(delta.x) < threshold && fabs(delta.z) < threshold && delta.y > 0 && delta.y < 3.0f)
            return GizmoAxis::Y;
        if (fabs(delta.x) < threshold && fabs(delta.y) < threshold && delta.z > 0 && delta.z < 3.0f)
            return GizmoAxis::Z;
        
        // Test center (all axes)
        float dist = sqrt(delta.x*delta.x + delta.y*delta.y + delta.z*delta.z);
        if (dist < threshold) return GizmoAxis::XYZ;
        
        return GizmoAxis::NONE;
    }
    
    void ApplyMove(const PCD::Vec3& delta) {
        if (state.selectedEntityIndex >= 0 && state.selectedEntityIndex < (int)state.map.entities.size()) {
            auto& ent = state.map.entities[state.selectedEntityIndex];
            ent.position = objectStartPos + delta;
        }
        
        if (state.selectedBrushIndex >= 0 && state.selectedBrushIndex < (int)state.map.brushes.size()) {
            auto& brush = state.map.brushes[state.selectedBrushIndex];
            PCD::Vec3 center = objectStartPos;
            
            for (auto& v : brush.vertices) {
                PCD::Vec3 offset(
                    v.position.x - center.x,
                    v.position.y - center.y,
                    v.position.z - center.z
                );
                v.position.x = center.x + offset.x + delta.x;
                v.position.y = center.y + offset.y + delta.y;
                v.position.z = center.z + offset.z + delta.z;
            }
        }
    }
    
    void ApplyRotation(float dx, float dy) {
        if (state.selectedEntityIndex >= 0 && state.selectedEntityIndex < (int)state.map.entities.size()) {
            auto& ent = state.map.entities[state.selectedEntityIndex];
            
            switch (activeAxis) {
                case GizmoAxis::X:
                    ent.rotation.x = objectStartRot.x + dy * 0.5f;
                    break;
                case GizmoAxis::Y:
                    ent.rotation.y = objectStartRot.y + dx * 0.5f;
                    break;
                case GizmoAxis::Z:
                    ent.rotation.z = objectStartRot.z + dx * 0.5f;
                    break;
                default:
                    ent.rotation.y = objectStartRot.y + dx * 0.5f;
                    break;
            }
        }
    }
    
    void ApplyScale(const PCD::Vec3& delta) {
        if (state.selectedEntityIndex >= 0 && state.selectedEntityIndex < (int)state.map.entities.size()) {
            auto& ent = state.map.entities[state.selectedEntityIndex];
            float scaleFactor = 1.0f + (delta.x + delta.y + delta.z) * 0.1f;
            
            switch (activeAxis) {
                case GizmoAxis::X:
                    ent.scale.x = std::max(0.1f, objectStartScale.x * (1.0f + delta.x * 0.1f));
                    break;
                case GizmoAxis::Y:
                    ent.scale.y = std::max(0.1f, objectStartScale.y * (1.0f + delta.y * 0.1f));
                    break;
                case GizmoAxis::Z:
                    ent.scale.z = std::max(0.1f, objectStartScale.z * (1.0f + delta.z * 0.1f));
                    break;
                default:
                    ent.scale.x = std::max(0.1f, objectStartScale.x * scaleFactor);
                    ent.scale.y = std::max(0.1f, objectStartScale.y * scaleFactor);
                    ent.scale.z = std::max(0.1f, objectStartScale.z * scaleFactor);
                    break;
            }
        }
    }
};

} // namespace Editor

#endif // MAP_EDITOR_H
