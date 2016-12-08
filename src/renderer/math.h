#pragma once

#define PI 3.14159265358979323846

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MIN3(a, b, c) (MIN(MIN(a, b), c))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MAX3(a, b, c) (MAX(MAX(a, b), c))

#define CLAMP(val, min, max) (MAX(min, MIN(max, val)))

#define RAD(x) ((x) * (PI / 180.0))

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

  Vec3f cross(Vec3f v);
  float dot(Vec3f v);
  float length();
  Vec3f normalized();
  Vec3f clamped(float min = 0.0, float max = 1.0);
  Vec3f transform(Mat44 mat, float *w);
} Vec3f;

typedef union Vec4f {
  struct {
    float x;
    float y;
    float z;
    float w;
  };

  struct {
    float a;
    float b;
    float c;
    float d;
  };

  float length();
  Vec4f normalized();
  float dot(Vec4f v);
} Vec4f;