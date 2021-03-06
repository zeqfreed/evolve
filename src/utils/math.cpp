#include <cmath>
#include "math.h"

//
// Vec3f
//

inline Vec3f Vec3f::make(float x, float y, float z)
{
  Vec3f result = {x, y, z};
  return result;
}

inline Vec3f operator+(Vec3f a, Vec3f b)
{
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3f operator-(Vec3f a, Vec3f b)
{
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3f operator-(Vec3f a)
{
  return {-a.x, -a.y, -a.z};
}

inline Vec3f operator*(Vec3f v, float scalar)
{
  return {v.x * scalar, v.y * scalar, v.z * scalar};
}

inline Vec3f operator/(Vec3f v, float scalar)
{
  return v * (1.0f / scalar);
}

inline Vec3f Vec3f::cross(Vec3f v)
{
  float nx = y * v.z - z * v.y;
  float ny = z * v.x - x * v.z;
  float nz = x * v.y - y * v.x;
  return {nx, ny, nz};
}

inline float Vec3f::dot(Vec3f v)
{
  return x * v.x + y * v.y + z * v.z;
}

inline float Vec3f::length()
{
  return sqrt(x * x + y * y + z * z);
}

inline Vec3f Vec3f::normalized()
{
  float factor = 1.0f / this->length();
  return {x *= factor, y *= factor, z *= factor};
}

inline Vec3f Vec3f::clamped(float min, float max)
{
  Vec3f result = {};
  result.x = CLAMP(x, min, max);
  result.y = CLAMP(y, min, max);
  result.z = CLAMP(z, min, max);

  return result;
}

inline Vec3f operator*(Vec3f v, Mat44 mat)
{
  Vec3f result = {};

  result.x = v.x * mat.a + v.y * mat.e + v.z * mat.i + mat.m;
  result.y = v.x * mat.b + v.y * mat.f + v.z * mat.j + mat.n;
  result.z = v.x * mat.c + v.y * mat.g + v.z * mat.k + mat.o;
  float w  = v.x * mat.d + v.y * mat.h + v.z * mat.l + mat.p;

  ASSERT(w != 0.0f);

  if (w != 1.0f) {
    result.x /= w;
    result.y /= w;
    result.z /= w;
  }

  return result;
}

inline Vec3f Vec3f::transform(Mat44 mat, float *w)
{
  Vec3f result = {};

  result.x = x * mat.a + y * mat.e + z * mat.i + mat.m;
  result.y = x * mat.b + y * mat.f + z * mat.j + mat.n;
  result.z = x * mat.c + y * mat.g + z * mat.k + mat.o;
  *w  = x * mat.d + y * mat.h + z * mat.l + mat.p;

  return result;
}

//
// Vec4f
//

inline Vec4f operator+(Vec4f a, Vec4f b)
{
  return Vec4f(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

inline Vec4f operator-(Vec4f a, Vec4f b)
{
  return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

inline Vec4f operator*(Vec4f v, float scalar)
{
  return {v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar};
}

inline float Vec4f::dot(Vec4f v)
{
  return x * v.x + y * v.y + z * v.z + w * v.w;
}

inline float Vec4f::length()
{
  return sqrt(x * x + y * y + z * z + w * w);
}

inline Vec4f Vec4f::normalized()
{
  float factor = 1.0f / this->length();
  return *this * factor;
}

inline Vec4f Vec4f::clamped(float min, float max)
{
  Vec4f result = {};
  result.x = CLAMP(x, min, max);
  result.y = CLAMP(y, min, max);
  result.z = CLAMP(z, min, max);
  result.w = CLAMP(w, min, max);

  return result;
}

inline Vec4f operator*(Vec4f v, Mat44 mat)
{
  Vec4f result = {};

  result.x = v.x * mat.a + v.y * mat.e + v.z * mat.i + v.w * mat.m;
  result.y = v.x * mat.b + v.y * mat.f + v.z * mat.j + v.w * mat.n;
  result.z = v.x * mat.c + v.y * mat.g + v.z * mat.k + v.w * mat.o;
  result.w = v.x * mat.d + v.y * mat.h + v.z * mat.l + v.w * mat.p;

  if (result.w != 1.0f) {
    float rw = 1.0f / result.w;
    result.x *= rw;
    result.y *= rw;
    result.z *= rw;
    result.w = 1.0f;
  }

  return result;
}

//
// Mat44
//

Mat44 Mat44::identity()
{
    Mat44 result = {};

    result.a = 1.0f;
    result.f = 1.0f;
    result.k = 1.0f;
    result.p = 1.0f;

    return result;
}

Mat44 Mat44::scale(float x, float y, float z)
{
    Mat44 result = {};

    result.a = x;
    result.f = y;
    result.k = z;
    result.p = 1.0;

    return result;
}

Mat44 Mat44::rotate_z(float angle)
{
  Mat44 result = {};

  result.a = cos(angle);
  result.e = -sin(angle);
  result.b = sin(angle);
  result.f = cos(angle);
  result.k = 1.0;
  result.p = 1.0;

  return result;
}

Mat44 Mat44::rotate_y(float angle)
{
  Mat44 result = {};

  result.a = cos(angle);
  result.i = sin(angle);
  result.c = -sin(angle);
  result.k = cos(angle);
  result.f = 1.0;
  result.p = 1.0;

  return result;
}

Mat44 Mat44::rotate_x(float angle)
{
 Mat44 result = {};

  result.a = 1.0f;
  result.f = cos(angle);
  result.g = -sin(angle);
  result.j = sin(angle);
  result.k = cos(angle);
  result.p = 1.0f;

  return result;
}

Mat44 Mat44::translate(float x, float y, float z)
{
    Mat44 result = {0};

    result.a = 1.0f;
    result.f = 1.0f;
    result.k = 1.0f;
    result.p = 1.0f;
    result.m = x;
    result.n = y;
    result.o = z;

    return result;
}

inline Mat44 operator*(Mat44 m1, Mat44 m2)
{
    Mat44 result = {};

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.el[i][j] += m1.el[i][k] * m2.el[k][j];
            }
        }
    }

    return result;
}

inline Mat44 operator*(Mat44 m, float factor)
{
  Mat44 result = {};
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      result.el[row][col] = m.el[row][col] * factor;
    }
  }

  return result;
}

#define DET2(a, b, c, d) (a * d - b * c)
#define DET3(a, b, c, d, e, f, g, h, i) (a * DET2(e, f, h, i) - b * DET2(d, f, g, i) + c * DET2(d, e, g, h))

Mat44 Mat44::inverse()
{
  Mat44 result = Mat44::identity();

  float deta = DET3(f, g, h, j, k, l, n, o, p);
  float detb = DET3(e, g, h, i, k, l, m, o, p);
  float detc = DET3(e, f, h, i, j, l, m, n, p);
  float detd = DET3(e, f, g, i, j, k, m, n, o);

  float det = a * deta - b * detb + c * detc - d * detd;

  ASSERT(det != 0.0);
  if (det == 0.0) {
    return *this;
  }

  result.a = deta;
  result.e = -detb;
  result.i = detc;
  result.m = -detd;

  result.b = -DET3(b, c, d, j, k, l, n, o, p);
  result.f = DET3(a, c, d, i, k, l, m, o, p);
  result.j = -DET3(a, b, d, i, j, l, m, n, p);
  result.n = DET3(a, b, c, i, j, k, m, n, o);

  result.c = DET3(b, c, d, f, g, h, n, o, p);
  result.g = -DET3(a, c, d, e, g, h, m, o, p);
  result.k = DET3(a, b, d, e, f, h, m, n, p);
  result.o = -DET3(a, b, c, e, f, g, m, n, o);

  result.d = -DET3(b, c, d, f, g, h, j, k, l);
  result.h = DET3(a, c, d, e, g, h, i, k, l);
  result.l = -DET3(a, b, d, e, f, h, i, j, l);
  result.p = DET3(a, b, c, e, f, g, i, j, k);

  result = result * (1.0f / det);
  return result;
}

Mat44 Mat44::transposed()
{
  Mat44 result = {};
  result.a = a;
  result.f = f;
  result.k = k;
  result.p = p;

  result.b = e;
  result.c = i;
  result.d = m;
  result.e = b;
  result.g = j;
  result.h = n;
  result.i = c;
  result.j = g;
  result.l = o;
  result.m = d;
  result.n = h;
  result.o = l;

  return result;
}

Mat44 Mat44::from_quaternion(Quaternion q)
{
  Mat44 result = {};

  float xx = q.x * q.x;
  float yy = q.y * q.y;
  float zz = q.z * q.z;
  float xy = q.x * q.y;
  float xz = q.x * q.z;
  float xw = q.x * q.w;
  float yz = q.y * q.z;
  float yw = q.y * q.w;
  float wz = q.z * q.w;

  result.a = 1.0f - 2.0f * yy - 2.0f * zz;
  result.b = 2.0f * xy + 2.0f * wz;
  result.c = 2.0f * xz - 2.0f * yw;
  result.e = 2.0f * xy - 2.0f * wz;
  result.f = 1.0f - 2.0f * xx - 2.0f * zz;
  result.g = 2.0f * yz + 2.0f * xw;
  result.i = 2.0f * xz + 2.0f * yw;
  result.j = 2.0f * yz - 2.0f * xw;
  result.k = 1.0f - 2.0f * xx - 2.0f * yy;
  result.p = 1.0f;

  return result;
}


//
// Fixed point
//

inline q8 to_q8(int32_t i) {
  return (q8) (i * Q_SCALE);
}

inline q8 to_q8(float f) {
  return (q8) ((f * Q_SCALE) + 0.5f);
}

inline q8 qmul(q8 a, q8 b) {
  int64_t tmp = (int64_t) a * (int64_t) b;
  return (q8) (tmp >> Q_BITS);
}

inline q8 qdiv(q8 a, q8 b) {
  return (q8) (((int64_t) (a) << Q_BITS) / b);
}

inline float to_float(q8 v) {
  return (float) (v * (1.0 / Q_SCALE));
}

//
// Vec3q
//

inline Vec3q operator+(Vec3q a, Vec3q b)
{
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3q operator-(Vec3q a, Vec3q b)
{
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3q operator-(Vec3q a)
{
  return {-a.x, -a.y, -a.z};
}

inline Vec3q operator*(Vec3q v, q8 q)
{
  return {qmul(v.x, q) , qmul(v.y, q), qmul(v.z, q)};
}

inline Vec3q operator*(q8 q, Vec3q v)
{
  return v * q;
}

inline Vec3q &operator*=(Vec3q &self, q8 q)
{
  self.x = qmul(self.x, q);
  self.y = qmul(self.y, q);
  self.z = qmul(self.z, q);
  return self;
}

void print(char *name, Vec3q v)
{
  printf("%s: %d %d %d\n", name, v.x, v.y, v.z);
}

//
// Quaternion
//
float Quaternion::length()
{
  return sqrt(x * x + y * y + z * z + w * w);
}

Quaternion Quaternion::normalized()
{
  Quaternion result = {};
  result.v = v / length();
  result.w = w;

  return result;
}

Quaternion Quaternion::conjugate()
{
  Quaternion result = {};
  result.v = -v;
  result.w = w;

  return result;
}

Quaternion Quaternion::axisAngle(Vec3f v, float angle)
{
  Quaternion result = {};
  result.x = v.x * sin(angle / 2.0f);
  result.y = v.y * sin(angle / 2.0f);
  result.z = v.z * sin(angle / 2.0f);
  result.w = cos(angle / 2.0f);

  return result;
}

inline Quaternion operator*(Quaternion a, Quaternion b)
{
  Quaternion result = {};
  result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
  result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
  result.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
  result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;

  return result;
}

inline Vec3f operator*(Quaternion a, Vec3f v)
{
  Quaternion b;
  b.v = v;
  b.w = 0;

  return (a*b).v;
}

inline Vec3f operator*(Vec3f v, Quaternion b)
{
  Quaternion a;
  a.v = v;
  a.w = 0;

  return (a*b).v;
}

inline Vec3f lerp(Vec3f a, Vec3f b, float t)
{
  return a * (1.0f - t) + b * t;
}

inline Vec4f lerp(Vec4f a, Vec4f b, float t)
{
  return a * (1.0f - t) + b * t;
}

inline Quaternion lerp(Quaternion a, Quaternion b, float t)
{
  Quaternion result;

  Vec4f v0 = {a.x, a.y, a.z, a.w};
  Vec4f v1 = {b.x, b.y, b.z, b.w};

  float dot = v0.dot(v1);

  if (fabs(dot) > 0.9995f) {
    result.v = lerp(a.v, b.v, t);
    result.w = a.w * (1 - t) + b.w * t;
  } else {
    float c = acos(dot) * t;
    Vec4f v = (v1 - v0 * dot).normalized();
    v = v0 * cos(c) + v * sin(c);

    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    result.w = v.w;
  }

  return result;
}
