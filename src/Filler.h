#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "DungeonGenerator.h"

constexpr uint32_t TileBit(Tile t) { return 1u << static_cast<uint8_t>(t); }

struct FillerColor { uint8_t r, g, b; };

// Describes one kind of filler (item, object, entity) and where it may spawn.
// The filler pass reads these fields and nothing else: to add a new filler, add a spec to
// GetFillerSpecs() in FillerSpecs.cpp. Leave a field at its default to ignore that criterion.
struct FillerSpec {
    std::string name;
    FillerColor color = { 255, 255, 255 };   // placeholder look until real models exist

    // Tile criteria (all must hold)
    uint32_t allowedTiles = TileBit(Tile::Floor);   // OR together TileBit() values
    uint8_t requiredWalls = 0;     // WallSide bits that must all be walls, e.g. WallNorth | WallWest for a corner
    uint8_t forbiddenWalls = 0;    // WallSide bits that must not be walls
    int minWallDistance = 0;       // 1 = touching a wall, 2+ = away from walls
    int maxWallDistance = 255;
    int minStartDistance = 0;      // walking steps from the player spawn
    int maxStartDistance = -1;     // -1 = no upper limit

    // How many to place (a count is picked at random between min and max, then placed as far as possible)
    int minCount = 0;
    int maxCount = 1;
    int maxPerRoom = 0;            // 0 = unlimited; only applies to tiles that belong to a room
    // Per-crypt mode: if maxPerCrypt > 0, every crypt gets a random number of this filler between
    // minPerCrypt and maxPerCrypt, and minCount/maxCount are ignored.
    int minPerCrypt = 0;
    int maxPerCrypt = 0;
    int minSpacing = 0;            // min Chebyshev distance between two of this same filler
};

struct FillerInstance {
    int spec;    // index into the spec list that was passed to RunFillerPass
    int x, y;    // tile coordinates
};

// All defined fillers, in priority order (earlier specs claim tiles first).
const std::vector<FillerSpec>& GetFillerSpecs();

// Runs after dungeon generation. Never modifies the dungeon. Pass a different seed each run for
// varied filler on the same floor, or reuse one to reproduce a placement.
std::vector<FillerInstance> RunFillerPass(const Dungeon& dungeon, const std::vector<FillerSpec>& specs, uint64_t seed);
