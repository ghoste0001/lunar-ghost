#pragma once
#include "Vector3Game.h"
#include <raylib.h>

struct RayGame {
    Vector3Game Origin;
    Vector3Game Direction;
    
    RayGame() : Origin(0, 0, 0), Direction(0, 0, -1) {}
    RayGame(const Vector3Game& origin, const Vector3Game& direction) 
        : Origin(origin), Direction(direction) {}
    
    // Convert to Raylib's Ray
    ::Ray toRaylib() const {
        return {Origin.toRay(), Direction.toRay()};
    }
    
    // Convert from Raylib's Ray
    static RayGame fromRaylib(const ::Ray& r) {
        return RayGame(Vector3Game::fromRay(r.position), Vector3Game::fromRay(r.direction));
    }
    
    // Get point along ray at distance t
    Vector3Game GetPoint(float t) const {
        return Origin + Direction * t;
    }
};
