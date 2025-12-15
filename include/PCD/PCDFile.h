#ifndef PCD_FILE_H
#define PCD_FILE_H

#include "PCDTypes.h"
#include <fstream>
#include <cstring>

namespace PCD {

const char MAGIC[4] = {'P', 'C', 'D', '1'};
const uint32_t VERSION = 1;

class PCDWriter {
public:
    static bool Save(const Map& map, const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        // Header
        file.write(MAGIC, 4);
        WriteU32(file, VERSION);
        WriteU32(file, 0); // Flags
        WriteU32(file, static_cast<uint32_t>(map.brushes.size()));
        WriteU32(file, static_cast<uint32_t>(map.entities.size()));
        
        char reserved[12] = {0};
        file.write(reserved, 12);
        
        // Metadata
        WriteString(file, map.name);
        WriteString(file, map.author);
        
        // Brushes
        for (const auto& brush : map.brushes) {
            WriteU32(file, static_cast<uint32_t>(brush.vertices.size()));
            WriteU32(file, static_cast<uint32_t>(brush.indices.size() / 3));
            WriteU32(file, brush.textureID);
            WriteU32(file, brush.flags);
            
            for (const auto& v : brush.vertices) {
                WriteFloat(file, v.position.x);
                WriteFloat(file, v.position.y);
                WriteFloat(file, v.position.z);
            }
            for (const auto& v : brush.vertices) {
                WriteFloat(file, v.normal.x);
                WriteFloat(file, v.normal.y);
                WriteFloat(file, v.normal.z);
            }
            for (const auto& v : brush.vertices) {
                WriteFloat(file, v.uv.u);
                WriteFloat(file, v.uv.v);
            }
            for (uint32_t idx : brush.indices) {
                WriteU32(file, idx);
            }
            
            WriteFloat(file, brush.color.x);
            WriteFloat(file, brush.color.y);
            WriteFloat(file, brush.color.z);
            WriteString(file, brush.name);
        }
        
        // Entities
        for (const auto& ent : map.entities) {
            WriteU32(file, static_cast<uint32_t>(ent.type));
            WriteFloat(file, ent.position.x);
            WriteFloat(file, ent.position.y);
            WriteFloat(file, ent.position.z);
            WriteFloat(file, ent.rotation.x);
            WriteFloat(file, ent.rotation.y);
            WriteFloat(file, ent.rotation.z);
            WriteFloat(file, ent.scale.x);
            WriteFloat(file, ent.scale.y);
            WriteFloat(file, ent.scale.z);
            
            WriteU32(file, static_cast<uint32_t>(ent.properties.size()));
            for (const auto& [key, value] : ent.properties) {
                WriteString(file, key);
                WriteString(file, value);
            }
            WriteString(file, ent.name);
        }
        
        return true;
    }
    
private:
    static void WriteU32(std::ofstream& f, uint32_t val) {
        f.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }
    static void WriteFloat(std::ofstream& f, float val) {
        f.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }
    static void WriteString(std::ofstream& f, const std::string& str) {
        uint32_t len = static_cast<uint32_t>(str.size());
        WriteU32(f, len);
        f.write(str.c_str(), len);
    }
};

class PCDReader {
public:
    static bool Load(Map& map, const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        char magic[4];
        file.read(magic, 4);
        if (memcmp(magic, MAGIC, 4) != 0) return false;
        
        uint32_t version = ReadU32(file);
        if (version > VERSION) return false;
        
        ReadU32(file); // flags
        uint32_t brushCount = ReadU32(file);
        uint32_t entityCount = ReadU32(file);
        
        file.seekg(12, std::ios::cur); // reserved
        
        map.Clear();
        map.name = ReadString(file);
        map.author = ReadString(file);
        
        // Brushes
        for (uint32_t i = 0; i < brushCount; i++) {
            Brush brush;
            brush.id = map.nextBrushID++;
            
            uint32_t vertexCount = ReadU32(file);
            uint32_t faceCount = ReadU32(file);
            brush.textureID = ReadU32(file);
            brush.flags = ReadU32(file);
            
            brush.vertices.resize(vertexCount);
            for (uint32_t v = 0; v < vertexCount; v++) {
                brush.vertices[v].position.x = ReadFloat(file);
                brush.vertices[v].position.y = ReadFloat(file);
                brush.vertices[v].position.z = ReadFloat(file);
            }
            for (uint32_t v = 0; v < vertexCount; v++) {
                brush.vertices[v].normal.x = ReadFloat(file);
                brush.vertices[v].normal.y = ReadFloat(file);
                brush.vertices[v].normal.z = ReadFloat(file);
            }
            for (uint32_t v = 0; v < vertexCount; v++) {
                brush.vertices[v].uv.u = ReadFloat(file);
                brush.vertices[v].uv.v = ReadFloat(file);
            }
            
            brush.indices.resize(faceCount * 3);
            for (uint32_t idx = 0; idx < faceCount * 3; idx++) {
                brush.indices[idx] = ReadU32(file);
            }
            
            brush.color.x = ReadFloat(file);
            brush.color.y = ReadFloat(file);
            brush.color.z = ReadFloat(file);
            brush.name = ReadString(file);
            
            map.brushes.push_back(brush);
        }
        
        // Entities
        for (uint32_t i = 0; i < entityCount; i++) {
            Entity ent;
            ent.id = map.nextEntityID++;
            ent.type = static_cast<EntityType>(ReadU32(file));
            ent.position.x = ReadFloat(file);
            ent.position.y = ReadFloat(file);
            ent.position.z = ReadFloat(file);
            ent.rotation.x = ReadFloat(file);
            ent.rotation.y = ReadFloat(file);
            ent.rotation.z = ReadFloat(file);
            ent.scale.x = ReadFloat(file);
            ent.scale.y = ReadFloat(file);
            ent.scale.z = ReadFloat(file);
            
            uint32_t propCount = ReadU32(file);
            for (uint32_t p = 0; p < propCount; p++) {
                std::string key = ReadString(file);
                std::string value = ReadString(file);
                ent.properties.push_back({key, value});
            }
            ent.name = ReadString(file);
            
            map.entities.push_back(ent);
        }
        
        return true;
    }
    
private:
    static uint32_t ReadU32(std::ifstream& f) {
        uint32_t val;
        f.read(reinterpret_cast<char*>(&val), sizeof(val));
        return val;
    }
    static float ReadFloat(std::ifstream& f) {
        float val;
        f.read(reinterpret_cast<char*>(&val), sizeof(val));
        return val;
    }
    static std::string ReadString(std::ifstream& f) {
        uint32_t len = ReadU32(f);
        std::string str(len, '\0');
        f.read(&str[0], len);
        return str;
    }
};

} // namespace PCD

#endif // PCD_FILE_H
