#pragma once

#include <vector>

#include "raylib.h"

#include "DungeonGenerator.h"

struct Projectile {
    Vector3 position;
    Vector3 velocity;
    float radius;
    float life;     // seconds remaining
};

// Owns and simulates all live projectiles
class ProjectileSystem
{
public:
    static constexpr float SPEED = 20.0f;
    static constexpr float RADIUS = 0.10f;
    static constexpr float GRAVITY = 4.0f;      // gentle drop so the arc is visible
    static constexpr float LIFETIME = 5.0f;

    // direction must be normalized
    void Spawn(Vector3 position, Vector3 direction);
    void Update(const Dungeon &dungeon, float delta);
    void Draw() const;

private:
    bool HitsWorld(const Dungeon &dungeon, const Projectile &p) const;

    std::vector<Projectile> projectiles;
};
