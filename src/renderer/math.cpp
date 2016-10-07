
#define PI 3.1415926

typedef struct Mat44 {
  float m[4][4];

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

  result.x = v.x * mat.m[0][0] + v.y * mat.m[0][1] + v.z * mat.m[0][2] + mat.m[0][3];
  result.y = v.x * mat.m[1][0] + v.y * mat.m[1][1] + v.z * mat.m[1][2] + mat.m[1][3];
  result.z = v.x * mat.m[2][0] + v.y * mat.m[2][1] + v.z * mat.m[2][2] + mat.m[2][3];
  float w  = v.x * mat.m[3][0] + v.y * mat.m[3][1] + v.z * mat.m[3][2] + mat.m[3][3];
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

    result.m[0][0] = 1.0;
    result.m[1][1] = 1.0;
    result.m[2][2] = 1.0;
    result.m[3][3] = 1.0;

    return result;    
}

Mat44 Mat44::scale(float x, float y, float z)
{
    Mat44 result = {};

    result.m[0][0] = x;
    result.m[1][1] = y;
    result.m[2][2] = z;
    result.m[3][3] = 1.0;

    return result;    
}

Mat44 Mat44::rotate_z(float angle)
{
  Mat44 result = {};

  result.m[0][0] = cos(angle);
  result.m[1][0] = sin(angle);
  result.m[0][1] = -sin(angle);
  result.m[1][1] = cos(angle);
  result.m[2][2] = 1.0;
  result.m[3][3] = 1.0;

  return result;
}

Mat44 Mat44::rotate_y(float angle)
{
  Mat44 result = {};

  result.m[0][0] = cos(angle);
  result.m[2][0] = -sin(angle);
  result.m[0][2] = sin(angle);
  result.m[2][2] = cos(angle);
  result.m[1][1] = 1.0;
  result.m[3][3] = 1.0;

  return result;
}

Mat44 Mat44::translate(float x, float y, float z)
{
    Mat44 result = {0};

    result.m[0][0] = 1.0;
    result.m[1][1] = 1.0;
    result.m[2][2] = 1.0;
    result.m[3][3] = 1.0;
    result.m[0][3] = x;
    result.m[1][3] = y;
    result.m[2][3] = z;

    return result;
}

inline Mat44 operator*(Mat44 m1, Mat44 m2)
{
    Mat44 result = {};

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += m1.m[i][k] * m2.m[k][j];
            }
        }
    }

    return result;
}
