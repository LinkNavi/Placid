#ifndef MAP_EDITOR_H
#define MAP_EDITOR_H

#include "Engine/Renderer.h"
#include "PCD/PCD.h"
#include <cmath>

namespace Editor {

enum class GizmoAxis { NONE, X, Y, Z, XY, XZ, YZ, XYZ };

class MapEditor {
private:
  PCD::EditorState state;
  PCD::EditorUI *ui;

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
  GizmoAxis GetGizmoAxisAtPosition(const PCD::Vec3 &clickPos,
                                   const PCD::Vec3 &objectPos, float *view,
                                   float *proj) {
    // Check each axis arrow
    float threshold = 0.5f; // Distance threshold for clicking on arrow

    // X axis (red) - extends along positive X
    PCD::Vec3 xEnd = objectPos + PCD::Vec3(3, 0, 0);
    if (DistanceToLineSegment(clickPos, objectPos, xEnd) < threshold) {
      return GizmoAxis::X;
    }

    // Y axis (green) - extends along positive Y
    PCD::Vec3 yEnd = objectPos + PCD::Vec3(0, 3, 0);
    if (DistanceToLineSegment(clickPos, objectPos, yEnd) < threshold) {
      return GizmoAxis::Y;
    }

    // Z axis (blue) - extends along positive Z
    PCD::Vec3 zEnd = objectPos + PCD::Vec3(0, 0, 3);
    if (DistanceToLineSegment(clickPos, objectPos, zEnd) < threshold) {
      return GizmoAxis::Z;
    }

    // Check center cube for XYZ movement
    float centerDist =
        sqrt((clickPos.x - objectPos.x) * (clickPos.x - objectPos.x) +
             (clickPos.y - objectPos.y) * (clickPos.y - objectPos.y) +
             (clickPos.z - objectPos.z) * (clickPos.z - objectPos.z));

    if (centerDist < 0.5f) {
      return GizmoAxis::XYZ;
    }

    return GizmoAxis::NONE;
  }
  struct Ray {
    PCD::Vec3 origin;
    PCD::Vec3 direction;
  };

  // Create a ray from screen coordinates
  Ray ScreenPointToRay(float screenX, float screenY, int screenWidth,
                       int screenHeight, float *view, float *proj) {
    // Convert screen to NDC
    float ndcX = (2.0f * screenX) / screenWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY) / screenHeight;

    // Unproject near and far points
    float invProj[16], invView[16];
    InvertMatrix(proj, invProj);
    InvertMatrix(view, invView);

    // Near point (NDC z = -1)
    float nearClip[4] = {ndcX, ndcY, -1.0f, 1.0f};
    float nearEye[4], nearWorld[4];

    MultiplyMatrixVector(invProj, nearClip, nearEye);
    nearEye[0] /= nearEye[3];
    nearEye[1] /= nearEye[3];
    nearEye[2] /= nearEye[3];
    nearEye[3] = 1.0f;

    MultiplyMatrixVector(invView, nearEye, nearWorld);

    // Far point (NDC z = 1)
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

  // Helper to invert a 4x4 matrix
  void InvertMatrix(const float *m, float *out) {
    float inv[16];

    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] +
             m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] +
             m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] +
             m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] +
             m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] +
              m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] +
              m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] +
             m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] +
             m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] +
             m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] -
             m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] +
              m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] +
             m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] -
             m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] +
              m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] -
              m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] -
             m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] +
             m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] -
              m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] +
              m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det != 0.0f) {
      det = 1.0f / det;
      for (int i = 0; i < 16; i++) {
        out[i] = inv[i] * det;
      }
    } else {
      // Identity if singular
      for (int i = 0; i < 16; i++) {
        out[i] = (i % 5 == 0) ? 1.0f : 0.0f;
      }
    }
  }

  void MultiplyMatrixVector(const float *mat, const float *vec, float *out) {
    out[0] =
        mat[0] * vec[0] + mat[4] * vec[1] + mat[8] * vec[2] + mat[12] * vec[3];
    out[1] =
        mat[1] * vec[0] + mat[5] * vec[1] + mat[9] * vec[2] + mat[13] * vec[3];
    out[2] =
        mat[2] * vec[0] + mat[6] * vec[1] + mat[10] * vec[2] + mat[14] * vec[3];
    out[3] =
        mat[3] * vec[0] + mat[7] * vec[1] + mat[11] * vec[2] + mat[15] * vec[3];
  }

  // Check ray intersection with cylinder (for arrow shafts)
  float RayCylinderIntersect(const Ray &ray, const PCD::Vec3 &start,
                             const PCD::Vec3 &end, float radius) {
    PCD::Vec3 axis = (end - start).Normalized();
    PCD::Vec3 oc = ray.origin - start;

    float axisLen = (end - start).Length();

    // Project onto axis
    float rayDotAxis = ray.direction.x * axis.x + ray.direction.y * axis.y +
                       ray.direction.z * axis.z;
    float ocDotAxis = oc.x * axis.x + oc.y * axis.y + oc.z * axis.z;

    PCD::Vec3 rayPerp(ray.direction.x - rayDotAxis * axis.x,
                      ray.direction.y - rayDotAxis * axis.y,
                      ray.direction.z - rayDotAxis * axis.z);

    PCD::Vec3 ocPerp(oc.x - ocDotAxis * axis.x, oc.y - ocDotAxis * axis.y,
                     oc.z - ocDotAxis * axis.z);

    float a =
        rayPerp.x * rayPerp.x + rayPerp.y * rayPerp.y + rayPerp.z * rayPerp.z;
    float b = 2.0f * (ocPerp.x * rayPerp.x + ocPerp.y * rayPerp.y +
                      ocPerp.z * rayPerp.z);
    float c = ocPerp.x * ocPerp.x + ocPerp.y * ocPerp.y + ocPerp.z * ocPerp.z -
              radius * radius;

    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
      return -1.0f; // No intersection
    }

    float t = (-b - sqrt(discriminant)) / (2.0f * a);
    if (t < 0.0f) {
      t = (-b + sqrt(discriminant)) / (2.0f * a);
    }

    if (t < 0.0f) {
      return -1.0f;
    }

    // Check if intersection is within cylinder bounds
    PCD::Vec3 hitPoint = ray.origin + ray.direction * t;
    PCD::Vec3 toHit = hitPoint - start;
    float projLen = toHit.x * axis.x + toHit.y * axis.y + toHit.z * axis.z;

    if (projLen < 0.0f || projLen > axisLen) {
      return -1.0f;
    }

    return t;
  }

  // Check ray intersection with sphere (for center gizmo)
  float RaySphereIntersect(const Ray &ray, const PCD::Vec3 &center,
                           float radius) {
    PCD::Vec3 oc = ray.origin - center;
    float a = ray.direction.x * ray.direction.x +
              ray.direction.y * ray.direction.y +
              ray.direction.z * ray.direction.z;
    float b = 2.0f * (oc.x * ray.direction.x + oc.y * ray.direction.y +
                      oc.z * ray.direction.z);
    float c = oc.x * oc.x + oc.y * oc.y + oc.z * oc.z - radius * radius;

    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
      return -1.0f;
    }

    float t = (-b - sqrt(discriminant)) / (2.0f * a);
    if (t < 0.0f) {
      t = (-b + sqrt(discriminant)) / (2.0f * a);
    }

    return t;
  }

  // Determine which gizmo axis was clicked
  GizmoAxis GetGizmoAxisFromRay(const Ray &ray, const PCD::Vec3 &objectPos) {
    float arrowLength = 3.0f;
    float arrowRadius = 0.15f;
    float centerRadius = 0.3f;

    // Check center sphere first (highest priority for XYZ movement)
    float centerDist = RaySphereIntersect(ray, objectPos, centerRadius);

    // Check each axis arrow
    float xDist = RayCylinderIntersect(
        ray, objectPos, objectPos + PCD::Vec3(arrowLength, 0, 0), arrowRadius);
    float yDist = RayCylinderIntersect(
        ray, objectPos, objectPos + PCD::Vec3(0, arrowLength, 0), arrowRadius);
    float zDist = RayCylinderIntersect(
        ray, objectPos, objectPos + PCD::Vec3(0, 0, arrowLength), arrowRadius);

    // Find closest intersection
    float closestDist = 999999.0f;
    GizmoAxis closestAxis = GizmoAxis::NONE;

    if (xDist >= 0.0f && xDist < closestDist) {
      closestDist = xDist;
      closestAxis = GizmoAxis::X;
    }
    if (yDist >= 0.0f && yDist < closestDist) {
      closestDist = yDist;
      closestAxis = GizmoAxis::Y;
    }
    if (zDist >= 0.0f && zDist < closestDist) {
      closestDist = zDist;
      closestAxis = GizmoAxis::Z;
    }
    if (centerDist >= 0.0f && centerDist < closestDist) {
      closestDist = centerDist;
      closestAxis = GizmoAxis::XYZ;
    }

    return closestAxis;
  }
  float DistanceToLineSegment(const PCD::Vec3 &point,
                              const PCD::Vec3 &lineStart,
                              const PCD::Vec3 &lineEnd) {
    PCD::Vec3 line = lineEnd - lineStart;
    PCD::Vec3 pointVec = point - lineStart;

    float lineLen = line.Length();
    if (lineLen < 0.001f) {
      return pointVec.Length();
    }

    // Project point onto line
    float t =
        (pointVec.x * line.x + pointVec.y * line.y + pointVec.z * line.z) /
        (lineLen * lineLen);
    t = std::max(0.0f, std::min(1.0f, t)); // Clamp to segment

    PCD::Vec3 projection = lineStart + line * t;
    PCD::Vec3 diff = point - projection;
    return diff.Length();
  }

public:
  explicit MapEditor()
      : isManipulating(false), isDragging(false), activeAxis(GizmoAxis::NONE),
        startMouseX(0), startMouseY(0), lastMouseX(0), lastMouseY(0) {
    ui = new PCD::EditorUI(state);
    state.map.name = "NewMap";
    state.map.author = "Unknown";
  }

  ~MapEditor() { delete ui; }

  // Core functionality
  PCD::Map &GetMap() { return state.map; }
  const PCD::Map &GetMap() const { return state.map; }
  PCD::EditorSettings &GetSettings() { return state.settings; }
  const PCD::EditorSettings &GetSettings() const { return state.settings; }

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
  void OnMouseClickWithRay(float screenX, float screenY, int screenWidth,
                           int screenHeight, float *view, float *proj,
                           bool shift) {
    // Check if we're using a manipulation tool and have something selected
    if ((state.currentTool == PCD::EditorTool::MOVE ||
         state.currentTool == PCD::EditorTool::ROTATE ||
         state.currentTool == PCD::EditorTool::SCALE) &&
        (state.selectedBrushIndex >= 0 || state.selectedEntityIndex >= 0)) {

      PCD::Vec3 objectPos = GetSelectedObjectPosition();
      Ray ray = ScreenPointToRay(screenX, screenY, screenWidth, screenHeight,
                                 view, proj);

      // Check if clicking on gizmo
      GizmoAxis clickedAxis = GetGizmoAxisFromRay(ray, objectPos);

      if (clickedAxis != GizmoAxis::NONE) {
        // Start manipulation on specific axis
        isManipulating = true;
        isDragging = false;
        activeAxis = clickedAxis;

        manipulationStart = objectPos;
        objectStartPos = objectPos;

        startMouseX = screenX;
        startMouseY = screenY;
        lastMouseX = screenX;
        lastMouseY = screenY;

        // Store initial state
        if (state.selectedEntityIndex >= 0 &&
            state.selectedEntityIndex < (int)state.map.entities.size()) {
          auto &ent = state.map.entities[state.selectedEntityIndex];
          objectStartScale = ent.scale;
          objectStartRot = ent.rotation;
        } else if (state.selectedBrushIndex >= 0 &&
                   state.selectedBrushIndex < (int)state.map.brushes.size()) {
          auto &brush = state.map.brushes[state.selectedBrushIndex];
          BrushBounds bounds = GetBrushBounds(brush);
          objectStartPos = bounds.center;
        }

        state.PushUndo();
        return;
      }
    }

    // If not clicking on gizmo, fall back to regular click handling
    // Convert ray to approximate world position (use ground plane)
    Ray ray = ScreenPointToRay(screenX, screenY, screenWidth, screenHeight,
                               view, proj);

    // Intersect with ground plane (y = gridHeight)
    float t = (state.settings.gridHeight - ray.origin.y) / ray.direction.y;
    PCD::Vec3 worldPos = ray.origin + ray.direction * t;

    OnMouseClick(worldPos.x, worldPos.y, worldPos.z, shift);
  }
  PCD::Vec3 GetSelectedObjectPosition() const {
    if (state.selectedBrushIndex >= 0 &&
        state.selectedBrushIndex < (int)state.map.brushes.size()) {
      auto &brush = state.map.brushes[state.selectedBrushIndex];
      if (!brush.vertices.empty()) {
        PCD::Vec3 center(0, 0, 0);
        for (auto &v : brush.vertices) {
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

  BrushBounds GetBrushBounds(const PCD::Brush &brush) const {
    BrushBounds bounds;
    if (brush.vertices.empty())
      return bounds;

    bounds.min = brush.vertices[0].position;
    bounds.max = brush.vertices[0].position;

    for (const auto &v : brush.vertices) {
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

      // Get object position
      PCD::Vec3 objectPos = GetSelectedObjectPosition();

      // Check if clicking on a gizmo axis
      // Note: For now, use simple 2D projection. In EditorApp, we'll pass
      // proper ray info
      GizmoAxis clickedAxis =
          GetGizmoAxisAtPosition(clickPos, objectPos, nullptr, nullptr);

      if (clickedAxis != GizmoAxis::NONE) {
        // Start manipulation on specific axis
        isManipulating = true;
        isDragging = false;
        activeAxis = clickedAxis;
      } else {
        // Start free manipulation
        isManipulating = true;
        isDragging = false;
        activeAxis = shift ? GizmoAxis::XZ : GizmoAxis::XYZ;
      }

      manipulationStart = clickPos;
      objectStartPos = objectPos;

      // Store initial mouse position
      startMouseX = worldX;
      startMouseY = worldY;
      lastMouseX = worldX;
      lastMouseY = worldY;

      // Store initial state
      if (state.selectedEntityIndex >= 0 &&
          state.selectedEntityIndex < (int)state.map.entities.size()) {
        auto &ent = state.map.entities[state.selectedEntityIndex];
        objectStartScale = ent.scale;
        objectStartRot = ent.rotation;
      } else if (state.selectedBrushIndex >= 0 &&
                 state.selectedBrushIndex < (int)state.map.brushes.size()) {
        auto &brush = state.map.brushes[state.selectedBrushIndex];
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
            state.map, state.entityToPlace, clickPos);
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
        auto &ent = state.map.entities[i];
        float dx = ent.position.x - clickPos.x;
        float dy = ent.position.y - clickPos.y;
        float dz = ent.position.z - clickPos.z;
        float dist = sqrt(dx * dx + dy * dy + dz * dz);

        if (dist < 2.0f) {
          state.selectedEntityIndex = i;
          state.selectedBrushIndex = -1;
          return;
        }
      }

      // Check brushes
      for (size_t i = 0; i < state.map.brushes.size(); i++) {
        auto &brush = state.map.brushes[i];

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

  void OnMouseDrag(float worldX, float worldY, float worldZ, float screenDX,
                   float screenDY, bool constrainAxis) {
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
          delta.x = std::round(delta.x / state.settings.gridSize) *
                    state.settings.gridSize;
        } else {
          delta.x = 0;
        }
        if (fabs(delta.y) > threshold) {
          delta.y = std::round(delta.y / state.settings.gridSize) *
                    state.settings.gridSize;
        } else {
          delta.y = 0;
        }
        if (fabs(delta.z) > threshold) {
          delta.z = std::round(delta.z / state.settings.gridSize) *
                    state.settings.gridSize;
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

    if (!state.isCreating)
      return;

    state.isCreating = false;

    PCD::Vec3 min(std::min(state.createStart.x, state.createEnd.x),
                  std::min(state.createStart.y, state.createEnd.y),
                  std::min(state.createStart.z, state.createEnd.z));

    PCD::Vec3 max(std::max(state.createStart.x, state.createEnd.x),
                  std::max(state.createStart.y, state.createEnd.y),
                  std::max(state.createStart.z, state.createEnd.z));

    if (max.x - min.x < 0.1f)
      max.x = min.x + state.settings.gridSize;
    if (max.y - min.y < 0.1f)
      max.y = min.y + state.settings.gridSize * 2;
    if (max.z - min.z < 0.1f)
      max.z = min.z + state.settings.gridSize;

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
      newBrush =
          PCD::BrushFactory::CreateCylinder(state.map, center, radius, height);
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

  PCD::Brush CreateBox(const PCD::Vec3 &min, const PCD::Vec3 &max) {
    return PCD::BrushFactory::CreateBox(state.map, min, max);
  }

  void RenderUI() { ui->Render(); }

  void RenderGizmo(Renderer *renderer, float *view, float *proj) {
    if (state.selectedBrushIndex < 0 && state.selectedEntityIndex < 0)
      return;

    if (state.currentTool != PCD::EditorTool::MOVE &&
        state.currentTool != PCD::EditorTool::ROTATE &&
        state.currentTool != PCD::EditorTool::SCALE)
      return;

    PCD::Vec3 pos = GetSelectedObjectPosition();
    int axisInt = static_cast<int>(activeAxis);
    renderer->RenderGizmo(pos, state.currentTool, axisInt, view, proj);
  }

private:
  float lastMouseZ = 0; // Add this member

  void ApplyMove(const PCD::Vec3 &delta) {
    // Apply axis constraints
    PCD::Vec3 constrainedDelta = delta;

    switch (activeAxis) {
    case GizmoAxis::X:
      constrainedDelta.y = 0;
      constrainedDelta.z = 0;
      break;
    case GizmoAxis::Y:
      constrainedDelta.x = 0;
      constrainedDelta.z = 0;
      break;
    case GizmoAxis::Z:
      constrainedDelta.x = 0;
      constrainedDelta.y = 0;
      break;
    case GizmoAxis::XZ:
      constrainedDelta.y = 0;
      break;
    case GizmoAxis::XY:
      constrainedDelta.z = 0;
      break;
    case GizmoAxis::YZ:
      constrainedDelta.x = 0;
      break;
    case GizmoAxis::XYZ:
    case GizmoAxis::NONE:
      // No constraints
      break;
    }

    // Move entity
    if (state.selectedEntityIndex >= 0 &&
        state.selectedEntityIndex < (int)state.map.entities.size()) {
      auto &ent = state.map.entities[state.selectedEntityIndex];
      ent.position = ent.position + constrainedDelta;
    }

    // Move brush
    if (state.selectedBrushIndex >= 0 &&
        state.selectedBrushIndex < (int)state.map.brushes.size()) {
      auto &brush = state.map.brushes[state.selectedBrushIndex];
      for (auto &v : brush.vertices) {
        v.position = v.position + constrainedDelta;
      }
    }
  }

  void ApplyRotation(float screenDX, float screenDY) {
    if (state.selectedEntityIndex >= 0 &&
        state.selectedEntityIndex < (int)state.map.entities.size()) {
      auto &ent = state.map.entities[state.selectedEntityIndex];

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

    // Clamp scale factor
    scaleFactor = std::max(0.9f, std::min(1.1f, scaleFactor));

    // Scale entity
    if (state.selectedEntityIndex >= 0 &&
        state.selectedEntityIndex < (int)state.map.entities.size()) {
      auto &ent = state.map.entities[state.selectedEntityIndex];

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
      case GizmoAxis::XYZ:
      case GizmoAxis::NONE:
        ent.scale.x = std::max(0.1f, ent.scale.x * scaleFactor);
        ent.scale.y = std::max(0.1f, ent.scale.y * scaleFactor);
        ent.scale.z = std::max(0.1f, ent.scale.z * scaleFactor);
        break;
      default:
        break;
      }
    }

    // Scale brush
    if (state.selectedBrushIndex >= 0 &&
        state.selectedBrushIndex < (int)state.map.brushes.size()) {
      auto &brush = state.map.brushes[state.selectedBrushIndex];
      BrushBounds bounds = GetBrushBounds(brush);
      PCD::Vec3 center = bounds.center;

      for (auto &v : brush.vertices) {
        PCD::Vec3 offset = v.position - center;

        switch (activeAxis) {
        case GizmoAxis::X:
          offset.x *= scaleFactor;
          break;
        case GizmoAxis::Y:
          offset.y *= scaleFactor;
          break;
        case GizmoAxis::Z:
          offset.z *= scaleFactor;
          break;
        case GizmoAxis::XYZ:
        case GizmoAxis::NONE:
          offset = offset * scaleFactor;
          break;
        default:
          break;
        }

        v.position = center + offset;
      }
    }
  }
};

} // namespace Editor

#endif // MAP_EDITOR_H
