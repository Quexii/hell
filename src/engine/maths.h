#ifndef MATHS_H
#define MATHS_H

#include <stdbool.h>
#include <stdint.h>

#define PI 3.14159265358979323846f
#define TAU (2.0f * PI)

#define RAD45 0.7853981633974483f
#define RAD90 1.570796326794897f
#define RAD180 PI
#define RAD270 4.712388980384690f
#define RAD360 TAU

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float x, y, z, w;
} Vec4;

typedef struct {
    float m[16];
} Mat4;

typedef struct {
    float x, y, w, h;
} Rect;

uint32_t RGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

uint32_t MutCol_R(uint32_t c, uint8_t v);

uint32_t MutCol_G(uint32_t c, uint8_t v);

uint32_t MutCol_B(uint32_t c, uint8_t v);

uint32_t MutCol_A(uint32_t c, uint8_t v);

uint32_t Clamp(uint32_t v, uint32_t a, uint32_t b);

uint32_t Random(uint32_t min, uint32_t max);

double Random_01(void);

float ToRadians(float degrees);

float ToDegree(float radians);

float WrapAngle(float angle);

float WrapAngleSigned(float angle);

float LerpAngle(float from, float to, float t);

float MoveAngle(float current, float target, float maxStep);

Vec2 AngleToDirection(float angle);

float AngleBetween(Vec2 from, Vec2 to);

Mat4 Mat4_Scale(Mat4 m, float x, float y, float z);

void Mat4_ScaleInPlace(Mat4 *m, float x, float y, float z);

Mat4 Mat4_ScaleVec(Mat4 m, Vec3 v);

void Mat4_ScaleVecInPlace(Mat4 *m, Vec3 v);

float AngleDifference(float from, float to);

Mat4 Mat4_RotateX(Mat4 m, float angle);

void Mat4_RotateXInPlace(Mat4 *m, float angle);

Mat4 Mat4_RotateY(Mat4 m, float angle);

void Mat4_RotateYInPlace(Mat4 *m, float angle);

Mat4 Mat4_RotateZ(Mat4 m, float angle);

void Mat4_RotateZInPlace(Mat4 *m, float angle);

Vec2 Vec2_Add(Vec2 a, Vec2 b);

Vec2 Vec2_Sub(Vec2 a, Vec2 b);

Vec2 Vec2_Mul(Vec2 a, Vec2 b);

Vec2 Vec2_Scale(Vec2 v, float s);

float Vec2_Dot(Vec2 a, Vec2 b);

float Vec2_LengthSq(Vec2 v);

float Vec2_Length(Vec2 v);

Vec2 Vec2_Normalize(Vec2 v);

float Vec2_DistanceSq(Vec2 a, Vec2 b);

float Vec2_Distance(Vec2 a, Vec2 b);

Vec2 Vec2_Lerp(Vec2 a, Vec2 b, float t);

Vec2 Vec2_Rotate(Vec2 v, float angle);

Vec3 Vec3_Add(Vec3 a, Vec3 b);

Vec3 Vec3_Sub(Vec3 a, Vec3 b);

Vec3 Vec3_Mul(Vec3 a, Vec3 b);

Vec3 Vec3_Scale(Vec3 v, float s);

float Vec3_Dot(Vec3 a, Vec3 b);

Vec3 Vec3_Cross(Vec3 a, Vec3 b);

float Vec3_LengthSq(Vec3 v);

float Vec3_Length(Vec3 v);

Vec3 Vec3_Normalize(Vec3 v);

float Vec3_DistanceSq(Vec3 a, Vec3 b);

float Vec3_Distance(Vec3 a, Vec3 b);

Vec3 Vec3_Lerp(Vec3 a, Vec3 b, float t);

Vec4 Vec4_Add(Vec4 a, Vec4 b);

Vec4 Vec4_Sub(Vec4 a, Vec4 b);

Vec4 Vec4_Mul(Vec4 a, Vec4 b);

Vec4 Vec4_Scale(Vec4 v, float s);

float Vec4_Dot(Vec4 a, Vec4 b);

float Vec4_LengthSq(Vec4 v);

float Vec4_Length(Vec4 v);

Vec4 Vec4_Normalize(Vec4 v);

float Vec4_DistanceSq(Vec4 a, Vec4 b);

float Vec4_Distance(Vec4 a, Vec4 b);

Vec4 Vec4_Lerp(Vec4 a, Vec4 b, float t);

Mat4 Mat4_Identity(void);

Mat4 Mat4_Zero(void);

Mat4 Mat4_Mul(Mat4 a, Mat4 b);

Mat4 Mat4_Transpose(Mat4 m);

void Mat4_TransposeInPlace(Mat4 *m);

Mat4 Mat4_Translate(Mat4 m, float x, float y, float z);

void Mat4_TranslateInPlace(Mat4 *m, float x, float y, float z);

Mat4 Mat4_TranslateVec(Mat4 m, Vec3 v);

void Mat5_TranslateVecInPlace(Mat4 *m, Vec3 v);

Mat4 Mat4_Scale(Mat4 m, float x, float y, float z);

void Mat4_ScaleInPlace(Mat4 *m, float x, float y, float z);

Mat4 Mat4_ScaleVec(Mat4 m, Vec3 v);

void Mat4_ScaleVecInPlace(Mat4 *m, Vec3 v);

Mat4 Mat4_RotateX(Mat4 m, float angle);

void Mat4_RotateXInPlace(Mat4 *m, float angle);

Mat4 Mat4_RotateY(Mat4 m, float angle);

void Mat4_RotateYInPlace(Mat4 *m, float angle);

Mat4 Mat4_RotateZ(Mat4 m, float angle);

void Mat4_RotateZInPlace(Mat4 *m, float angle);

Mat4 Mat4_Ortho(float left, float right, float bottom, float top, float near, float far);

Vec4 Mat4_TransformVec4(Mat4 m, Vec4 v);

bool Rect_Compare(const Rect *a, const Rect *b);

Rect Rect_Intersect(const Rect *a, const Rect *b);

#endif /* ifndef MATHS_H */
