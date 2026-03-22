#include "raylib.h"
#include <math.h>

int main(void)
{
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Voxonaut - 3D Engine");

    // Define the camera to look into our 3d world
    Camera3D camera = { 0 };
    camera.position = Vector3{ 10.0f, 10.0f, 10.0f }; // Camera position
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera mode type

    // Define cube position
    Vector3 cubePosition = Vector3{ 0.0f, 0.0f, 0.0f };
    float time = 0.0f;

    SetTargetFPS(60);                   // Set our game to run at 60 frames-per-second

    // Main game loop
    while (!WindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        // Move the cube on the X and Z axis
        time += GetFrameTime();
        cubePosition.x = sinf(time * 2.0f) * 3.0f;
        cubePosition.z = cosf(time * 2.0f) * 3.0f;
        // Keep it slightly above the grid
        cubePosition.y = 1.0f;

        // Draw
        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                // Draw X, Y, Z axes
                // X - Red
                DrawLine3D(Vector3{0, 0, 0}, Vector3{5, 0, 0}, RED);
                // Y - Green
                DrawLine3D(Vector3{0, 0, 0}, Vector3{0, 5, 0}, GREEN);
                // Z - Blue
                DrawLine3D(Vector3{0, 0, 0}, Vector3{0, 0, 5}, BLUE);

                // Draw a grid for reference (on the XZ plane)
                DrawGrid(10, 1.0f);

                // Draw the moving cube
                DrawCube(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);
                // Draw a wireframe over the cube so its edges are visible
                DrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, DARKGRAY);

            EndMode3D();

            DrawText("Voxonaut - Game Window Initialized", 10, 10, 20, DARKGRAY);
            DrawText("X Axis: Red | Y Axis: Green | Z Axis: Blue", 10, 40, 20, DARKGRAY);

        EndDrawing();
    }

    // De-Initialization
    CloseWindow();        // Close window and OpenGL context

    return 0;
}
