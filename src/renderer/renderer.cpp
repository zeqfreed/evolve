#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "game.h"

#define ASSERT(x) if (!(x)) { printf("Assertion at %s, line %d failed: %s\n", __FILE__, __LINE__, #x); *((uint32_t *)1) = 0xDEADCAFE; }

#include "math.cpp"
#include "tga.cpp"
#include "model.cpp"

static void hsv_to_rgb(float h, float s, float v, float *r, float *g, float *b)
{
  h = fmod(h / 60.0, 6);
  float c = v * s;
  float x = c * (1 - fabs(fmod(h, 2) - 1));  
  float m = v - c;

  int hh = (int) h;

  *r = m;
  *g = m;
  *b = m;  
  
  if (hh == 0) {
    *r += c;
    *g += x;
  } else if (hh == 1) {
    *r += x;
    *g += c;
  } else if (hh == 2) {
    *g += c;
    *b += x;
  } else if (hh == 3) {
    *g += x;
    *b += c;
  } else if (hh == 4) {
    *r += x;
    *b += c;
  } else if (hh == 5) {
    *r += c;
    *b += x;
  }
}

static inline void set_pixel(DrawingBuffer *buffer, int32_t x, int32_t y, Vec3f color)
{
  uint8_t r = (uint8_t) (255.0 * color.r);
  uint8_t g = (uint8_t) (255.0 * color.g);
  uint8_t b = (uint8_t) (255.0 * color.b);
  uint32_t c = 0xFF000000 | (b << 16) | (g << 8) | r;

  int idx = (y * buffer->pitch + x) * buffer->bits_per_pixel;
  *((uint32_t *) &((uint8_t *)buffer->pixels)[idx]) = c;
}

#define ZBUFFER_MAX 0xFFFFFFFF

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MIN3(a, b, c) (MIN(MIN(a, b), c))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MAX3(a, b, c) (MAX(MAX(a, b), c))

#define CLAMP(val, min, max) (MAX(min, MIN(max, val)))

typedef struct RenderingContext {
  DrawingBuffer *target;
  uint32_t *zbuffer;

  TgaImage *diffuse;

  Vec3f clear_color;
  Vec3f light;

  Mat44 model_mat;
  Mat44 view_mat;
  Mat44 projection_mat;
  Mat44 viewport_mat;

  Mat44 modelview_mat;
  Mat44 normal_mat;
} RenderingContext;

struct IShader {
  Vec3f positions[3];

  virtual ~IShader() {};
  virtual void vertex(RenderingContext *ctx, int idx, Vec3f position, Vec3f normal, Vec3f texture, Vec3f color) = 0;
  virtual bool fragment(RenderingContext *ctx, float t0, float t1, float t2, Vec3f *color) = 0;
};

struct ModelShader : public IShader {
  Vec3f normals[3];
  Vec3f uvs[3];

  void vertex(RenderingContext *ctx, int idx, Vec3f position, Vec3f normal, Vec3f texture, Vec3f color)
  {
    positions[idx] = position * ctx->modelview_mat * ctx->projection_mat;
    uvs[idx] = (Vec3f){texture.x, texture.y, 0};
    normals[idx] = (normal * ctx->normal_mat).normalized();
  }

  bool fragment(RenderingContext *ctx, float t0, float t1, float t2, Vec3f *color)
  {
    Vec3f normal = normals[0] * t0 + normals[1] * t1 + normals[2] * t2;
    float intensity = normal.dot(ctx->light);

    Vec3f uv = uvs[0] * t0 + uvs[1] * t1 + uvs[2] * t2;
    int texX = uv.x * ctx->diffuse->width;
    int texY = uv.y * ctx->diffuse->height;

    Vec3f tcolor = ctx->diffuse->pixels[texY*ctx->diffuse->width+texX];
    *color = (Vec3f){tcolor.r, tcolor.g, tcolor.b} * intensity;

    color->r = CLAMP(color->r, 0, 1);
    color->g = CLAMP(color->g, 0, 1);
    color->b = CLAMP(color->b, 0, 1);

    return true;
  }
};

struct FloorShader : public IShader {
  Vec3f uvzs[3];
  Vec3f colors[3];
  Vec3f normals[3];

  void vertex(RenderingContext *ctx, int idx, Vec3f position, Vec3f normal, Vec3f texture, Vec3f color)
  {
    Vec3f cam_pos = position * ctx->modelview_mat;
    positions[idx] = cam_pos * ctx->projection_mat;

    // Perspective correct attribute mapping
    float iz = 1 / cam_pos.z;
    uvzs[idx] = (Vec3f){texture.x * iz, texture.y * iz, iz};
    colors[idx] = (Vec3f){color.r * iz, color.g * iz, color.b * iz};

    normals[idx] = (normal * ctx->normal_mat).normalized();
  }

  bool fragment(RenderingContext *ctx, float t0, float t1, float t2, Vec3f *color)
  {
    Vec3f normal = normals[0];
    float intensity = normal.dot(ctx->light);

    Vec3f vcolor = (Vec3f){colors[0].r * t0 + colors[1].r * t1 + colors[2].r * t2,
                           colors[0].g * t0 + colors[1].g * t1 + colors[2].g * t2,
                           colors[0].b * t0 + colors[1].b * t1 + colors[2].b * t2};

    Vec3f uvz = uvzs[0] * t0 + uvzs[1] * t1 + uvzs[2] * t2;
    float z = 1 / uvz.z;
    int texX = uvz.x * z * 5;
    int texY = uvz.y * z * 5;

    Vec3f tcolor;

    if (texX % 2 == texY % 2) {
      tcolor = (Vec3f){1, 1, 1};
    } else {
      tcolor = (Vec3f){0, 0, 0};
    }

    vcolor = vcolor * z;
    *color = (Vec3f){vcolor.r * tcolor.r, vcolor.g * tcolor.g, vcolor.b * tcolor.b} * intensity;

    color->r = CLAMP(color->r, 0, 1);
    color->g = CLAMP(color->g, 0, 1);
    color->b = CLAMP(color->b, 0, 1);

    return true;
  }
};

static void precalculate_matrices(RenderingContext *ctx)
{
  ctx->normal_mat = ctx->model_mat.inverse().transposed();
  ctx->modelview_mat = ctx->model_mat * ctx->view_mat;
}

inline static float edge_func(float x0, float y0, float x1, float y1)
{
  return (x0 * y1) - (x1 * y0);
}

static void draw_triangle(RenderingContext *ctx, IShader *shader)
{
  Vec3f p0 = shader->positions[0] * ctx->viewport_mat;
  Vec3f p1 = shader->positions[1] * ctx->viewport_mat;
  Vec3f p2 = shader->positions[2] * ctx->viewport_mat;

  int minx = MIN3(p0.x, p1.x, p2.x);
  int miny = MIN3(p0.y, p1.y, p2.y);
  int maxx = MAX3(p0.x, p1.x, p2.x);
  int maxy = MAX3(p0.y, p1.y, p2.y);

  int target_width = ctx->target->width;
  int target_height = ctx->target->height;

  // Clip bounding rect to target rect
  minx = MAX(0, minx);
  miny = MAX(0, miny);
  maxx = MIN(maxx, target_width - 1);
  maxy = MIN(maxy, target_height - 1);

  for (int j = miny; j <= maxy; j++) {
    bool inside = false;

    for (int i = minx; i <= maxx; i++) {
      float tx = i + 0.5;
      float ty = j + 0.5;

      float area = edge_func(p1.x - p0.x, p1.y - p0.y, p2.x - p1.x, p2.y - p1.y);
      float t0 = edge_func(p2.x - p1.x, p2.y - p1.y, tx - p1.x, ty - p1.y);
      float t1 = edge_func(p0.x - p2.x, p0.y - p2.y, tx - p2.x, ty - p2.y);
      float t2 = edge_func(p1.x - p0.x, p1.y - p0.y, tx - p0.x, ty - p0.y);

      if (t0 >= 0.0 && t1 >= 0.0 && t2 >= 0.0) {
        inside = true;

        t0 /= area;
        t1 /= area;
        t2 /= area;

        uint32_t zvalue = (1 - (p0.z * t0 + p1.z * t1 + p2.z * t2)) * ZBUFFER_MAX;
        if (zvalue > ctx->zbuffer[j*target_width+i]) {
          ctx->zbuffer[j*target_width+i] = zvalue;

          Vec3f color;
          if (shader->fragment(ctx, t0, t1, t2, &color)) {
            set_pixel(ctx->target, i, j, color);
          }
        }
      } else {
        if (inside) {
          break;
        }
      }
    }
  }
}

static void load_model(GlobalState *state, char *filename, Model *model)
{
  FileContents contents = state->platform_api.read_file_contents(filename);
  if (contents.size <= 0) {
    printf("Failed to load model\n");
    return;
  }

  printf("Read model file of %d bytes\n", contents.size);
  model_read(contents.bytes, contents.size, model);
}

static void load_texture(GlobalState *state, char *filename, TgaImage *target)
{
  FileContents contents = state->platform_api.read_file_contents(filename);
  if (contents.size <= 0) {
    printf("Failed to load texture\n");
    return;
  }

  printf("Read texture file of %d bytes\n", contents.size);
  tga_read_image(contents.bytes, contents.size, target);
}

static void clear_buffer(RenderingContext *ctx)
{
  for (int j = 0; j <= ctx->target->height; j++) {
    for (int i = 0; i <= ctx->target->width; i++) {
      set_pixel(ctx->target, i, j, ctx->clear_color);
    }
  }  
}

static void clear_zbuffer(RenderingContext *ctx)
{
  for (int i = 0; i < 800; i++) {
    for (int j = 0; j < 600; j++) {
      ctx->zbuffer[j*800+i] = 0;
    }
  }
}

static Mat44 projection_matrix(float near, float far, float fov)
{
  Mat44 result = Mat44::identity();
  result.k = (-far / (far - near));
  result.o = (-(far * near) / (far - near));
  result.l = -1.0;
  result.p = 0.0;

  float s = 1.0 / (tan(fov / 2.0 * PI / 180.0));
  result.a = s;
  result.f = s;

  return result;
}

static Mat44 viewport_matrix(float width, float height)
{
  Mat44 result = Mat44::identity();

  float hw = width / 2.0;
  float hh = height / 2.0;

  result.a = hw / (hw / hh);
  result.m = hw;
  result.f = hh;
  result.n = hh;

  return result;
}

static bool initialized = false;

static TgaImage diffuseTexture;
static Model model;
static uint32_t zbuffer[800][600];
static RenderingContext rendering_context;
static ModelShader modelShader;
static FloorShader floorShader;

static float angle = 0;
static float hue = 0.0;

#define WHITE (Vec3f){1, 1, 1}
#define BLACK (Vec3f){0, 0, 0}
#define RED (Vec3f){1, 0, 0}
#define GREEN (Vec3f){0, 1, 0}
#define BLUE (Vec3f){0, 0, 1}

static void initialize(GlobalState *state, DrawingBuffer *buffer)
{
  initialized = true;

  RenderingContext *ctx = &rendering_context;
  ctx->target = buffer;
  ctx->clear_color = BLACK;

  ctx->model_mat = Mat44::identity();
  ctx->viewport_mat = viewport_matrix(buffer->width, buffer->height);
  ctx->projection_mat = projection_matrix(0.1, 10, 90);
  ctx->light = ((Vec3f){0, -1, 0}).normalized();

  // TODO: Allocate dynamically
  ctx->diffuse = &diffuseTexture;
  ctx->zbuffer = (uint32_t *) &zbuffer;

  load_model(state, (char *) "data/rabbit/rabbit.obj", &model);
  load_texture(state, (char *) "data/rabbit/diffuse.tga", rendering_context.diffuse);
}

static void render_floor(RenderingContext *ctx)
{
  ctx->model_mat = Mat44::translate(0, -0.4, 0);

  Vec3f vertices[4][2] = {
    {{-0.5, 0, 0.5}, {0, 0, 0}},
    {{0.5, 0, 0.5}, {1, 0, 0}},
    {{0.5, 0, -0.5}, {1, 1, 0}},
    {{-0.5, 0, -0.5}, {0, 1, 0}}
  };
  Vec3f normal = {0, -1, 0};

  precalculate_matrices(ctx);

  floorShader.vertex(ctx, 0, vertices[0][0], normal, vertices[0][1], RED);
  floorShader.vertex(ctx, 1, vertices[1][0], normal, vertices[1][1], GREEN);
  floorShader.vertex(ctx, 2, vertices[2][0], normal, vertices[2][1], BLUE);
  draw_triangle(ctx, &floorShader);

  floorShader.vertex(ctx, 0, vertices[0][0], normal, vertices[0][1], RED);
  floorShader.vertex(ctx, 1, vertices[2][0], normal, vertices[2][1], BLUE);
  floorShader.vertex(ctx, 2, vertices[3][0], normal, vertices[3][1], WHITE);
  draw_triangle(ctx, &floorShader);
}

static void render_model(RenderingContext *ctx, Model *model, bool wireframe = false)
{
  // hue += 0.5;
  // if (hue > 360.0) {
  //   hue -= 360.0;
  // }

  // float r, g, b;
  // hsv_to_rgb(hue, 1.0, 1.0, &r, &g, &b);
  Vec3f color = {1, 1, 1}; //{r, g, b};

  ctx->model_mat = Mat44::translate(0, -0.4, 0);

  precalculate_matrices(ctx);

  for (int fi = 0; fi < model->fcount; fi++) {
    int *face = model->faces[fi];

    for (int vi = 0; vi < 3; vi++) {
      Vec3f position = model->vertices[face[vi*3]];
      Vec3f texture = model->texture_coords[face[vi*3+1]];
      Vec3f normal = model->normals[face[vi*3+2]];

      modelShader.vertex(ctx, vi, position, normal, texture, color);
    }

    draw_triangle(ctx, &modelShader);
  }  
}

C_LINKAGE void draw_frame(GlobalState *state, DrawingBuffer *drawing_buffer)
{
  if (!initialized) {
    initialize(state, drawing_buffer);
  }

  angle += 0.005; // TODO: Use frame dt
  if (angle > 2*PI) {
    angle -= 2*PI;
  }

  rendering_context.view_mat = (Mat44::translate(0, 0, 1) * Mat44::rotate_y(angle)).inverse();

  clear_buffer(&rendering_context);
  clear_zbuffer(&rendering_context);

  render_floor(&rendering_context);
  render_model(&rendering_context, &model);
}
