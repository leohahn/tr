#ifndef LT_MATH_HPP
#define LT_MATH_HPP

#include "lt.hpp"

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
    i32 vals[2];
    struct {
        i32 x, y;
    };

    Vec2i(i32 x, i32 y): x(x), y(y) {}
};

union Vec3f {
    f32 vals[3];
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
    i32 vals[3];
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
    i32 vals[4];
    struct {
        i32 x, y, z, w;
    };
    struct {
        i32 r, g, b, a;
    };

    Vec4i();
    Vec4i(i32 x, i32 y, i32 z, i32 w);
};

Vec3i vec_add      (const Vec3i a, const Vec3i b);
Vec3f vec_add      (const Vec3f a, const Vec3f b);
Vec3i vec_sub      (const Vec3i a, const Vec3i b);
Vec3f vec_sub      (const Vec3f a, const Vec3f b);
Vec4i vec_add      (const Vec4i a, const Vec4i b);
Vec4i vec_sub      (const Vec4i a, const Vec4i b);
f32   vec_dot      (const Vec3f a, const Vec3f b);
Vec3f vec_normalize(const Vec3f a);

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
