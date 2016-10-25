#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "game.h"

#define ASSERT(x) if (!(x)) { printf("Assertion at %s, line %d failed: %s\n", __FILE__, __LINE__, #x); *((uint32_t *)1) = 0xDEADCAFE; }

#include "math.cpp"
#include "tga.cpp"

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
  uint32_t c = 0xFF000000 | (r << 16) | (g << 8) | b;

  int idx = (y * buffer->pitch + x) * buffer->bits_per_pixel;
  *((uint32_t *) &((uint8_t *)buffer->pixels)[idx]) = c;
}

static inline void set_pixel_safe(DrawingBuffer *buffer, int32_t x, int32_t y, Vec3f color)
{
  if (x < 0) { return; }
  if (y < 0) { return; }
  if (x >= buffer->width) { return; }
  if (y >= buffer->height) { return; }

  set_pixel(buffer, x, y, color);
}

static inline int32_t abs(int32_t v)
{
  return v > 0.0 ? v : -v;
}

static void draw_line(DrawingBuffer *buffer, int32_t x0, int32_t y0, int32_t x1, int32_t y1, Vec3f color)
{
  int32_t t;
  bool transposed = abs(y1-y0) > abs(x1-x0);
  if (transposed) {
    t = x0;
    x0 = y0;
    y0 = t;

    t = x1;
    x1 = y1;
    y1 = t;
  }

  if (x0 > x1) {
    t = x0;
    x0 = x1;
    x1 = t;

    t = y0;
    y0 = y1;
    y1 = t;
  }

  int32_t dx = x1 - x0;
  int32_t dy = y1 - y0;

  int32_t y = y0;
  int32_t slope = 2 * abs(dy);
  int32_t error = 0;
  int32_t inc = dy > 0 ? 1 : -1;

  for (int32_t x = x0; x <= x1; x++) {
    if (transposed) {
      set_pixel_safe(buffer, y, x, color);
    } else {
      set_pixel_safe(buffer, x, y, color);
    }

    error += slope;
    if (error > dx) {
      y += inc;
      error -= 2*dx;
    }
  }
}

typedef struct Vertex {
  Vec3f pos;
  Vec3f color;
  Vec3f normal;
  Vec3f texture_coords;
} Vertex;

#define ZBUFFER_MAX 0xFFFFFFFF

typedef struct Model {
  int vcount;
  int tcount;
  int ncount;
  int fcount;
  Vec3f vertices[2048];
  Vec3f normals[2048];
  Vec3f texture_coords[2048];
  int faces[4000][9];
} Model;

typedef struct RenderingContext {
  TgaImage *diffuse;
  Model *model;
  uint32_t *zbuffer;
  DrawingBuffer *target;
  Vec3f light;
  Mat44 modelview;
  Mat44 projection;
  Mat44 viewport;
} RenderingContext;

typedef struct Shader {
  Vec3f positions[3];
  Vec3f normals[3];
  Vec3f texture_coords[3];
  Vec3f colors[3];

  void vertex(RenderingContext *ctx, int idx, Vec3f position, Vec3f normal, Vec3f texture, Vec3f color);
  bool fragment(RenderingContext *ctx, float t0, float t1, float t2, Vec3f *color);
} Shader;

inline static float edge_func(float x0, float y0, float x1, float y1)
{
  return (x0 * y1) - (x1 * y0);
}

static void clear_zbuffer(RenderingContext *ctx)
{
  for (int i = 0; i < 800; i++) {
    for (int j = 0; j < 600; j++) {
      ctx->zbuffer[j*800+i] = 0;
    }
  }
}

void Shader::vertex(RenderingContext *ctx, int idx, Vec3f position, Vec3f normal, Vec3f texture, Vec3f color)
{
  Mat44 mvp = ctx->modelview * ctx->projection;

  positions[idx] = position * mvp;
  normals[idx] = (normal * ctx->modelview).normalized(); // TODO: Calculate correct normal
  texture_coords[idx] = texture;
  colors[idx] = color;
}


bool Shader::fragment(RenderingContext *ctx, float t0, float t1, float t2, Vec3f *color)
{
  Vec3f normal = normals[0] * t0 + normals[1] * t1 + normals[2] * t2;
  float intensity = normal.dot(ctx->light);

  Vec3f vcolor = (Vec3f){colors[0].r * t0 + colors[1].r * t1 + colors[2].r * t2,
                         colors[0].g * t0 + colors[1].g * t1 + colors[2].g * t2,
                         colors[0].b * t0 + colors[1].b * t1 + colors[2].b * t2};

  Vec3f tex = texture_coords[0] * t0 + texture_coords[1] * t1 + texture_coords[2] * t2;
  int texX = tex.x * ctx->diffuse->width;
  int texY = tex.y * ctx->diffuse->height;

  Vec3f tcolor = ctx->diffuse->pixels[texY*ctx->diffuse->width+texX];
  *color = (Vec3f){vcolor.r * tcolor.r, vcolor.g * tcolor.g, vcolor.b * tcolor.b} * intensity;

  if (color->r < 0) { color->r = 0.0; }
  if (color->g < 0) { color->g = 0.0; }
  if (color->b < 0) { color->b = 0.0; }
        
  return true;
}

static void draw_triangle(RenderingContext *ctx, Shader *shader)
{
  Vec3f p0 = shader->positions[0] * ctx->viewport;
  Vec3f p1 = shader->positions[1] * ctx->viewport;
  Vec3f p2 = shader->positions[2] * ctx->viewport;

  int minx = p0.x;
  if (p1.x < minx) { minx = p1.x; }
  if (p2.x < minx) { minx = p2.x; }

  int miny = p0.y;
  if (p1.y < miny) { miny = p1.y; }
  if (p2.y < miny) { miny = p2.y; }  

  int maxx = p0.x;
  if (p1.x > maxx) { maxx = p1.x; }
  if (p2.x > maxx) { maxx = p2.x; }  

  int maxy = p0.y;
  if (p1.y > maxy) { maxy = p1.y; }
  if (p2.y > maxy) { maxy = p2.y; }


  int target_width = ctx->target->width;
  int target_height = ctx->target->height;

  // Clip bounding rect to buffer rect
  if (minx < 0) { minx = 0; }
  if (miny < 0) { miny = 0; }
  if (maxx > target_width - 1) { maxx = target_width - 1; }
  if (maxy > target_height - 1) { maxy = target_height - 1; }


  // As of now x and y are in canvas space and z is in normalized space,
  // thus we can't calculate the normal to the face
  //
  //Flat shading
  //Vec3f normal = ((p2 - p0).cross(p1 -//  p0)).normalized();
  // float intensity = normal.dot(light);

  // // Backface culling
  // Vec3f face_normal = ((p2 - p0).cross(p1 - p0)).normalized();
  // float face_visibility = face_normal.dot((Vec3f){0,0,-1});
  // if (face_visibility < 0.0) {
  //   return;
  // }

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

static bool initialized = false;

static TgaImage diffuseTexture;
static Model model;
static uint32_t zbuffer[800][600];
static RenderingContext rendering_context;
static Shader shader;

static void load_model(GlobalState *state, char *filename, Model *model)
{
  FileContents contents = state->platform_api.read_file_contents(filename);
  if (contents.size <= 0) {
    printf("Failed to load model\n");
    return;
  }

  printf("Read model file of %d bytes\n", contents.size);

  int vi = 0;
  int fi = 0;
  int ni = 0;
  int ti = 0;

  float minx = 0;
  float maxx = 0;
  float miny = 0;
  float maxy = 0;
  float minz = 0;
  float maxz = 0;  
  float accx = 0.0;
  float accy = 0.0;
  float accz = 0.0;

  char *p = (char *) contents.bytes;
  while(p < (char *) contents.bytes + contents.size) {

    float x = 0;
    float y = 0;
    float z = 0;

    if (*p == 'v') {
      p++;
      if (*p == ' ') {
        int consumed = 0;
        if (sscanf(p, "%f %f %f%n", &x, &y, &z, &consumed) != 3) {
          continue;
        }
        p += consumed;

        model->vertices[vi++] = (Vec3f){x, y, z};

        accx += x;
        accy += y;
        accz += z;

        if (x < minx) {
          minx = x;
        } else if (x > maxx) {
          maxx = x;
        }

        if (y < miny) {
          miny = y;
        } else if (y > maxy) {
          maxy = y;
        }        

        if (z < minz) {
          minz = z;
        } else if (z > maxz) {
          maxz = z;
        }        
      } else if (*p == 'n') {
        p++;
        int consumed = 0;
        if (sscanf(p, "%f %f %f%n", &x, &y, &z, &consumed) != 3) {
          continue;
        }
        p += consumed;

        model->normals[ni++] = (Vec3f){-x, -y, -z};
      } else if (*p == 't') {
        p++;
        int consumed = 0;
        if (sscanf(p, "%f %f %f%n", &x, &y, &z, &consumed) < 2) {
          continue;
        }
        p += consumed;

        model->texture_coords[ti++] = (Vec3f){x, y, z};
      }
    } else if (*p == 'f') {
      p++;

      int vi0, ti0, ni0, vi1, ti1, ni1, vi2, ti2, ni2;
      int consumed = 0;
      if (sscanf(p, "%d/%d/%d %d/%d/%d %d/%d/%d%n", &vi0, &ti0, &ni0, &vi1, &ti1, &ni1, &vi2, &ti2, &ni2, &consumed) != 9) {
        continue;
      }
      p += consumed;

      int *face = model->faces[fi++];
      face[0] = vi0 - 1;
      face[1] = ti0 - 1;
      face[2] = ni0 - 1;

      face[3] = vi1 - 1;
      face[4] = ti1 - 1;
      face[5] = ni1 - 1;

      face[6] = vi2 - 1;
      face[7] = ti2 - 1;
      face[8] = ni2 - 1;
    }

    p++;
  }

  model->vcount = vi;
  model->fcount = fi;
  model->ncount = ni;
  model->tcount = ti;
  printf("read %d vertices, %d texture coords, %d normals, %d faces\n", model->vcount, model->tcount, model->ncount, model->fcount);

  printf("Model bbox x in (%.2f .. %.2f); y in (%.2f .. %.2f); z in (%.2f .. %.2f)\n",
         minx, maxx, miny, maxy, minz, maxz);

  float dx = maxx - minx;
  float dy = maxy - miny;
  float dz = maxz - minz;
  float scale = 1.0 / dx;
  if (dy > dx) {
    scale = 1.0 / dy;
  }
  if ((dz > dy) && (dz > dx)) {
    scale = 1.0 / dz;
  }

#if 0
  float ox = accx / vi;
  float oy = accy / vi;
  float oz = accz / vi;
  printf("Model center offset x: %.3f, y: %.3f, z: %.3f\n", ox, oy, oz);
#else
  float ox, oy, oz = 0.0;
#endif

  for (int i = 0; i < model->vcount; i++) {
    model->vertices[i].x = model->vertices[i].x * scale - ox * scale;
    model->vertices[i].y = model->vertices[i].y * scale - oy * scale;
    model->vertices[i].z = model->vertices[i].z * scale - oz * scale;
  }  
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

static void initialize(GlobalState *state)
{
  initialized = true;

  // TODO: Allocate dynamically
  rendering_context.model = &model;
  rendering_context.diffuse = &diffuseTexture;
  rendering_context.zbuffer = (uint32_t *) &zbuffer;

  load_model(state, (char *) "data/rabbit/rabbit.obj", rendering_context.model);
  load_texture(state, (char *) "data/rabbit/diffuse.tga", rendering_context.diffuse);
}

#define WHITE (Vec3f){1, 1, 1}
#define BLACK (Vec3f){0, 0, 0}

static void clear_buffer(DrawingBuffer *buffer, Vec3f color)
{
  for (int j = 0; j <= buffer->height; j++) {
    for (int i = 0; i <= buffer->width; i++) {
      set_pixel(buffer, i, j, color);
    }
  }  
}

static float angle = 0;
static float hue = 0.0;

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

static void render_triangle(DrawingBuffer *buffer)
{
  Vec3f p0 = {-0.866, -0.5, 0};
  Vec3f p1 = {0.866, -0.5, 0};
  Vec3f p2 = {0, 1, 0};

  Mat44 mat_trans = Mat44::translate(0, 0, -1);
  Mat44 mat_rot = Mat44::rotate_y(angle);
  Mat44 view_mat = viewport_matrix(buffer->width, buffer->height);
  Mat44 proj_mat = projection_matrix(0.1, 10, 90);

  p0 = p0 * mat_rot * mat_trans * proj_mat * view_mat;
  p1 = p1 * mat_rot * mat_trans * proj_mat * view_mat;
  p2 = p2 * mat_rot * mat_trans * proj_mat * view_mat;

  Vec3f red = {1, 0, 0};
  Vec3f green = {0, 1, 0};
  Vec3f blue = {0, 0, 1};
  Vec3f white = {1, 1, 1};

  Vertex v0 = {p0, white, {0, 0, 1}, {0, 0, 0}};
  Vertex v1 = {p1, white, {0, 0, 1}, {1, 0, 0}};
  Vertex v2 = {p2, white, {0, 0, 1}, {0, 1, 0}};
  v0.normal = v0.normal * mat_rot;
  v1.normal = v1.normal * mat_rot;
  v2.normal = v2.normal * mat_rot;

  Vec3f light = {0, 0, 1};

  clear_zbuffer(&rendering_context);
  draw_triangle(&rendering_context, &shader);

  angle += 0.005;
  if (angle > 2*PI) {
    angle -= 2*PI;
  }  
}

static void render_model(DrawingBuffer *buffer, bool wireframe = false)
{
  angle += 0.005;
  if (angle > 2*PI) {
    angle -= 2*PI;
  }

  // hue += 0.5;
  // if (hue > 360.0) {
  //   hue -= 360.0;
  // }

  // float r, g, b;
  // hsv_to_rgb(hue, 1.0, 1.0, &r, &g, &b);
  Vec3f color = {1, 1, 1}; //{r, g, b};

  Mat44 cam_mat = Mat44::translate(0, 0, 1) * Mat44::rotate_y(angle);
  Mat44 model_mat = Mat44::translate(0, -0.4, 0);

  rendering_context.modelview = model_mat * cam_mat.inverse();
  rendering_context.projection = projection_matrix(0.1, 10, 90);
  rendering_context.viewport = viewport_matrix(buffer->width, buffer->height);
  rendering_context.light = ((Vec3f){0, -1, 0}).normalized();

  clear_zbuffer(&rendering_context);

  for (int fi = 0; fi < model.fcount; fi++) {
    int *face = model.faces[fi];

    for (int vi = 0; vi < 3; vi++) {
      Vec3f position = model.vertices[face[vi*3]];
      Vec3f texture = model.texture_coords[face[vi*3+1]];
      Vec3f normal = model.normals[face[vi*3+2]];

      shader.vertex(&rendering_context, vi, position, normal, texture, color);
    }

    draw_triangle(&rendering_context, &shader);
  }  
}

C_LINKAGE void draw_frame(GlobalState *state, DrawingBuffer *drawing_buffer)
{
  if (!initialized) {
    initialize(state);
    rendering_context.target = drawing_buffer;
  }

  clear_buffer(drawing_buffer, BLACK);
  //render_triangle(drawing_buffer);
  render_model(drawing_buffer, false);
}
