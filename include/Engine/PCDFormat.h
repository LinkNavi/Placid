#ifndef PCD_FORMAT_H
#define PCD_FORMAT_H

#include <string>
#include <vector>
#include <fstream>
#include <cstring>
#include <cstdint>

namespace PCD {

// PCD File Format Specification:
// Header (32 bytes):
//   - Magic: "PCD1" (4 bytes)
//   - Version: uint32_t (4 bytes)
//   - Flags: uint32_t (4 bytes)
//   - BrushCount: uint32_t (4 bytes)
//   - EntityCount: uint32_t (4 bytes)
//   - Reserved: 12 bytes
// 
// Brushes Section:
//   For each brush:
//     - VertexCount: uint32_t
//     - FaceCount: uint32_t
//     - TextureID: uint32_t
//     - Flags: uint32_t (solid, detail, trigger, etc.)
//     - Vertices: float[VertexCount * 3]
//     - Normals: float[VertexCount * 3]
//     - UVs: float[VertexCount * 2]
//     - Indices: uint32_t[FaceCount * 3]
//
// Entities Section:
//   For each entity:
//     - Type: uint32_t
//     - Position: float[3]
//     - Rotation: float[3]
//     - Scale: float[3]
//     - PropertyCount: uint32_t
//     - Properties: (key_len, key, value_len, value) pairs

const char MAGIC[4] = {'P', 'C', 'D', '1'};
const uint32_t VERSION = 1;

// Brush flags
enum BrushFlags : uint32_t {
    BRUSH_SOLID     = 1 << 0,
    BRUSH_DETAIL    = 1 << 1,
    BRUSH_TRIGGER   = 1 << 2,
    BRUSH_WATER     = 1 << 3,
    BRUSH_LAVA      = 1 << 4,
    BRUSH_SLIME     = 1 << 5,
    BRUSH_LADDER    = 1 << 6,
    BRUSH_CLIP      = 1 << 7,      // Player collision only
    BRUSH_SKYBOX    = 1 << 8,
    BRUSH_NOCOLLIDE = 1 << 9,
};

// Entity types
enum EntityType : uint32_t {
    ENT_INFO_PLAYER_START = 0,
    ENT_INFO_PLAYER_DEATHMATCH = 1,
    ENT_INFO_TEAM_SPAWN_RED = 2,
    ENT_INFO_TEAM_SPAWN_BLUE = 3,
    ENT_LIGHT = 10,
    ENT_LIGHT_SPOT = 11,
    ENT_LIGHT_ENV = 12,
    ENT_TRIGGER_ONCE = 20,
    ENT_TRIGGER_MULTIPLE = 21,
    ENT_TRIGGER_HURT = 22,
    ENT_TRIGGER_PUSH = 23,
    ENT_TRIGGER_TELEPORT = 24,
    ENT_FUNC_DOOR = 30,
    ENT_FUNC_BUTTON = 31,
    ENT_FUNC_PLATFORM = 32,
    ENT_FUNC_ROTATING = 33,
    ENT_ITEM_HEALTH = 40,
    ENT_ITEM_ARMOR = 41,
    ENT_ITEM_AMMO = 42,
    ENT_WEAPON_SHOTGUN = 50,
    ENT_WEAPON_ROCKET = 51,
    ENT_WEAPON_RAILGUN = 52,
    ENT_WEAPON_PLASMA = 53,
    ENT_TARGET_DESTINATION = 60,
    ENT_TARGET_RELAY = 61,
    ENT_AMBIENT_SOUND = 70,
    ENT_ENV_PARTICLE = 80,
    ENT_CUSTOM = 255,
};

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Vec2 {
    float u, v;
    Vec2() : u(0), v(0) {}
    Vec2(float u, float v) : u(u), v(v) {}
};

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
};

struct Brush {
    uint32_t id;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t textureID;
    uint32_t flags;
    Vec3 color;  // Editor color
    std::string name;
    
    Brush() : id(0), textureID(0), flags(BRUSH_SOLID), color(0.5f, 0.5f, 0.5f) {}
};

struct Entity {
    uint32_t id;
    EntityType type;
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
    std::vector<std::pair<std::string, std::string>> properties;
    std::string name;
    
    Entity() : id(0), type(ENT_INFO_PLAYER_START), scale(1, 1, 1) {}
    
    std::string GetProperty(const std::string& key, const std::string& def = "") const {
        for (const auto& [k, v] : properties) {
            if (k == key) return v;
        }
        return def;
    }
    
    void SetProperty(const std::string& key, const std::string& value) {
        for (auto& [k, v] : properties) {
            if (k == key) { v = value; return; }
        }
        properties.push_back({key, value});
    }
};

struct Map {
    std::string name;
    std::string author;
    std::vector<Brush> brushes;
    std::vector<Entity> entities;
    uint32_t nextBrushID;
    uint32_t nextEntityID;
    
    Map() : name("Untitled"), author("Unknown"), nextBrushID(1), nextEntityID(1) {}
    
    void Clear() {
        brushes.clear();
        entities.clear();
        nextBrushID = 1;
        nextEntityID = 1;
    }
};

class PCDWriter {
public:
    static bool Save(const Map& map, const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        // Write header
        file.write(MAGIC, 4);
        WriteU32(file, VERSION);
        WriteU32(file, 0); // Flags
        WriteU32(file, static_cast<uint32_t>(map.brushes.size()));
        WriteU32(file, static_cast<uint32_t>(map.entities.size()));
        
        // Reserved (12 bytes)
        char reserved[12] = {0};
        file.write(reserved, 12);
        
        // Write map metadata as first "meta" section
        WriteString(file, map.name);
        WriteString(file, map.author);
        
        // Write brushes
        for (const auto& brush : map.brushes) {
            WriteU32(file, static_cast<uint32_t>(brush.vertices.size()));
            WriteU32(file, static_cast<uint32_t>(brush.indices.size() / 3));
            WriteU32(file, brush.textureID);
            WriteU32(file, brush.flags);
            
            // Write vertices
            for (const auto& v : brush.vertices) {
                WriteFloat(file, v.position.x);
                WriteFloat(file, v.position.y);
                WriteFloat(file, v.position.z);
            }
            
            // Write normals
            for (const auto& v : brush.vertices) {
                WriteFloat(file, v.normal.x);
                WriteFloat(file, v.normal.y);
                WriteFloat(file, v.normal.z);
            }
            
            // Write UVs
            for (const auto& v : brush.vertices) {
                WriteFloat(file, v.uv.u);
                WriteFloat(file, v.uv.v);
            }
            
            // Write indices
            for (uint32_t idx : brush.indices) {
                WriteU32(file, idx);
            }
            
            // Write color
            WriteFloat(file, brush.color.x);
            WriteFloat(file, brush.color.y);
            WriteFloat(file, brush.color.z);
            
            WriteString(file, brush.name);
        }
        
        // Write entities
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
        
        // Read and verify header
        char magic[4];
        file.read(magic, 4);
        if (memcmp(magic, MAGIC, 4) != 0) return false;
        
        uint32_t version = ReadU32(file);
        if (version > VERSION) return false;
        
        uint32_t flags = ReadU32(file);
        uint32_t brushCount = ReadU32(file);
        uint32_t entityCount = ReadU32(file);
        
        // Skip reserved
        file.seekg(12, std::ios::cur);
        
        map.Clear();
        
        // Read metadata
        map.name = ReadString(file);
        map.author = ReadString(file);
        
        // Read brushes
        for (uint32_t i = 0; i < brushCount; i++) {
            Brush brush;
            brush.id = map.nextBrushID++;
            
            uint32_t vertexCount = ReadU32(file);
            uint32_t faceCount = ReadU32(file);
            brush.textureID = ReadU32(file);
            brush.flags = ReadU32(file);
            
            brush.vertices.resize(vertexCount);
            
            // Read positions
            for (uint32_t v = 0; v < vertexCount; v++) {
                brush.vertices[v].position.x = ReadFloat(file);
                brush.vertices[v].position.y = ReadFloat(file);
                brush.vertices[v].position.z = ReadFloat(file);
            }
            
            // Read normals
            for (uint32_t v = 0; v < vertexCount; v++) {
                brush.vertices[v].normal.x = ReadFloat(file);
                brush.vertices[v].normal.y = ReadFloat(file);
                brush.vertices[v].normal.z = ReadFloat(file);
            }
            
            // Read UVs
            for (uint32_t v = 0; v < vertexCount; v++) {
                brush.vertices[v].uv.u = ReadFloat(file);
                brush.vertices[v].uv.v = ReadFloat(file);
            }
            
            // Read indices
            brush.indices.resize(faceCount * 3);
            for (uint32_t idx = 0; idx < faceCount * 3; idx++) {
                brush.indices[idx] = ReadU32(file);
            }
            
            // Read color
            brush.color.x = ReadFloat(file);
            brush.color.y = ReadFloat(file);
            brush.color.z = ReadFloat(file);
            
            brush.name = ReadString(file);
            
            map.brushes.push_back(brush);
        }
        
        // Read entities
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

// Helper to get entity type name
inline const char* GetEntityTypeName(EntityType type) {
    switch (type) {
        case ENT_INFO_PLAYER_START: return "info_player_start";
        case ENT_INFO_PLAYER_DEATHMATCH: return "info_player_deathmatch";
        case ENT_INFO_TEAM_SPAWN_RED: return "info_team_spawn_red";
        case ENT_INFO_TEAM_SPAWN_BLUE: return "info_team_spawn_blue";
        case ENT_LIGHT: return "light";
        case ENT_LIGHT_SPOT: return "light_spot";
        case ENT_LIGHT_ENV: return "light_environment";
        case ENT_TRIGGER_ONCE: return "trigger_once";
        case ENT_TRIGGER_MULTIPLE: return "trigger_multiple";
        case ENT_TRIGGER_HURT: return "trigger_hurt";
        case ENT_TRIGGER_PUSH: return "trigger_push";
        case ENT_TRIGGER_TELEPORT: return "trigger_teleport";
        case ENT_FUNC_DOOR: return "func_door";
        case ENT_FUNC_BUTTON: return "func_button";
        case ENT_FUNC_PLATFORM: return "func_platform";
        case ENT_FUNC_ROTATING: return "func_rotating";
        case ENT_ITEM_HEALTH: return "item_health";
        case ENT_ITEM_ARMOR: return "item_armor";
        case ENT_ITEM_AMMO: return "item_ammo";
        case ENT_WEAPON_SHOTGUN: return "weapon_shotgun";
        case ENT_WEAPON_ROCKET: return "weapon_rocket";
        case ENT_WEAPON_RAILGUN: return "weapon_railgun";
        case ENT_WEAPON_PLASMA: return "weapon_plasma";
        case ENT_TARGET_DESTINATION: return "target_destination";
        case ENT_TARGET_RELAY: return "target_relay";
        case ENT_AMBIENT_SOUND: return "ambient_sound";
        case ENT_ENV_PARTICLE: return "env_particle";
        case ENT_CUSTOM: return "custom";
        default: return "unknown";
    }
}

inline const char* GetBrushFlagName(BrushFlags flag) {
    switch (flag) {
        case BRUSH_SOLID: return "Solid";
        case BRUSH_DETAIL: return "Detail";
        case BRUSH_TRIGGER: return "Trigger";
        case BRUSH_WATER: return "Water";
        case BRUSH_LAVA: return "Lava";
        case BRUSH_SLIME: return "Slime";
        case BRUSH_LADDER: return "Ladder";
        case BRUSH_CLIP: return "Clip";
        case BRUSH_SKYBOX: return "Skybox";
        case BRUSH_NOCOLLIDE: return "No Collide";
        default: return "Unknown";
    }
}

} // namespace PCD

#endif // PCD_FORMAT_H
