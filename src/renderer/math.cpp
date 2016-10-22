
#define PI 3.1415926

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
  static inline Mat44 translate(float x, float y, float z);
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
} Vec3f;

inline Vec3f operator+(Vec3f a, Vec3f b)
{
  return (Vec3f){a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3f operator-(Vec3f a, Vec3f b)
{
  return (Vec3f){a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3f operator-(Vec3f a)
{
  return (Vec3f){-a.x, -a.y, -a.z};
}

inline Vec3f operator*(Vec3f v, float scalar)
{
  return (Vec3f){v.x * scalar, v.y * scalar, v.z * scalar};
}

inline Vec3f Vec3f::cross(Vec3f v)
{
  float nx = y * v.z - z * v.y;
  float ny = z * v.x - x * v.z;
  float nz = x * v.y - y * v.x;
  return (Vec3f){nx, ny, nz};
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
  float factor = 1.0 / this->length();
  return (Vec3f){x *= factor, y *= factor, z *= factor};
}

inline Vec3f operator*(Vec3f v, Mat44 mat)
{
  Vec3f result = {};

  result.x = v.x * mat.a + v.y * mat.e + v.z * mat.i + mat.m;
  result.y = v.x * mat.b + v.y * mat.f + v.z * mat.j + mat.n;
  result.z = v.x * mat.c + v.y * mat.g + v.z * mat.k + mat.o;
  float w  = v.x * mat.d + v.y * mat.h + v.z * mat.l + mat.p;

  if (w != 1.0) {
    result.x /= w;
    result.y /= w;
    result.z /= w;
  }

  return result;
}

//
// Matrices
//

Mat44 Mat44::identity()
{
    Mat44 result = {};

    result.a = 1.0;
    result.f = 1.0;
    result.k = 1.0;
    result.p = 1.0;

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

Mat44 Mat44::translate(float x, float y, float z)
{
    Mat44 result = {0};

    result.a = 1.0;
    result.f = 1.0;
    result.k = 1.0;
    result.p = 1.0;
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
