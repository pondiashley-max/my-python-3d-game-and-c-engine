#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace maximus {

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 6.28318530717958647692f;
constexpr float DEG2RAD = PI / 180.0f;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    Vec3 operator-() const { return {-x, -y, -z}; }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float length_sq() const { return x * x + y * y + z * z; }
    float length() const { return std::sqrt(length_sq()); }

    Vec3 normalized() const {
        float l = length();
        if (l < 1e-6f) return {0.0f, 1.0f, 0.0f};
        return {x / l, y / l, z / l};
    }
};

inline Vec3 operator*(float s, const Vec3& v) { return v * s; }

inline float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
    return a + (b - a) * t;
}

inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float smoothstep(float edge0, float edge1, float x) {
    float t = clampf((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Column-major 4x4 matrix directly compatible with OpenGL / GLSL mat4.
struct Mat4 {
    float m[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    static Mat4 identity() { return Mat4(); }

    float& at(int col, int row) { return m[col * 4 + row]; }
    float at(int col, int row) const { return m[col * 4 + row]; }

    static Mat4 translate(const Vec3& t) {
        Mat4 r;
        r.m[12] = t.x;
        r.m[13] = t.y;
        r.m[14] = t.z;
        return r;
    }

    static Mat4 scale(const Vec3& s) {
        Mat4 r;
        r.m[0] = s.x;
        r.m[5] = s.y;
        r.m[10] = s.z;
        return r;
    }

    static Mat4 rotate_x(float a) {
        Mat4 r;
        float c = std::cos(a), s = std::sin(a);
        r.m[5] = c;  r.m[9] = -s;
        r.m[6] = s;  r.m[10] = c;
        return r;
    }

    static Mat4 rotate_y(float a) {
        Mat4 r;
        float c = std::cos(a), s = std::sin(a);
        r.m[0] = c;  r.m[8] = s;
        r.m[2] = -s; r.m[10] = c;
        return r;
    }

    static Mat4 rotate_z(float a) {
        Mat4 r;
        float c = std::cos(a), s = std::sin(a);
        r.m[0] = c;  r.m[4] = -s;
        r.m[1] = s;  r.m[5] = c;
        return r;
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 out;
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                out.m[c * 4 + r] =
                    m[0 * 4 + r] * b.m[c * 4 + 0] +
                    m[1 * 4 + r] * b.m[c * 4 + 1] +
                    m[2 * 4 + r] * b.m[c * 4 + 2] +
                    m[3 * 4 + r] * b.m[c * 4 + 3];
            }
        }
        return out;
    }

    Vec3 transform_point(const Vec3& p) const {
        return {
            m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12],
            m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
            m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]
        };
    }

    static Mat4 trs(const Vec3& pos, float yaw, float pitch, float roll, const Vec3& sc) {
        Mat4 M = translate(pos);
        if (std::fabs(yaw) > 1e-5f) M = M * rotate_y(yaw);
        if (std::fabs(pitch) > 1e-5f) M = M * rotate_x(pitch);
        if (std::fabs(roll) > 1e-5f) M = M * rotate_z(roll);
        if (std::fabs(sc.x - 1.0f) > 1e-5f || std::fabs(sc.y - 1.0f) > 1e-5f || std::fabs(sc.z - 1.0f) > 1e-5f) {
            M = M * scale(sc);
        }
        return M;
    }

    static Mat4 perspective(float fov_rad, float aspect, float znear, float zfar) {
        Mat4 r;
        for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
        float f = 1.0f / std::tan(fov_rad * 0.5f);
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = (zfar + znear) / (znear - zfar);
        r.m[11] = -1.0f;
        r.m[14] = (2.0f * zfar * znear) / (znear - zfar);
        return r;
    }

    static Mat4 ortho(float left, float right, float bottom, float top, float znear, float zfar) {
        Mat4 r;
        for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
        r.m[0] = 2.0f / (right - left);
        r.m[5] = 2.0f / (top - bottom);
        r.m[10] = -2.0f / (zfar - znear);
        r.m[12] = -(right + left) / (right - left);
        r.m[13] = -(top + bottom) / (top - bottom);
        r.m[14] = -(zfar + znear) / (zfar - znear);
        r.m[15] = 1.0f;
        return r;
    }

    static Mat4 look_at(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalized();
        Vec3 s = cross(f, up).normalized();
        Vec3 u = cross(s, f);
        Mat4 r = identity();
        r.m[0] = s.x;  r.m[4] = s.y;  r.m[8]  = s.z;  r.m[12] = -dot(s, eye);
        r.m[1] = u.x;  r.m[5] = u.y;  r.m[9]  = u.z;  r.m[13] = -dot(u, eye);
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z; r.m[14] = dot(f, eye);
        return r;
    }
};

struct AABB {
    Vec3 min;
    Vec3 max;

    bool intersects(const AABB& o) const {
        return (min.x <= o.max.x && max.x >= o.min.x) &&
               (min.y <= o.max.y && max.y >= o.min.y) &&
               (min.z <= o.max.z && max.z >= o.min.z);
    }
};

// Fast deterministic PCG-style PRNG
class FastRng {
public:
    explicit FastRng(uint64_t seed = 2372026ULL) : state_(seed ? seed : 2372026ULL) {}

    void seed(uint64_t s) { state_ = s ? s : 2372026ULL; }

    uint32_t next_u32() {
        uint64_t x = state_;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        state_ = x;
        return static_cast<uint32_t>((x * 2685821657736338717ULL) >> 32);
    }

    float next_float() {
        return static_cast<float>(next_u32() & 0xFFFFFF) / static_cast<float>(0x1000000);
    }

    float range(float lo, float hi) {
        return lo + (hi - lo) * next_float();
    }

    int range_int(int lo, int hi_inclusive) {
        if (hi_inclusive <= lo) return lo;
        return lo + static_cast<int>(next_u32() % static_cast<uint32_t>(hi_inclusive - lo + 1));
    }

    bool chance(float p) {
        return next_float() < p;
    }

private:
    uint64_t state_;
};

}  // namespace maximus
