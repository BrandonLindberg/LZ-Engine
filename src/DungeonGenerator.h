#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class Tile : uint8_t { Wall = 0, Floor = 1, Corridor = 2, Crypt = 3, Walkway = 4 };   // Walkway: kept clear of filler (e.g. a crypt's entrance path)

// Bits for TileData::wallMask. North is -y, East is +x, South is +y, West is -x.
enum WallSide : uint8_t { WallNorth = 1, WallEast = 2, WallSouth = 4, WallWest = 8 };

// Everything known about a tile once generation finishes (filled in by a final analysis pass).
struct TileData {
    Tile type = Tile::Wall;
    int16_t roomId = -1;         // index into Dungeon::rooms for room floor tiles, -1 otherwise
    int16_t cryptId = -1;        // index into Dungeon::crypts for crypt tiles (including its entrance), -1 otherwise
    uint8_t wallMask = 0;        // WallSide bits for each of the 4 neighbors that is a wall (no diagonals)
    uint8_t wallDistance = 0;    // steps to the nearest wall; 0 for walls, 1 for tiles touching one
    int16_t startDistance = -1;  // walking steps from the first room's center; -1 if unreachable or wall

    bool TouchesWall() const { return wallMask != 0; }
};

struct Room {
    int x, y, w, h;
    int CenterX() const { return x + w / 2; }
    int CenterY() const { return y + h / 2; }
};

struct DungeonParams {
    int width = 64;
    int height = 64;
    int maxRooms = 20;
    int minRoomSize = 5;
    int maxRoomSize = 10;
    int roomSpacing = 4;      // min gap between rooms; leaves space for a 2-wide hallway plus a wall on each side
    int placementAttempts = 200;
    int extraCorridors = 2;   // extra loops beyond the spanning chain
    int maxCrypts = 4;        // upper limit on 3x3 crypts tucked into leftover wall space
};

struct Dungeon {
    uint64_t seed = 0;
    int width = 0;
    int height = 0;
    std::vector<TileData> tiles;  // row-major, index = y * width + x
    std::vector<Room> rooms;
    std::vector<Room> crypts;     // 3x3 crypt interiors (entrance and first interior tile are Tile::Walkway)

    bool InBounds(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
    const TileData& Data(int x, int y) const { return tiles[y * width + x]; }
    Tile At(int x, int y) const { return Data(x, y).type; }
    bool IsWalkable(int x, int y) const { return InBounds(x, y) && At(x, y) != Tile::Wall; }
};

// Deterministic: the same seed and params always yield the same dungeon on any platform.
Dungeon GenerateDungeon(uint64_t seed, const DungeonParams& params = DungeonParams());
