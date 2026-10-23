#pragma once

#include "raylib.h"

#include "DungeonGenerator.h"

// Body structure
struct Body {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    bool isGrounded;
};

// First-person character controller: movement, wall collision, look and head animation
class CharacterController
{
public:
    static constexpr float RADIUS = 0.6f;

    CharacterController();

    void SetPosition(float x, float z);
    const Vector3& GetPosition() const { return player.position; }

    // Place the camera at the player's head and orient it
    void InitCamera(Camera *camera);

    // Read input, move the player against the dungeon and update the camera
    void Update(const Dungeon &dungeon, Camera *camera);

private:
    void ResolveWallCollisions(const Dungeon &dungeon, Body *body);
    void UpdateBody(const Dungeon &dungeon, Body *body, float rot, char side, char forward);
    void UpdateCameraFPS(Camera *camera);

    Vector2 sensitivity;

    Body player;
    Vector2 lookRotation;
    float headTimer;
    float walkLerp;
    float headLerp;
    Vector2 lean;
};
