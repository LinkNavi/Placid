#ifndef RENDERER_H
#define RENDERER_H

#include <glad/gl.h>
#include <vector>

// Forward declarations
struct Vec3;
namespace PCD {
    struct EditorSettings;
    struct Brush;
    struct Entity;
    struct Vec3;
    enum class EditorTool;
}

class Renderer {
private:
    GLuint shaderProgram;
    GLuint vao, vbo, ebo;
    
public:
    Renderer();
    ~Renderer();
    
    bool Initialize();
    void Shutdown();
    
    // Rendering functions
    void RenderGrid(const PCD::EditorSettings& settings, const PCD::Vec3& target, float* view, float* proj);
    void RenderBrushes(const std::vector<PCD::Brush>& brushes, int selectedIdx, float* view, float* proj);
    void RenderEntities(const std::vector<PCD::Entity>& entities, int selectedIdx, bool showIcons, float* view, float* proj);
    void RenderCreationPreview(const PCD::Vec3& start, const PCD::Vec3& end, float gridSize, float* view, float* proj);
    void RenderGizmo(const PCD::Vec3& position, PCD::EditorTool tool, int activeAxis, float* view, float* proj);
    
private:
    GLuint CompileShader(GLenum type, const char* src);
    void SetIdentityMatrix(float* mat);
    void RenderArrow(const PCD::Vec3& pos, const PCD::Vec3& dir, float r, float g, float b, bool highlight, float* view, float* proj);
    void RenderCube(const PCD::Vec3& pos, float size, float r, float g, float b, float* view, float* proj);
};

#endif // RENDERER_H
