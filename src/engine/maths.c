#include "maths.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

uint32_t RGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (uint32_t) r | ((uint32_t) g << 8) | ((uint32_t) b << 16) | ((uint32_t) a << 24);
}

uint32_t MutCol_R(uint32_t c, uint8_t v) {
    return (c & 0xFFFFFF00u) | (uint32_t) v;
}

uint32_t MutCol_G(uint32_t c, uint8_t v) {
    return (c & 0xFFFF00FFu) | ((uint32_t) v << 8);
}

uint32_t MutCol_B(uint32_t c, uint8_t v) {
    return (c & 0xFF00FFFFu) | ((uint32_t) v << 16);
}

uint32_t MutCol_A(uint32_t c, uint8_t v) {
    return (c & 0x00FFFFFFu) | ((uint32_t) v << 24);
}

uint32_t Clamp(uint32_t v, uint32_t a, uint32_t b) {
    if (v < a)
        return a;
    if (v > b)
        return b;
    return v;
}

uint32_t Random(uint32_t min, uint32_t max) {
    return Clamp(rand(), min, max);
}

double Random_01(void) {
    return (double) rand() / RAND_MAX;
}

float ToRadians(float degrees) {
    return degrees * (PI / 180.f);
}

float ToDegree(float radians) {
    return radians * (180.f / PI);
}

float WrapAngle(float angle) {
    angle = fmodf(angle, TAU);
    return angle < 0.0f ? angle + TAU : angle;
}

float WrapAngleSigned(float angle) {
    return WrapAngle(angle + PI) - PI;
}

float AngleDifference(float from, float to) {
    return WrapAngleSigned(to - from);
}

float LerpAngle(float from, float to, float t) {
    return WrapAngle(from + AngleDifference(from, to) * t);
}

float MoveAngle(float current, float target, float maxStep) {
    float delta = AngleDifference(current, target);
    float step = fmaxf(0.0f, maxStep);

    if (fabsf(delta) <= step)
        return WrapAngle(target);

    return WrapAngle(current + copysignf(step, delta));
}

Vec2 AngleToDirection(float angle) {
    return (Vec2){ cosf(angle), sinf(angle) };
}

float AngleBetween(Vec2 from, Vec2 to) {
    return atan2f(to.y - from.y, to.x - from.x);
}

Vec2 Vec2_Add(Vec2 a, Vec2 b) {
    return (Vec2){ a.x + b.x, a.y + b.y };
}

Vec2 Vec2_Sub(Vec2 a, Vec2 b) {
    return (Vec2){ a.x - b.x, a.y - b.y };
}

Vec2 Vec2_Mul(Vec2 a, Vec2 b) {
    return (Vec2){ a.x * b.x, a.y * b.y };
}

Vec2 Vec2_Scale(Vec2 v, float s) {
    return (Vec2){ v.x * s, v.y * s };
}

float Vec2_Dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

float Vec2_LengthSq(Vec2 v) {
    return Vec2_Dot(v, v);
}

float Vec2_Length(Vec2 v) {
    return sqrtf(Vec2_LengthSq(v));
}

Vec2 Vec2_Normalize(Vec2 v) {
    float len = Vec2_Length(v);
    return len > 0.0f ? Vec2_Scale(v, 1.0f / len) : (Vec2){ 0 };
}

float Vec2_DistanceSq(Vec2 a, Vec2 b) {
    return Vec2_LengthSq(Vec2_Sub(a, b));
}

float Vec2_Distance(Vec2 a, Vec2 b) {
    return sqrtf(Vec2_DistanceSq(a, b));
}

Vec2 Vec2_Lerp(Vec2 a, Vec2 b, float t) {
    return Vec2_Add(a, Vec2_Scale(Vec2_Sub(b, a), t));
}

Vec2 Vec2_Rotate(Vec2 v, float angle) {
    float c = cosf(angle), s = sinf(angle);
    return (Vec2){ v.x * c - v.y * s, v.x * s + v.y * c };
}

Vec3 Vec3_Add(Vec3 a, Vec3 b) {
    return (Vec3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec3 Vec3_Sub(Vec3 a, Vec3 b) {
    return (Vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3 Vec3_Mul(Vec3 a, Vec3 b) {
    return (Vec3){ a.x * b.x, a.y * b.y, a.z * b.z };
}

Vec3 Vec3_Scale(Vec3 v, float s) {
    return (Vec3){ v.x * s, v.y * s, v.z * s };
}

float Vec3_Dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Vec3_Cross(Vec3 a, Vec3 b) {
    return (Vec3){ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

float Vec3_LengthSq(Vec3 v) {
    return Vec3_Dot(v, v);
}

float Vec3_Length(Vec3 v) {
    return sqrtf(Vec3_LengthSq(v));
}

Vec3 Vec3_Normalize(Vec3 v) {
    float len = Vec3_Length(v);
    return len > 0.0f ? Vec3_Scale(v, 1.0f / len) : (Vec3){ 0 };
}

float Vec3_DistanceSq(Vec3 a, Vec3 b) {
    return Vec3_LengthSq(Vec3_Sub(a, b));
}

float Vec3_Distance(Vec3 a, Vec3 b) {
    return sqrtf(Vec3_DistanceSq(a, b));
}

Vec3 Vec3_Lerp(Vec3 a, Vec3 b, float t) {
    return Vec3_Add(a, Vec3_Scale(Vec3_Sub(b, a), t));
}

Vec4 Vec4_Add(Vec4 a, Vec4 b) {
    return (Vec4){ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

Vec4 Vec4_Sub(Vec4 a, Vec4 b) {
    return (Vec4){ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

Vec4 Vec4_Mul(Vec4 a, Vec4 b) {
    return (Vec4){ a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
}

Vec4 Vec4_Scale(Vec4 v, float s) {
    return (Vec4){ v.x * s, v.y * s, v.z * s, v.w * s };
}

float Vec4_Dot(Vec4 a, Vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float Vec4_LengthSq(Vec4 v) {
    return Vec4_Dot(v, v);
}

float Vec4_Length(Vec4 v) {
    return sqrtf(Vec4_LengthSq(v));
}

Vec4 Vec4_Normalize(Vec4 v) {
    float len = Vec4_Length(v);
    return len > 0.0f ? Vec4_Scale(v, 1.0f / len) : (Vec4){ 0 };
}

float Vec4_DistanceSq(Vec4 a, Vec4 b) {
    return Vec4_LengthSq(Vec4_Sub(a, b));
}

float Vec4_Distance(Vec4 a, Vec4 b) {
    return sqrtf(Vec4_DistanceSq(a, b));
}

Vec4 Vec4_Lerp(Vec4 a, Vec4 b, float t) {
    return Vec4_Add(a, Vec4_Scale(Vec4_Sub(b, a), t));
}

Mat4 Mat4_Identity(void) {
    return (Mat4){
        { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }
    };
}

Mat4 Mat4_Zero(void) {
    return (Mat4){ 0 };
}

Mat4 Mat4_Mul(Mat4 a, Mat4 b) {
    Mat4 out;

    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            out.m[col * 4 + row] = a.m[row] * b.m[col * 4] + a.m[4 + row] * b.m[col * 4 + 1] +
                                   a.m[8 + row] * b.m[col * 4 + 2] +
                                   a.m[12 + row] * b.m[col * 4 + 3];
        }
    }

    return out;
}

Mat4 Mat4_Transpose(Mat4 m) {
    return (Mat4){
        { m.m[0],
         m.m[4],
         m.m[8],
         m.m[12],
         m.m[1],
         m.m[5],
         m.m[9],
         m.m[13],
         m.m[2],
         m.m[6],
         m.m[10],
         m.m[14],
         m.m[3],
         m.m[7],
         m.m[11],
         m.m[15] }
    };
}

void Mat4_TransposeInPlace(Mat4 *m) {
    const float a01 = m->m[1], a02 = m->m[2], a03 = m->m[3];
    const float a12 = m->m[6], a13 = m->m[7];
    const float a23 = m->m[11];

    m->m[1] = m->m[4];
    m->m[2] = m->m[8];
    m->m[3] = m->m[12];
    m->m[4] = a01;
    m->m[6] = m->m[9];
    m->m[7] = m->m[13];
    m->m[8] = a02;
    m->m[9] = a12;
    m->m[11] = m->m[14];
    m->m[12] = a03;
    m->m[13] = a13;
    m->m[14] = a23;
}

Mat4 Mat4_Translate(Mat4 m, float x, float y, float z) {
    return (Mat4){
        {
         m.m[0],
         m.m[1],
         m.m[2],
         m.m[3],
         m.m[4],
         m.m[5],
         m.m[6],
         m.m[7],
         m.m[8],
         m.m[9],
         m.m[10],
         m.m[11],
         m.m[0] * x + m.m[4] * y + m.m[8] * z + m.m[12],
         m.m[1] * x + m.m[5] * y + m.m[9] * z + m.m[13],
         m.m[2] * x + m.m[6] * y + m.m[10] * z + m.m[14],
         m.m[3] * x + m.m[7] * y + m.m[11] * z + m.m[15],
         }
    };
}

void Mat4_TranslateInPlace(Mat4 *m, float x, float y, float z) {
    m->m[12] = m->m[0] * x + m->m[4] * y + m->m[8] * z + m->m[12];
    m->m[13] = m->m[1] * x + m->m[5] * y + m->m[9] * z + m->m[13];
    m->m[14] = m->m[2] * x + m->m[6] * y + m->m[10] * z + m->m[14];
    m->m[15] = m->m[3] * x + m->m[7] * y + m->m[11] * z + m->m[15];
}

Mat4 Mat4_TranslateVec(Mat4 m, Vec3 v) {
    return Mat4_Translate(m, v.x, v.y, v.z);
}

void Mat4_TranslateVecInPlace(Mat4 *m, Vec3 v) {
    Mat4_TranslateInPlace(m, v.x, v.y, v.z);
}

Mat4 Mat4_Scale(Mat4 m, float x, float y, float z) {
    Mat4_ScaleInPlace(&m, x, y, z);
    return m;
}

void Mat4_ScaleInPlace(Mat4 *m, float x, float y, float z) {
    for (int i = 0; i < 4; i++) {
        m->m[i] *= x;
        m->m[4 + i] *= y;
        m->m[8 + i] *= z;
    }
}

Mat4 Mat4_ScaleVec(Mat4 m, Vec3 v) {
    return Mat4_Scale(m, v.x, v.y, v.z);
}

void Mat4_ScaleVecInPlace(Mat4 *m, Vec3 v) {
    Mat4_ScaleInPlace(m, v.x, v.y, v.z);
}

Mat4 Mat4_RotateX(Mat4 m, float angle) {
    Mat4_RotateXInPlace(&m, angle);
    return m;
}

void Mat4_RotateXInPlace(Mat4 *m, float angle) {
    float c = cosf(angle), s = sinf(angle);

    for (int i = 0; i < 4; i++) {
        float y = m->m[4 + i];
        float z = m->m[8 + i];

        m->m[4 + i] = y * c + z * s;
        m->m[8 + i] = z * c - y * s;
    }
}

Mat4 Mat4_RotateY(Mat4 m, float angle) {
    Mat4_RotateYInPlace(&m, angle);
    return m;
}

void Mat4_RotateYInPlace(Mat4 *m, float angle) {
    float c = cosf(angle), s = sinf(angle);

    for (int i = 0; i < 4; i++) {
        float x = m->m[i];
        float z = m->m[8 + i];

        m->m[i] = x * c - z * s;
        m->m[8 + i] = x * s + z * c;
    }
}

Mat4 Mat4_RotateZ(Mat4 m, float angle) {
    Mat4_RotateZInPlace(&m, angle);
    return m;
}

void Mat4_RotateZInPlace(Mat4 *m, float angle) {
    float c = cosf(angle), s = sinf(angle);

    for (int i = 0; i < 4; i++) {
        float x = m->m[i];
        float y = m->m[4 + i];

        m->m[i] = x * c + y * s;
        m->m[4 + i] = y * c - x * s;
    }
}

Mat4 Mat4_Ortho(float left, float right, float bottom, float top, float near, float far) {
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;

    return (Mat4){
        { 2.0f / rl,
         0, 0,
         0, 0,
         2.0f / tb,
         0, 0,
         0, 0,
         1.0f / (near - far),
         0, -(right + left) / rl,
         -(top + bottom) / tb,
         near / fn,
         1 }
    };
}

Vec4 Mat4_TransformVec4(Mat4 m, Vec4 v) {
    return (Vec4){ m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12] * v.w,
                   m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13] * v.w,
                   m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w,
                   m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w };
}

bool Rect_Compare(const Rect *a, const Rect *b) {
    if (!a || !b)
        return false;
    return a->x == b->x && a->y == b->y && a->w == b->w && a->h == b->h;
}

Rect Rect_Intersect(const Rect *a, const Rect *b) {
    float x1 = fmaxf(a->x, b->x);
    float y1 = fmaxf(a->y, b->y);

    float x2 = fminf(a->x + a->w, b->x + b->w);
    float y2 = fminf(a->y + a->h, b->y + b->h);

    if (x2 <= x1 || y2 <= y1)
        return (Rect){ 0 };

    return (Rect){ .x = x1, .y = y1, .w = x2 - x1, .h = y2 - y1 };
}
