#include "Filler.h"

#include <algorithm>
#include <cstdlib>

#include "Rng.h"

namespace {

bool TileMatches(const FillerSpec& s, const TileData& t) {
    if ((s.allowedTiles & TileBit(t.type)) == 0) return false;
    if ((t.wallMask & s.requiredWalls) != s.requiredWalls) return false;
    if ((t.wallMask & s.forbiddenWalls) != 0) return false;
    if (t.wallDistance < s.minWallDistance || t.wallDistance > s.maxWallDistance) return false;
    if (t.startDistance < s.minStartDistance) return false;   // also rejects unreachable (-1) tiles
    if (s.maxStartDistance >= 0 && t.startDistance > s.maxStartDistance) return false;
    return true;
}

} // namespace

std::vector<FillerInstance> RunFillerPass(const Dungeon& d, const std::vector<FillerSpec>& specs, uint64_t seed) {
    std::vector<FillerInstance> placed;
    std::vector<bool> occupied(d.tiles.size(), false);
    Rng rng(seed);

    for (int si = 0; si < static_cast<int>(specs.size()); si++) {
        const FillerSpec& spec = specs[si];

        std::vector<int> candidates;
        for (int i = 0; i < static_cast<int>(d.tiles.size()); i++)
            if (TileMatches(spec, d.tiles[i])) candidates.push_back(i);

        for (int i = static_cast<int>(candidates.size()) - 1; i > 0; i--)
            std::swap(candidates[i], candidates[rng.Range(0, i)]);

        bool perCryptMode = spec.maxPerCrypt > 0;
        std::vector<int> cryptTargets(d.crypts.size(), 0);
        std::vector<int> perCrypt(d.crypts.size(), 0);
        int target = rng.Range(spec.minCount, spec.maxCount);
        if (perCryptMode) {
            target = 0;
            for (int& t : cryptTargets) {
                t = rng.Range(spec.minPerCrypt, spec.maxPerCrypt);
                target += t;
            }
        }
        std::vector<int> perRoom(d.rooms.size(), 0);
        std::vector<int> mine;   // tile indices placed for this spec, for spacing checks

        for (int idx : candidates) {
            if (static_cast<int>(mine.size()) >= target) break;
            if (occupied[idx]) continue;

            int cryptId = d.tiles[idx].cryptId;
            if (perCryptMode) {
                if (cryptId < 0 || perCrypt[cryptId] >= cryptTargets[cryptId]) continue;
            }

            int roomId = d.tiles[idx].roomId;
            if (spec.maxPerRoom > 0 && roomId >= 0 && perRoom[roomId] >= spec.maxPerRoom) continue;

            int x = idx % d.width, y = idx / d.width;
            bool tooClose = false;
            for (int other : mine) {
                if (std::max(std::abs(other % d.width - x), std::abs(other / d.width - y)) < spec.minSpacing) {
                    tooClose = true;
                    break;
                }
            }
            if (tooClose) continue;

            occupied[idx] = true;
            mine.push_back(idx);
            if (roomId >= 0) perRoom[roomId]++;
            if (perCryptMode) perCrypt[cryptId]++;
            placed.push_back({ si, x, y });
        }
    }

    return placed;
}
