//
// Created by Loyal on 9/30/26.
//

#ifndef EUCLASESOUND_VEC_H
#define EUCLASESOUND_VEC_H

#include <cmath>

namespace Euclase {

struct vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr vec2() = default;
    constexpr vec2(float x, float y) : x(x), y(y) {}

    constexpr vec2 operator+(const vec2& other) const {
        return {x + other.x, y + other.y};
    }

    constexpr vec2 operator-(const vec2& other) const {
        return {x - other.x, y - other.y};
    }

    constexpr vec2 operator*(float scalar) const {
        return {x * scalar, y * scalar};
    }

    constexpr vec2 operator/(float scalar) const {
        return {x / scalar, y / scalar};
    }

    constexpr vec2& operator+=(const vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr vec2& operator-=(const vec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr vec2& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr vec2& operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    constexpr float operator[](size_t index) const {
        return index == 0 ? x : y;
    }

    constexpr float& operator[](size_t index) {
        return index == 0 ? x : y;
    }

    float Length() const {
        return std::sqrt(x * x + y * y);
    }

    constexpr float LengthSquared() const {
        return x * x + y * y;
    }

    constexpr vec2 &operator*=(int i) {
        x *= i;
        y *= i;
        return *this;
    }

};

constexpr vec2 operator*(float scalar, const vec2& v) {
    return v * scalar;
}
    struct vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr vec3() = default;
    constexpr vec3(float x, float y, float z)
        : x(x), y(y), z(z) {}


    constexpr vec3 operator+(const vec3& other) const {
        return {
            x + other.x,
            y + other.y,
            z + other.z
        };
    }

    constexpr vec3 operator-(const vec3& other) const {
        return {
            x - other.x,
            y - other.y,
            z - other.z
        };
    }

    constexpr vec3 operator*(float scalar) const {
        return {
            x * scalar,
            y * scalar,
            z * scalar
        };
    }

    constexpr vec3 operator/(float scalar) const {
        return {
            x / scalar,
            y / scalar,
            z / scalar
        };
    }

    constexpr vec3& operator+=(const vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    constexpr vec3& operator-=(const vec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    constexpr vec3& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr vec3& operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    constexpr float operator[](size_t index) const {
        return index == 0 ? x :
               index == 1 ? y : z;
    }

    constexpr float& operator[](size_t index) {
        return index == 0 ? x :
               index == 1 ? y : z;
    }

    float Length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    constexpr float LengthSquared() const {
        return x * x + y * y + z * z;
    }
};

    constexpr vec3 operator*(float scalar, const vec3& v) {
        return v * scalar;
    }

struct vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    constexpr vec4() = default;
    constexpr vec4(float x, float y, float z, float w = 1.0f)
        : x(x), y(y), z(z), w(w) {}

    constexpr vec4 operator+(const vec4& other) const {
        return {
            x + other.x,
            y + other.y,
            z + other.z,
            w + other.w
        };
    }

    constexpr vec4 operator-(const vec4& other) const {
        return {
            x - other.x,
            y - other.y,
            z - other.z,
            w - other.w
        };
    }

    constexpr vec4 operator*(float scalar) const {
        return {
            x * scalar,
            y * scalar,
            z * scalar,
            w * scalar
        };
    }

    constexpr vec4 operator/(float scalar) const {
        return {
            x / scalar,
            y / scalar,
            z / scalar,
            w / scalar
        };
    }

    constexpr vec4& operator+=(const vec4& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    constexpr vec4& operator-=(const vec4& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    constexpr vec4& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    constexpr vec4& operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
    }

    constexpr float operator[](size_t index) const {
        return index == 0 ? x :
               index == 1 ? y :
               index == 2 ? z : w;
    }

    constexpr float& operator[](size_t index) {
        return index == 0 ? x :
               index == 1 ? y :
               index == 2 ? z : w;
    }

    float Length() const {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }

    constexpr float LengthSquared() const {
        return x * x + y * y + z * z + w * w;
    }
};

constexpr vec4 operator*(float scalar, const vec4& v) {
    return v * scalar;
}

} // namespace Euclase

#endif // EUCLASESOUND_VEC_H