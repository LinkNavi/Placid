#ifndef PCD_BRUSH_FACTORY_H
#define PCD_BRUSH_FACTORY_H

#include "PCDTypes.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace PCD {

class BrushFactory {
public:
    static Brush CreateBox(Map& map, Vec3 min, Vec3 max) {
        Brush brush;
        brush.id = map.nextBrushID++;
        brush.flags = BRUSH_SOLID;
        
        // Front face (+Z)
        brush.vertices.push_back({{min.x, min.y, max.z}, {0, 0, 1}, {0, 0}});
        brush.vertices.push_back({{max.x, min.y, max.z}, {0, 0, 1}, {1, 0}});
        brush.vertices.push_back({{max.x, max.y, max.z}, {0, 0, 1}, {1, 1}});
        brush.vertices.push_back({{min.x, max.y, max.z}, {0, 0, 1}, {0, 1}});
        
        // Back face (-Z)
        brush.vertices.push_back({{max.x, min.y, min.z}, {0, 0, -1}, {0, 0}});
        brush.vertices.push_back({{min.x, min.y, min.z}, {0, 0, -1}, {1, 0}});
        brush.vertices.push_back({{min.x, max.y, min.z}, {0, 0, -1}, {1, 1}});
        brush.vertices.push_back({{max.x, max.y, min.z}, {0, 0, -1}, {0, 1}});
        
        // Top face (+Y)
        brush.vertices.push_back({{min.x, max.y, max.z}, {0, 1, 0}, {0, 0}});
        brush.vertices.push_back({{max.x, max.y, max.z}, {0, 1, 0}, {1, 0}});
        brush.vertices.push_back({{max.x, max.y, min.z}, {0, 1, 0}, {1, 1}});
        brush.vertices.push_back({{min.x, max.y, min.z}, {0, 1, 0}, {0, 1}});
        
        // Bottom face (-Y)
        brush.vertices.push_back({{min.x, min.y, min.z}, {0, -1, 0}, {0, 0}});
        brush.vertices.push_back({{max.x, min.y, min.z}, {0, -1, 0}, {1, 0}});
        brush.vertices.push_back({{max.x, min.y, max.z}, {0, -1, 0}, {1, 1}});
        brush.vertices.push_back({{min.x, min.y, max.z}, {0, -1, 0}, {0, 1}});
        
        // Right face (+X)
        brush.vertices.push_back({{max.x, min.y, max.z}, {1, 0, 0}, {0, 0}});
        brush.vertices.push_back({{max.x, min.y, min.z}, {1, 0, 0}, {1, 0}});
        brush.vertices.push_back({{max.x, max.y, min.z}, {1, 0, 0}, {1, 1}});
        brush.vertices.push_back({{max.x, max.y, max.z}, {1, 0, 0}, {0, 1}});
        
        // Left face (-X)
        brush.vertices.push_back({{min.x, min.y, min.z}, {-1, 0, 0}, {0, 0}});
        brush.vertices.push_back({{min.x, min.y, max.z}, {-1, 0, 0}, {1, 0}});
        brush.vertices.push_back({{min.x, max.y, max.z}, {-1, 0, 0}, {1, 1}});
        brush.vertices.push_back({{min.x, max.y, min.z}, {-1, 0, 0}, {0, 1}});
        
        // Indices for 6 faces
        for (int face = 0; face < 6; face++) {
            uint32_t base = face * 4;
            brush.indices.push_back(base + 0);
            brush.indices.push_back(base + 1);
            brush.indices.push_back(base + 2);
            brush.indices.push_back(base + 0);
            brush.indices.push_back(base + 2);
            brush.indices.push_back(base + 3);
        }
        
        return brush;
    }
    
    static Brush CreateCylinder(Map& map, Vec3 center, float radius, float height, int segments = 16) {
        Brush brush;
        brush.id = map.nextBrushID++;
        brush.flags = BRUSH_SOLID;
        
        float halfHeight = height / 2.0f;
        
        // Side vertices
        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            float nextAngle = 2.0f * M_PI * (i + 1) / segments;
            
            float x1 = center.x + radius * std::cos(angle);
            float z1 = center.z + radius * std::sin(angle);
            float x2 = center.x + radius * std::cos(nextAngle);
            float z2 = center.z + radius * std::sin(nextAngle);
            
            Vec3 normal1(std::cos(angle), 0, std::sin(angle));
            Vec3 normal2(std::cos(nextAngle), 0, std::sin(nextAngle));
            
            float u1 = (float)i / segments;
            float u2 = (float)(i + 1) / segments;
            
            uint32_t base = static_cast<uint32_t>(brush.vertices.size());
            
            brush.vertices.push_back({{x1, center.y - halfHeight, z1}, normal1, {u1, 0}});
            brush.vertices.push_back({{x2, center.y - halfHeight, z2}, normal2, {u2, 0}});
            brush.vertices.push_back({{x2, center.y + halfHeight, z2}, normal2, {u2, 1}});
            brush.vertices.push_back({{x1, center.y + halfHeight, z1}, normal1, {u1, 1}});
            
            brush.indices.push_back(base + 0);
            brush.indices.push_back(base + 1);
            brush.indices.push_back(base + 2);
            brush.indices.push_back(base + 0);
            brush.indices.push_back(base + 2);
            brush.indices.push_back(base + 3);
        }
        
        // Top cap
        uint32_t topCenter = static_cast<uint32_t>(brush.vertices.size());
        brush.vertices.push_back({{center.x, center.y + halfHeight, center.z}, {0, 1, 0}, {0.5f, 0.5f}});
        
        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            float x = center.x + radius * std::cos(angle);
            float z = center.z + radius * std::sin(angle);
            brush.vertices.push_back({{x, center.y + halfHeight, z}, {0, 1, 0}, 
                {0.5f + 0.5f * std::cos(angle), 0.5f + 0.5f * std::sin(angle)}});
        }
        
        for (int i = 0; i < segments; i++) {
            brush.indices.push_back(topCenter);
            brush.indices.push_back(topCenter + 1 + i);
            brush.indices.push_back(topCenter + 1 + (i + 1) % segments);
        }
        
        // Bottom cap
        uint32_t bottomCenter = static_cast<uint32_t>(brush.vertices.size());
        brush.vertices.push_back({{center.x, center.y - halfHeight, center.z}, {0, -1, 0}, {0.5f, 0.5f}});
        
        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            float x = center.x + radius * std::cos(angle);
            float z = center.z + radius * std::sin(angle);
            brush.vertices.push_back({{x, center.y - halfHeight, z}, {0, -1, 0}, 
                {0.5f + 0.5f * std::cos(angle), 0.5f + 0.5f * std::sin(angle)}});
        }
        
        for (int i = 0; i < segments; i++) {
            brush.indices.push_back(bottomCenter);
            brush.indices.push_back(bottomCenter + 1 + (i + 1) % segments);
            brush.indices.push_back(bottomCenter + 1 + i);
        }
        
        return brush;
    }
    
    static Brush CreateWedge(Map& map, Vec3 min, Vec3 max) {
        Brush brush;
        brush.id = map.nextBrushID++;
        brush.flags = BRUSH_SOLID;
        
        // Calculate slope normal
        float dx = max.x - min.x;
        float dy = max.y - min.y;
        float len = std::sqrt(dx * dx + dy * dy);
        Vec3 slopeNormal(-dy / len, dx / len, 0);
        
        // Front triangle (+Z)
        brush.vertices.push_back({{min.x, min.y, max.z}, {0, 0, 1}, {0, 0}});
        brush.vertices.push_back({{max.x, min.y, max.z}, {0, 0, 1}, {1, 0}});
        brush.vertices.push_back({{max.x, max.y, max.z}, {0, 0, 1}, {1, 1}});
        
        // Back triangle (-Z)
        brush.vertices.push_back({{max.x, min.y, min.z}, {0, 0, -1}, {0, 0}});
        brush.vertices.push_back({{min.x, min.y, min.z}, {0, 0, -1}, {1, 0}});
        brush.vertices.push_back({{max.x, max.y, min.z}, {0, 0, -1}, {0, 1}});
        
        // Bottom face (-Y)
        brush.vertices.push_back({{min.x, min.y, min.z}, {0, -1, 0}, {0, 0}});
        brush.vertices.push_back({{max.x, min.y, min.z}, {0, -1, 0}, {1, 0}});
        brush.vertices.push_back({{max.x, min.y, max.z}, {0, -1, 0}, {1, 1}});
        brush.vertices.push_back({{min.x, min.y, max.z}, {0, -1, 0}, {0, 1}});
        
        // Slope face
        brush.vertices.push_back({{min.x, min.y, max.z}, slopeNormal, {0, 0}});
        brush.vertices.push_back({{max.x, max.y, max.z}, slopeNormal, {1, 1}});
        brush.vertices.push_back({{max.x, max.y, min.z}, slopeNormal, {1, 0}});
        brush.vertices.push_back({{min.x, min.y, min.z}, slopeNormal, {0, 1}});
        
        // Right face (+X)
        brush.vertices.push_back({{max.x, min.y, max.z}, {1, 0, 0}, {0, 0}});
        brush.vertices.push_back({{max.x, min.y, min.z}, {1, 0, 0}, {1, 0}});
        brush.vertices.push_back({{max.x, max.y, min.z}, {1, 0, 0}, {1, 1}});
        brush.vertices.push_back({{max.x, max.y, max.z}, {1, 0, 0}, {0, 1}});
        
        // Indices
        // Front triangle
        brush.indices.push_back(0);
        brush.indices.push_back(1);
        brush.indices.push_back(2);
        
        // Back triangle
        brush.indices.push_back(3);
        brush.indices.push_back(4);
        brush.indices.push_back(5);
        
        // Bottom quad
        brush.indices.push_back(6);
        brush.indices.push_back(7);
        brush.indices.push_back(8);
        brush.indices.push_back(6);
        brush.indices.push_back(8);
        brush.indices.push_back(9);
        
        // Slope quad
        brush.indices.push_back(10);
        brush.indices.push_back(11);
        brush.indices.push_back(12);
        brush.indices.push_back(10);
        brush.indices.push_back(12);
        brush.indices.push_back(13);
        
        // Right quad
        brush.indices.push_back(14);
        brush.indices.push_back(15);
        brush.indices.push_back(16);
        brush.indices.push_back(14);
        brush.indices.push_back(16);
        brush.indices.push_back(17);
        
        return brush;
    }
    
    static Entity CreateEntity(Map& map, EntityType type, Vec3 position) {
        Entity ent;
        ent.id = map.nextEntityID++;
        ent.type = type;
        ent.position = position;
        ent.scale = Vec3(1, 1, 1);
        
        // Set default properties
        switch (type) {
            case ENT_LIGHT:
            case ENT_LIGHT_SPOT:
                ent.SetProperty("color_r", "1");
                ent.SetProperty("color_g", "1");
                ent.SetProperty("color_b", "1");
                ent.SetProperty("intensity", "1");
                ent.SetProperty("radius", "10");
                break;
            case ENT_ITEM_HEALTH:
                ent.SetProperty("amount", "25");
                ent.SetProperty("respawn_time", "30");
                break;
            case ENT_ITEM_ARMOR:
                ent.SetProperty("amount", "50");
                ent.SetProperty("respawn_time", "30");
                break;
            case ENT_ITEM_AMMO:
                ent.SetProperty("amount", "10");
                ent.SetProperty("respawn_time", "15");
                break;
            case ENT_TRIGGER_HURT:
                ent.SetProperty("damage", "10");
                break;
            case ENT_TRIGGER_PUSH:
                ent.SetProperty("force_x", "0");
                ent.SetProperty("force_y", "10");
                ent.SetProperty("force_z", "0");
                break;
            default:
                break;
        }
        
        return ent;
    }
};

} // namespace PCD

#endif // PCD_BRUSH_FACTORY_H
