#pragma once
#include "raylib.h"
#include <vector>
#include <cmath>
#include <unordered_map>

struct Voxel {
    Vector3 pos;
    Color col;
};

struct Chunk {
    std::vector<Voxel> terrain;
    std::vector<Voxel> clouds;
    Mesh terrainMesh;
    Mesh cloudMesh;
    bool loaded = false;
    bool meshReady = false;
};

// Procedural 3D Noise Generator (Perlin-based)
class PlanetGenerator {
    int p[512];
    
    float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
    float lerp(float t, float a, float b) { return a + t * (b - a); }
    float grad(int hash, float x, float y, float z) {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
    
public:
    PlanetGenerator() {
        int permutation[256] = { 151,160,137,91,90,15,
        131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
        190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
        88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
        77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
        102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
        135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
        5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
        223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
        129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
        251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
        49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180 };
        for (int i=0; i < 256 ; i++) {
            p[i] = permutation[i];
            p[256+i] = permutation[i];
        }
    }

    float perlin3d(float x, float y, float z) {
        int X = (int)floor(x) & 255; int Y = (int)floor(y) & 255; int Z = (int)floor(z) & 255;
        x -= floor(x); y -= floor(y); z -= floor(z);
        float u = fade(x); float v = fade(y); float w = fade(z);
        int A = p[X]+Y, AA = p[A]+Z, AB = p[A+1]+Z;
        int B = p[X+1]+Y, BA = p[B]+Z, BB = p[B+1]+Z;
        return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x-1, y, z)),
                               lerp(u, grad(p[AB], x, y-1, z), grad(p[BB], x-1, y-1, z))),
                       lerp(v, lerp(u, grad(p[AA+1], x, y, z-1), grad(p[BA+1], x-1, y, z-1)),
                               lerp(u, grad(p[AB+1], x, y-1, z-1), grad(p[BB+1], x-1, y-1, z-1))));
    }

    float FBM(float x, float y, float z) {
        float total = 0.0f; float frequency = 1.0f; float amplitude = 1.0f; float maxV = 0.0f;
        for(int i=0;i<4;i++) {
            total += perlin3d(x*frequency, y*frequency, z*frequency) * amplitude;
            maxV += amplitude; amplitude *= 0.5f; frequency *= 2.0f;
        }
        return total / maxV;
    }

    // Unified Noise Logic for both Space Planet and Surface Chunks
    void EvaluatePoint(int x, int y, int z, float terrainScale, float cloudScale, int rSq, int rInnerSq, int rCloudSq, int rCloudInnerSq, float blockSize, std::vector<Voxel>& terrain, std::vector<Voxel>& clouds) {
        float distSq = (float)(x * x + y * y + z * z);
        float dist = sqrtf(distSq);
        
        Color colDeepWater    = { 20, 60, 140, 255 };
        Color colShallowWater = { 40, 110, 180, 255 };
        Color colSand         = { 210, 190, 120, 255 };
        Color colGrass        = { 85, 170, 75, 255 };
        Color colForest       = { 45, 120, 55, 255 };
        Color colSnow         = { 245, 250, 255, 255 };
        Color colCloud        = { 255, 255, 255, 210 };

        if (distSq <= (float)rSq && distSq >= (float)rInnerSq) {
            float noiseVal = FBM(x * terrainScale, y * terrainScale, z * terrainScale);
            // Height-based biome refinement for poles
            float normalizedY = (float)y / (sqrtf((float)rSq));
            
            Color color;
            if (fabsf(normalizedY) > 0.88f && noiseVal > -0.1f) color = colSnow; // Snowy Poles
            else if (noiseVal > 0.18f) color = colForest;
            else if (noiseVal > 0.04f) color = colGrass;
            else if (noiseVal > -0.05f) color = colSand;
            else if (noiseVal > -0.22f) color = colShallowWater;
            else color = colDeepWater;
            
            terrain.push_back({Vector3{(float)x * blockSize, (float)y * blockSize, (float)z * blockSize}, color});
        }
        
        if (distSq <= (float)rCloudSq && distSq >= (float)rCloudInnerSq) {
            float cloudNoise = FBM(x * cloudScale + 100.0f, y * cloudScale + 100.0f, z * cloudScale + 100.0f);
            if (cloudNoise > 0.11f) {
                clouds.push_back({Vector3{(float)x * blockSize, (float)y * blockSize, (float)z * blockSize}, colCloud});
            }
        }
    }

    void GenerateEarthData(int radius, float blockSize, std::vector<Voxel>& terrain, std::vector<Voxel>& clouds) {
        int rSq = radius * radius;
        // Moderate thickness (radius - 5.0) provides perfect star-blocking with great performance
        int rInnerSq = (radius - 5.0f) * (radius - 5.0f);
        int rCloudSq = (radius + 2) * (radius + 2);
        int rCloudInnerSq = (radius - 1) * (radius - 1);
        
        float terrainScale = 0.08f * (34.0f / radius); 
        float cloudScale   = 0.04f * (34.0f / radius);

        int bound = radius + 3;
        for (int x = -bound; x <= bound; x++) {
            for (int y = -bound; y <= bound; y++) {
                for (int z = -bound; z <= bound; z++) {
                    EvaluatePoint(x, y, z, terrainScale, cloudScale, rSq, rInnerSq, rCloudSq, rCloudInnerSq, blockSize, terrain, clouds);
                }
            }
        }
    }

    void GenerateChunkData(int cx, int cy, int cz, int chunkSize, int planetRadius, std::vector<Voxel>& terrain, std::vector<Voxel>& clouds) {
        int rSq = planetRadius * planetRadius;
        int rInnerSq = (planetRadius - 2) * (planetRadius - 2);
        int rCloudSq = (planetRadius + 2) * (planetRadius + 2);
        int rCloudInnerSq = (planetRadius - 1) * (planetRadius - 1);
        
        float terrainScale = 0.08f * (34.0f / planetRadius); 
        float cloudScale   = 0.04f * (34.0f / planetRadius);

        for (int x = cx; x < cx + chunkSize; x++) {
            for (int y = cy; y < cy + chunkSize; y++) {
                for (int z = cz; z < cz + chunkSize; z++) {
                    EvaluatePoint(x, y, z, terrainScale, cloudScale, rSq, rInnerSq, rCloudSq, rCloudInnerSq, 1.0f, terrain, clouds);
                }
            }
        }
    }

    Mesh CompileToMesh(const std::vector<Voxel>& voxels, float blockSize) {
        if (voxels.empty()) return Mesh{0};
        
        // OPTIMIZATION: Use a temporary 3D spatial map to perform Hidden Face Removal (HFR)
        // This cuts triangle counts by 80-90% because we never render internal voxel faces.
        std::unordered_map<long long, bool> voxelMap;
        for (const auto& v : voxels) {
            long long key = ((long long)(v.pos.x/blockSize) + 1000000) ^ 
                            (((long long)(v.pos.y/blockSize) + 1000000) << 20) ^ 
                            (((long long)(v.pos.z/blockSize) + 1000000) << 40);
            voxelMap[key] = true;
        }

        auto isFilled = [&](float x, float y, float z) {
            long long key = ((long long)(x/blockSize) + 1000000) ^ 
                            (((long long)(y/blockSize) + 1000000) << 20) ^ 
                            (((long long)(z/blockSize) + 1000000) << 40);
            return voxelMap.count(key) > 0;
        };

        std::vector<float> verts, norms, texs;
        std::vector<unsigned char> cols;

        float h = blockSize / 2.0f;
        for (const auto& vox : voxels) {
            float x = vox.pos.x, y = vox.pos.y, z = vox.pos.z;
            Color c = vox.col;

            auto addFace = [&](Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4, Vector3 n) {
                verts.insert(verts.end(), {v1.x,v1.y,v1.z, v3.x,v3.y,v3.z, v2.x,v2.y,v2.z, v3.x,v3.y,v3.z, v4.x,v4.y,v4.z, v2.x,v2.y,v2.z});
                for(int i=0; i<6; i++) {
                    norms.insert(norms.end(), {n.x, n.y, n.z});
                    cols.insert(cols.end(), {c.r, c.g, c.b, c.a});
                    texs.insert(texs.end(), {0.0f, 0.0f});
                }
            };

            // Only build faces that are touching AIR
            if (!isFilled(x, y, z + blockSize)) addFace({x-h, y+h, z+h}, {x+h, y+h, z+h}, {x-h, y-h, z+h}, {x+h, y-h, z+h}, {0,0,1}); 
            if (!isFilled(x, y, z - blockSize)) addFace({x+h, y+h, z-h}, {x-h, y+h, z-h}, {x+h, y-h, z-h}, {x-h, y-h, z-h}, {0,0,-1}); 
            if (!isFilled(x, y + blockSize, z)) addFace({x-h, y+h, z-h}, {x+h, y+h, z-h}, {x-h, y+h, z+h}, {x+h, y+h, z+h}, {0,1,0});
            if (!isFilled(x, y - blockSize, z)) addFace({x-h, y-h, z+h}, {x+h, y-h, z+h}, {x-h, y-h, z-h}, {x+h, y-h, z-h}, {0,-1,0}); 
            if (!isFilled(x + blockSize, y, z)) addFace({x+h, y+h, z+h}, {x+h, y+h, z-h}, {x+h, y-h, z+h}, {x+h, y-h, z-h}, {1,0,0}); 
            if (!isFilled(x - blockSize, y, z)) addFace({x-h, y+h, z-h}, {x-h, y+h, z+h}, {x-h, y-h, z-h}, {x-h, y-h, z+h}, {-1,0,0}); 
        }

        Mesh m = {0};
        m.vertexCount = (int)verts.size() / 3;
        m.triangleCount = m.vertexCount / 3;
        m.vertices = (float*)MemAlloc(verts.size() * sizeof(float));
        memcpy(m.vertices, verts.data(), verts.size() * sizeof(float));
        m.normals = (float*)MemAlloc(norms.size() * sizeof(float));
        memcpy(m.normals, norms.data(), norms.size() * sizeof(float));
        m.colors = (unsigned char*)MemAlloc(cols.size() * sizeof(unsigned char));
        memcpy(m.colors, cols.data(), cols.size() * sizeof(unsigned char));
        m.texcoords = (float*)MemAlloc(texs.size() * sizeof(float));
        memcpy(m.texcoords, texs.data(), texs.size() * sizeof(float));

        UploadMesh(&m, false);
        return m;
    }
};
