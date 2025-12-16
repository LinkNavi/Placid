#ifndef MAP_EDITOR_H
#define MAP_EDITOR_H

#include "Engine/Renderer.h"
#include "PCD/PCD.h"
#include <cmath>
#include <algorithm>
#include <deque>
#include <functional>
#include <chrono>

namespace Editor {

enum class GizmoAxis { NONE, X, Y, Z, XY, XZ, YZ, XYZ };
enum class GizmoMode { TRANSLATE, ROTATE, SCALE };
enum class SelectionMode { OBJECT, VERTEX, FACE };
enum class ClipMode { NONE, FIRST_POINT, SECOND_POINT };

struct ClipboardItem {
    bool isBrush;
    PCD::Brush brush;
    PCD::Entity entity;
};

struct CameraBookmark {
    std::string name;
    float x, y, z;
    float yaw, pitch;
    float distance;
};

struct MeasurementPoint {
    PCD::Vec3 position;
    bool active;
};

class MapEditor {
private:
    PCD::EditorState state;
    PCD::EditorUI* ui;
    
    // Gizmo state
    GizmoMode gizmoMode;
    GizmoAxis activeAxis;
    bool isManipulating;
    bool isDragging;
    
    // Manipulation tracking
    PCD::Vec3 manipulationStart;
    PCD::Vec3 objectStartPos;
    PCD::Vec3 objectStartScale;
    PCD::Vec3 objectStartRot;
    float startMouseX, startMouseY;
    float lastMouseX, lastMouseY;
    float accumulatedDeltaX, accumulatedDeltaY, accumulatedDeltaZ;
    
    // Multi-selection
    std::vector<int> selectedBrushIndices;
    std::vector<int> selectedEntityIndices;
    bool multiSelectMode;
    
    // Clipboard
    std::vector<ClipboardItem> clipboard;
    
    // Vertex editing
    SelectionMode selectionMode;
    std::vector<int> selectedVertexIndices;
    
    // Clipping tool
    ClipMode clipMode;
    PCD::Vec3 clipPoint1, clipPoint2;
    
    // Camera bookmarks
    std::vector<CameraBookmark> bookmarks;
    
    // Measurement
    MeasurementPoint measureStart, measureEnd;
    bool isMeasuring;
    
    // Auto-save
    std::chrono::steady_clock::time_point lastAutoSave;
    float autoSaveInterval;
    bool autoSaveEnabled;
    
    // Snap settings
    float rotationSnapAngle;
    float scaleSnapIncrement;
    bool snapRotation;
    bool snapScale;
    
    // Recent files
    std::deque<std::string> recentFiles;
    static const int MAX_RECENT_FILES = 10;
    
    // Statistics
    struct MapStats {
        int totalVertices;
        int totalTriangles;
        int totalBrushes;
        int totalEntities;
        int totalTextures;
        PCD::Vec3 mapBoundsMin;
        PCD::Vec3 mapBoundsMax;
    } stats;

    struct BrushBounds {
        PCD::Vec3 min, max, center;
    };

    struct Ray {
        PCD::Vec3 origin;
        PCD::Vec3 direction;
    };

public:
    explicit MapEditor()
        : gizmoMode(GizmoMode::TRANSLATE)
        , activeAxis(GizmoAxis::NONE)
        , isManipulating(false)
        , isDragging(false)
        , startMouseX(0), startMouseY(0)
        , lastMouseX(0), lastMouseY(0)
        , accumulatedDeltaX(0), accumulatedDeltaY(0), accumulatedDeltaZ(0)
        , multiSelectMode(false)
        , selectionMode(SelectionMode::OBJECT)
        , clipMode(ClipMode::NONE)
        , isMeasuring(false)
        , autoSaveInterval(300.0f) // 5 minutes
        , autoSaveEnabled(true)
        , rotationSnapAngle(15.0f)
        , scaleSnapIncrement(0.25f)
        , snapRotation(true)
        , snapScale(false)
    {
        ui = new PCD::EditorUI(state);
        state.map.name = "NewMap";
        state.map.author = "Unknown";
        lastAutoSave = std::chrono::steady_clock::now();
        measureStart.active = false;
        measureEnd.active = false;
        UpdateStats();
    }

    ~MapEditor() { delete ui; }

    // Core getters
    PCD::Map& GetMap() { return state.map; }
    const PCD::Map& GetMap() const { return state.map; }
    PCD::EditorSettings& GetSettings() { return state.settings; }
    const PCD::EditorSettings& GetSettings() const { return state.settings; }
    int GetSelectedBrushIndex() const { return state.selectedBrushIndex; }
    int GetSelectedEntityIndex() const { return state.selectedEntityIndex; }
    bool IsCreating() const { return state.isCreating; }
    PCD::Vec3 GetCreateStart() const { return state.createStart; }
    PCD::Vec3 GetCreateEnd() const { return state.createEnd; }
    GizmoMode GetGizmoMode() const { return gizmoMode; }
    SelectionMode GetSelectionMode() const { return selectionMode; }
    const MapStats& GetStats() const { return stats; }
    
    // Gizmo mode
    void SetGizmoMode(GizmoMode mode) { gizmoMode = mode; }
    void CycleGizmoMode() {
        gizmoMode = static_cast<GizmoMode>((static_cast<int>(gizmoMode) + 1) % 3);
    }
    
    // Selection mode
    void SetSelectionMode(SelectionMode mode) { 
        selectionMode = mode;
        if (mode != SelectionMode::VERTEX) {
            selectedVertexIndices.clear();
        }
    }

    // Actions
    void NewMap() { 
        state.NewMap(); 
        UpdateStats();
    }
    
    void SaveMap() {
        if (state.currentFilePath.empty()) {
            state.currentFilePath = "map.pcd";
        }
        PCD::PCDWriter::Save(state.map, state.currentFilePath);
        state.hasUnsavedChanges = false;
        AddRecentFile(state.currentFilePath);
    }
    
    void SaveMapAs(const std::string& path) {
        state.currentFilePath = path;
        SaveMap();
    }
    
    bool LoadMap(const std::string& path) {
        if (PCD::PCDReader::Load(state.map, path)) {
            state.currentFilePath = path;
            state.hasUnsavedChanges = false;
            AddRecentFile(path);
            UpdateStats();
            return true;
        }
        return false;
    }

    void Undo() { state.Undo(); UpdateStats(); }
    void Redo() { state.Redo(); UpdateStats(); }
    void DeleteSelected() { 
        state.DeleteSelected(); 
        UpdateStats();
    }
    void DuplicateSelected() { 
        state.DuplicateSelected(); 
        UpdateStats();
    }
    void SelectAll() { state.SelectAll(); }
    
    void DeselectAll() {
        state.DeselectAll();
        selectedBrushIndices.clear();
        selectedEntityIndices.clear();
        selectedVertexIndices.clear();
        isManipulating = false;
        isDragging = false;
    }

    void SetTool(PCD::EditorTool tool) {
        state.currentTool = tool;
        isManipulating = false;
        isDragging = false;
        
        // Set gizmo mode based on tool
        switch (tool) {
            case PCD::EditorTool::MOVE:
                gizmoMode = GizmoMode::TRANSLATE;
                break;
            case PCD::EditorTool::ROTATE:
                gizmoMode = GizmoMode::ROTATE;
                break;
            case PCD::EditorTool::SCALE:
                gizmoMode = GizmoMode::SCALE;
                break;
            default:
                break;
        }
    }

    // Copy/Paste
    void Copy() {
        clipboard.clear();
        
        if (state.selectedBrushIndex >= 0 && 
            state.selectedBrushIndex < (int)state.map.brushes.size()) {
            ClipboardItem item;
            item.isBrush = true;
            item.brush = state.map.brushes[state.selectedBrushIndex];
            clipboard.push_back(item);
        }
        
        if (state.selectedEntityIndex >= 0 && 
            state.selectedEntityIndex < (int)state.map.entities.size()) {
            ClipboardItem item;
            item.isBrush = false;
            item.entity = state.map.entities[state.selectedEntityIndex];
            clipboard.push_back(item);
        }
        
        // Multi-selection
        for (int idx : selectedBrushIndices) {
            if (idx >= 0 && idx < (int)state.map.brushes.size()) {
                ClipboardItem item;
                item.isBrush = true;
                item.brush = state.map.brushes[idx];
                clipboard.push_back(item);
            }
        }
        
        for (int idx : selectedEntityIndices) {
            if (idx >= 0 && idx < (int)state.map.entities.size()) {
                ClipboardItem item;
                item.isBrush = false;
                item.entity = state.map.entities[idx];
                clipboard.push_back(item);
            }
        }
    }
    
    void Cut() {
        Copy();
        DeleteSelected();
    }
    
    void Paste() {
        if (clipboard.empty()) return;
        
        state.PushUndo();
        DeselectAll();
        
        PCD::Vec3 offset(2.0f, 0.0f, 2.0f);
        
        for (auto& item : clipboard) {
            if (item.isBrush) {
                PCD::Brush newBrush = item.brush;
                newBrush.id = state.map.nextBrushID++;
                newBrush.name = newBrush.name + "_paste";
                
                for (auto& v : newBrush.vertices) {
                    v.position = v.position + offset;
                }
                
                state.map.brushes.push_back(newBrush);
                state.selectedBrushIndex = state.map.brushes.size() - 1;
            } else {
                PCD::Entity newEnt = item.entity;
                newEnt.id = state.map.nextEntityID++;
                newEnt.name = newEnt.name + "_paste";
                newEnt.position = newEnt.position + offset;
                
                state.map.entities.push_back(newEnt);
                state.selectedEntityIndex = state.map.entities.size() - 1;
            }
        }
        
        state.hasUnsavedChanges = true;
        UpdateStats();
    }
    
    bool HasClipboard() const { return !clipboard.empty(); }

    // Alignment tools
    void AlignToGrid() {
        if (state.selectedBrushIndex >= 0) {
            state.PushUndo();
            auto& brush = state.map.brushes[state.selectedBrushIndex];
            for (auto& v : brush.vertices) {
                v.position = state.SnapToGrid(v.position);
            }
            state.hasUnsavedChanges = true;
        }
        if (state.selectedEntityIndex >= 0) {
            state.PushUndo();
            auto& ent = state.map.entities[state.selectedEntityIndex];
            ent.position = state.SnapToGrid(ent.position);
            state.hasUnsavedChanges = true;
        }
    }
    
    void AlignSelectedToX() { AlignSelectedToAxis(0); }
    void AlignSelectedToY() { AlignSelectedToAxis(1); }
    void AlignSelectedToZ() { AlignSelectedToAxis(2); }
    
    void AlignSelectedToAxis(int axis) {
        if (selectedBrushIndices.size() < 2 && selectedEntityIndices.size() < 2) return;
        
        // Find average position
        float sum = 0;
        int count = 0;
        
        for (int idx : selectedBrushIndices) {
            PCD::Vec3 center = GetBrushBounds(state.map.brushes[idx]).center;
            sum += (axis == 0 ? center.x : (axis == 1 ? center.y : center.z));
            count++;
        }
        for (int idx : selectedEntityIndices) {
            PCD::Vec3 pos = state.map.entities[idx].position;
            sum += (axis == 0 ? pos.x : (axis == 1 ? pos.y : pos.z));
            count++;
        }
        
        if (count == 0) return;
        float avg = sum / count;
        
        state.PushUndo();
        
        for (int idx : selectedBrushIndices) {
            auto& brush = state.map.brushes[idx];
            BrushBounds bounds = GetBrushBounds(brush);
            float current = (axis == 0 ? bounds.center.x : (axis == 1 ? bounds.center.y : bounds.center.z));
            float delta = avg - current;
            
            for (auto& v : brush.vertices) {
                if (axis == 0) v.position.x += delta;
                else if (axis == 1) v.position.y += delta;
                else v.position.z += delta;
            }
        }
        
        for (int idx : selectedEntityIndices) {
            auto& ent = state.map.entities[idx];
            if (axis == 0) ent.position.x = avg;
            else if (axis == 1) ent.position.y = avg;
            else ent.position.z = avg;
        }
        
        state.hasUnsavedChanges = true;
    }

    // Brush operations
    void HollowBrush(float thickness = 0.25f) {
        if (state.selectedBrushIndex < 0) return;
        
        state.PushUndo();
        
        PCD::Brush& original = state.map.brushes[state.selectedBrushIndex];
        BrushBounds bounds = GetBrushBounds(original);
        
        // Create inner bounds
        PCD::Vec3 innerMin(bounds.min.x + thickness, bounds.min.y + thickness, bounds.min.z + thickness);
        PCD::Vec3 innerMax(bounds.max.x - thickness, bounds.max.y - thickness, bounds.max.z - thickness);
        
        // Check if hollow is possible
        if (innerMax.x <= innerMin.x || innerMax.y <= innerMin.y || innerMax.z <= innerMin.z) {
            return; // Too thin to hollow
        }
        
        // Delete original and create 6 walls
        state.map.brushes.erase(state.map.brushes.begin() + state.selectedBrushIndex);
        
        auto createWall = [this](PCD::Vec3 min, PCD::Vec3 max, const std::string& name) {
            PCD::Brush wall = PCD::BrushFactory::CreateBox(state.map, min, max);
            wall.name = name;
            wall.color = PCD::Vec3(0.6f, 0.6f, 0.6f);
            state.map.brushes.push_back(wall);
        };
        
        // Bottom
        createWall(bounds.min, PCD::Vec3(bounds.max.x, bounds.min.y + thickness, bounds.max.z), "Wall_Bottom");
        // Top
        createWall(PCD::Vec3(bounds.min.x, bounds.max.y - thickness, bounds.min.z), bounds.max, "Wall_Top");
        // Front
        createWall(PCD::Vec3(bounds.min.x, innerMin.y, bounds.max.z - thickness), 
                   PCD::Vec3(bounds.max.x, innerMax.y, bounds.max.z), "Wall_Front");
        // Back
        createWall(PCD::Vec3(bounds.min.x, innerMin.y, bounds.min.z), 
                   PCD::Vec3(bounds.max.x, innerMax.y, bounds.min.z + thickness), "Wall_Back");
        // Left
        createWall(PCD::Vec3(bounds.min.x, innerMin.y, innerMin.z), 
                   PCD::Vec3(bounds.min.x + thickness, innerMax.y, innerMax.z), "Wall_Left");
        // Right
        createWall(PCD::Vec3(bounds.max.x - thickness, innerMin.y, innerMin.z), 
                   PCD::Vec3(bounds.max.x, innerMax.y, innerMax.z), "Wall_Right");
        
        state.selectedBrushIndex = -1;
        state.hasUnsavedChanges = true;
        UpdateStats();
    }
    
    void FlipBrushX() { FlipBrush(0); }
    void FlipBrushY() { FlipBrush(1); }
    void FlipBrushZ() { FlipBrush(2); }
    
    void FlipBrush(int axis) {
        if (state.selectedBrushIndex < 0) return;
        
        state.PushUndo();
        auto& brush = state.map.brushes[state.selectedBrushIndex];
        BrushBounds bounds = GetBrushBounds(brush);
        
        for (auto& v : brush.vertices) {
            if (axis == 0) {
                v.position.x = bounds.max.x - (v.position.x - bounds.min.x);
                v.normal.x = -v.normal.x;
            } else if (axis == 1) {
                v.position.y = bounds.max.y - (v.position.y - bounds.min.y);
                v.normal.y = -v.normal.y;
            } else {
                v.position.z = bounds.max.z - (v.position.z - bounds.min.z);
                v.normal.z = -v.normal.z;
            }
        }
        
        // Reverse winding order
        for (size_t i = 0; i < brush.indices.size(); i += 3) {
            std::swap(brush.indices[i + 1], brush.indices[i + 2]);
        }
        
        state.hasUnsavedChanges = true;
    }
    
    void RotateBrush90(int axis) {
        if (state.selectedBrushIndex < 0) return;
        
        state.PushUndo();
        auto& brush = state.map.brushes[state.selectedBrushIndex];
        BrushBounds bounds = GetBrushBounds(brush);
        
        for (auto& v : brush.vertices) {
            PCD::Vec3 rel = v.position - bounds.center;
            PCD::Vec3 relN = v.normal;
            
            if (axis == 0) { // Around X
                float newY = -rel.z;
                float newZ = rel.y;
                rel.y = newY; rel.z = newZ;
                newY = -relN.z; newZ = relN.y;
                relN.y = newY; relN.z = newZ;
            } else if (axis == 1) { // Around Y
                float newX = rel.z;
                float newZ = -rel.x;
                rel.x = newX; rel.z = newZ;
                newX = relN.z; newZ = -relN.x;
                relN.x = newX; relN.z = newZ;
            } else { // Around Z
                float newX = -rel.y;
                float newY = rel.x;
                rel.x = newX; rel.y = newY;
                newX = -relN.y; newY = relN.x;
                relN.x = newX; relN.y = newY;
            }
            
            v.position = bounds.center + rel;
            v.normal = relN;
        }
        
        state.hasUnsavedChanges = true;
    }

    // Measurement
    void StartMeasurement(const PCD::Vec3& point) {
        measureStart.position = point;
        measureStart.active = true;
        measureEnd.active = false;
        isMeasuring = true;
    }
    
    void EndMeasurement(const PCD::Vec3& point) {
        measureEnd.position = point;
        measureEnd.active = true;
        isMeasuring = false;
    }
    
    float GetMeasurementDistance() const {
        if (!measureStart.active || !measureEnd.active) return 0;
        return (measureEnd.position - measureStart.position).Length();
    }
    
    void ClearMeasurement() {
        measureStart.active = false;
        measureEnd.active = false;
        isMeasuring = false;
    }
    
    bool IsMeasuring() const { return isMeasuring; }
    bool HasMeasurement() const { return measureStart.active && measureEnd.active; }
    PCD::Vec3 GetMeasureStart() const { return measureStart.position; }
    PCD::Vec3 GetMeasureEnd() const { return measureEnd.position; }

    // Camera bookmarks
    void AddBookmark(const std::string& name, float x, float y, float z, 
                     float yaw, float pitch, float distance) {
        CameraBookmark bm;
        bm.name = name;
        bm.x = x; bm.y = y; bm.z = z;
        bm.yaw = yaw; bm.pitch = pitch;
        bm.distance = distance;
        bookmarks.push_back(bm);
    }
    
    const std::vector<CameraBookmark>& GetBookmarks() const { return bookmarks; }
    
    void RemoveBookmark(int index) {
        if (index >= 0 && index < (int)bookmarks.size()) {
            bookmarks.erase(bookmarks.begin() + index);
        }
    }

    // Recent files
    void AddRecentFile(const std::string& path) {
        // Remove if already exists
        auto it = std::find(recentFiles.begin(), recentFiles.end(), path);
        if (it != recentFiles.end()) {
            recentFiles.erase(it);
        }
        
        recentFiles.push_front(path);
        
        while (recentFiles.size() > MAX_RECENT_FILES) {
            recentFiles.pop_back();
        }
    }
    
    const std::deque<std::string>& GetRecentFiles() const { return recentFiles; }

    // Auto-save
    void CheckAutoSave() {
        if (!autoSaveEnabled || !state.hasUnsavedChanges) return;
        
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - lastAutoSave).count();
        
        if (elapsed >= autoSaveInterval) {
            std::string autoSavePath = state.currentFilePath.empty() ? 
                "autosave.pcd" : state.currentFilePath + ".autosave";
            PCD::PCDWriter::Save(state.map, autoSavePath);
            lastAutoSave = now;
        }
    }
    
    void SetAutoSaveEnabled(bool enabled) { autoSaveEnabled = enabled; }
    void SetAutoSaveInterval(float seconds) { autoSaveInterval = seconds; }

    // Statistics
    void UpdateStats() {
        stats.totalBrushes = state.map.brushes.size();
        stats.totalEntities = state.map.entities.size();
        stats.totalTextures = state.map.textures.size();
        stats.totalVertices = 0;
        stats.totalTriangles = 0;
        
        bool first = true;
        for (const auto& brush : state.map.brushes) {
            stats.totalVertices += brush.vertices.size();
            stats.totalTriangles += brush.indices.size() / 3;
            
            for (const auto& v : brush.vertices) {
                if (first) {
                    stats.mapBoundsMin = v.position;
                    stats.mapBoundsMax = v.position;
                    first = false;
                } else {
                    stats.mapBoundsMin.x = std::min(stats.mapBoundsMin.x, v.position.x);
                    stats.mapBoundsMin.y = std::min(stats.mapBoundsMin.y, v.position.y);
                    stats.mapBoundsMin.z = std::min(stats.mapBoundsMin.z, v.position.z);
                    stats.mapBoundsMax.x = std::max(stats.mapBoundsMax.x, v.position.x);
                    stats.mapBoundsMax.y = std::max(stats.mapBoundsMax.y, v.position.y);
                    stats.mapBoundsMax.z = std::max(stats.mapBoundsMax.z, v.position.z);
                }
            }
        }
    }

    // Snap settings
    void SetRotationSnap(float angle) { rotationSnapAngle = angle; }
    void SetScaleSnap(float increment) { scaleSnapIncrement = increment; }
    void SetSnapRotation(bool snap) { snapRotation = snap; }
    void SetSnapScale(bool snap) { snapScale = snap; }
    float GetRotationSnap() const { return rotationSnapAngle; }
    float GetScaleSnap() const { return scaleSnapIncrement; }
    bool IsSnapRotation() const { return snapRotation; }
    bool IsSnapScale() const { return snapScale; }

    // Main interaction methods - FIXED BUG HERE
    void OnMouseClickWithRay(float screenX, float screenY, int screenWidth,
                             int screenHeight, float* view, float* proj, bool shift) {
        multiSelectMode = shift;
        
        if ((state.currentTool == PCD::EditorTool::MOVE ||
             state.currentTool == PCD::EditorTool::ROTATE ||
             state.currentTool == PCD::EditorTool::SCALE) &&
            (state.selectedBrushIndex >= 0 || state.selectedEntityIndex >= 0)) {

            PCD::Vec3 objectPos = GetSelectedObjectPosition();
            Ray ray = ScreenPointToRay(screenX, screenY, screenWidth, screenHeight, view, proj);
            GizmoAxis clickedAxis = GetGizmoAxisFromRay(ray, objectPos);

            if (clickedAxis != GizmoAxis::NONE) {
                isManipulating = true;
                isDragging = false;
                activeAxis = clickedAxis;
                manipulationStart = objectPos;
                objectStartPos = objectPos;
                
                // Reset accumulated deltas
                accumulatedDeltaX = 0;
                accumulatedDeltaY = 0;
                accumulatedDeltaZ = 0;
                
                startMouseX = screenX;
                startMouseY = screenY;
                lastMouseX = screenX;
                lastMouseY = screenY;

                if (state.selectedEntityIndex >= 0 &&
                    state.selectedEntityIndex < (int)state.map.entities.size()) {
                    auto& ent = state.map.entities[state.selectedEntityIndex];
                    objectStartScale = ent.scale;
                    objectStartRot = ent.rotation;
                } else if (state.selectedBrushIndex >= 0 &&
                           state.selectedBrushIndex < (int)state.map.brushes.size()) {
                    auto& brush = state.map.brushes[state.selectedBrushIndex];
                    BrushBounds bounds = GetBrushBounds(brush);
                    objectStartPos = bounds.center;
                }

                state.PushUndo();
                return;
            }
        }

        Ray ray = ScreenPointToRay(screenX, screenY, screenWidth, screenHeight, view, proj);
        float t = (state.settings.gridHeight - ray.origin.y) / ray.direction.y;
        PCD::Vec3 worldPos = ray.origin + ray.direction * t;

        OnMouseClick(worldPos.x, worldPos.y, worldPos.z, shift);
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
                    state.map, state.entityToPlace, clickPos);
                state.map.entities.push_back(ent);
                state.selectedEntityIndex = state.map.entities.size() - 1;
                state.hasUnsavedChanges = true;
            }
            UpdateStats();
            break;

        case PCD::EditorTool::SELECT:
        case PCD::EditorTool::MOVE:
        case PCD::EditorTool::ROTATE:
        case PCD::EditorTool::SCALE:
            if (!shift) {
                state.DeselectAll();
                selectedBrushIndices.clear();
                selectedEntityIndices.clear();
            }

            // Check entities first
            for (size_t i = 0; i < state.map.entities.size(); i++) {
                auto& ent = state.map.entities[i];
                float dx = ent.position.x - clickPos.x;
                float dy = ent.position.y - clickPos.y;
                float dz = ent.position.z - clickPos.z;
                float dist = sqrt(dx*dx + dy*dy + dz*dz);

                if (dist < 2.0f) {
                    if (shift) {
                        auto it = std::find(selectedEntityIndices.begin(), 
                                           selectedEntityIndices.end(), (int)i);
                        if (it != selectedEntityIndices.end()) {
                            selectedEntityIndices.erase(it);
                        } else {
                            selectedEntityIndices.push_back(i);
                        }
                    }
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
                        if (shift) {
                            auto it = std::find(selectedBrushIndices.begin(), 
                                               selectedBrushIndices.end(), (int)i);
                            if (it != selectedBrushIndices.end()) {
                                selectedBrushIndices.erase(it);
                            } else {
                                selectedBrushIndices.push_back(i);
                            }
                        }
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

    // FIXED: OnMouseDrag now correctly handles delta values
    void OnMouseDrag(float deltaX, float deltaY, float deltaZ, 
                     float screenDX, float screenDY, bool constrainAxis) {
        
        if (isManipulating && (state.currentTool == PCD::EditorTool::MOVE ||
                               state.currentTool == PCD::EditorTool::ROTATE ||
                               state.currentTool == PCD::EditorTool::SCALE)) {
            isDragging = true;

            // The values passed are DELTAS, not absolute positions
            // Apply them directly based on gizmo mode
            
            switch (state.currentTool) {
            case PCD::EditorTool::MOVE: {
                // Accumulate movement
                accumulatedDeltaX += deltaX;
                accumulatedDeltaY += deltaY;
                accumulatedDeltaZ += deltaZ;
                
                PCD::Vec3 delta(deltaX, deltaY, deltaZ);
                
                // Apply axis constraints based on active gizmo axis
                switch (activeAxis) {
                case GizmoAxis::X:
                    delta.y = 0;
                    delta.z = 0;
                    break;
                case GizmoAxis::Y:
                    delta.x = 0;
                    delta.z = 0;
                    break;
                case GizmoAxis::Z:
                    delta.x = 0;
                    delta.y = 0;
                    break;
                case GizmoAxis::XZ:
                    delta.y = 0;
                    break;
                case GizmoAxis::XY:
                    delta.z = 0;
                    break;
                case GizmoAxis::YZ:
                    delta.x = 0;
                    break;
                default:
                    break;
                }
                
                // Apply grid snapping to accumulated delta if needed
                if (state.settings.snapToGrid && (delta.x != 0 || delta.y != 0 || delta.z != 0)) {
                    // Only apply if delta is significant
                    float threshold = state.settings.gridSize * 0.1f;
                    if (std::abs(delta.x) < threshold) delta.x = 0;
                    if (std::abs(delta.y) < threshold) delta.y = 0;
                    if (std::abs(delta.z) < threshold) delta.z = 0;
                }
                
                ApplyMove(delta);
                state.hasUnsavedChanges = true;
                break;
            }
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
            PCD::Vec3 dragPos(deltaX, deltaY, deltaZ);
            
            // For creation, we need to calculate actual position
            // Add delta to previous end position
            state.createEnd = state.createEnd + dragPos;
            
            if (state.settings.snapToGrid) {
                state.createEnd = state.SnapToGrid(state.createEnd);
            }
        }
    }

    void OnMouseRelease() {
        if (isManipulating) {
            isManipulating = false;
            isDragging = false;
            activeAxis = GizmoAxis::NONE;
            accumulatedDeltaX = 0;
            accumulatedDeltaY = 0;
            accumulatedDeltaZ = 0;
            UpdateStats();
            return;
        }

        if (!state.isCreating) return;

        state.isCreating = false;

        PCD::Vec3 min(std::min(state.createStart.x, state.createEnd.x),
                      std::min(state.createStart.y, state.createEnd.y),
                      std::min(state.createStart.z, state.createEnd.z));

        PCD::Vec3 max(std::max(state.createStart.x, state.createEnd.x),
                      std::max(state.createStart.y, state.createEnd.y),
                      std::max(state.createStart.z, state.createEnd.z));

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
            PCD::Vec3 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f,
                             (min.z + max.z) * 0.5f);
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
        UpdateStats();
    }

    PCD::Brush CreateBox(const PCD::Vec3& min, const PCD::Vec3& max) {
        return PCD::BrushFactory::CreateBox(state.map, min, max);
    }

    void RenderUI() { 
        ui->Render(); 
        CheckAutoSave();
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

    PCD::Vec3 GetSelectedObjectPosition() const {
        if (state.selectedBrushIndex >= 0 &&
            state.selectedBrushIndex < (int)state.map.brushes.size()) {
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
        if (state.selectedEntityIndex >= 0 &&
            state.selectedEntityIndex < (int)state.map.entities.size()) {
            return state.map.entities[state.selectedEntityIndex].position;
        }
        return PCD::Vec3(0, 0, 0);
    }

private:
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

    Ray ScreenPointToRay(float screenX, float screenY, int screenWidth,
                         int screenHeight, float* view, float* proj) {
        float ndcX = (2.0f * screenX) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * screenY) / screenHeight;

        float invProj[16], invView[16];
        InvertMatrix(proj, invProj);
        InvertMatrix(view, invView);

        float nearClip[4] = {ndcX, ndcY, -1.0f, 1.0f};
        float nearEye[4], nearWorld[4];

        MultiplyMatrixVector(invProj, nearClip, nearEye);
        nearEye[0] /= nearEye[3];
        nearEye[1] /= nearEye[3];
        nearEye[2] /= nearEye[3];
        nearEye[3] = 1.0f;

        MultiplyMatrixVector(invView, nearEye, nearWorld);

        float farClip[4] = {ndcX, ndcY, 1.0f, 1.0f};
        float farEye[4], farWorld[4];

        MultiplyMatrixVector(invProj, farClip, farEye);
        farEye[0] /= farEye[3];
        farEye[1] /= farEye[3];
        farEye[2] /= farEye[3];
        farEye[3] = 1.0f;

        MultiplyMatrixVector(invView, farEye, farWorld);

        Ray ray;
        ray.origin = PCD::Vec3(nearWorld[0], nearWorld[1], nearWorld[2]);
        PCD::Vec3 farPoint(farWorld[0], farWorld[1], farWorld[2]);
        ray.direction = (farPoint - ray.origin).Normalized();

        return ray;
    }

    void InvertMatrix(const float* m, float* out) {
        float inv[16];

        inv[0] = m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] +
                 m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
        inv[4] = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] -
                 m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
        inv[8] = m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] +
                 m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
        inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] -
                  m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
        inv[1] = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] -
                 m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
        inv[5] = m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] +
                 m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
        inv[9] = -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] -
                 m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
        inv[13] = m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] +
                  m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
        inv[2] = m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15] +
                 m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
        inv[6] = -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15] -
                 m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
        inv[10] = m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15] +
                  m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
        inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14] -
                  m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
        inv[3] = -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11] -
                 m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
        inv[7] = m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11] +
                 m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
        inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11] -
                  m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
        inv[15] = m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10] +
                  m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

        float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];

        if (det != 0.0f) {
            det = 1.0f / det;
            for (int i = 0; i < 16; i++) {
                out[i] = inv[i] * det;
            }
        } else {
            for (int i = 0; i < 16; i++) {
                out[i] = (i % 5 == 0) ? 1.0f : 0.0f;
            }
        }
    }

    void MultiplyMatrixVector(const float* mat, const float* vec, float* out) {
        out[0] = mat[0]*vec[0] + mat[4]*vec[1] + mat[8]*vec[2] + mat[12]*vec[3];
        out[1] = mat[1]*vec[0] + mat[5]*vec[1] + mat[9]*vec[2] + mat[13]*vec[3];
        out[2] = mat[2]*vec[0] + mat[6]*vec[1] + mat[10]*vec[2] + mat[14]*vec[3];
        out[3] = mat[3]*vec[0] + mat[7]*vec[1] + mat[11]*vec[2] + mat[15]*vec[3];
    }

   float RayCylinderIntersect(const Ray& ray, const PCD::Vec3& start,
                           const PCD::Vec3& end, float radius) {
    PCD::Vec3 axis = (end - start).Normalized();
    PCD::Vec3 oc = ray.origin - start;
    float axisLen = (end - start).Length();

    float rayDotAxis = ray.direction.x*axis.x + ray.direction.y*axis.y + ray.direction.z*axis.z;
    float ocDotAxis = oc.x*axis.x + oc.y*axis.y + oc.z*axis.z;

    PCD::Vec3 rayPerp(ray.direction.x - rayDotAxis*axis.x,
                      ray.direction.y - rayDotAxis*axis.y,
                      ray.direction.z - rayDotAxis*axis.z);
    PCD::Vec3 ocPerp(oc.x - ocDotAxis*axis.x,
                     oc.y - ocDotAxis*axis.y,
                     oc.z - ocDotAxis*axis.z);

    float a = rayPerp.x*rayPerp.x + rayPerp.y*rayPerp.y + rayPerp.z*rayPerp.z;
    float b = 2.0f*(ocPerp.x*rayPerp.x + ocPerp.y*rayPerp.y + ocPerp.z*rayPerp.z);
    float c = ocPerp.x*ocPerp.x + ocPerp.y*ocPerp.y + ocPerp.z*ocPerp.z - radius*radius;

    float discriminant = b*b - 4.0f*a*c;
    if (discriminant < 0.0f) return -1.0f;

    float t = (-b - sqrt(discriminant)) / (2.0f*a);
    if (t < 0.0f) t = (-b + sqrt(discriminant)) / (2.0f*a);
    if (t < 0.0f) return -1.0f;

    PCD::Vec3 hitPoint = ray.origin + ray.direction * t;
    PCD::Vec3 toHit = hitPoint - start;
    float projLen = toHit.x*axis.x + toHit.y*axis.y + toHit.z*axis.z;

    // IMPROVED: Add tolerance at ends
    float tolerance = 0.5f; // Allow clicking slightly beyond arrow
    if (projLen < -tolerance || projLen > axisLen + tolerance) return -1.0f;

    return t;
}

    float RaySphereIntersect(const Ray& ray, const PCD::Vec3& center, float radius) {
        PCD::Vec3 oc = ray.origin - center;
        float a = ray.direction.x*ray.direction.x + ray.direction.y*ray.direction.y + 
                  ray.direction.z*ray.direction.z;
        float b = 2.0f*(oc.x*ray.direction.x + oc.y*ray.direction.y + oc.z*ray.direction.z);
        float c = oc.x*oc.x + oc.y*oc.y + oc.z*oc.z - radius*radius;

        float discriminant = b*b - 4.0f*a*c;
        if (discriminant < 0.0f) return -1.0f;

        float t = (-b - sqrt(discriminant)) / (2.0f*a);
        if (t < 0.0f) t = (-b + sqrt(discriminant)) / (2.0f*a);

        return t;
    }

    GizmoAxis GetGizmoAxisFromRay(const Ray& ray, const PCD::Vec3& objectPos) {
        float arrowLength = 3.0f;
        float arrowRadius = 0.2f;
        float centerRadius = 0.4f;

        float centerDist = RaySphereIntersect(ray, objectPos, centerRadius);
        float xDist = RayCylinderIntersect(ray, objectPos, 
            objectPos + PCD::Vec3(arrowLength, 0, 0), arrowRadius);
        float yDist = RayCylinderIntersect(ray, objectPos, 
            objectPos + PCD::Vec3(0, arrowLength, 0), arrowRadius);
        float zDist = RayCylinderIntersect(ray, objectPos, 
            objectPos + PCD::Vec3(0, 0, arrowLength), arrowRadius);

        float closestDist = 999999.0f;
        GizmoAxis closestAxis = GizmoAxis::NONE;

        if (xDist >= 0.0f && xDist < closestDist) { closestDist = xDist; closestAxis = GizmoAxis::X; }
        if (yDist >= 0.0f && yDist < closestDist) { closestDist = yDist; closestAxis = GizmoAxis::Y; }
        if (zDist >= 0.0f && zDist < closestDist) { closestDist = zDist; closestAxis = GizmoAxis::Z; }
        if (centerDist >= 0.0f && centerDist < closestDist) { closestDist = centerDist; closestAxis = GizmoAxis::XYZ; }

        return closestAxis;
    }

    void ApplyMove(const PCD::Vec3& delta) {
        if (state.selectedEntityIndex >= 0 &&
            state.selectedEntityIndex < (int)state.map.entities.size()) {
            auto& ent = state.map.entities[state.selectedEntityIndex];
            ent.position = ent.position + delta;
        }

        if (state.selectedBrushIndex >= 0 &&
            state.selectedBrushIndex < (int)state.map.brushes.size()) {
            auto& brush = state.map.brushes[state.selectedBrushIndex];
            for (auto& v : brush.vertices) {
                v.position = v.position + delta;
            }
        }
        
        // Multi-selection
        for (int idx : selectedEntityIndices) {
            if (idx != state.selectedEntityIndex && idx >= 0 && 
                idx < (int)state.map.entities.size()) {
                state.map.entities[idx].position = state.map.entities[idx].position + delta;
            }
        }
        
        for (int idx : selectedBrushIndices) {
            if (idx != state.selectedBrushIndex && idx >= 0 && 
                idx < (int)state.map.brushes.size()) {
                for (auto& v : state.map.brushes[idx].vertices) {
                    v.position = v.position + delta;
                }
            }
        }
    }

    void ApplyRotation(float screenDX, float screenDY) {
        float rotSpeed = 0.5f;
        float rotX = screenDY * rotSpeed;
        float rotY = screenDX * rotSpeed;
        
        if (snapRotation) {
            rotX = std::round(rotX / rotationSnapAngle) * rotationSnapAngle;
            rotY = std::round(rotY / rotationSnapAngle) * rotationSnapAngle;
        }
        
        if (state.selectedEntityIndex >= 0 &&
            state.selectedEntityIndex < (int)state.map.entities.size()) {
            auto& ent = state.map.entities[state.selectedEntityIndex];
            
            switch (activeAxis) {
            case GizmoAxis::X: ent.rotation.x += rotX; break;
            case GizmoAxis::Y: ent.rotation.y += rotY; break;
            case GizmoAxis::Z: ent.rotation.z += rotX; break;
            default:
                ent.rotation.y += rotY;
                ent.rotation.x += rotX;
                break;
            }
        }
        
        // Rotate brush around center
        if (state.selectedBrushIndex >= 0 &&
            state.selectedBrushIndex < (int)state.map.brushes.size()) {
            auto& brush = state.map.brushes[state.selectedBrushIndex];
            BrushBounds bounds = GetBrushBounds(brush);
            
            float angle = rotY * 0.01f;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);
            
            for (auto& v : brush.vertices) {
                float relX = v.position.x - bounds.center.x;
                float relZ = v.position.z - bounds.center.z;
                
                v.position.x = bounds.center.x + relX * cosA - relZ * sinA;
                v.position.z = bounds.center.z + relX * sinA + relZ * cosA;
                
                // Rotate normal
                float nrelX = v.normal.x;
                float nrelZ = v.normal.z;
                v.normal.x = nrelX * cosA - nrelZ * sinA;
                v.normal.z = nrelX * sinA + nrelZ * cosA;
            }
        }
    }

    void ApplyScale(float screenDX, float screenDY) {
        float scaleDelta = (screenDX + screenDY) * 0.005f;
        float scaleFactor = 1.0f + scaleDelta;
        scaleFactor = std::max(0.95f, std::min(1.05f, scaleFactor));
        
        if (snapScale) {
            scaleFactor = std::round(scaleFactor / scaleSnapIncrement) * scaleSnapIncrement;
        }

        if (state.selectedEntityIndex >= 0 &&
            state.selectedEntityIndex < (int)state.map.entities.size()) {
            auto& ent = state.map.entities[state.selectedEntityIndex];

            switch (activeAxis) {
            case GizmoAxis::X:
                ent.scale.x = std::max(0.1f, ent.scale.x * scaleFactor);
                break;
            case GizmoAxis::Y:
                ent.scale.y = std::max(0.1f, ent.scale.y * scaleFactor);
                break;
            case GizmoAxis::Z:
                ent.scale.z = std::max(0.1f, ent.scale.z * scaleFactor);
                break;
            default:
                ent.scale.x = std::max(0.1f, ent.scale.x * scaleFactor);
                ent.scale.y = std::max(0.1f, ent.scale.y * scaleFactor);
                ent.scale.z = std::max(0.1f, ent.scale.z * scaleFactor);
                break;
            }
        }

        if (state.selectedBrushIndex >= 0 &&
            state.selectedBrushIndex < (int)state.map.brushes.size()) {
            auto& brush = state.map.brushes[state.selectedBrushIndex];
            BrushBounds bounds = GetBrushBounds(brush);

            for (auto& v : brush.vertices) {
                PCD::Vec3 offset = v.position - bounds.center;

                switch (activeAxis) {
                case GizmoAxis::X: offset.x *= scaleFactor; break;
                case GizmoAxis::Y: offset.y *= scaleFactor; break;
                case GizmoAxis::Z: offset.z *= scaleFactor; break;
                default: offset = offset * scaleFactor; break;
                }

                v.position = bounds.center + offset;
            }
        }
    }
};

} // namespace Editor

#endif // MAP_EDITOR_H
