#include "lt_math.hpp"
#include <math.h>
#include <stdio.h>

/////////////////////////////////////////////////////////
//
// Vector implementation
//
Vec3i::Vec3i() {}
Vec3i::Vec3i(i32 x, i32 y, i32 z) : x(x), y(y), z(z) {}

Vec3f::Vec3f() {}
Vec3f::Vec3f(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}

Vec4i::Vec4i() {}
Vec4i::Vec4i(i32 x, i32 y, i32 z, i32 w) : x(x), y(y), z(z), w(w) {}

Mat4::Mat4(f32 m00, f32 m01, f32 m02, f32 m03,
           f32 m10, f32 m11, f32 m12, f32 m13,
           f32 m20, f32 m21, f32 m22, f32 m23,
           f32 m30, f32 m31, f32 m32, f32 m33)
    : m00(m00), m01(m01), m02(m02), m03(m03)
    , m10(m10), m11(m11), m12(m12), m13(m13)
    , m20(m20), m21(m21), m22(m22), m23(m23)
    , m30(m30), m31(m31), m32(m32), m33(m33) {}

Mat4 mat4_identity() {
    return Mat4(1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
}

Mat4 mat4_perspective(f32 fovy, f32 aspect_ratio, f32 znear, f32 zfar) {
    f32 fovy_rad = fovy / 180 * LT_PI;
    f32 f = 1.0/tanf(fovy_rad/2.0);
    f32 zz = (zfar+znear)/(znear-zfar);
    f32 zw = (2*zfar*znear)/(znear-zfar);

    return Mat4(
        f/aspect_ratio,   0,                0,                         0,
        0,                f,                0,                         0,
        0,                0,               zz,                        zw,
        0,                0,               -1,                         0);
}

Mat4 mat4_look_at(const Vec3f eye, const Vec3f center, const Vec3f up) {
    Vec3f f = vec_normalize(center - eye);
    Vec3f s = vec_normalize(vec_cross(f, up));
    Vec3f u = vec_cross(s, f);

    /* Mat4 view = mat4( */
    /*          s.x,              u.x,               -f.x,       0, */
    /*          s.y,              u.y,               -f.y,       0, */
    /*          s.z,              u.z,               -f.z,       0, */
    /*     -vec_dot(s, eye), -vec_dot(u, eye),  vec_dot(f, eye), 1); */
    return Mat4( s.x,   s.y,   s.z, -vec_dot(s, eye),
                 u.x,   u.y,   u.z, -vec_dot(u, eye),
                -f.x,  -f.y,  -f.z,  vec_dot(f, eye),
                 0.0f,  0.0f,  0.0f,       1.0f    );
}
