#pragma once

#include <cstdint>

#define PI 3.14159265358979323846f

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MIN3(a, b, c) (MIN(MIN(a, b), c))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MAX3(a, b, c) (MAX(MAX(a, b), c))

#define CLAMP(val, min, max) (MAX(min, MIN(max, val)))

#define RAD(x) ((x) * (PI / 180.0f))

#define Q_BITS 8
#define Q_SCALE (1 << Q_BITS)
#define Q_ONE Q_SCALE
#define Q_FMASK ((1 << Q_BITS) - 1)
#define qadd(a, b) ((q8) a + (q8) b)
#define qsub(a, b) ((q8) a - (q8) b)
#define qint(v) ((v) >> Q_BITS)

typedef int32_t q8;

typedef union Vec3q {
  struct { q8 x, y, z; };
  struct { q8 a, b, c; };
  q8 i[3];
} Vec3q;

union Quaternion;
typedef Quaternion Quaternion;

typedef struct Mat44 {
  union {
    float el[4][4];
    struct {
      float a, b, c, d;
      float e, f, g, h;
      float i, j, k, l;
      float m, n, o, p;
    };
  };

  static inline Mat44 identity();
  static inline Mat44 scale(float x, float y, float z);
  static inline Mat44 rotate_z(float angle);
  static inline Mat44 rotate_y(float angle);
  static inline Mat44 rotate_x(float angle);
  static inline Mat44 translate(float x, float y, float z);
  static inline Mat44 from_quaternion(Quaternion q);

  void print();
  Mat44 inverse();
  Mat44 transposed();
} Mat44;

typedef union Vec3f {
  struct {
    float x;
    float y;
    float z;
  };

  struct {
    float r;
    float g;
    float b;
  };

  constexpr Vec3f(): x(0.0f), y(0.0f), z(0.0f) {};
  constexpr Vec3f(const float x, const float y, const float z): x(x), y(y), z(z) {};

  static Vec3f make(float x, float y, float z);

  Vec3f cross(Vec3f v);
  float dot(Vec3f v);
  float length();
  Vec3f normalized();
  Vec3f clamped(float min = 0.0, float max = 1.0);
  Vec3f transform(Mat44 mat, float *w);
} Vec3f;

typedef union Vec4f {
  union {
    Vec3f xyz;
    Vec3f rgb;

    struct {
      float x;
      float y;
      float z;
      float w;
    };

    struct {
      float r;
      float g;
      float b;
      float a;
    };

    float e[4];
  };

  float length();
  Vec4f normalized();
  float dot(Vec4f v);

  constexpr Vec4f(): x(0.0f), y(0.0f), z(0.0f), w(0.0f) {};
  constexpr Vec4f(Vec3f xyz, float w = 0.0f): x(xyz.x), y(xyz.y), z(xyz.z), w(w) {};
  constexpr Vec4f(float x, float y, float z, float w): x(x), y(y), z(z), w(w) {};
} Vec4f;

typedef union Quaternion {
  Vec3f v;
  struct {
    float x;
    float y;
    float z;
    float w;
  };

  constexpr Quaternion(): x(0.0f), y(0.0f), z(0.0f), w(0.0f) {};
  constexpr Quaternion(const Vec3f v, const float w = 0.0f): x(v.x), y(v.y), z(v.z), w(w) {};
  constexpr Quaternion(const float x, const float y, const float z, const float w): x(x), y(y), z(z), w(w) {};

  float length();
  Quaternion normalized();
  Quaternion conjugate();
  static Quaternion axisAngle(Vec3f v, float angle);
} Quaternion;
