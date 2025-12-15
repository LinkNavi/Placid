// include/Engine/TextureLoader.h - Texture loading with PNG/JPG support

#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include "PCD/PCD.h"
#include <glad/gl.h>
#include <fstream>
#include <iostream>
#include <cstring>

// stb_image for PNG/JPG loading

#include "stb/stb_image.h"

namespace TextureLoader {

// Load PNG, JPG, BMP using stb_image
inline bool LoadImage(const std::string& filename, PCD::Texture& tex) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        std::cerr << "[Texture] Failed to load: " << filename << "\n";
        std::cerr << "[Texture] Error: " << stbi_failure_reason() << "\n";
        return false;
    }
    
    tex.width = width;
    tex.height = height;
    tex.channels = 4; // Force RGBA
    tex.data.assign(data, data + (width * height * 4));
    
    stbi_image_free(data);
    
    // Extract filename without path
    size_t lastSlash = filename.find_last_of("/\\");
    tex.name = (lastSlash != std::string::npos) ? filename.substr(lastSlash + 1) : filename;
    
    std::cout << "[Texture] Loaded: " << tex.name << " (" << width << "x" << height << ")\n";
    return true;
}

// Simple BMP loader (fallback - not needed with stb_image but kept for reference)
inline bool LoadBMP(const std::string& filename, PCD::Texture& tex) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Texture] Failed to open: " << filename << "\n";
        return false;
    }
    
    // Read BMP header
    char header[54];
    file.read(header, 54);
    
    if (header[0] != 'B' || header[1] != 'M') {
        std::cerr << "[Texture] Not a BMP file: " << filename << "\n";
        return false;
    }
    
    uint32_t dataPos = *(uint32_t*)&header[0x0A];
    uint32_t imageSize = *(uint32_t*)&header[0x22];
    uint32_t width = *(uint32_t*)&header[0x12];
    uint32_t height = *(uint32_t*)&header[0x16];
    
    if (imageSize == 0) imageSize = width * height * 3;
    if (dataPos == 0) dataPos = 54;
    
    // Read pixel data (BGR format)
    std::vector<uint8_t> bgrData(imageSize);
    file.seekg(dataPos);
    file.read((char*)bgrData.data(), imageSize);
    file.close();
    
    // Convert BGR to RGBA
    tex.width = width;
    tex.height = height;
    tex.channels = 4;
    tex.data.resize(width * height * 4);
    
    for (uint32_t i = 0; i < width * height; i++) {
        tex.data[i * 4 + 0] = bgrData[i * 3 + 2]; // R
        tex.data[i * 4 + 1] = bgrData[i * 3 + 1]; // G
        tex.data[i * 4 + 2] = bgrData[i * 3 + 0]; // B
        tex.data[i * 4 + 3] = 255;                 // A
    }
    
    // Extract filename without path
    size_t lastSlash = filename.find_last_of("/\\");
    tex.name = (lastSlash != std::string::npos) ? filename.substr(lastSlash + 1) : filename;
    
    std::cout << "[Texture] Loaded BMP: " << tex.name << " (" << width << "x" << height << ")\n";
    return true;
}

// Create OpenGL texture from PCD texture data
inline GLuint CreateGLTexture(const PCD::Texture& tex) {
    if (tex.data.empty()) {
        std::cerr << "[Texture] No texture data\n";
        return 0;
    }
    
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
    
    std::cout << "[Texture] Created OpenGL texture: " << textureID << "\n";
    return textureID;
}

// Load all textures in a map and create OpenGL textures
inline void LoadMapTextures(PCD::Map& map) {
    for (auto& [id, tex] : map.textures) {
        if (tex.glTextureID == 0 && !tex.data.empty()) {
            tex.glTextureID = CreateGLTexture(tex);
        }
    }
    std::cout << "[Texture] Loaded " << map.textures.size() << " textures\n";
}

// Create a simple checkerboard texture (fallback)
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

// Free OpenGL textures
inline void FreeMapTextures(PCD::Map& map) {
    for (auto& [id, tex] : map.textures) {
        if (tex.glTextureID != 0) {
            glDeleteTextures(1, &tex.glTextureID);
            tex.glTextureID = 0;
        }
    }
}

} // namespace TextureLoader

#endif // TEXTURE_LOADER_H
