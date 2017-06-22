#ifndef LT_MATH_HPP
#define LT_MATH_HPP

#include <stdio.h>

#include "lt.hpp"
#include "math.h"

#ifndef LT_PI
#define LT_PI 3.14159265358979323846
#endif

/////////////////////////////////////////////////////////
//
// Vector
//
// Definition of a vector structure.
//
union Vec2i {
    i32 val[2];
    struct {
        i32 x, y;
    };

    Vec2i(i32 x, i32 y): x(x), y(y) {}
};

union Vec2f {
    f32 val[2];
    struct {
        f32 x, y;
    };

    Vec2f(f32 x, f32 y): x(x), y(y) {}
};

union Vec3f {
    f32 val[3];
    struct {
        f32 x, y, z;
    };
    struct {
        f32 r, g, b;
    };

    Vec3f();
    Vec3f(f32 x, f32 y, f32 z);
};

union Vec3i {
    i32 val[3];
    struct {
        i32 x, y, z;
    };
    struct {
        i32 r, g, b;
    };

    Vec3i();
    Vec3i(i32 x, i32 y, i32 z);
};

union Vec4i {
    i32 val[4];
    struct {
        i32 x, y, z, w;
    };
    struct {
        i32 r, g, b, a;
    };

    Vec4i();
    Vec4i(i32 x, i32 y, i32 z, i32 w);
};


inline Vec2i operator-(const Vec2i lhs, const Vec2i rhs) {return Vec2i(lhs.x - rhs.x, lhs.y - rhs.y);}
inline Vec2i operator*(const Vec2i v, const i32 k) {return Vec2i(v.x * k, v.y * k);}
inline Vec2i operator*(const i32 k, const Vec2i v) {return Vec2i(v.x * k, v.y * k);}
inline Vec2i operator-(const Vec2i v) {return Vec2i(-v.x, -v.y);}
inline Vec3f operator-(const Vec3f a, const Vec3f b) {return Vec3f(a.x-b.x, a.y-b.y, a.z-b.z);}
inline Vec3f operator+(const Vec3f a, const Vec3f b) {return Vec3f(a.x+b.x, a.y+b.y, a.z+b.z);}
inline Vec3f operator-(const Vec3f v) {return Vec3f(-v.x, -v.y, -v.z);}
inline f32 vec_len(const Vec3f v) {return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);}

inline i32 vec_dot(const Vec2i a, const Vec2i b) {return (a.x * b.x) + (a.y * b.y);}
inline f32
vec_dot(const Vec3f lhs, const Vec3f rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

inline Vec3f
vec_normalize(const Vec3f vec)
{
    f32 size = vec_len(vec);
    LT_Assert(size >= 0);
    return Vec3f(vec.x / size, vec.y / size, vec.z / size);
}

inline Vec2i
vec_proj(const Vec2i p, const Vec2i plane)
{
    i32 alpha = vec_dot(p, plane) / vec_dot(plane, plane);
    return alpha * plane;
}

inline Vec3f
vec_cross(const Vec3f a, const Vec3f b)
{
    return Vec3f((a.y * b.z) - (a.z * b.y),
                 (a.z * b.x) - (a.x * b.z),
                 (a.x * b.y) - (a.y * b.x));
}


/////////////////////////////////////////////////////////
//
// Matrix
//
// Column major
//
union Mat4 {
    f32 m[4][4];
    struct {
        f32 m00, m01, m02, m03;
        f32 m10, m11, m12, m13;
        f32 m20, m21, m22, m23;
        f32 m30, m31, m32, m33;
    };

    Mat4();
    Mat4(f32 m00, f32 m01, f32 m02, f32 m03,
         f32 m10, f32 m11, f32 m12, f32 m13,
         f32 m20, f32 m21, f32 m22, f32 m23,
         f32 m30, f32 m31, f32 m32, f32 m33);
};

Mat4 mat4_identity   ();
Mat4 mat4_perspective(f32 fovy, f32 aspect_ratio, f32 znear, f32 zfar);
Mat4 mat4_look_at    (const Vec3f eye, const Vec3f center, const Vec3f up);
#endif // LT_MATH_HPP
