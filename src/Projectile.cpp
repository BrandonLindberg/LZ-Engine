#include "Projectile.h"

#include <algorithm>
#include <cmath>

#include "raymath.h"

#include "GameConstants.h"

void ProjectileSystem::Spawn(Vector3 position, Vector3 direction)
{
    projectiles.push_back({ position, Vector3Scale(direction, SPEED), RADIUS, LIFETIME });
}

// Sphere against floor, ceiling and wall tiles
bool ProjectileSystem::HitsWorld(const Dungeon &dungeon, const Projectile &p) const
{
    if (p.position.y - p.radius <= 0.0f || p.position.y + p.radius >= WALL_HEIGHT) return true;

    const float half = TILE_SIZE/2.0f;
    int cx = (int)floorf(p.position.x/TILE_SIZE + 0.5f);
    int cz = (int)floorf(p.position.z/TILE_SIZE + 0.5f);

    for (int ty = cz - 1; ty <= cz + 1; ty++)
    {
        for (int tx = cx - 1; tx <= cx + 1; tx++)
        {
            if (dungeon.IsWalkable(tx, ty)) continue;

            float nearestX = Clamp(p.position.x, tx*TILE_SIZE - half, tx*TILE_SIZE + half);
            float nearestZ = Clamp(p.position.z, ty*TILE_SIZE - half, ty*TILE_SIZE + half);
            float dx = p.position.x - nearestX;
            float dz = p.position.z - nearestZ;
            if (dx*dx + dz*dz < p.radius*p.radius) return true;
        }
    }
    return false;
}

void ProjectileSystem::Update(const Dungeon &dungeon, float delta)
{
    for (Projectile &p : projectiles)
    {
        p.velocity.y -= GRAVITY*delta;
        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, delta));
        p.life -= delta;
    }

    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(),
            [&](const Projectile &p) { return p.life <= 0.0f || HitsWorld(dungeon, p); }),
        projectiles.end());
}

void ProjectileSystem::Draw() const
{
    for (const Projectile &p : projectiles)
        DrawSphere(p.position, p.radius, ORANGE);
}
