#pragma once
// Petites primitives mathématiques utilisées partout dans le moteur.
#include <cmath>
#include <cstdint>

namespace td {

struct Vec2 {
    float x = 0.f;
    float y = 0.f;

    constexpr Vec2() = default;
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(float s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    constexpr bool operator==(Vec2 o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(Vec2 o) const { return !(*this == o); }

    float length() const { return std::sqrt(x * x + y * y); }
    constexpr float lengthSq() const { return x * x + y * y; }
    Vec2 normalized() const {
        const float len = length();
        return len > 0.f ? Vec2{x / len, y / len} : Vec2{};
    }
};

inline float distance(Vec2 a, Vec2 b) { return (a - b).length(); }
inline constexpr float distanceSq(Vec2 a, Vec2 b) { return (a - b).lengthSq(); }

/// Position entière sur la grille (colonne x, ligne y).
struct GridPos {
    int x = 0;
    int y = 0;

    constexpr GridPos() = default;
    constexpr GridPos(int x_, int y_) : x(x_), y(y_) {}
    constexpr bool operator==(GridPos o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(GridPos o) const { return !(*this == o); }
};

struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

template <typename T>
constexpr T clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

} // namespace td
