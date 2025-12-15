#include "Engine/Renderer.h"
#include "Engine/Camera.h"
#include "PCD/PCD.h"
#include <iostream>
#include <cmath>
#include <algorithm>

static const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 vertexColor;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

static const char* fragmentShaderSrc = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

Renderer::Renderer() : shaderProgram(0), vao(0), vbo(0), ebo(0) {}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize() {
    // Compile shaders
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, log);
        std::cerr << "Shader linking error:\n" << log << std::endl;
        return false;
    }
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    // Create buffers
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    return true;
}

void Renderer::Shutdown() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

GLuint Renderer::CompileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compilation error:\n" << log << std::endl;
    }
    
    return shader;
}

void Renderer::SetIdentityMatrix(float* mat) {
    for (int i = 0; i < 16; i++) 
        mat[i] = (i % 5 == 0) ? 1.0f : 0.0f;
}

void Renderer::RenderGrid(const PCD::EditorSettings& settings, const PCD::Vec3& target, float* view, float* proj) {
    if (!settings.showGrid) return;
    
    std::vector<float> gridVerts;
    float extent = settings.gridExtent;
    float step = settings.gridSize;
    float y = settings.gridHeight;
    
    // Grid lines
    for (float x = -extent; x <= extent; x += step) {
        gridVerts.insert(gridVerts.end(), {x, y, -extent, 0.3f, 0.3f, 0.3f});
        gridVerts.insert(gridVerts.end(), {x, y, extent, 0.3f, 0.3f, 0.3f});
    }
    for (float z = -extent; z <= extent; z += step) {
        gridVerts.insert(gridVerts.end(), {-extent, y, z, 0.3f, 0.3f, 0.3f});
        gridVerts.insert(gridVerts.end(), {extent, y, z, 0.3f, 0.3f, 0.3f});
    }
    
    // Axes (X=red, Z=blue, Y=green)
    gridVerts.insert(gridVerts.end(), {-extent, y, 0, 1.0f, 0.2f, 0.2f});
    gridVerts.insert(gridVerts.end(), {extent, y, 0, 1.0f, 0.2f, 0.2f});
    gridVerts.insert(gridVerts.end(), {0, y, -extent, 0.2f, 0.2f, 1.0f});
    gridVerts.insert(gridVerts.end(), {0, y, extent, 0.2f, 0.2f, 1.0f});
    gridVerts.insert(gridVerts.end(), {0, -extent, 0, 0.2f, 1.0f, 0.2f});
    gridVerts.insert(gridVerts.end(), {0, extent, 0, 0.2f, 1.0f, 0.2f});
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, gridVerts.size() * sizeof(float), gridVerts.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glUseProgram(shaderProgram);
    float model[16];
    SetIdentityMatrix(model);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, proj);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model);
    
    glDrawArrays(GL_LINES, 0, gridVerts.size() / 6);
}

void Renderer::RenderBrushes(const std::vector<PCD::Brush>& brushes, int selectedIdx, float* view, float* proj) {
    for (size_t i = 0; i < brushes.size(); i++) {
        const auto& brush = brushes[i];
        
        std::vector<float> verts;
        for (const auto& v : brush.vertices) {
            float r = brush.color.x, g = brush.color.y, b = brush.color.z;
            
            // Highlight selected
            if ((int)i == selectedIdx) {
                r = 1.0f; g = 0.8f; b = 0.2f;
            }
            
            // Color by brush type
            if (brush.flags & PCD::BRUSH_TRIGGER) { r = 0.8f; g = 0.2f; b = 0.8f; }
            if (brush.flags & PCD::BRUSH_WATER) { r = 0.2f; g = 0.4f; b = 0.8f; }
            if (brush.flags & PCD::BRUSH_LAVA) { r = 0.9f; g = 0.3f; b = 0.1f; }
            if (brush.flags & PCD::BRUSH_CLIP) { r = 0.5f; g = 0.5f; b = 0.0f; }
            
            verts.push_back(v.position.x);
            verts.push_back(v.position.y);
            verts.push_back(v.position.z);
            verts.push_back(r);
            verts.push_back(g);
            verts.push_back(b);
        }
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, brush.indices.size() * sizeof(unsigned int), 
                     brush.indices.data(), GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glUseProgram(shaderProgram);
        float model[16];
        SetIdentityMatrix(model);
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, proj);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model);
        
        // Wireframe for selected
        if ((int)i == selectedIdx) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(2.0f);
        }
        
        glDrawElements(GL_TRIANGLES, brush.indices.size(), GL_UNSIGNED_INT, 0);
        
        if ((int)i == selectedIdx) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }
}

void Renderer::RenderEntities(const std::vector<PCD::Entity>& entities, int selectedIdx, 
                               bool showIcons, float* view, float* proj) {
    if (!showIcons) return;
    
    float boxSize = 0.5f;
    float boxVerts[] = {
        -boxSize, 0, -boxSize,  0, 0, 0,
         boxSize, 0, -boxSize,  0, 0, 0,
         boxSize, boxSize*2, -boxSize,  0, 0, 0,
        -boxSize, boxSize*2, -boxSize,  0, 0, 0,
        -boxSize, 0,  boxSize,  0, 0, 0,
         boxSize, 0,  boxSize,  0, 0, 0,
         boxSize, boxSize*2,  boxSize,  0, 0, 0,
        -boxSize, boxSize*2,  boxSize,  0, 0, 0,
    };
    
    unsigned int boxIndices[] = {
        0,1,2, 0,2,3, 4,5,6, 4,6,7,
        0,4,7, 0,7,3, 1,5,6, 1,6,2,
        3,2,6, 3,6,7, 0,1,5, 0,5,4
    };
    
    for (size_t i = 0; i < entities.size(); i++) {
        const auto& ent = entities[i];
        
        float r = 0.5f, g = 0.5f, b = 0.5f;
        
        // Color by type
        if ((int)i == selectedIdx) {
            r = 1.0f; g = 0.9f; b = 0.3f;
        } else {
            switch (ent.type) {
                case PCD::ENT_INFO_PLAYER_START:
                case PCD::ENT_INFO_PLAYER_DEATHMATCH:
                    r = 0.2f; g = 1.0f; b = 0.2f; break;
                case PCD::ENT_INFO_TEAM_SPAWN_RED:
                    r = 1.0f; g = 0.2f; b = 0.2f; break;
                case PCD::ENT_INFO_TEAM_SPAWN_BLUE:
                    r = 0.2f; g = 0.2f; b = 1.0f; break;
                case PCD::ENT_LIGHT:
                case PCD::ENT_LIGHT_SPOT:
                case PCD::ENT_LIGHT_ENV:
                    r = 1.0f; g = 1.0f; b = 0.5f; break;
                case PCD::ENT_ITEM_HEALTH:
                    r = 1.0f; g = 0.3f; b = 0.3f; break;
                case PCD::ENT_ITEM_ARMOR:
                    r = 0.3f; g = 0.6f; b = 1.0f; break;
                default: break;
            }
        }
        
        std::vector<float> coloredVerts;
        for (int v = 0; v < 8; v++) {
            coloredVerts.push_back(boxVerts[v*6 + 0]);
            coloredVerts.push_back(boxVerts[v*6 + 1]);
            coloredVerts.push_back(boxVerts[v*6 + 2]);
            coloredVerts.push_back(r);
            coloredVerts.push_back(g);
            coloredVerts.push_back(b);
        }
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, coloredVerts.size() * sizeof(float), 
                     coloredVerts.data(), GL_DYNAMIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(boxIndices), boxIndices, GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glUseProgram(shaderProgram);
        float model[16];
        SetIdentityMatrix(model);
        model[12] = ent.position.x;
        model[13] = ent.position.y;
        model[14] = ent.position.z;
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, proj);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model);
        
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
}

void Renderer::RenderCreationPreview(const PCD::Vec3& start, const PCD::Vec3& end, 
                                      float gridSize, float* view, float* proj) {
    float minX = std::min(start.x, end.x);
    float minY = std::min(start.y, end.y);
    float minZ = std::min(start.z, end.z);
    float maxX = std::max(start.x, end.x);
    float maxY = std::max(start.y, end.y);
    float maxZ = std::max(start.z, end.z);
    
    if (maxY - minY < 0.1f) maxY = minY + gridSize * 2;
    
    float previewVerts[] = {
        minX, minY, minZ,  0.5f, 0.8f, 1.0f,
        maxX, minY, minZ,  0.5f, 0.8f, 1.0f,
        maxX, maxY, minZ,  0.5f, 0.8f, 1.0f,
        minX, maxY, minZ,  0.5f, 0.8f, 1.0f,
        minX, minY, maxZ,  0.5f, 0.8f, 1.0f,
        maxX, minY, maxZ,  0.5f, 0.8f, 1.0f,
        maxX, maxY, maxZ,  0.5f, 0.8f, 1.0f,
        minX, maxY, maxZ,  0.5f, 0.8f, 1.0f,
    };
    
    unsigned int lineIndices[] = {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(previewVerts), previewVerts, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(lineIndices), lineIndices, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glUseProgram(shaderProgram);
    float model[16];
    SetIdentityMatrix(model);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, proj);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model);
    
    glLineWidth(2.0f);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glLineWidth(1.0f);
}
