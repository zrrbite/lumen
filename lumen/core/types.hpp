#pragma once

#include <cstdint>

namespace lumen {

struct Point {
    int16_t x = 0;
    int16_t y = 0;
};

struct Size {
    uint16_t w = 0;
    uint16_t h = 0;
};

struct Rect {
    int16_t x = 0;
    int16_t y = 0;
    uint16_t w = 0;
    uint16_t h = 0;

    constexpr int16_t right() const { return x + static_cast<int16_t>(w); }
    constexpr int16_t bottom() const { return y + static_cast<int16_t>(h); }
    constexpr uint32_t area() const { return static_cast<uint32_t>(w) * h; }
    constexpr bool empty() const { return w == 0 || h == 0; }

    constexpr bool contains(Point p) const {
        return p.x >= x && p.x < right() && p.y >= y && p.y < bottom();
    }

    constexpr bool intersects(const Rect& o) const {
        return x < o.right() && right() > o.x && y < o.bottom() && bottom() > o.y;
    }

    constexpr Rect intersection(const Rect& o) const {
        int16_t ix = x > o.x ? x : o.x;
        int16_t iy = y > o.y ? y : o.y;
        int16_t ir = right() < o.right() ? right() : o.right();
        int16_t ib = bottom() < o.bottom() ? bottom() : o.bottom();
        if (ix >= ir || iy >= ib) return {};
        return {ix, iy, static_cast<uint16_t>(ir - ix), static_cast<uint16_t>(ib - iy)};
    }

    constexpr Rect bounding_union(const Rect& o) const {
        if (empty()) return o;
        if (o.empty()) return *this;
        int16_t ux = x < o.x ? x : o.x;
        int16_t uy = y < o.y ? y : o.y;
        int16_t ur = right() > o.right() ? right() : o.right();
        int16_t ub = bottom() > o.bottom() ? bottom() : o.bottom();
        return {ux, uy, static_cast<uint16_t>(ur - ux), static_cast<uint16_t>(ub - uy)};
    }
};

struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;

    constexpr uint16_t to_rgb565() const {
        return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    }

    constexpr uint32_t to_argb8888() const {
        return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8) | b;
    }

    static constexpr Color from_rgb565(uint16_t c) {
        return {static_cast<uint8_t>((c >> 11) << 3), static_cast<uint8_t>(((c >> 5) & 0x3F) << 2),
                static_cast<uint8_t>((c & 0x1F) << 3), 255};
    }

    static constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b) { return {r, g, b, 255}; }
    static constexpr Color rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return {r, g, b, a}; }

    // Common colors
    static constexpr Color black() { return {0, 0, 0, 255}; }
    static constexpr Color white() { return {255, 255, 255, 255}; }
    static constexpr Color red() { return {255, 0, 0, 255}; }
    static constexpr Color green() { return {0, 255, 0, 255}; }
    static constexpr Color blue() { return {0, 0, 255, 255}; }
};

/// Pixel format tags for compile-time display configuration.
struct Rgb565 {
    using pixel_t = uint16_t;
    static constexpr uint8_t bytes_per_pixel = 2;
};

struct Argb8888 {
    using pixel_t = uint32_t;
    static constexpr uint8_t bytes_per_pixel = 4;
};

/// Milliseconds since boot (wraps at ~49 days).
using TickMs = uint32_t;

}  // namespace lumen
