#include "CharacterController.h"

#include "raymath.h"

#include "GameConstants.h"

// Movement constants
#define GRAVITY         32.0f
#define MAX_SPEED        6.0f
#define MAX_ACCEL      150.0f
// Grounded drag
#define FRICTION         0.86f
// Increasing air drag, increases strafing speed
#define AIR_DRAG         0.98f
// Responsiveness for turning movement direction to looked direction
#define CONTROL         15.0f
#define STAND_HEIGHT     1.0f
#define BOTTOM_HEIGHT    0.5f

#define NORMALIZE_INPUT  0

CharacterController::CharacterController()
    : sensitivity{ 0.0015f, 0.0015f }
    , player{ 0 }
    , lookRotation{ 0 }
    , headTimer(0.0f)
    , walkLerp(0.0f)
    , headLerp(STAND_HEIGHT)
    , lean{ 0 }
{
}

void CharacterController::SetPosition(float x, float z)
{
    player.position.x = x;
    player.position.z = z;
}

void CharacterController::InitCamera(Camera *camera)
{
    camera->position = (Vector3){
        player.position.x,
        player.position.y + (BOTTOM_HEIGHT + headLerp),
        player.position.z,
    };

    UpdateCameraFPS(camera); // Update camera parameters
}

void CharacterController::Update(const Dungeon &dungeon, Camera *camera)
{
    Vector2 mouseDelta = GetMouseDelta();
    lookRotation.x -= mouseDelta.x*sensitivity.x;
    lookRotation.y += mouseDelta.y*sensitivity.y;

    char sideway = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
    char forward = (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
    UpdateBody(dungeon, &player, lookRotation.x, sideway, forward);

    float delta = GetFrameTime();
    headLerp = Lerp(headLerp, STAND_HEIGHT, 20.0f*delta);
    camera->position = (Vector3){
        player.position.x,
        player.position.y + (BOTTOM_HEIGHT + headLerp),
        player.position.z,
    };

    if (player.isGrounded && ((forward != 0) || (sideway != 0)))
    {
        headTimer += delta*3.0f;
        walkLerp = Lerp(walkLerp, 0.5f, 10.0f*delta);
    }
    else
    {
        walkLerp = Lerp(walkLerp, 0.0f, 10.0f*delta);
    }

    lean.x = Lerp(lean.x, sideway*0.01f, 10.0f*delta);

    UpdateCameraFPS(camera);
}

// Push a circular body (XZ plane) out of any wall tiles it overlaps
void CharacterController::ResolveWallCollisions(const Dungeon &dungeon, Body *body)
{
    const float half = TILE_SIZE/2.0f;
    int cx = (int)floorf(body->position.x/TILE_SIZE + 0.5f);
    int cz = (int)floorf(body->position.z/TILE_SIZE + 0.5f);

    for (int ty = cz - 1; ty <= cz + 1; ty++)
    {
        for (int tx = cx - 1; tx <= cx + 1; tx++)
        {
            if (dungeon.IsWalkable(tx, ty)) continue;

            // Closest point on the tile's square to the body center
            float nearestX = Clamp(body->position.x, tx*TILE_SIZE - half, tx*TILE_SIZE + half);
            float nearestZ = Clamp(body->position.z, ty*TILE_SIZE - half, ty*TILE_SIZE + half);
            float dx = body->position.x - nearestX;
            float dz = body->position.z - nearestZ;
            float distSq = dx*dx + dz*dz;
            if (distSq >= RADIUS*RADIUS || distSq < 0.000001f) continue;

            float dist = sqrtf(distSq);
            float nx = dx/dist;
            float nz = dz/dist;
            body->position.x += nx*(RADIUS - dist);
            body->position.z += nz*(RADIUS - dist);

            // Cancel velocity into the wall so the body slides along it
            float into = body->velocity.x*nx + body->velocity.z*nz;
            if (into < 0.0f)
            {
                body->velocity.x -= into*nx;
                body->velocity.z -= into*nz;
            }
        }
    }
}

// Update body considering current world state
void CharacterController::UpdateBody(const Dungeon &dungeon, Body *body, float rot, char side, char forward)
{
    Vector2 input = (Vector2){ (float)side, (float)-forward };

#if defined(NORMALIZE_INPUT)
    // Slow down diagonal movement
    if ((side != 0) && (forward != 0)) input = Vector2Normalize(input);
#endif

    float delta = GetFrameTime();

    if (!body->isGrounded) body->velocity.y -= GRAVITY*delta;

    Vector3 front = (Vector3){ sinf(rot), 0.f, cosf(rot) };
    Vector3 right = (Vector3){ cosf(-rot), 0.f, sinf(-rot) };

    Vector3 desiredDir = (Vector3){ input.x*right.x + input.y*front.x, 0.0f, input.x*right.z + input.y*front.z, };
    body->dir = Vector3Lerp(body->dir, desiredDir, CONTROL*delta);

    float decel = (body->isGrounded ? FRICTION : AIR_DRAG);
    Vector3 hvel = (Vector3){ body->velocity.x*decel, 0.0f, body->velocity.z*decel };

    float hvelLength = Vector3Length(hvel); // Magnitude
    if (hvelLength < (MAX_SPEED*0.01f)) hvel = (Vector3){ 0 };

    // This is what creates strafing
    float speed = Vector3DotProduct(hvel, body->dir);

    // Whenever the amount of acceleration to add is clamped by the maximum acceleration constant,
    // a Player can make the speed faster by bringing the direction closer to horizontal velocity angle
    // More info here: https://youtu.be/v3zT3Z5apaM?t=165
    float maxSpeed = MAX_SPEED;
    float accel = Clamp(maxSpeed - speed, 0.f, MAX_ACCEL*delta);
    hvel.x += body->dir.x*accel;
    hvel.z += body->dir.z*accel;

    body->velocity.x = hvel.x;
    body->velocity.z = hvel.z;

    body->position.x += body->velocity.x*delta;
    body->position.y += body->velocity.y*delta;
    body->position.z += body->velocity.z*delta;

    ResolveWallCollisions(dungeon, body);

    // Fancy collision system against the floor
    if (body->position.y <= 0.0f)
    {
        body->position.y = 0.0f;
        body->velocity.y = 0.0f;
        body->isGrounded = true; // Enable jumping
    }
}

// Update camera for FPS behaviour
void CharacterController::UpdateCameraFPS(Camera *camera)
{
    const Vector3 up = (Vector3){ 0.0f, 1.0f, 0.0f };
    const Vector3 targetOffset = (Vector3){ 0.0f, 0.0f, -1.0f };

    // Left and right
    Vector3 yaw = Vector3RotateByAxisAngle(targetOffset, up, lookRotation.x);

    // Clamp view up
    float maxAngleUp = Vector3Angle(up, yaw);
    maxAngleUp -= 0.001f; // Avoid numerical errors
    if ( -(lookRotation.y) > maxAngleUp) { lookRotation.y = -maxAngleUp; }

    // Clamp view down
    float maxAngleDown = Vector3Angle(Vector3Negate(up), yaw);
    maxAngleDown *= -1.0f; // Downwards angle is negative
    maxAngleDown += 0.001f; // Avoid numerical errors
    if ( -(lookRotation.y) < maxAngleDown) { lookRotation.y = -maxAngleDown; }

    // Up and down
    Vector3 right = Vector3Normalize(Vector3CrossProduct(yaw, up));

    // Rotate view vector around right axis
    float pitchAngle = -lookRotation.y - lean.y;
    pitchAngle = Clamp(pitchAngle, -PI/2 + 0.0001f, PI/2 - 0.0001f); // Clamp angle so it doesn't go past straight up or straight down
    Vector3 pitch = Vector3RotateByAxisAngle(yaw, right, pitchAngle);

    // Head animation
    // Rotate up direction around forward axis
    float headSin = sinf(headTimer*PI);
    float headCos = cosf(headTimer*PI);
    const float stepRotation = 0.00f;   // sideways rotation when walking
    camera->up = Vector3RotateByAxisAngle(up, pitch, headSin*stepRotation + lean.x);

    // Camera BOB
    const float bobSide = 0.1f;
    const float bobUp = 0.1f;
    Vector3 bobbing = Vector3Scale(right, headSin*bobSide);
    bobbing.y = fabsf(headCos*bobUp);

    camera->position = Vector3Add(camera->position, Vector3Scale(bobbing, walkLerp));
    camera->target = Vector3Add(camera->position, pitch);
}
