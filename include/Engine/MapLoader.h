#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>

struct Vec3f {
    float x, y, z;
    Vec3f() : x(0), y(0), z(0) {}
    Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3f operator-(const Vec3f& v) const { return Vec3f(x-v.x, y-v.y, z-v.z); }
    Vec3f cross(const Vec3f& v) const {
        return Vec3f(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
    }
    Vec3f normalized() const {
        float len = sqrt(x*x + y*y + z*z);
        return len > 0 ? Vec3f(x/len, y/len, z/len) : Vec3f(0,0,0);
    }
};

struct MapPlane {
    Vec3f p1, p2, p3;
    std::string texture;
    float offsetX, offsetY, rotation, scaleX, scaleY;
};

struct MapBrush {
    std::vector<MapPlane> planes;
};

struct MapEntity {
    std::string classname;
    std::vector<MapBrush> brushes;
    std::vector<std::pair<std::string, std::string>> properties;
    
    std::string GetProperty(const std::string& key, const std::string& defaultVal = "") const {
        for (const auto& [k, v] : properties) {
            if (k == key) return v;
        }
        return defaultVal;
    }
};

struct MapGeometry {
    std::vector<float> vertices;  // x,y,z,nx,ny,nz per vertex
    std::vector<unsigned int> indices;
    std::string texture;
};

class MapLoader {
private:
    std::vector<MapEntity> entities;

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    Vec3f parseVec3(const std::string& str) {
        std::stringstream ss(str);
        Vec3f v;
        ss >> v.x >> v.y >> v.z;
        return v;
    }

    MapPlane parsePlane(const std::string& line) {
        MapPlane plane;
        size_t pos = 0;
        
        // Parse 3 points
        pos = line.find('(', pos);
        size_t end = line.find(')', pos);
        plane.p1 = parseVec3(line.substr(pos+1, end-pos-1));
        
        pos = line.find('(', end);
        end = line.find(')', pos);
        plane.p2 = parseVec3(line.substr(pos+1, end-pos-1));
        
        pos = line.find('(', end);
        end = line.find(')', pos);
        plane.p3 = parseVec3(line.substr(pos+1, end-pos-1));
        
        // Parse texture and alignment
        std::stringstream ss(line.substr(end+1));
        ss >> plane.texture >> plane.offsetX >> plane.offsetY >> plane.rotation >> plane.scaleX >> plane.scaleY;
        
        return plane;
    }

public:
    bool Load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;

        std::string line;
        MapEntity* currentEntity = nullptr;
        MapBrush* currentBrush = nullptr;

        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '/' || line[0] == '#') continue;

            if (line == "{") {
                if (!currentEntity) {
                    entities.push_back(MapEntity());
                    currentEntity = &entities.back();
                } else if (currentEntity && !currentBrush) {
                    currentEntity->brushes.push_back(MapBrush());
                    currentBrush = &currentEntity->brushes.back();
                }
            } else if (line == "}") {
                if (currentBrush) {
                    currentBrush = nullptr;
                } else if (currentEntity) {
                    currentEntity = nullptr;
                }
            } else if (line[0] == '"' && currentEntity && !currentBrush) {
                // Parse property
                size_t quote1 = 0;
                size_t quote2 = line.find('"', quote1 + 1);
                size_t quote3 = line.find('"', quote2 + 1);
                size_t quote4 = line.find('"', quote3 + 1);
                
                if (quote2 != std::string::npos && quote4 != std::string::npos) {
                    std::string key = line.substr(quote1 + 1, quote2 - quote1 - 1);
                    std::string value = line.substr(quote3 + 1, quote4 - quote3 - 1);
                    currentEntity->properties.push_back({key, value});
                    
                    if (key == "classname") {
                        currentEntity->classname = value;
                    }
                }
            } else if (line[0] == '(' && currentBrush) {
                // Parse plane
                currentBrush->planes.push_back(parsePlane(line));
            }
        }

        return !entities.empty();
    }

    const std::vector<MapEntity>& GetEntities() const {
        return entities;
    }

    // Convert brushes to renderable geometry (simplified CSG)
    std::vector<MapGeometry> BuildGeometry() const {
        std::vector<MapGeometry> geometries;

        for (const auto& entity : entities) {
            for (const auto& brush : entity.brushes) {
                if (brush.planes.size() < 4) continue;

                // For each plane, generate a quad (simplified - real CSG is complex)
                for (const auto& plane : brush.planes) {
                    MapGeometry geom;
                    geom.texture = plane.texture;

                    // Calculate plane normal
                    Vec3f v1 = plane.p2 - plane.p1;
                    Vec3f v2 = plane.p3 - plane.p1;
                    Vec3f normal = v1.cross(v2).normalized();

                    // Generate quad vertices (simplified)
                    Vec3f center((plane.p1.x + plane.p2.x + plane.p3.x) / 3.0f,
                                (plane.p1.y + plane.p2.y + plane.p3.y) / 3.0f,
                                (plane.p1.z + plane.p2.z + plane.p3.z) / 3.0f);

                    // Add triangle (p1, p2, p3)
                    geom.vertices = {
                        plane.p1.x, plane.p1.y, plane.p1.z, normal.x, normal.y, normal.z,
                        plane.p2.x, plane.p2.y, plane.p2.z, normal.x, normal.y, normal.z,
                        plane.p3.x, plane.p3.y, plane.p3.z, normal.x, normal.y, normal.z
                    };
                    geom.indices = {0, 1, 2};

                    geometries.push_back(geom);
                }
            }
        }

        return geometries;
    }

    // Extract spawn points
    std::vector<Vec3f> GetSpawnPoints() const {
        std::vector<Vec3f> spawns;
        for (const auto& entity : entities) {
            if (entity.classname == "info_player_start" || 
                entity.classname == "info_player_deathmatch") {
                std::string origin = entity.GetProperty("origin", "0 0 0");
                spawns.push_back(parseVec3(origin));
            }
        }
        return spawns;
    }

    // Get worldspawn (geometry)
    const MapEntity* GetWorldspawn() const {
        for (const auto& entity : entities) {
            if (entity.classname == "worldspawn") return &entity;
        }
        return nullptr;
    }
};

#endif
