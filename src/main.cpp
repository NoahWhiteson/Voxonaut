#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "PlanetGenerator.h"
#include "SunComponent.h"
#include "Spaceship.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <future>
#include <algorithm>

enum GameState { MENU, SETTINGS, EXPLORE, BUILD };
enum WindowMode { MODE_WINDOWED, MODE_BORDERLESS, MODE_FULLSCREEN };

struct Star {
    Vector3 pos;
    float size;
    Color col;
};

struct ChunkPos {
    int x, y, z;
    bool operator==(const ChunkPos& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct ChunkPosHash {
    std::size_t operator()(const ChunkPos& k) const {
        return ((std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1)) >> 1) ^ (std::hash<int>()(k.z) << 1);
    }
};

struct AsyncChunk {
    Chunk data;
    std::future<void> worker;
    bool processing = false;
};

const char* vsLighting = R"(
#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec4 fragColor;
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
void main() {
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));
    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

const char* fsLighting = R"(
#version 330
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;
out vec4 finalColor;
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform float ambient;
uniform vec3 viewPos;
uniform float fogDensity;
uniform vec4 fogColor;

void main() {
    vec3 n = normalize(fragNormal);
    vec3 l = normalize(lightDir);
    vec3 v = normalize(viewPos - fragPosition);

    float distFromCenter = length(fragPosition);
    float NdotL = dot(n, l);
    float diffuseFactor = max(NdotL, 0.0);

    float atmosphereHeight = 60.0;
    float planetRadius = 1000.0;
    float alt = max(distFromCenter - planetRadius, 0.0);
    float atmosphereDensity = exp(-alt / 22.0) * clamp((atmosphereHeight - alt) / atmosphereHeight, 0.0, 1.0);

    float sunLitSide = smoothstep(-0.2, 0.6, dot(normalize(fragPosition), l));
    float rimFlare = pow(1.0 - max(dot(v, n), 0.0), 3.0) * 1.8 * sunLitSide;
    vec3 rimCol = fogColor.rgb * rimFlare * atmosphereDensity;

    vec3 h = normalize(l + v);
    float spec = (fragColor.r < 0.3 && fragColor.b > 0.5) ? pow(max(dot(n, h), 0.0), 128.0) * 0.9 : 0.0;

    vec3 spaceAmbient = vec3(0.06, 0.09, 0.16);
    vec3 atmosphereGlow = (fogColor.rgb * ambient * 0.5) * atmosphereDensity;
    vec3 totalAmbient = max(spaceAmbient, atmosphereGlow);
    vec3 finalRGB = fragColor.rgb * (totalAmbient + lightColor.rgb * diffuseFactor) + (lightColor.rgb * spec) + rimCol;

    vec3 skyMix = mix(finalRGB, fogColor.rgb * sunLitSide, atmosphereDensity * 0.7);
    finalRGB = mix(finalRGB, skyMix, atmosphereDensity);

    finalRGB = vec3(1.0) - exp(-finalRGB * 1.1);
    finalColor = vec4(finalRGB, fragColor.a);
}
)";

#include "UIComponent.h"

int main() {
    int sw = 800, sh = 600;
    InitWindow(sw, sh, "Planetary Engineering");
    SetTargetFPS(60);
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    UIComponent ui;
    std::string lastError = "";
    bool crashed = false;
    Shader shader = { 0 };

    try {
        GameState state = EXPLORE;
        PartType selectedPart = PartType::HULL;
        Spaceship ship;
        ship.AddPart(0, 0, 0, PartType::COCKPIT);

        Camera3D camera = { 0 };
        camera.target = { 0, 0, 0 };
        camera.up = { 0, 1, 0 };
        camera.fovy = 50.0f;
        camera.projection = CAMERA_PERSPECTIVE;

        float cameraAngleX = 0.0f, cameraAngleY = 0.5f, cameraRadius = 3000.0f;
        const int PLANET_RADIUS = 1000;
        const int CHUNK_SIZE = 32;
        const float INT_PROXY_ACTIVATE = 2200.0f;
        const float SURFACE_ACTIVATE_RADIUS = 1500.0f;

        PlanetGenerator pg;
        std::vector<Voxel> spaceT, spaceC;
        pg.GenerateEarthData(85, 12.0f, spaceT, spaceC);
        Mesh spaceTerrainMesh = pg.CompileToMesh(spaceT, 12.0f);
        Mesh spaceCloudMesh = pg.CompileToMesh(spaceC, 12.0f);

        std::vector<Voxel> intT, intC;
        pg.GenerateEarthData(200, 5.0f, intT, intC);
        Mesh intTerrainMesh = pg.CompileToMesh(intT, 5.0f);
        Mesh intCloudMesh = pg.CompileToMesh(intC, 5.0f);

        Mesh coreSphere = GenMeshSphere((float)PLANET_RADIUS - 10.0f, 64, 64);
        Model coreModel = LoadModelFromMesh(coreSphere);

        std::unordered_map<ChunkPos, AsyncChunk, ChunkPosHash> chunkCache;
        shader = LoadShaderFromMemory(vsLighting, fsLighting);
        int viewPosLoc = GetShaderLocation(shader, "viewPos");
        int fogDensityLoc = GetShaderLocation(shader, "fogDensity");
        int fogColorLoc = GetShaderLocation(shader, "fogColor");

        Material mat = LoadMaterialDefault();
        mat.shader = shader;
        coreModel.materials[0].shader = shader;

        SunComponent sun({ 1000000.0f, 0.0f, 0.0f }, { 255, 230, 180, 255 }, 0.05f, 20000.0f, 8);

        std::vector<Star> stars;
        for (int i = 0; i < 3000; i++) {
            float theta = GetRandomValue(0, 360) * DEG2RAD;
            float phi = acosf(2.0f * (GetRandomValue(0, 1000) / 1000.0f) - 1.0f);
            Vector3 pos = { 4000.0f*sinf(phi)*cosf(theta), 4000.0f*cosf(phi), 4000.0f*sinf(phi)*sinf(theta) };
            stars.push_back({ pos, (float)GetRandomValue(5, 15) / 10.0f, {255, 255, 255, 255} });
        }

        while (!WindowShouldClose()) {
            float dt = GetFrameTime();
            sw = GetScreenWidth();
            sh = GetScreenHeight();

            if (ui.currentScreen == UIComponent::EXPLORE) state = EXPLORE;
            if (ui.currentScreen == UIComponent::BUILD) state = BUILD;
            if (ui.currentScreen == UIComponent::MENU) state = MENU;
            if (ui.currentScreen == UIComponent::SETTINGS) state = SETTINGS;

            if (state == EXPLORE && IsKeyPressed(KEY_B)) { state = BUILD; ui.currentScreen = UIComponent::BUILD; }
            else if (state == BUILD && IsKeyPressed(KEY_B)) { state = EXPLORE; ui.currentScreen = UIComponent::EXPLORE; }

            if (ui.currentScreen == UIComponent::MENU || ui.currentScreen == UIComponent::SETTINGS) {
                cameraAngleX += 0.002f;
                camera.position = { cosf(cameraAngleX)*cosf(0.5f)*3000.0f, sinf(0.5f)*3000.0f, sinf(cameraAngleX)*cosf(0.5f)*3000.0f };
            } else if (state == EXPLORE) {
                float alt = cameraRadius - PLANET_RADIUS;
                float speedScale = (alt < 100) ? 0.00005f : (alt < 300) ? 0.0002f : (alt < 600) ? 0.001f : 0.005f;
                if (IsMouseButtonDown(1)) {
                    Vector2 d = GetMouseDelta();
                    cameraAngleX += d.x * speedScale;
                    cameraAngleY += d.y * speedScale;
                    cameraAngleY = Clamp(cameraAngleY, -1.5f, 1.5f);
                }
                float scrollSpeed = (alt < 50) ? 0.4f : (alt < 150) ? 1.5f : (alt < 500) ? 8.0f : 60.0f;
                cameraRadius -= GetMouseWheelMove() * scrollSpeed;
                cameraRadius = Clamp(cameraRadius, (float)PLANET_RADIUS + 3.0f, 6000.0f);
                camera.position = { cosf(cameraAngleX)*cosf(cameraAngleY)*cameraRadius, sinf(cameraAngleY)*cameraRadius, sinf(cameraAngleX)*cosf(cameraAngleY)*cameraRadius };
                camera.target = { 0, 0, 0 };
            } else {
                if (IsMouseButtonDown(1)) {
                    Vector2 d = GetMouseDelta();
                    cameraAngleX += d.x * 0.005f; cameraAngleY += d.y * 0.005f;
                    cameraAngleY = Clamp(cameraAngleY, -1.5f, 1.5f);
                }
                cameraRadius -= GetMouseWheelMove() * 2.0f;
                cameraRadius = Clamp(cameraRadius, 5.0f, 100.0f);
                camera.position = { cosf(cameraAngleX)*cosf(cameraAngleY)*cameraRadius, sinf(cameraAngleY)*cameraRadius, sinf(cameraAngleX)*cosf(cameraAngleY)*cameraRadius };

                Ray ray = GetMouseRay(GetMousePosition(), camera);
                Spaceship::HitInfo hit = ship.GetHitInfo(ray);

                Vector3 targetP = hit.hit ? Vector3Add(hit.pos, hit.normal) : Vector3{0,0,0};
                if (!hit.hit) {
                    float dist = -ray.position.y / ray.direction.y;
                    targetP = Vector3Add(ray.position, Vector3Scale(ray.direction, dist));
                }
                Vector3 finalP = { (float)round(targetP.x), (float)round(targetP.y), (float)round(targetP.z) };

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && GetMousePosition().x > 260)
                    ship.AddPart((int)finalP.x, (int)finalP.y, (int)finalP.z, selectedPart);
                if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
                    if (hit.hit) ship.RemovePart((int)hit.pos.x, (int)hit.pos.y, (int)hit.pos.z);
                ship.UpdateMesh();
            }

            if (cameraRadius < SURFACE_ACTIVATE_RADIUS + 400.0f && state == EXPLORE) {
                Vector3 focus = Vector3Scale(Vector3Normalize(camera.position), (float)PLANET_RADIUS);
                int fx = (int)(focus.x / CHUNK_SIZE), fy = (int)(focus.y / CHUNK_SIZE), fz = (int)(focus.z / CHUNK_SIZE);
                int newJobs = 0;
                for (int x = fx - 3; x <= fx + 3 && newJobs < 2; x++) {
                    for (int y = fy - 3; y <= fy + 3 && newJobs < 2; y++) {
                        for (int z = fz - 3; z <= fz + 3 && newJobs < 2; z++) {
                            ChunkPos cp = { x, y, z };
                            if (chunkCache.find(cp) == chunkCache.end()) {
                                AsyncChunk& ac = chunkCache[cp];
                                ac.processing = true;
                                ac.worker = std::async(std::launch::async, [&pg, x, y, z, &ac, PLANET_RADIUS, CHUNK_SIZE]() {
                                    pg.GenerateChunkData(x*CHUNK_SIZE, y*CHUNK_SIZE, z*CHUNK_SIZE, CHUNK_SIZE, PLANET_RADIUS, ac.data.terrain, ac.data.clouds);
                                });
                                newJobs++;
                            }
                        }
                    }
                }
            }

            int uploads = 0;
            for (auto& pair : chunkCache) {
                AsyncChunk& ac = pair.second;
                if (ac.processing && ac.worker.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    ac.worker.get();
                    if (uploads < 1) {
                        if (!ac.data.terrain.empty()) {
                            ac.data.terrainMesh = pg.CompileToMesh(ac.data.terrain, 1.0f);
                            ac.data.meshReady = true;
                        }
                        if (!ac.data.clouds.empty())
                            ac.data.cloudMesh = pg.CompileToMesh(ac.data.clouds, 1.0f);
                        uploads++;
                    }
                    ac.data.loaded = true; ac.processing = false;
                }
            }

            BeginDrawing();
            ClearBackground(BLACK);
            BeginMode3D(camera);
                for (const auto& s : stars) DrawPoint3D(s.pos, s.col);
                sun.Draw(camera); sun.ApplyToShader(shader, camera.target);
                DrawModel(coreModel, {0,0,0}, 0.98f, Color{40, 110, 190, 255});
                float cloudRot = (float)GetTime() * 0.01f;
                if (cameraRadius > INT_PROXY_ACTIVATE) DrawMesh(spaceTerrainMesh, mat, MatrixScale(0.985f, 0.985f, 0.985f));
                else DrawMesh(intTerrainMesh, mat, MatrixScale(0.990f, 0.990f, 0.990f));

                if (state == EXPLORE) {
                    if (cameraRadius < SURFACE_ACTIVATE_RADIUS) {
                        for (auto& pair : chunkCache) if (pair.second.data.loaded && pair.second.data.meshReady) {
                            DrawMesh(pair.second.data.terrainMesh, mat, MatrixIdentity());
                            DrawMesh(pair.second.data.cloudMesh, mat, MatrixRotateY(cloudRot));
                        }
                    }
                } else if (state == BUILD) {
                    DrawGrid(20, 1.0f);
                    if (ship.shipMesh.vertexCount > 0) DrawModel(ship.shipModel, {0,0,0}, 1.0f, WHITE);
                    Ray ray = GetMouseRay(GetMousePosition(), camera);
                    float dist = -ray.position.y / ray.direction.y;
                    Vector3 p = Vector3Add(ray.position, Vector3Scale(ray.direction, dist));
                    DrawCubeWires({ (float)round(p.x), (float)round(p.y), (float)round(p.z) }, 1.1f, 1.1f, 1.1f, YELLOW);
                }
            EndMode3D();

            ui.Draw(dt, sw, sh, cameraRadius - PLANET_RADIUS);

            if (state == BUILD) {
                auto drawP = [&](int y, const char* name, PartType type, Color c) {
                    Rectangle r = { sw - 220.0f, (float)y, 200, 35 };
                    bool over = CheckCollisionPointRec(GetMousePosition(), r);
                    DrawRectangleRounded(r, 0.2f, 6, Fade((selectedPart == type) ? c : (over ? GRAY : DARKGRAY), 0.5f));
                    DrawText(name, (int)sw - 200, y + 10, 12, WHITE);
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && over) selectedPart = type;
                };
                drawP(100, "STRUCTURAL HULL", PartType::HULL, GRAY);
                drawP(145, "PROPULSION SYSTEM", PartType::THRUSTER, RED);
                drawP(190, "FLIGHT STABILITY", PartType::GYRO, SKYBLUE);
                drawP(235, "FUEL RESERVOIR", PartType::FUEL, ORANGE);
            }

            DrawFPS(sw - 80, 10);
            EndDrawing();
        }
    } catch (const std::exception& e) {
        lastError = e.what(); crashed = true;
    } catch (...) {
        lastError = "Critical Hardware Exception"; crashed = true;
    }

    if (crashed) {
        while (!WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(DARKBLUE);
            DrawText("SYSTEM CRASH DETECTED", 10, 10, 30, RED);
            DrawText(TextFormat("ERROR: %s", lastError.c_str()), 10, 100, 16, YELLOW);
            EndDrawing();
        }
    }
    UnloadShader(shader); CloseWindow(); return 0;
}
