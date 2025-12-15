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
    
private:
    GLuint CompileShader(GLenum type, const char* src);
    void SetIdentityMatrix(float* mat);
};

#endif // RENDERER_H
