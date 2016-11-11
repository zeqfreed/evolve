
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

//
// Vec3f
//

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

  if (w != 1.0) {
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
  return (Vec4f){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

inline Vec4f operator-(Vec4f a, Vec4f b)
{
  return (Vec4f){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

inline Vec4f operator*(Vec4f v, float scalar)
{
  return (Vec4f){v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar};
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
  float factor = 1.0 / this->length();
  return *this * factor;
}

inline void print(const char *tag, Vec4f v)
{
  printf("%s:\tx: %.3f; y: %.3f; z: %.3f; w: %.3f\n", tag, v.x, v.y, v.z, v.w);
}

inline Vec4f operator*(Vec4f v, Mat44 mat)
{
  Vec4f result = {};

  result.x = v.x * mat.a + v.y * mat.e + v.z * mat.i + v.w * mat.m;
  result.y = v.x * mat.b + v.y * mat.f + v.z * mat.j + v.w * mat.n;
  result.z = v.x * mat.c + v.y * mat.g + v.z * mat.k + v.w * mat.o;
  result.w = v.x * mat.d + v.y * mat.h + v.z * mat.l + v.w * mat.p;

  if (result.w != 1.0) {
    float rw = 1.0 / result.w;
    result.x *= rw;
    result.y *= rw;
    result.z *= rw;
    result.w = 1.0;
  }

  return result;
}

//
// Mat44
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

Mat44 Mat44::rotate_x(float angle)
{
 Mat44 result = {};

  result.a = 1.0;
  result.f = cos(angle);
  result.g = -sin(angle);
  result.j = sin(angle);
  result.k = cos(angle);
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

  if (det == 0.0) {
    this->print();
    ASSERT(det != 0.0);
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

  result = result * (1.0 / det);
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

void Mat44::print()
{
  for (int row = 0; row < 4; row++) {
    printf("%.5f %.5f %.5f %.5f\n", el[row][0], el[row][1], el[row][2], el[row][3]);
  }
}
