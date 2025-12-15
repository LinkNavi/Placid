#ifndef PCD_TYPES_H
#define PCD_TYPES_H

#include <string>
#include <vector>
#include <cstdint>
#include <cmath>

namespace PCD {

// Brush flags
enum BrushFlags : uint32_t {
    BRUSH_SOLID     = 1 << 0,
    BRUSH_DETAIL    = 1 << 1,
    BRUSH_TRIGGER   = 1 << 2,
    BRUSH_WATER     = 1 << 3,
    BRUSH_LAVA      = 1 << 4,
    BRUSH_SLIME     = 1 << 5,
    BRUSH_LADDER    = 1 << 6,
    BRUSH_CLIP      = 1 << 7,
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
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    // Add arithmetic operators
    Vec3 operator+(const Vec3& v) const { 
        return Vec3(x + v.x, y + v.y, z + v.z); 
    }
    
    Vec3 operator-(const Vec3& v) const { 
        return Vec3(x - v.x, y - v.y, z - v.z); 
    }
    
    Vec3 operator*(float s) const { 
        return Vec3(x * s, y * s, z * s); 
    }
    
    Vec3 operator/(float s) const { 
        return Vec3(x / s, y / s, z / s); 
    }
    
    Vec3& operator+=(const Vec3& v) {
        x += v.x; y += v.y; z += v.z;
        return *this;
    }
    
    Vec3& operator-=(const Vec3& v) {
        x -= v.x; y -= v.y; z -= v.z;
        return *this;
    }
    
    Vec3& operator*=(float s) {
        x *= s; y *= s; z *= s;
        return *this;
    }
    
    float Length() const {
        return std::sqrt(x * x + y * y + z * z);
    }
    
    Vec3 Normalized() const {
        float len = Length();
        return len > 0 ? Vec3(x / len, y / len, z / len) : Vec3(0, 0, 0);
    }
};

struct Vec2 {
    float u = 0, v = 0;
    Vec2() = default;
    Vec2(float u, float v) : u(u), v(v) {}
};

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
};

struct Brush {
    uint32_t id = 0;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t textureID = 0;
    uint32_t flags = BRUSH_SOLID;
    Vec3 color{0.5f, 0.5f, 0.5f};
    std::string name;
};

struct Entity {
    uint32_t id = 0;
    EntityType type = ENT_INFO_PLAYER_START;
    Vec3 position;
    Vec3 rotation;
    Vec3 scale{1, 1, 1};
    std::vector<std::pair<std::string, std::string>> properties;
    std::string name;
    
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
    std::string name = "Untitled";
    std::string author = "Unknown";
    std::vector<Brush> brushes;
    std::vector<Entity> entities;
    uint32_t nextBrushID = 1;
    uint32_t nextEntityID = 1;
    
    void Clear() {
        brushes.clear();
        entities.clear();
        nextBrushID = 1;
        nextEntityID = 1;
    }
};

// Helper functions
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

#endif // PCD_TYPES_H
