#include <ctime>

#include "platform/platform.h"

#include "utils/math.cpp"
#include "utils/tga.cpp"
#include "utils/texture.cpp"
#include "utils/memory.cpp"
#include "utils/assets.cpp"

#include "renderer/renderer.cpp"
#include "renderer/font.cpp"

#ifndef CUBES_GRID_SIZE
#define CUBES_GRID_SIZE 6
#endif

#define CUBES_TEXTURE_DITHERING 0
#define CUBES_CORRECT_PERSPECTIVE 1
#define CUBES_DEBUG_GRID 0

typedef struct Vertex {
  Vec3f position;
  Vec3f texture_coords;
} Vertex;

typedef struct State {
  PlatformAPI *platform_api;
  KeyboardState *keyboard;
  MemoryArena *main_arena;
  MemoryArena *cubes_arena;

  uint32_t verticesCount;
  Vertex *vertices;

  RenderingContext rendering_context;
  Font font;
  Texture *texture;

  uint32_t seed;
  float xRot;
  float yRot;
  float fov;
  float screen_width;
  float screen_height;
} State;

typedef struct ShaderData {
  Vec3f pos[3];
  Vec3f uvs[3];
  Vec3f uv0;
  Vec3f duv[2];
  Texture *texture;
} ShaderData;

FRAGMENT_FUNC(fragment)
{
  ShaderData *d = (ShaderData *) shader_data;
  Texture *texture = d->texture;

#if CUBES_CORRECT_PERSPECTIVE
  float z = 1.0f / (d->uv0.z + t1 * d->duv[0].z + t2 * d->duv[1].z);
  float fu = (d->uv0.x + t1 * d->duv[0].x + t2 * d->duv[1].x) * z;
  float fv = (d->uv0.y + t1 * d->duv[0].y + t2 * d->duv[1].y) * z;
#else
  float fu = d->uv0.x + t1 * d->duv[0].x + t2 * d->duv[1].x;
  float fv = d->uv0.y + t1 * d->duv[0].y + t2 * d->duv[1].y;
#endif

#if CUBES_TEXTURE_DITHERING
  int ab = (x & 1) | ((y & 1) << 1);
  switch (ab) {
    case 0: fu -= 0.25f; fv -= 0.25f; break;
    case 1: fu += 0.25f; fv -= 0.25f; break;
    case 2: fu -= 0.25f; fv += 0.25f; break;
    case 3: fu += 0.25f; fv += 0.25f; break;
  }
#endif

  int u = ((int) fu) & (128 - 1);
  int v = ((int) fv) & (16 - 1);

  Vec3f tcolor = TEXEL3F(texture, u, v);
  *color = {tcolor.r, tcolor.g, tcolor.b, 1.0f};

  return true;
 }

static Texture *load_texture(State *state, char *filename)
{
  LoadedFile file = load_file(state->platform_api, state->main_arena, filename);
  if (!file.size) {
    printf("Failed to load texture: %s\n", filename);
    return NULL;
  }

  TgaImage image;
  image.read_header(file.contents, file.size);

  TgaHeader *header = &image.header;
  printf("TGA Image width: %d; height: %d; type: %d; bpp: %d\n",
         header->width, header->height, header->imageType, header->pixelDepth);

  printf("X offset: %d; Y offset: %d; FlipX: %d; FlipY: %d\n",
         header->xOffset, header->yOffset, image.flipX, image.flipY);
  
  Texture *texture = texture_create(state->main_arena, header->width, header->height);
  image.read_into_texture(file.contents, file.size, texture);
  return texture;
}

static void initialize(State *state, DrawingBuffer *buffer)
{
  RenderingContext *ctx = &state->rendering_context;
  set_target(ctx, buffer);

  ctx->model_mat = Mat44::identity();
  ctx->projection_mat = perspective_matrix(0.1f, 10.0f, 60.0f);
  ctx->light = { 0.0f, 0.0f, 0.0f };

  ctx->zbuffer = (zval_t *) state->main_arena->allocate(buffer->width * buffer->height * sizeof(zval_t));

  state->texture = load_texture(state, (char *) "data/cubes.tga");

  state->font.texture = load_texture(state, (char *) "data/font.tga");
  state->font.spec.char_width = 19;
  state->font.spec.char_height = 31;
  state->font.spec.chars_per_line = 13;

  state->xRot = 0.0f;
  state->yRot = 0.0f;
  state->fov = 60.0f;
  state->screen_width = (float) buffer->width;
  state->screen_height = (float) buffer->height;
}

static void write_cube_vertices(Vertex *vertices, Vec3f offset, int textureIdx)
{
  ASSERT(textureIdx >= 0 && textureIdx < 11);

  const static Vec3f texture_coords[] = {
    {0, 0, 0},
    {1, 0, 0},
    {1, 1, 0},
    {0, 1, 0}
  };

  const static Vec3f positions[] = {
    {-0.5, -0.5, 0.5},
    {0.5, -0.5, 0.5},
    {0.5, -0.5, -0.5},
    {-0.5, -0.5, -0.5},

    {-0.5, 0.5, 0.5},
    {0.5, 0.5, 0.5},
    {0.5, 0.5, -0.5},
    {-0.5, 0.5, -0.5},
  };

  const static Vec3f normal = {0, 0, 0};

  const static int indices[36][2] = {
    // Bottom
    {3, 0}, {2, 1}, {1, 2},
    {3, 0}, {1, 2}, {0, 3},
    
    // Front
    {0, 0}, {1, 1}, {5, 2},
    {0, 0}, {5, 2}, {4, 3},

    // Top
    {4, 0}, {5, 1}, {6, 2},
    {4, 0}, {6, 2}, {7, 3},

    // Back
    {2, 0}, {3, 1}, {7, 2},
    {2, 0}, {7, 2}, {6, 3},

    // Left
    {3, 0}, {0, 1}, {4, 2},
    {3, 0}, {4, 2}, {7, 3},

    // Right
    {1, 0}, {2, 1}, {6, 2},
    {1, 0}, {6, 2}, {5, 3}
  };

  float u0 = textureIdx * 16.0f;

  for (int vi = 0; vi < 36; vi++) {
    Vec3f tex = texture_coords[indices[vi][1]];
    tex.x = u0 + tex.x * 16.0f;
    tex.y = tex.y * 16.0f;

    vertices[vi].position = positions[indices[vi][0]] + offset;
    vertices[vi].texture_coords = tex;
  }
}

#define RAND(min, max) ((((float) rand(NULL) / 0xFFFFFFFF) * (max - min)) + min)
static uint32_t rand(uint32_t *newseed)
{
  static int seed = 0;
  if (newseed) {
    seed = *newseed;
  }

  seed = (1103515245 * seed + 12345) % 0xFFFFFFFF;
  return seed;
}

static void regenerate_cube_grid(State *state, uint8_t side)
{
  rand(&state->seed);

  size_t verticesCount = side*side*side * 36;
  size_t requiredMemory = verticesCount * sizeof(Vertex);

  ASSERT(state->cubes_arena->total_size >= requiredMemory);

  if (state->cubes_arena->total_size < requiredMemory) {
    state->verticesCount = 0;
    return;
  }

  state->cubes_arena->taken = 0;

  state->verticesCount = verticesCount;
  state->vertices = (Vertex *) state->cubes_arena->allocate(requiredMemory);
  Vertex *vertices = state->vertices;

  for (int i = 0; i < side; i++) {
    for (int j = 0; j < side; j++) {
      for (int k = 0; k < side; k++) {
        uint8_t textureIdx = (uint8_t) RAND(0.0f, 10.0f);

        float a = 0.75f;
        float xoffset = 2 * i * a + 1 * a - side * a;
        float yoffset = 2 * j * a + 1 * a - side * a;
        float zoffset = 2 * k * a + 1 * a - side * a;
        Vec3f offset = {xoffset, yoffset, zoffset};

        write_cube_vertices(vertices, offset, textureIdx);
        vertices += 36;
      }
    }
  }
}

static void render_text(State *state, RenderingContext *ctx)
{
  ctx->viewport_mat = viewport_matrix(state->screen_width, state->screen_height, false);
  ctx->projection_mat = orthographic_matrix(0.1f, 100.0f, 0.0f, state->screen_height, state->screen_width, 0.0f);
  ctx->view_mat = Mat44::identity();
  ctx->model_mat = Mat44::translate(0.0f, 0.0f, 0.0f);
  precalculate_matrices(ctx);

  state->font.render_string(ctx, 10.0f, 10.0f, (char *) "Welcome to the wonderful world of software rendering :)");
}

static void render_cubes(State *state, RenderingContext *ctx)
{
  ctx->viewport_mat = viewport_matrix(state->screen_width, state->screen_height, true);
  ctx->projection_mat = perspective_matrix(0.1f, 1000.0f, state->fov);

  Mat44 view_mat = Mat44::translate(0.0f, 0.0f, -15.0f);
  ctx->view_mat = Mat44::rotate_y(-state->yRot) *
                  Mat44::rotate_x(-state->xRot) * view_mat;

  ctx->model_mat = Mat44::translate(0, 0, 0);
  const static Vec3f normal = {0, 0, 0};

  precalculate_matrices(ctx);

  Vertex vertices[3];
  Vec3f positions[3] = {};
  ShaderData data = {};
  data.texture = state->texture;

  for (int i = 0; i < state->verticesCount; i++) {
    int idx = i % 3;
    vertices[idx] = state->vertices[i];
    
#if CUBES_CORRECT_PERSPECTIVE
    Vec3f cam_pos = vertices[idx].position * ctx->modelview_mat;
    positions[idx] = cam_pos * ctx->projection_mat;
    float iz = 1.0f / cam_pos.z;
    data.uvs[idx] = {vertices[idx].texture_coords.x * iz, vertices[idx].texture_coords.y * iz, iz};
#else
    data.uvs[idx] = {vertices[idx].texture_coords.x, vertices[idx].texture_coords.y, 0.0f};
    positions[idx] = vertices[idx].position * ctx->mvp_mat;
#endif

    if (idx == 2) {
      #define VEC3_DOT_PLANE(vec3, p) (vec3.x*p.x + vec3.y*p.y + vec3.z*p.z + p.w)
      if ((VEC3_DOT_PLANE(vertices[0].position, ctx->near_clip_plane) < 0.0f) ||
          (VEC3_DOT_PLANE(vertices[1].position, ctx->near_clip_plane) < 0.0f) ||
          (VEC3_DOT_PLANE(vertices[2].position, ctx->near_clip_plane) < 0.0f)) {
        continue;
      }
      #undef VEC3_DOT_PLANE

      data.uv0 = data.uvs[0];
      data.duv[0] = data.uvs[1] - data.uvs[0];
      data.duv[1] = data.uvs[2] - data.uvs[0];

      ctx->draw_triangle(ctx, &fragment, (void *) &data, positions[0], positions[1], positions[2]);
    }
  }
}

#define Y_RAD_PER_SEC RAD(120.0f)
#define X_RAD_PER_SEC RAD(120.0f)
#define FOV_DEG_PER_SEC 30.0f

static void update_camera(State *state, float dt)
{
  float da = 0.0f;
  if (KEY_IS_DOWN(state->keyboard, KB_LEFT_ARROW)) {
    da -= Y_RAD_PER_SEC * dt;
  }
  if (KEY_IS_DOWN(state->keyboard, KB_RIGHT_ARROW)) {
    da += Y_RAD_PER_SEC * dt;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_LEFT_SHIFT)) {
    da *= 3.0f;
  }

  state->yRot += da;
  if (state->yRot > 2*PI) {
    state->yRot -= 2*PI;
  }

  da = 0.0;
  if (KEY_IS_DOWN(state->keyboard, KB_UP_ARROW)) {
    da += X_RAD_PER_SEC * dt;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_DOWN_ARROW)) {
    da -= X_RAD_PER_SEC * dt;
  }

  state->xRot += da;
  state->xRot = CLAMP(state->xRot, -PI / 2.0f, PI / 2.0f);

  da = 0.0;
  if (KEY_IS_DOWN(state->keyboard, KB_PLUS)) {
    da -= FOV_DEG_PER_SEC * dt;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_MINUS)) {
    da += FOV_DEG_PER_SEC * dt;
  }

  state->fov += da;
  state->fov = CLAMP(state->fov, 3.0f, 170.0f);
}

static inline void clear_buffer(DrawingBuffer *buffer, Vec4f color)
{
  uint32_t iterCount = (buffer->width * buffer->height) / 4;

  __m128i value = _mm_set1_epi32(rgba_color(color));
  __m128i *p = (__m128i *) buffer->pixels;

  while (iterCount--) {
    *p++ = value;
  }
}

C_LINKAGE EXPORT void draw_frame(GlobalState *global_state, DrawingBuffer *drawing_buffer, float dt)
{
  State *state = (State *) global_state->state;

  if (!state) {
    MemoryArena *arena = MemoryArena::initialize(global_state->platform_api.allocate_memory(MB(64)), MB(64));

    state = (State *) arena->allocate(MB(1));
    global_state->state = state;

    state->main_arena = arena;
    state->cubes_arena = arena->subarena(MB(8));

    state->platform_api = &global_state->platform_api;
    state->keyboard = global_state->keyboard;

    state->seed = (uint32_t) time(NULL);

    initialize(state, drawing_buffer);
    regenerate_cube_grid(state, CUBES_GRID_SIZE);
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_ESCAPE)) {
    state->platform_api->terminate();
    return;
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_W)) {
    state->seed = (uint32_t) time(NULL) ^ (uint32_t) (dt * (state->seed));
    regenerate_cube_grid(state, CUBES_GRID_SIZE);
  }

  RenderingContext *ctx = &state->rendering_context;

  update_camera(state, dt);

  clear_buffer(drawing_buffer, {0.0f, 0.0f, 0.0f, 0.0f});
  clear_zbuffer(ctx);

  render_cubes(state, ctx);
  render_text(state, ctx);

#if CUBES_DEBUG_GRID
  for (int j = 0; j < ctx->target->height; j++) {
    for (int i = 0; i < ctx->target->width; i++) {
      if ((i % 8 == 0) && (j % 8 == 0)) {
        set_pixel(ctx->target, i, j, (Vec3f){0, 1, 0});
      }
    }
  }
#endif
}
