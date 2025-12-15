// include/Engine/TextureLoader.h - Texture loading with PNG/JPG support

#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include "PCD/PCD.h"
#include <glad/gl.h>
#include <fstream>
#include <iostream>
#include <cstring>

#include "stb/stb_image.h"

namespace TextureLoader {

// Load PNG, JPG, BMP using stb_image
inline bool LoadImage(const std::string& filename, PCD::Texture& tex) {
    // Check if file exists
    std::ifstream testFile(filename);
    if (!testFile.good()) {
        std::cerr << "[Texture] File not found: " << filename << "\n";
        std::cerr << "[Texture] Tip: Use full path like 'Assets/textures/wall.png'\n";
        return false;
    }
    testFile.close();
    
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        std::cerr << "[Texture] Failed to load: " << filename << "\n";
        std::cerr << "[Texture] Error: " << stbi_failure_reason() << "\n";
        return false;
    }
    
    // Validate it's not a tiny system icon/cursor
    if (width < 8 || height < 8) {
        std::cerr << "[Texture] Image too small (" << width << "x" << height << ") - might be system icon\n";
        stbi_image_free(data);
        return false;
    }
    
    tex.width = width;
    tex.height = height;
    tex.channels = 4;
    tex.data.assign(data, data + (width * height * 4));
    
    stbi_image_free(data);
    
    size_t lastSlash = filename.find_last_of("/\\");
    tex.name = (lastSlash != std::string::npos) ? filename.substr(lastSlash + 1) : filename;
    
    std::cout << "[Texture] Loaded: " << tex.name << " (" << width << "x" << height << ")\n";
    return true;
}

inline GLuint CreateGLTexture(const PCD::Texture& tex) {
    if (tex.data.empty()) return 0;
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    GLenum format = (tex.channels == 4) ? GL_RGBA : GL_RGB;
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, tex.width, tex.height, 0, 
                 format, GL_UNSIGNED_BYTE, tex.data.data());
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    std::cout << "[Texture] Created GL texture: " << textureID << "\n";
    return textureID;
}

inline void LoadMapTextures(PCD::Map& map) {
    for (auto& [id, tex] : map.textures) {
        if (tex.glTextureID == 0 && !tex.data.empty()) {
            tex.glTextureID = CreateGLTexture(tex);
        }
    }
    
    // Update brush texture IDs to OpenGL IDs
    for (auto& brush : map.brushes) {
        if (brush.textureID > 0) {
            auto* tex = map.GetTexture(brush.textureID);
            if (tex && tex->glTextureID > 0) {
                brush.textureID = tex->glTextureID;
            }
        }
    }
    
    std::cout << "[Texture] Loaded " << map.textures.size() << " textures\n";
}

inline PCD::Texture CreateCheckerboardTexture(uint32_t size = 64) {
    PCD::Texture tex;
    tex.name = "checkerboard";
    tex.width = size;
    tex.height = size;
    tex.channels = 4;
    tex.data.resize(size * size * 4);
    
    for (uint32_t y = 0; y < size; y++) {
        for (uint32_t x = 0; x < size; x++) {
            bool checker = ((x / 8) + (y / 8)) % 2 == 0;
            uint8_t color = checker ? 255 : 128;
            
            uint32_t i = (y * size + x) * 4;
            tex.data[i + 0] = color;
            tex.data[i + 1] = color;
            tex.data[i + 2] = color;
            tex.data[i + 3] = 255;
        }
    }
    
    tex.glTextureID = CreateGLTexture(tex);
    return tex;
}

inline void FreeMapTextures(PCD::Map& map) {
    for (auto& [id, tex] : map.textures) {
        if (tex.glTextureID != 0) {
            glDeleteTextures(1, &tex.glTextureID);
            tex.glTextureID = 0;
        }
    }
}

} // namespace TextureLoader

#endif
