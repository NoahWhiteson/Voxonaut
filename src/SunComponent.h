#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

class SunComponent {
public:
    Vector3 position;
    Color color;
    float ambientIntensity;
    float realRadius;
    int voxelRes;
    std::vector<Vector3> sunOffsets;

    SunComponent(Vector3 pos, Color col, float ambient, float radius, int res)
        : position(pos), color(col), ambientIntensity(ambient), realRadius(radius), voxelRes(res) 
    {
        // Calculate the sun as a spherical shell just like the earth, using dynamic block resolutions
        float rInnerSq = (res - 1.5f) * (res - 1.5f);
        float rOuterSq = (float)(res * res);
        
        for(int x = -res; x <= res; x++) {
            for(int y = -res; y <= res; y++) {
                for(int z = -res; z <= res; z++) {
                    float distSq = (float)(x*x + y*y + z*z);
                    if (distSq <= rOuterSq && distSq >= rInnerSq) {
                        // Store normalized offset vectors
                        sunOffsets.push_back(Vector3{(float)x/(float)res, (float)y/(float)res, (float)z/(float)res});
                    }
                }
            }
        }
    }

    // Mathematically scales the sun down to bypass OpenGL's native 1000.0 Far-Plane clipping
    void Draw(Camera3D camera) {
        // Find exactly how far the sun is to maintain pure angular precision
        float scientificDistance = Vector3Distance(camera.position, position);
        Vector3 dirToSun = Vector3Normalize(Vector3Subtract(position, camera.position));
        
        // Anchor onto a 4500-unit skybox so the sun is ALWAYS behind the planet (radius 500)
        Vector3 skyboxPos = Vector3Add(camera.position, Vector3Scale(dirToSun, 4500.0f));
        
        // Correcting visual scale to maintain scientific angular diameter at the new deeper distance
        float visualRadius = realRadius * (4500.0f / scientificDistance);
        float vSize = (visualRadius * 2.0f) / (voxelRes * 2.0f);

        for (const auto& normOff : sunOffsets) {
            Vector3 localPos = Vector3Add(skyboxPos, Vector3Scale(normOff, visualRadius));
            
            // Render the shell representing the flaming surface
            DrawCube(localPos, vSize, vSize, vSize, color);
            // Render the interior bright core
            DrawCube(localPos, vSize * 0.8f, vSize * 0.8f, vSize * 0.8f, WHITE);
        }
    }

    void ApplyToShader(Shader shader, Vector3 targetPos = {0.0f, 0.0f, 0.0f}) {
        int lightDirLoc = GetShaderLocation(shader, "lightDir");
        int lightColLoc = GetShaderLocation(shader, "lightColor");
        int ambientLoc = GetShaderLocation(shader, "ambient");
        
        // Emits strictly parallel light rays from the millions of units away exactly like real photon paths
        Vector3 lightDir = Vector3Normalize(Vector3Subtract(position, targetPos));
        float lightColorNormalized[4] = { 
            (float)color.r / 255.0f, 
            (float)color.g / 255.0f, 
            (float)color.b / 255.0f, 
            1.0f 
        };
        
        SetShaderValue(shader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, lightColLoc, lightColorNormalized, SHADER_UNIFORM_VEC4);
        SetShaderValue(shader, ambientLoc, &ambientIntensity, SHADER_UNIFORM_FLOAT);
    }
};
