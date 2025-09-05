#pragma once
#include "LuaDatatypes.h"
#include <raylib.h>
#include <cmath> // For sqrtf

struct Vector2Game {
    float x, y;

    constexpr Vector2Game(): x(0),y(0) {}
    constexpr Vector2Game(float X,float Y): x(X),y(Y) {}
    
    // Convert to Raylib's Vector2
    ::Vector2 toRay() const { return {x,y}; }
    // Convert from Raylib's Vector2
    static Vector2Game fromRay(const ::Vector2& v) { return {v.x, v.y}; }

    // --- Operators ---
    Vector2Game operator+(const Vector2Game& v) const { return {x + v.x, y + v.y}; }
    Vector2Game operator-(const Vector2Game& v) const { return {x - v.x, y - v.y}; }
    Vector2Game operator-() const { return {-x, -y}; }
    Vector2Game operator*(float s) const { return {x * s, y * s}; }
    Vector2Game operator/(float s) const { return {x / s, y / s}; }

    // --- Methods ---
    float magnitudeSquared() const { return x*x + y*y; }
    float magnitude() const { return sqrtf(magnitudeSquared()); }
    Vector2Game normalized() const {
        float mag = magnitude();
        if (mag > 0.00001f) return *this / mag;
        return {0, 0};
    }
    float dot(const Vector2Game& v) const { return x*v.x + y*v.y; }
    Vector2Game lerp(const Vector2Game& goal, float alpha) const {
        return *this + (goal - *this) * alpha;
    }
};

// Traits specialization
namespace lb {
template<> struct Traits<Vector2Game> {
    static const char* MetaName()    { return "Librebox.Vector2"; }
    static const char* GlobalName()  { return "Vector2"; }
    static lua_CFunction Ctor();
    static const luaL_Reg* Methods();
    static const luaL_Reg* MetaMethods();
    static const luaL_Reg* Statics();
};
} // namespace lb
