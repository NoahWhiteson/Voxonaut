#include "raylib.h"
#include "raymath.h"
#include "PhysicsComponent.h"
#include <string>

int main(void)
{
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 600;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Voxonaut - 3D Engine with Physics");

    // Define the camera to look into our 3d world
    Camera3D camera = { 0 };
    camera.position = Vector3{ 10.0f, 10.0f, 10.0f }; // Camera position
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera mode type

    // Define cube position & its physics component
    Vector3 cubePosition = Vector3{ 0.0f, 5.0f, 0.0f };
    PhysicsComponent cubePhysics;
    
    // Some basic gravity configurations
    float gravityEarth = -9.81f;
    float gravityMoon = -1.62f;
    float gravitySpace = 0.0f;
    cubePhysics.gravityConstant = gravityEarth;

    SetTargetFPS(60);

    // Interaction variables
    bool isDragging = false;
    float dragDistance = 0.0f;
    Vector3 dragOffset = Vector3{0, 0, 0};
    
    // Main game loop
    while (!WindowShouldClose())
    {
        // -------------
        // UPDATE
        // -------------
        float dt = GetFrameTime();

        // 1. Switch gravity based on input keys
        if (IsKeyPressed(KEY_ONE)) cubePhysics.gravityConstant = gravityEarth;
        if (IsKeyPressed(KEY_TWO)) cubePhysics.gravityConstant = gravityMoon;
        if (IsKeyPressed(KEY_THREE)) cubePhysics.gravityConstant = gravitySpace;

        // 2. Drag and Drop Raycasting
        Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
        
        // Define bounding box for the cube (cube size is 2.0 so offsets are +-1.0)
        BoundingBox cubeBox = {
            Vector3{cubePosition.x - 1.0f, cubePosition.y - 1.0f, cubePosition.z - 1.0f},
            Vector3{cubePosition.x + 1.0f, cubePosition.y + 1.0f, cubePosition.z + 1.0f}
        };

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            RayCollision collision = GetRayCollisionBox(mouseRay, cubeBox);
            if (collision.hit) {
                isDragging = true;
                dragDistance = collision.distance;
                // Compute offset so it drags precisely from where grabbed instead of snapping to center
                Vector3 hitPoint = Vector3Add(mouseRay.position, Vector3Scale(mouseRay.direction, dragDistance));
                dragOffset = Vector3Subtract(cubePosition, hitPoint);
                cubePhysics.velocity = Vector3{0.0f, 0.0f, 0.0f}; // stop mid-air
            }
        }

        if (isDragging) {
            if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
                isDragging = false;
            } else {
                // Determine new position by following mouse ray
                Vector3 newHitPoint = Vector3Add(mouseRay.position, Vector3Scale(mouseRay.direction, dragDistance));
                Vector3 newPos = Vector3Add(newHitPoint, dragOffset);
                
                // Keep track of how much the cube moved to simulate throwing velocity
                Vector3 velocity = Vector3Scale(Vector3Subtract(newPos, cubePosition), 1.0f / dt);
                
                // Simple cap on velocity so players don't launch it out of bounds instantly
                velocity.x = Clamp(velocity.x, -50.0f, 50.0f);
                velocity.y = Clamp(velocity.y, -50.0f, 50.0f);
                velocity.z = Clamp(velocity.z, -50.0f, 50.0f);
                
                // Overwrite the velocity in the physics component
                cubePhysics.velocity = velocity;
                // Update the real position
                cubePosition = newPos;
            }
        } else {
            // Apply physics only when not being interacted with
            cubePhysics.Update(dt, cubePosition);
        }

        // -------------
        // DRAW
        // -------------
        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginMode3D(camera);

                // Draw X, Y, Z axes
                DrawLine3D(Vector3{0, 0, 0}, Vector3{5, 0, 0}, RED);
                DrawLine3D(Vector3{0, 0, 0}, Vector3{0, 5, 0}, GREEN);
                DrawLine3D(Vector3{0, 0, 0}, Vector3{0, 0, 5}, BLUE);

                // Draw a grid for reference
                DrawGrid(10, 1.0f);

                // Draw the cube
                DrawCube(cubePosition, 2.0f, 2.0f, 2.0f, isDragging ? ORANGE : MAROON);
                DrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, DARKGRAY);

            EndMode3D();

            // Status Text
            DrawText("VOXONAUT - Test Build", 10, 10, 20, DARKGRAY);
            
            char fpsText[32];
            snprintf(fpsText, sizeof(fpsText), "FPS: %d", GetFPS());
            DrawText(fpsText, 10, 40, 20, DARKGREEN);

            char gravText[64];
            snprintf(gravText, sizeof(gravText), "G_CONSTANT: %.2f", cubePhysics.gravityConstant);
            DrawText(gravText, 10, 70, 20, BLUE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
