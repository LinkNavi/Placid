#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "Engine/Camera.h"
#include "Engine/PCD.h"
#include "Game/KeybindManager.h"
#include "imgui.h"
#include <iostream>
#include <cmath>

// Application modes
enum class AppMode {
    GAME,
    EDITOR
};

// Global state
Camera camera;
KeybindManager keybinds;
Editor::MapEditor* mapEditor = nullptr;
AppMode currentMode = AppMode::EDITOR;  // Start in editor mode
double lastX = 640, lastY = 360;
bool firstMouse = true;
bool cursorCaptured = false;

// Editor camera controls
float editorCamDist = 30.0f;
float editorCamYaw = 0.45f;
float editorCamPitch = 0.6f;
Vec3 editorCamTarget(0, 0, 0);

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (currentMode == AppMode::GAME) {
        keybinds.OnKeyEvent(key, action);
    }
    
    if (action == GLFW_PRESS) {
        // Toggle between modes with F1
        if (key == GLFW_KEY_F1) {
            currentMode = (currentMode == AppMode::GAME) ? AppMode::EDITOR : AppMode::GAME;
            if (currentMode == AppMode::GAME) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                cursorCaptured = true;
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                cursorCaptured = false;
            }
            firstMouse = true;
        }
        
        // Editor hotkeys
        if (currentMode == AppMode::EDITOR && mapEditor) {
            if (mods & GLFW_MOD_CONTROL) {
                if (key == GLFW_KEY_N) mapEditor->NewMap();
                if (key == GLFW_KEY_S) mapEditor->SaveMap();
                if (key == GLFW_KEY_Z) mapEditor->Undo();
                if (key == GLFW_KEY_Y) mapEditor->Redo();
                if (key == GLFW_KEY_D) mapEditor->DuplicateSelected();
                if (key == GLFW_KEY_A) mapEditor->SelectAll();
            } else {
                if (key == GLFW_KEY_DELETE) mapEditor->DeleteSelected();
                if (key == GLFW_KEY_ESCAPE) mapEditor->DeselectAll();
                mapEditor->OnKeyPress(key);
            }
        }
        
        // Escape to release cursor in game mode
        if (currentMode == AppMode::GAME && key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        }
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (currentMode == AppMode::GAME) {
        keybinds.OnMouseButtonEvent(button, action);
    }
    
    // Editor mouse handling
    if (currentMode == AppMode::EDITOR && mapEditor && action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            // Calculate world position from mouse (simplified - uses grid plane)
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            
            // Simple ray casting to grid plane
            float ndcX = (2.0f * mx / width) - 1.0f;
            float ndcY = 1.0f - (2.0f * my / height);
            
            auto& settings = mapEditor->GetSettings();
            float gridY = settings.gridHeight;
            
            // Calculate world position on grid (simplified)
            float worldX = editorCamTarget.x + ndcX * editorCamDist * 0.5f;
            float worldZ = editorCamTarget.z + ndcY * editorCamDist * 0.5f;
            
            bool shift = mods & GLFW_MOD_SHIFT;
            mapEditor->OnMouseClick(worldX, gridY, worldZ, shift);
        }
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (currentMode == AppMode::EDITOR) {
        editorCamDist -= yoffset * 2.0f;
        if (editorCamDist < 5.0f) editorCamDist = 5.0f;
        if (editorCamDist > 200.0f) editorCamDist = 200.0f;
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float dx = xpos - lastX;
    float dy = ypos - lastY;
    lastX = xpos;
    lastY = ypos;
    
    if (currentMode == AppMode::GAME && cursorCaptured) {
        camera.ProcessMouse(dx, dy);
    }
    
    // Editor camera rotation with right mouse button
    if (currentMode == AppMode::EDITOR) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            editorCamYaw += dx * 0.005f;
            editorCamPitch += dy * 0.005f;
            if (editorCamPitch < 0.1f) editorCamPitch = 0.1f;
            if (editorCamPitch > 1.5f) editorCamPitch = 1.5f;
        }
        
        // Pan with middle mouse button
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            float panSpeed = editorCamDist * 0.002f;
            editorCamTarget.x -= dx * panSpeed * cos(editorCamYaw);
            editorCamTarget.z -= dx * panSpeed * sin(editorCamYaw);
            editorCamTarget.y += dy * panSpeed;
        }
        
        // Update drag for brush creation
        if (mapEditor && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            
            float ndcX = (2.0f * xpos / width) - 1.0f;
            float ndcY = 1.0f - (2.0f * ypos / height);
            
            auto& settings = mapEditor->GetSettings();
            float gridY = settings.gridHeight;
            float worldX = editorCamTarget.x + ndcX * editorCamDist * 0.5f;
            float worldZ = editorCamTarget.z + ndcY * editorCamDist * 0.5f;
            
            mapEditor->OnMouseDrag(worldX, gridY, worldZ);
        }
    }
}

void processInput(GLFWwindow* window, float dt) {
    if (currentMode == AppMode::GAME) {
        float forward = keybinds.GetAxis(KeybindManager::MOVE_FORWARD, KeybindManager::MOVE_BACKWARD);
        float right = keybinds.GetAxis(KeybindManager::MOVE_RIGHT, KeybindManager::MOVE_LEFT);
        
        float speed = keybinds.IsPressed(KeybindManager::SPRINT) ? 10.0f : 5.0f;
        
        if (forward != 0 || right != 0) {
            Vec3 fwd = camera.GetForward();
            Vec3 rgt = camera.GetRight();
            
            fwd.y = 0;
            fwd = fwd.normalized();
            
            camera.position = camera.position + fwd * forward * dt * speed;
            camera.position = camera.position + rgt * right * dt * speed;
        }
    }
    
    // Editor WASD camera movement
    if (currentMode == AppMode::EDITOR) {
        float moveSpeed = editorCamDist * 0.02f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            editorCamTarget.z -= moveSpeed * cos(editorCamYaw);
            editorCamTarget.x -= moveSpeed * sin(editorCamYaw);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            editorCamTarget.z += moveSpeed * cos(editorCamYaw);
            editorCamTarget.x += moveSpeed * sin(editorCamYaw);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            editorCamTarget.x -= moveSpeed * cos(editorCamYaw);
            editorCamTarget.z += moveSpeed * sin(editorCamYaw);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            editorCamTarget.x += moveSpeed * cos(editorCamYaw);
            editorCamTarget.z -= moveSpeed * sin(editorCamYaw);
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            editorCamTarget.y -= moveSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            editorCamTarget.y += moveSpeed;
        }
    }
}

// Shader code
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

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader error:\n" << log << std::endl;
    }
    return s;
}

void setIdentityMatrix(float* mat) {
    for (int i = 0; i < 16; i++) mat[i] = (i % 5 == 0) ? 1.0f : 0.0f;
}

void getEditorViewMatrix(float* mat) {
    float camX = editorCamTarget.x + editorCamDist * sin(editorCamYaw) * cos(editorCamPitch);
    float camY = editorCamTarget.y + editorCamDist * sin(editorCamPitch);
    float camZ = editorCamTarget.z + editorCamDist * cos(editorCamYaw) * cos(editorCamPitch);
    
    Vec3 eye(camX, camY, camZ);
    Vec3 target(editorCamTarget.x, editorCamTarget.y, editorCamTarget.z);
    Vec3 up(0, 1, 0);
    
    Vec3 f = (target - eye).normalized();
    Vec3 r = f.cross(up).normalized();
    Vec3 u = r.cross(f);
    
    mat[0] = r.x;  mat[4] = r.y;  mat[8] = r.z;   mat[12] = -r.x*eye.x - r.y*eye.y - r.z*eye.z;
    mat[1] = u.x;  mat[5] = u.y;  mat[9] = u.z;   mat[13] = -u.x*eye.x - u.y*eye.y - u.z*eye.z;
    mat[2] = -f.x; mat[6] = -f.y; mat[10] = -f.z; mat[14] = f.x*eye.x + f.y*eye.y + f.z*eye.z;
    mat[3] = 0;    mat[7] = 0;    mat[11] = 0;    mat[15] = 1;
}

// Rendering functions for map geometry
void renderGrid(GLuint vao, GLuint vbo, GLuint program, float* view, float* proj) {
    if (!mapEditor) return;
    
    auto& settings = mapEditor->GetSettings();
    if (!settings.showGrid) return;
    
    std::vector<float> gridVerts;
    float extent = settings.gridExtent;
    float step = settings.gridSize;
    float y = settings.gridHeight;
    
    // Generate grid lines
    for (float x = -extent; x <= extent; x += step) {
        // X-parallel lines
        gridVerts.insert(gridVerts.end(), {x, y, -extent, 0.3f, 0.3f, 0.3f});
        gridVerts.insert(gridVerts.end(), {x, y, extent, 0.3f, 0.3f, 0.3f});
    }
    for (float z = -extent; z <= extent; z += step) {
        // Z-parallel lines
        gridVerts.insert(gridVerts.end(), {-extent, y, z, 0.3f, 0.3f, 0.3f});
        gridVerts.insert(gridVerts.end(), {extent, y, z, 0.3f, 0.3f, 0.3f});
    }
    
    // Origin axes
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
    
    glUseProgram(program);
    
    float model[16];
    setIdentityMatrix(model);
    
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, proj);
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, model);
    
    glDrawArrays(GL_LINES, 0, gridVerts.size() / 6);
}

void renderMapBrushes(GLuint vao, GLuint vbo, GLuint ebo, GLuint program, float* view, float* proj) {
    if (!mapEditor) return;
    
    const auto& map = mapEditor->GetMap();
    int selectedIdx = mapEditor->GetSelectedBrushIndex();
    
    for (size_t i = 0; i < map.brushes.size(); i++) {
        const auto& brush = map.brushes[i];
        
        // Build vertex data
        std::vector<float> verts;
        for (const auto& v : brush.vertices) {
            float r = brush.color.x;
            float g = brush.color.y;
            float b = brush.color.z;
            
            // Highlight selected
            if ((int)i == selectedIdx) {
                r = 1.0f; g = 0.8f; b = 0.2f;
            }
            
            // Different colors for special brush types
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, brush.indices.size() * sizeof(unsigned int), brush.indices.data(), GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glUseProgram(program);
        
        float model[16];
        setIdentityMatrix(model);
        
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, proj);
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, model);
        
        // Wireframe for selected brush
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

void renderEntities(GLuint vao, GLuint vbo, GLuint ebo, GLuint program, float* view, float* proj) {
    if (!mapEditor) return;
    
    const auto& map = mapEditor->GetMap();
    auto& settings = mapEditor->GetSettings();
    if (!settings.showEntityIcons) return;
    
    int selectedIdx = mapEditor->GetSelectedEntityIndex();
    
    // Simple box representation for entities
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
        0,1,2, 0,2,3,  // Front
        4,5,6, 4,6,7,  // Back
        0,4,7, 0,7,3,  // Left
        1,5,6, 1,6,2,  // Right
        3,2,6, 3,6,7,  // Top
        0,1,5, 0,5,4   // Bottom
    };
    
    for (size_t i = 0; i < map.entities.size(); i++) {
        const auto& ent = map.entities[i];
        
        // Color based on entity type
        float r = 0.5f, g = 0.5f, b = 0.5f;
        
        switch (ent.type) {
            case PCD::ENT_INFO_PLAYER_START:
            case PCD::ENT_INFO_PLAYER_DEATHMATCH:
                r = 0.2f; g = 1.0f; b = 0.2f;
                break;
            case PCD::ENT_INFO_TEAM_SPAWN_RED:
                r = 1.0f; g = 0.2f; b = 0.2f;
                break;
            case PCD::ENT_INFO_TEAM_SPAWN_BLUE:
                r = 0.2f; g = 0.2f; b = 1.0f;
                break;
            case PCD::ENT_LIGHT:
            case PCD::ENT_LIGHT_SPOT:
            case PCD::ENT_LIGHT_ENV:
                r = 1.0f; g = 1.0f; b = 0.5f;
                break;
            case PCD::ENT_TRIGGER_ONCE:
            case PCD::ENT_TRIGGER_MULTIPLE:
            case PCD::ENT_TRIGGER_HURT:
            case PCD::ENT_TRIGGER_PUSH:
            case PCD::ENT_TRIGGER_TELEPORT:
                r = 0.8f; g = 0.4f; b = 0.8f;
                break;
            case PCD::ENT_ITEM_HEALTH:
                r = 1.0f; g = 0.3f; b = 0.3f;
                break;
            case PCD::ENT_ITEM_ARMOR:
                r = 0.3f; g = 0.6f; b = 1.0f;
                break;
            case PCD::ENT_ITEM_AMMO:
                r = 0.8f; g = 0.6f; b = 0.2f;
                break;
            case PCD::ENT_WEAPON_SHOTGUN:
            case PCD::ENT_WEAPON_ROCKET:
            case PCD::ENT_WEAPON_RAILGUN:
            case PCD::ENT_WEAPON_PLASMA:
                r = 0.9f; g = 0.5f; b = 0.1f;
                break;
            default:
                break;
        }
        
        // Highlight selected
        if ((int)i == selectedIdx) {
            r = 1.0f; g = 0.9f; b = 0.3f;
        }
        
        // Apply colors to vertices
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
        glBufferData(GL_ARRAY_BUFFER, coloredVerts.size() * sizeof(float), coloredVerts.data(), GL_DYNAMIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(boxIndices), boxIndices, GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glUseProgram(program);
        
        float model[16];
        setIdentityMatrix(model);
        model[12] = ent.position.x;
        model[13] = ent.position.y;
        model[14] = ent.position.z;
        
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, proj);
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, model);
        
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
}

void renderCreationPreview(GLuint vao, GLuint vbo, GLuint ebo, GLuint program, float* view, float* proj) {
    if (!mapEditor || !mapEditor->IsCreating()) return;
    
    PCD::Vec3 start = mapEditor->GetCreateStart();
    PCD::Vec3 end = mapEditor->GetCreateEnd();
    
    float minX = std::min(start.x, end.x);
    float minY = std::min(start.y, end.y);
    float minZ = std::min(start.z, end.z);
    float maxX = std::max(start.x, end.x);
    float maxY = std::max(start.y, end.y);
    float maxZ = std::max(start.z, end.z);
    
    // Default height if flat
    if (maxY - minY < 0.1f) maxY = minY + mapEditor->GetSettings().gridSize * 2;
    
    // Preview wireframe box
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
        0,1, 1,2, 2,3, 3,0,  // Front
        4,5, 5,6, 6,7, 7,4,  // Back
        0,4, 1,5, 2,6, 3,7   // Connections
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
    
    glUseProgram(program);
    
    float model[16];
    setIdentityMatrix(model);
    
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, proj);
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, model);
    
    glLineWidth(2.0f);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glLineWidth(1.0f);
}

int main(int argc, char* argv[]) {

bool editorOnly = false;

for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "--editor") {
        editorOnly = true;
    }
}


if (editorOnly) {
    // Minimal editor-only boot
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Placid Editor", nullptr, nullptr);
    if (!window) return -1;

    glfwMakeContextCurrent(window);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "GLAD failed\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // PCD editor setup
    PCD::EditorState editorState;
    PCD::EditorUI editorUI(editorState);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        editorUI.Render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


    if (!glfwInit()) return -1;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Placid Map Editor", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "GLAD failed" << std::endl;
        return -1;
    }
    
    glEnable(GL_DEPTH_TEST);
    
    // Initialize ImGui
    ImGui::Init(window);
    
    // Create map editor
    mapEditor = new Editor::MapEditor();
    
    // Create default arena if starting fresh
    if (mapEditor->GetMap().brushes.empty()) {
        // Add floor
        PCD::Brush floor = mapEditor->CreateBox(
            PCD::Vec3(-20, -1, -20),
            PCD::Vec3(20, 0, 20)
        );
        floor.name = "Floor";
        mapEditor->GetMap().brushes.push_back(floor);
        
        // Add a player spawn
        PCD::Entity spawn;
        spawn.id = mapEditor->GetMap().nextEntityID++;
        spawn.type = PCD::ENT_INFO_PLAYER_START;
        spawn.position = PCD::Vec3(0, 0.1f, 0);
        spawn.name = "PlayerSpawn";
        mapEditor->GetMap().entities.push_back(spawn);
    }
    
    // Shader setup
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    // VAO/VBO for dynamic rendering
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    float lastFrame = 0;
    
    std::cout << "=== Placid Map Editor ===" << std::endl;
    std::cout << "F1 - Toggle Editor/Game mode" << std::endl;
    std::cout << "WASD/QE - Move camera" << std::endl;
    std::cout << "Right Mouse - Rotate view" << std::endl;
    std::cout << "Middle Mouse - Pan view" << std::endl;
    std::cout << "Scroll - Zoom" << std::endl;
    std::cout << "Left Click - Select/Create" << std::endl;
    std::cout << "Ctrl+S - Save map (.pcd)" << std::endl;
    std::cout << "Delete - Delete selected" << std::endl;
    std::cout << "=========================" << std::endl;
    
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput(window, dt);
        
        // Clear
        if (currentMode == AppMode::EDITOR) {
            glClearColor(0.18f, 0.18f, 0.22f, 1.0f);
        } else {
            glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Get view/projection matrices
        float view[16], proj[16];
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / height;
        
        if (currentMode == AppMode::EDITOR) {
            getEditorViewMatrix(view);
            // Orthographic-ish perspective for editor
            float fov = 45.0f;
            float f = 1.0f / tan(fov * 3.14159f / 360.0f);
            float near = 0.1f, far = 500.0f;
            
            proj[0] = f/aspect; proj[4] = 0; proj[8] = 0;                      proj[12] = 0;
            proj[1] = 0;        proj[5] = f; proj[9] = 0;                      proj[13] = 0;
            proj[2] = 0;        proj[6] = 0; proj[10] = (far+near)/(near-far); proj[14] = (2*far*near)/(near-far);
            proj[3] = 0;        proj[7] = 0; proj[11] = -1;                    proj[15] = 0;
        } else {
            camera.GetViewMatrix(view);
            camera.GetProjectionMatrix(proj, aspect);
        }
        
        // Render
        glEnable(GL_DEPTH_TEST);
        
        if (currentMode == AppMode::EDITOR) {
            renderGrid(vao, vbo, program, view, proj);
            renderMapBrushes(vao, vbo, ebo, program, view, proj);
            renderEntities(vao, vbo, ebo, program, view, proj);
            renderCreationPreview(vao, vbo, ebo, program, view, proj);
            
            // Render ImGui
            ImGui::NewFrame();
            mapEditor->RenderUI();
            ImGui::Render();
        } else {
            // Game mode - render map brushes as gameplay geometry
            renderMapBrushes(vao, vbo, ebo, program, view, proj);
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    delete mapEditor;
    ImGui::Shutdown();
    
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
