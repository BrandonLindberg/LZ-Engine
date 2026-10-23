#include "raylib.h"

#include "raymath.h"

#include <cstdlib>
#include <ctime>
#include <random>
#include <vector>

#include "CharacterController.h"
#include "DungeonGenerator.h"
#include "Filler.h"
#include "Projectile.h"
#include "GameConstants.h"

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static CharacterController player;
static ProjectileSystem projectiles;

static Dungeon dungeon;
static std::vector<FillerInstance> filler;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void DrawLevel(bool drawCeiling);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    // Usage: Core [seed] -- random seed when omitted
    uint64_t seed = (argc > 1)? std::strtoull(argv[1], nullptr, 10) : (uint64_t)std::time(nullptr);
    dungeon = GenerateDungeon(seed);
    TraceLog(LOG_INFO, "Dungeon seed: %llu", (unsigned long long)seed);

    // Filler deliberately uses its own non-deterministic seed so the same floor gets different contents
    filler = RunFillerPass(dungeon, GetFillerSpecs(), std::random_device{}());

    // Spawn in the center of the first room
    if (!dungeon.rooms.empty())
    {
        player.SetPosition(dungeon.rooms[0].CenterX()*TILE_SIZE, dungeon.rooms[0].CenterY()*TILE_SIZE);
    }

    SetConfigFlags(FLAG_MSAA_4X_HINT);

    const int screenWidth = 1920;
    const int screenHeight = 1080;

    InitWindow(screenWidth, screenHeight, "dungeon");

    // Initialize camera variables
    // NOTE: UpdateCameraFPS() takes care of the rest
    Camera camera = { 0 };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    player.InitCamera(&camera);

    // Debug overview camera (toggle with F1, mouse wheel zooms): orthographic, looking straight down
    Camera debugCamera = { 0 };
    debugCamera.projection = CAMERA_ORTHOGRAPHIC;
    debugCamera.up = (Vector3){ 0.0f, 0.0f, -1.0f };
    debugCamera.position = (Vector3){ dungeon.width*TILE_SIZE/2.0f, 200.0f, dungeon.height*TILE_SIZE/2.0f };
    debugCamera.target = (Vector3){ debugCamera.position.x, 0.0f, debugCamera.position.z };
    debugCamera.fovy = dungeon.height*TILE_SIZE;    // visible world height
    bool debugView = false;

    DisableCursor();        // Limit cursor to relative movement inside the window

    SetTargetFPS(100);       // Set our game to run at target FPS
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        player.Update(dungeon, &camera);

        // Cast from the camera along its view direction (left mouse button)
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
            Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
            Vector3 offset = {-0.3f, -0.2f, 0.5f}; // left/right, up/down, forwards/backwards
            Vector3 worldOffset = Vector3Add(Vector3Scale(right, offset.x), Vector3Add(Vector3Scale(camera.up, offset.y), Vector3Scale(forward, offset.z)));
            Vector3 projectilePosition = Vector3Add(camera.position, worldOffset);
            projectiles.Spawn(projectilePosition, forward);
        }
        projectiles.Update(dungeon, GetFrameTime());

        if (IsKeyPressed(KEY_F1)) debugView = !debugView;
        if (debugView) debugCamera.fovy = Clamp(debugCamera.fovy*(1.0f - GetMouseWheelMove()*0.1f), 20.0f, 600.0f);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(BLACK);

            BeginMode3D(debugView? debugCamera : camera);
                DrawLevel(!debugView);
                projectiles.Draw();
                if (debugView) DrawCylinder(player.GetPosition(), 0.0f, CharacterController::RADIUS*2.0f, 6.0f, 8, RED);   // player marker
            EndMode3D();

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Draw game level
static void DrawLevel(bool drawCeiling)
{
    const Color wallColor = (DARKGRAY);
    Color floorColor;

    for (int y = 0; y < dungeon.height; y++)
    {
        for (int x = 0; x < dungeon.width; x++)
        {
            Vector3 center = (Vector3){ x*TILE_SIZE, 0.0f, y*TILE_SIZE };

            if (dungeon.IsWalkable(x, y))
            {
                switch (dungeon.At(x, y))
                {
                    case Tile::Floor: floorColor = (DARKGRAY); break;
                    case Tile::Crypt: floorColor = (PURPLE); break;
                    case Tile::Walkway: floorColor = (VIOLET); break;
                    default: floorColor = (GRAY); break;
                }

                DrawPlane(center, (Vector2){ TILE_SIZE, TILE_SIZE }, floorColor);

                // A thin slab rather than a plane, since planes only face up and are culled from below
                if (drawCeiling)
                {
                    const float slab = 0.1f;
                    DrawCubeV((Vector3){ center.x, WALL_HEIGHT + slab/2.0f, center.z }, (Vector3){ TILE_SIZE, slab, TILE_SIZE }, DARKGRAY);
                }
                continue;
            }

            // Only draw walls that border a walkable tile; the rest are never seen
            bool visible = false;
            for (int dy = -1; dy <= 1 && !visible; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    if (dungeon.IsWalkable(x + dx, y + dy)) { visible = true; break; }
            if (!visible) continue;

            Vector3 wallPos = (Vector3){ center.x, WALL_HEIGHT/2.0f, center.z };
            Vector3 wallSize = (Vector3){ TILE_SIZE, WALL_HEIGHT, TILE_SIZE };
            DrawCubeV(wallPos, wallSize, wallColor);
            DrawCubeWiresV(wallPos, wallSize, BLACK);
        }
    }

    // Placeholder markers for filler until real models exist
    const std::vector<FillerSpec>& specs = GetFillerSpecs();
    for (const FillerInstance& f : filler)
    {
        const FillerColor& c = specs[f.spec].color;
        Vector3 pos = (Vector3){ f.x*TILE_SIZE, 0.5f, f.y*TILE_SIZE };
        DrawCubeV(pos, (Vector3){ 1.0f, 1.0f, 1.0f }, (Color){ c.r, c.g, c.b, 255 });
        DrawCubeWiresV(pos, (Vector3){ 1.0f, 1.0f, 1.0f }, BLACK);
    }
}
