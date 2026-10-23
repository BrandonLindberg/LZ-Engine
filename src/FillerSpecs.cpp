#include "Filler.h"

// Add new filler here. Order matters: earlier entries claim tiles first.
const std::vector<FillerSpec>& GetFillerSpecs() {
    static const std::vector<FillerSpec> specs = [] {
        std::vector<FillerSpec> v;

        {   // Treasure inside crypts: 1-2 per crypt
            FillerSpec s;
            s.name = "CryptChest";
            s.color = { 255, 215, 0 };
            s.allowedTiles = TileBit(Tile::Crypt);
            s.minPerCrypt = 1;   // every crypt gets at least one
            s.maxPerCrypt = 2;
            s.minSpacing = 2;    // keep two chests in one crypt from touching
            v.push_back(s);
        }
        {   // Chest tucked against a wall, away from the spawn room
            FillerSpec s;
            s.name = "Chest";
            s.color = { 160, 100, 40 };
            s.allowedTiles = TileBit(Tile::Floor);
            s.minWallDistance = 1;
            s.maxWallDistance = 1;
            s.minStartDistance = 10;
            s.minCount = 3;
            s.maxCount = 10;
            s.maxPerRoom = 2;
            s.minSpacing = 6;
            v.push_back(s);
        }
        {   // Barrel in a room corner
            FillerSpec s;
            s.name = "Barrel";
            s.color = { 120, 80, 50 };
            s.allowedTiles = TileBit(Tile::Floor);
            s.requiredWalls = WallNorth | WallWest;
            s.minCount = 4;
            s.maxCount = 8;
            s.maxPerRoom = 2;
            v.push_back(s);
        }
        {   // Trap on any corridor tile, away from the spawn
            FillerSpec s;
            s.name = "CorridorTrap";
            s.color = { 220, 40, 40 };
            s.allowedTiles = TileBit(Tile::Corridor);
            s.minStartDistance = 10;
            s.minCount = 3;
            s.maxCount = 5;
            s.minSpacing = 5;
            v.push_back(s);
        }
            FillerSpec s;
            s.name = "RoomTrap";
            s.color = { 220, 40, 40 };
            s.allowedTiles = TileBit(Tile::Floor);
            s.minWallDistance = 2;
            s.maxWallDistance = 20;
            s.minStartDistance = 10;
            s.minCount = 3;
            s.maxCount = 5;
            s.minSpacing = 8;
            v.push_back(s);

        return v;
    }();
    return specs;
}
