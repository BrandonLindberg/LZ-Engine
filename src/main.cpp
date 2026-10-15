#include "raylib.h"

int main()
{
    // Create a window.
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "LZ Engine");

    // Run the game at a target frame rate.
    SetTargetFPS(60);

    // A placeholder player position.
    Vector2 playerPosition = { 640.0f, 360.0f };
    const float playerSpeed = 500.0f;

    // Main game loop.
    while (!WindowShouldClose())
    {
        // Time elapsed since the previous frame, in seconds.
        float deltaTime = GetFrameTime();

        // Read player input.
        if (IsKeyDown(KEY_W)) playerPosition.y -= playerSpeed * deltaTime;
        if (IsKeyDown(KEY_S)) playerPosition.y += playerSpeed * deltaTime;
        if (IsKeyDown(KEY_A)) playerPosition.x -= playerSpeed * deltaTime;
        if (IsKeyDown(KEY_D)) playerPosition.x += playerSpeed * deltaTime;

        // Draw the frame.
        BeginDrawing();

        ClearBackground(DARKGRAY);

        DrawText("Procedural Engine - First Test",
                 20, 20, 24, RAYWHITE);

        DrawText("Move with WASD",
                 20, 55, 18, LIGHTGRAY);

        DrawRectangleV(playerPosition, { 32.0f, 32.0f }, SKYBLUE);

        EndDrawing();
    }

    // Release window and graphics resources.
    CloseWindow();

    return 0;
}
