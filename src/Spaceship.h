#pragma once
#include "raylib.h"
#include <vector>
#include <unordered_map>

// Part Types (mapped to colors for now)
enum class PartType { HULL, THRUSTER, GYRO, FUEL, COCKPIT };

struct Part {
    Vector3 pos; // Local grid position
    PartType type;
    Color color;
};

struct ShipGridPos {
    int x, y, z;
    bool operator==(const ShipGridPos& o) const { return x == o.x && y == o.y && z == o.z; }
};

struct ShipGridHash {
    size_t operator()(const ShipGridPos& k) const {
        return ((size_t)k.x * 73856093) ^ ((size_t)k.y * 19349663) ^ ((size_t)k.z * 83492791);
    }
};

class Spaceship {
public:
    std::unordered_map<ShipGridPos, Part, ShipGridHash> parts;
    Mesh shipMesh = {0};
    Model shipModel = {0};
    bool needsUpdate = false;

    void AddPart(int x, int y, int z, PartType type) {
        Color c = WHITE;
        if (type == PartType::HULL) c = GRAY;
        else if (type == PartType::THRUSTER) c = RED;
        else if (type == PartType::GYRO) c = SKYBLUE;
        else if (type == PartType::FUEL) c = ORANGE;
        else if (type == PartType::COCKPIT) c = LIME;
        
        parts[{x, y, z}] = {{ (float)x, (float)y, (float)z }, type, c};
        needsUpdate = true;
    }

    void RemovePart(int x, int y, int z) {
        parts.erase({x, y, z});
        needsUpdate = true;
    }

    void AddArtisticVoxels(std::vector<float>& verts, std::vector<float>& norms, std::vector<unsigned char>& cols, Vector3 center, PartType type, Color baseCol) {
        float s = 1.0f; // Main block size
        float hs = s/2.0f;
        
        auto addV = [&](float x, float y, float z, float sz, Color c) {
            float h = sz/2.0f;
            auto f = [&](Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4, Vector3 n) {
                verts.insert(verts.end(), {v1.x,v1.y,v1.z, v3.x,v3.y,v3.z, v2.x,v2.y,v2.z, v3.x,v3.y,v3.z, v4.x,v4.y,v4.z, v2.x,v2.y,v2.z});
                for(int i=0; i<6; i++) { norms.insert(norms.end(), {n.x,n.y,n.z}); cols.insert(cols.end(), {c.r,c.g,c.b,c.a}); }
            };
            f({x-h, y+h, z+h}, {x+h, y+h, z+h}, {x-h, y-h, z+h}, {x+h, y-h, z+h}, {0,0,1}); f({x+h, y+h, z-h}, {x-h, y+h, z-h}, {x+h, y-h, z-h}, {x-h, y-h, z-h}, {0,0,-1});
            f({x-h, y+h, z-h}, {x+h, y+h, z-h}, {x-h, y+h, z+h}, {x+h, y+h, z+h}, {0,1,0}); f({x-h, y-h, z+h}, {x+h, y-h, z+h}, {x-h, y-h, z-h}, {x+h, y-h, z-h}, {0,-1,0});
            f({x+h, y+h, z+h}, {x+h, y+h, z-h}, {x+h, y-h, z+h}, {x+h, y-h, z-h}, {1,0,0}); f({x-h, y+h, z-h}, {x-h, y+h, z+h}, {x-h, y-h, z-h}, {x-h, y-h, z+h}, {-1,0,0});
        };

        if (type == PartType::HULL) addV(center.x, center.y, center.z, 0.95f, baseCol);
        else if (type == PartType::THRUSTER) {
            // Voxel Art Nozzle (conical)
            for(float i=0; i<0.8f; i+=0.2f) addV(center.x, center.y - 0.4f + i, center.z, 0.8f - i*0.5f, (i < 0.2f) ? ORANGE : DARKGRAY);
            addV(center.x, center.y + 0.3f, center.z, 0.9f, baseCol);
        } else if (type == PartType::GYRO) {
            // Spinning ring look
            addV(center.x, center.y, center.z, 0.4f, SKYBLUE);
            addV(center.x+0.3f, center.y, center.z, 0.2f, WHITE); addV(center.x-0.3f, center.y, center.z, 0.2f, WHITE);
            addV(center.x, center.y, center.z+0.3f, 0.2f, WHITE); addV(center.x, center.y, center.z-0.3f, 0.2f, WHITE);
        } else if (type == PartType::FUEL) {
            // Cylindrical tank
            for(float i=-0.4f; i<=0.4f; i+=0.2f) addV(center.x, center.y+i, center.z, 0.85f, baseCol);
            addV(center.x, center.y+0.45f, center.z, 0.6f, YELLOW);
        } else if (type == PartType::COCKPIT) {
            // Glass dome
            addV(center.x, center.y-0.2f, center.z, 0.95f, baseCol);
            addV(center.x, center.y+0.2f, center.z, 0.7f, BLUE); 
        }
    }

    void UpdateMesh() {
        if (!needsUpdate) return;
        
        // CRASH FIX: UnloadModel already frees its internal meshes! 
        // Manual UnloadMesh() afterward causes a 'Double Free' corruption.
        if (shipModel.meshCount > 0) {
            UnloadModel(shipModel);
            shipModel = {0};
            shipMesh = {0}; // Mesh was already freed by UnloadModel
        }

        std::vector<float> verts, norms;
        std::vector<unsigned char> cols;

        for (auto const& [pos, part] : parts) {
            AddArtisticVoxels(verts, norms, cols, part.pos, part.type, part.color);
        }

        if (verts.empty()) { needsUpdate = false; return; }

        shipMesh.vertexCount = (int)verts.size() / 3;
        shipMesh.triangleCount = shipMesh.vertexCount / 3;
        shipMesh.vertices = (float*)MemAlloc(verts.size() * sizeof(float));
        memcpy(shipMesh.vertices, verts.data(), verts.size() * sizeof(float));
        shipMesh.normals = (float*)MemAlloc(norms.size() * sizeof(float));
        memcpy(shipMesh.normals, norms.data(), norms.size() * sizeof(float));
        shipMesh.colors = (unsigned char*)MemAlloc(cols.size() * sizeof(unsigned char));
        memcpy(shipMesh.colors, cols.data(), cols.size() * sizeof(unsigned char));
        
        UploadMesh(&shipMesh, false);
        shipModel = LoadModelFromMesh(shipMesh);
        needsUpdate = false;
    }

    struct HitInfo { bool hit; Vector3 pos; Vector3 normal; };
    HitInfo GetHitInfo(Ray ray) {
        HitInfo best = {false, {0,0,0}, {0,0,0}};
        float minDist = 10000.0f;
        for (auto const& [pos, part] : parts) {
            RayCollision col = GetRayCollisionBox(ray, {Vector3{part.pos.x-0.5f, part.pos.y-0.5f, part.pos.z-0.5f}, 
                                                        Vector3{part.pos.x+0.5f, part.pos.y+0.5f, part.pos.z+0.5f}});
            if (col.hit && col.distance < minDist) {
                minDist = col.distance;
                best = {true, part.pos, col.normal};
            }
        }
        return best;
    }
};
