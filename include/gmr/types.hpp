#ifndef GMR_TYPES_HPP
#define GMR_TYPES_HPP

#include <cstdint>
#include <optional>

namespace gmr {

// Handle types - Ruby only sees these, never raw pointers/resources
using TextureHandle = int32_t;
using SoundHandle = int32_t;
using MusicHandle = int32_t;
using FontHandle = int32_t;

constexpr int32_t INVALID_HANDLE = -1;

// Color (our own type, not raylib's)
struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
    
    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) 
        : r(r), g(g), b(b), a(a) {}
};

// Vector2 (our own type)
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}
    
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
};

// Rectangle (our own type)
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    
    Rect() = default;
    Rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
};

} // namespace gmr

#endif
