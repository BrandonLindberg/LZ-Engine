#include "DungeonGenerator.h"
#include "Rng.h"

#include <algorithm>
#include <queue>

namespace {

bool Overlaps(const Room& a, const Room& b, int padding) {
    return a.x - padding < b.x + b.w && a.x + a.w + padding > b.x &&
           a.y - padding < b.y + b.h && a.y + a.h + padding > b.y;
}

void CarveRoom(Dungeon& d, const Room& r, int roomId) {
    for (int y = r.y; y < r.y + r.h; y++) {
        for (int x = r.x; x < r.x + r.w; x++) {
            TileData& t = d.tiles[y * d.width + x];
            t.type = Tile::Floor;
            t.roomId = static_cast<int16_t>(roomId);
        }
    }
}

// Carves 2-wide L-shaped corridors and remembers each corridor tile's direction so new
// corridors can avoid running right beside (and merging with) parallel ones.
class CorridorCarver {
public:
    explicit CorridorCarver(Dungeon& dungeon)
        : d(dungeon), dirs(dungeon.tiles.size(), 0) {}

    // Tries to link two rooms without breaking the spacing rules. The first attempt uses the room
    // centers; later attempts pick random anchor points inside each room. Both elbow orientations
    // are tried for each. Returns false if nothing fits, unless force is set.
    bool Connect(const Room& a, const Room& b, Rng& rng, bool force) {
        const int anchorAttempts = 12;
        for (int i = 0; i < anchorAttempts; i++) {
            int ax = a.CenterX(), ay = a.CenterY();
            int bx = b.CenterX(), by = b.CenterY();
            if (i > 0) {
                // Anchors leave room for the 2-wide band to stay inside the room
                ax = rng.Range(a.x, a.x + a.w - 2); ay = rng.Range(a.y, a.y + a.h - 2);
                bx = rng.Range(b.x, b.x + b.w - 2); by = rng.Range(b.y, b.y + b.h - 2);
            }
            bool horizontalFirst = rng.Range(0, 1) == 0;
            if (TryElbow(ax, ay, bx, by, horizontalFirst, false)) return true;
            if (TryElbow(ax, ay, bx, by, !horizontalFirst, false)) return true;
        }
        if (!force) return false;
        return TryElbow(a.CenterX(), a.CenterY(), b.CenterX(), b.CenterY(), rng.Range(0, 1) == 0, true);
    }

private:
    enum : uint8_t { DirH = 1, DirV = 2 };

    Dungeon& d;
    std::vector<uint8_t> dirs;

    bool TryElbow(int ax, int ay, int bx, int by, bool horizontalFirst, bool force) {
        int hy = horizontalFirst ? ay : by;
        int vx = horizontalFirst ? bx : ax;
        if (!force && (HorizontalBlocked(ax, bx, hy) || VerticalBlocked(ay, by, vx))) return false;
        CarveHorizontal(ax, bx, hy);
        CarveVertical(ay, by, vx);
        return true;
    }

    bool HasDir(int x, int y, uint8_t dir) const {
        return d.InBounds(x, y) && (dirs[y * d.width + x] & dir) != 0;
    }

    // A horizontal corridor occupies rows y and y+1; an existing horizontal corridor on
    // row y-1 or y+2 would be directly adjacent. Sharing the exact same rows is fine.
    // It also must not run along a room's top or bottom wall (row just outside the room).
    bool HorizontalBlocked(int x1, int x2, int y) const {
        int lo = std::min(x1, x2), hi = std::max(x1, x2) + 1;   // +1: the band's second column
        for (int x = lo; x <= hi; x++)
            if (HasDir(x, y - 1, DirH) || HasDir(x, y + 2, DirH)) return true;
        for (const Room& r : d.rooms) {
            bool columnsOverlap = hi >= r.x && lo <= r.x + r.w - 1;
            bool touchesWall = (y <= r.y - 1 && y + 1 >= r.y - 1) || (y <= r.y + r.h && y + 1 >= r.y + r.h);
            if (columnsOverlap && touchesWall) return true;
        }
        return false;
    }

    bool VerticalBlocked(int y1, int y2, int x) const {
        int lo = std::min(y1, y2), hi = std::max(y1, y2) + 1;   // +1: the band's second row
        for (int y = lo; y <= hi; y++)
            if (HasDir(x - 1, y, DirV) || HasDir(x + 2, y, DirV)) return true;
        for (const Room& r : d.rooms) {
            bool rowsOverlap = hi >= r.y && lo <= r.y + r.h - 1;
            bool touchesWall = (x <= r.x - 1 && x + 1 >= r.x - 1) || (x <= r.x + r.w && x + 1 >= r.x + r.w);
            if (rowsOverlap && touchesWall) return true;
        }
        return false;
    }

    // Never carve the outermost ring so the map edge stays solid
    void CarveTile(int x, int y, uint8_t dir) {
        if (x < 1 || y < 1 || x > d.width - 2 || y > d.height - 2) return;
        Tile& t = d.tiles[y * d.width + x].type;
        if (t == Tile::Wall) t = Tile::Corridor;
        if (t == Tile::Corridor) dirs[y * d.width + x] |= dir;
    }

    // Corridors are 2 tiles wide: the extra row/column sits on the +y / +x side
    void CarveHorizontal(int x1, int x2, int y) {
        // Runs one tile past the far end so it fills the 2x2 corner where it meets a vertical leg
        for (int x = std::min(x1, x2); x <= std::max(x1, x2) + 1; x++) {
            CarveTile(x, y, DirH);
            CarveTile(x, y + 1, DirH);
        }
    }

    void CarveVertical(int y1, int y2, int x) {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2) + 1; y++) {
            CarveTile(x, y, DirV);
            CarveTile(x + 1, y, DirV);
        }
    }
};

struct CryptSpot {
    int x, y;   // top-left of the 3x3 interior
    int dir;    // side the entrance is on: 0 north, 1 east, 2 south, 3 west
};

// Entrance tile (centered on the chosen side) and the walkable tile it connects to
void CryptEntrance(const CryptSpot& c, int& ex, int& ey, int& wx, int& wy) {
    static const int ox[4] = { 1, 3, 1, -1 };
    static const int oy[4] = { -1, 1, 3, 1 };
    static const int sx[4] = { 0, 1, 0, -1 };
    static const int sy[4] = { -1, 0, 1, 0 };
    ex = c.x + ox[c.dir];
    ey = c.y + oy[c.dir];
    wx = ex + sx[c.dir];
    wy = ey + sy[c.dir];
}

// The interior plus a 1-tile border (which holds the entrance) must be solid wall, and the
// entrance must open onto a room or corridor tile.
bool CryptFits(const Dungeon& d, const CryptSpot& c) {
    int ex, ey, wx, wy;
    CryptEntrance(c, ex, ey, wx, wy);
    if (!d.InBounds(wx, wy) || !d.InBounds(c.x - 1, c.y - 1) || !d.InBounds(c.x + 3, c.y + 3)) return false;

    for (int y = c.y - 1; y <= c.y + 3; y++)
        for (int x = c.x - 1; x <= c.x + 3; x++)
            if (d.At(x, y) != Tile::Wall) return false;

    Tile outside = d.At(wx, wy);
    return outside == Tile::Floor || outside == Tile::Corridor;
}

// Tucks small crypts into leftover wall space. Runs after rooms and corridors, so it never
// changes them; candidates are shuffled so placement varies with the seed.
void PlaceCrypts(Dungeon& d, int maxCrypts, Rng& rng) {
    std::vector<CryptSpot> spots;
    for (int y = 1; y < d.height - 3; y++)
        for (int x = 1; x < d.width - 3; x++)
            for (int dir = 0; dir < 4; dir++) {
                CryptSpot c{ x, y, dir };
                if (CryptFits(d, c)) spots.push_back(c);
            }

    for (int i = static_cast<int>(spots.size()) - 1; i > 0; i--)
        std::swap(spots[i], spots[rng.Range(0, i)]);

    for (const CryptSpot& c : spots) {
        if (static_cast<int>(d.crypts.size()) >= maxCrypts) break;
        if (!CryptFits(d, c)) continue;   // an earlier crypt may have taken the space

        int cryptId = static_cast<int>(d.crypts.size());
        for (int y = c.y; y < c.y + 3; y++) {
            for (int x = c.x; x < c.x + 3; x++) {
                d.tiles[y * d.width + x].type = Tile::Crypt;
                d.tiles[y * d.width + x].cryptId = static_cast<int16_t>(cryptId);
            }
        }
        int ex, ey, wx, wy;
        CryptEntrance(c, ex, ey, wx, wy);
        // The entrance and the interior tile just inside it stay clear so nothing blocks the way in
        d.tiles[ey * d.width + ex].type = Tile::Walkway;
        d.tiles[ey * d.width + ex].cryptId = static_cast<int16_t>(cryptId);
        d.tiles[(2 * ey - wy) * d.width + (2 * ex - wx)].type = Tile::Walkway;
        d.crypts.push_back(Room{ c.x, c.y, 3, 3 });
    }
}

// Fills in wallMask, wallDistance and startDistance for every tile
void AnalyzeTiles(Dungeon& d) {
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    static const uint8_t sides[4] = { WallNorth, WallEast, WallSouth, WallWest };

    // Out-of-bounds counts as wall, though the border ring is always wall anyway
    for (int y = 0; y < d.height; y++) {
        for (int x = 0; x < d.width; x++) {
            TileData& t = d.tiles[y * d.width + x];
            t.wallMask = 0;
            for (int i = 0; i < 4; i++)
                if (!d.IsWalkable(x + dx[i], y + dy[i])) t.wallMask |= sides[i];
        }
    }

    // Multi-source BFS outward from every wall tile
    std::queue<int> frontier;
    for (int i = 0; i < static_cast<int>(d.tiles.size()); i++) {
        TileData& t = d.tiles[i];
        t.wallDistance = 255;
        t.startDistance = -1;
        if (t.type == Tile::Wall) { t.wallDistance = 0; frontier.push(i); }
    }
    while (!frontier.empty()) {
        int i = frontier.front(); frontier.pop();
        int x = i % d.width, y = i / d.width;
        for (int k = 0; k < 4; k++) {
            int nx = x + dx[k], ny = y + dy[k];
            if (!d.InBounds(nx, ny)) continue;
            TileData& n = d.tiles[ny * d.width + nx];
            if (n.wallDistance == 255) {
                n.wallDistance = static_cast<uint8_t>(d.tiles[i].wallDistance + 1);
                frontier.push(ny * d.width + nx);
            }
        }
    }

    // Walking distance from the first room's center (the player spawn)
    if (d.rooms.empty()) return;
    int start = d.rooms[0].CenterY() * d.width + d.rooms[0].CenterX();
    d.tiles[start].startDistance = 0;
    frontier.push(start);
    while (!frontier.empty()) {
        int i = frontier.front(); frontier.pop();
        int x = i % d.width, y = i / d.width;
        for (int k = 0; k < 4; k++) {
            int nx = x + dx[k], ny = y + dy[k];
            if (!d.IsWalkable(nx, ny)) continue;
            TileData& n = d.tiles[ny * d.width + nx];
            if (n.startDistance == -1) {
                n.startDistance = static_cast<int16_t>(d.tiles[i].startDistance + 1);
                frontier.push(ny * d.width + nx);
            }
        }
    }
}

} // namespace

Dungeon GenerateDungeon(uint64_t seed, const DungeonParams& p) {
    Dungeon d;
    d.seed = seed;
    d.width = p.width;
    d.height = p.height;
    d.tiles.assign(static_cast<size_t>(p.width) * p.height, TileData());

    Rng rng(seed);

    // Keep rooms one tile away from the border so the outer edge is always wall
    for (int attempt = 0; attempt < p.placementAttempts && static_cast<int>(d.rooms.size()) < p.maxRooms; attempt++) {
        int w = rng.Range(p.minRoomSize, p.maxRoomSize);
        int h = rng.Range(p.minRoomSize, p.maxRoomSize);
        if (w > p.width - 2 || h > p.height - 2) continue;
        Room room{ rng.Range(1, p.width - w - 1), rng.Range(1, p.height - h - 1), w, h };

        bool ok = true;
        for (const Room& other : d.rooms) {
            if (Overlaps(room, other, p.roomSpacing)) { ok = false; break; }
        }
        if (!ok) continue;

        CarveRoom(d, room, static_cast<int>(d.rooms.size()));
        d.rooms.push_back(room);
    }

    CorridorCarver carver(d);

    // Every room links to an earlier room so the whole floor is reachable. Prefer the previous
    // room, fall back to older ones if the hallway would run beside another, and as a last
    // resort carve it anyway rather than leave a room disconnected.
    for (int i = 1; i < static_cast<int>(d.rooms.size()); i++) {
        bool connected = false;
        for (int j = i - 1; j >= 0 && !connected; j--)
            connected = carver.Connect(d.rooms[j], d.rooms[i], rng, false);
        if (!connected) carver.Connect(d.rooms[i - 1], d.rooms[i], rng, true);
    }

    // Extra random links create loops; skipped if they'd break the spacing rule
    if (d.rooms.size() > 2) {
        int n = static_cast<int>(d.rooms.size());
        for (int i = 0; i < p.extraCorridors; i++) {
            int a = rng.Range(0, n - 1);
            int b = rng.Range(0, n - 1);
            if (a != b) carver.Connect(d.rooms[a], d.rooms[b], rng, false);
        }
    }

    PlaceCrypts(d, p.maxCrypts, rng);
    AnalyzeTiles(d);
    return d;
}