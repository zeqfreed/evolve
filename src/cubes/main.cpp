#include <ctime>

#include "platform/platform.h"

#include "renderer/memory.cpp"
#include "renderer/math.cpp"
#include "renderer/tga.cpp"
#include "renderer/renderer.cpp"

#ifndef CUBES_GRID_SIZE
#define CUBES_GRID_SIZE 6
#endif

#define CUBES_CORRECT_PERSPECTIVE 1

typedef struct Vertex {
  Vec3f position;
  Vec3f texture_coords;
} Vertex;

typedef struct State {
  PlatformAPI *platform_api;
  KeyboardState *keyboard;
  MemoryArena *main_arena;
  MemoryArena *cubes_arena;

  uint16_t verticesCount;
  Vertex *vertices;

  RenderingContext rendering_context;
  IShader *shader;

  uint32_t seed;
  float xRot;
  float yRot;
  float fov;
} State;

struct CubeShader : public IShader {
  Vec3f pos[3];
  Vec3f uvs[3];
  Vec3f uv0;
  Vec3f duv[2];

  void vertex(RenderingContext *ctx, int idx, Vec3f position, Vec3f normal, Vec3f texture, Vec3f color)
  {
    positions[idx] = position * ctx->mvp_mat;

#ifdef CUBES_CORRECT_PERSPECTIVE
    Vec3f pos = position * ctx->modelview_mat;
    float iz = 1 / pos.z;
    uvs[idx] = (Vec3f){texture.x * iz, texture.y * iz, iz};
#else
    uvs[idx] = (Vec3f){texture.x, texture.y, 0};
#endif

    if (idx == 2) {
      uv0 = uvs[0];
      duv[0] = uvs[1] - uvs[0];
      duv[1] = uvs[2] - uvs[0];
    }
  }

  bool fragment(RenderingContext *ctx, float t0, float t1, float t2, Vec3f *color)
  {
#ifdef CUBES_CORRECT_PERSPECTIVE
    float z = 1 / (uv0.z + t1 * duv[0].z + t2 * duv[1].z);
    int u = (uv0.x + t1 * duv[0].x + t2 * duv[1].x) * z;
    int v = (uv0.y + t1 * duv[0].y + t2 * duv[1].y) * z;
#else
    int u = uv0.x + t1 * duv[0].x + t2 * duv[1].x;
    int v = uv0.y + t1 * duv[0].y + t2 * duv[1].y;
#endif

    Vec3f tcolor = ctx->diffuse->pixels[v * ctx->diffuse->width + u];
    *color = (Vec3f){tcolor.r, tcolor.g, tcolor.b};

    return true;
  }
};

static Texture *load_texture(State *state, char *filename)
{
  FileContents contents = state->platform_api->read_file_contents(filename);
  if (contents.size <= 0) {
    printf("Failed to load texture\n");
    return NULL;
  }

  printf("Read texture file of %d bytes\n", contents.size);

  TgaImage image;
  image.read_header(contents.bytes, contents.size);

  TgaHeader *header = &image.header;
  printf("TGA Image width: %d; height: %d; type: %d; bpp: %d\n",
         header->width, header->height, header->imageType, header->pixelDepth);

  printf("X offset: %d; Y offset: %d; FlipX: %d; FlipY: %d\n",
         header->xOffset, header->yOffset, image.flipX, image.flipY);
  
  Texture *texture = (Texture *) state->main_arena->allocate(sizeof(Texture));
  texture->width = header->width;
  texture->height = header->height;
  texture->pixels = (Vec3f *) state->main_arena->allocate(sizeof(Vec3f) * texture->width * texture->height);

  image.read_into_texture(contents.bytes, contents.size, texture);
  return texture;
}

static void initialize(State *state, DrawingBuffer *buffer)
{
  RenderingContext *ctx = &state->rendering_context;
  ctx->target = buffer;
  ctx->clear_color = BLACK;

  ctx->model_mat = Mat44::identity();
  ctx->viewport_mat = viewport_matrix(buffer->width, buffer->height);
  ctx->projection_mat = perspective_matrix(0.1, 10, 60);
  ctx->light = (Vec3f){0, 0, 0};

  ctx->zbuffer = (zval_t *) state->main_arena->allocate(buffer->width * buffer->height * sizeof(zval_t));

  // TODO: DON'T ALLOCATE IN DYLIB !!!
  state->shader = new CubeShader();

  ctx->diffuse = load_texture(state, (char *) "data/cubes.tga");

  state->xRot = 0.0;
  state->yRot = 0.0;
  state->fov = 60.0;
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

  float u0 = textureIdx * 16;

  for (int vi = 0; vi < 36; vi++) {
    Vec3f tex = texture_coords[indices[vi][1]];
    tex.x = u0 + tex.x * 16;
    tex.y = tex.y * 16;

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
        int textureIdx = RAND(0, 10);

        float a = 0.75;
        float xoffset = 2 * i * a + 1 * a - side * a;
        float yoffset = 2 * j * a + 1 * a - side * a;
        float zoffset = 2 * k * a + 1 * a - side * a;
        Vec3f offset = (Vec3f){xoffset, yoffset, zoffset};

        write_cube_vertices(vertices, offset, textureIdx);
        vertices += 36;
      }
    }
  }
}

static void render_cubes(State *state, RenderingContext *ctx)
{
  ctx->model_mat = Mat44::translate(0, 0, 0);
  const static Vec3f normal = {0, 0, 0};

  precalculate_matrices(ctx);

  Vertex vertices[3];

  for (int i = 0; i < state->verticesCount; i++) {
    int idx = i % 3;
    vertices[idx] = state->vertices[i];

    if (idx == 2) {
      #define VEC3_DOT_PLANE(vec3, p) (vec3.x*p.x + vec3.y*p.y + vec3.z*p.z + p.w)
      if ((VEC3_DOT_PLANE(vertices[0].position, ctx->near_clip_plane) < 0) ||
          (VEC3_DOT_PLANE(vertices[1].position, ctx->near_clip_plane) < 0) ||
          (VEC3_DOT_PLANE(vertices[2].position, ctx->near_clip_plane) < 0)) {
        continue;
      }
      #undef VEC3_DOT_PLANE

      state->shader->vertex(ctx, 0, vertices[0].position, normal, vertices[0].texture_coords, WHITE);
      state->shader->vertex(ctx, 1, vertices[1].position, normal, vertices[1].texture_coords, WHITE);
      state->shader->vertex(ctx, 2, vertices[2].position, normal, vertices[2].texture_coords, WHITE);
      draw_triangle(ctx, state->shader);
    }
  }
}

#define Y_RAD_PER_SEC RAD(120.0)
#define X_RAD_PER_SEC RAD(120.0)
#define FOV_DEG_PER_SEC 30.0

static void update_camera(State *state, float dt)
{
  float da = 0.0;
  if (state->keyboard->downedKeys[KB_LEFT_ARROW]) {
    da -= Y_RAD_PER_SEC * dt;
  }
  if (state->keyboard->downedKeys[KB_RIGHT_ARROW]) {
    da += Y_RAD_PER_SEC * dt;
  }

  if (state->keyboard->downedKeys[KB_LEFT_SHIFT]) {
    da *= 3.0;
  }

  state->yRot += da;
  if (state->yRot > 2*PI) {
    state->yRot -= 2*PI;
  }

  da = 0.0;
  if (state->keyboard->downedKeys[KB_UP_ARROW]) {
    da += X_RAD_PER_SEC * dt;
  }

  if (state->keyboard->downedKeys[KB_DOWN_ARROW]) {
    da -= X_RAD_PER_SEC * dt;
  }

  state->xRot += da;
  state->xRot = CLAMP(state->xRot, -PI / 2.0, PI / 2.0);

  da = 0.0;
  if (state->keyboard->downedKeys[KB_PLUS]) {
    da -= FOV_DEG_PER_SEC * dt;
  }

  if (state->keyboard->downedKeys[KB_MINUS]) {
    da += FOV_DEG_PER_SEC * dt;
  }

  state->fov += da;
  state->fov = CLAMP(state->fov, 3.0, 170.0);
}

C_LINKAGE void draw_frame(GlobalState *global_state, DrawingBuffer *drawing_buffer, float dt)
{
  State *state = (State *) global_state->state;

  if (!state) {
    MemoryArena *arena = MemoryArena::initialize(global_state->platform_api.allocate_memory(MB(16)), MB(16));

    state = (State *) arena->allocate(MB(1));
    global_state->state = state;

    state->main_arena = arena;
    state->cubes_arena = arena->subarena(MB(8));

    state->platform_api = &global_state->platform_api;
    state->keyboard = global_state->keyboard;

    state->seed = time(NULL);

    initialize(state, drawing_buffer);
    regenerate_cube_grid(state, CUBES_GRID_SIZE);
  }

  if (state->keyboard->downedKeys[KB_ESCAPE]) {
    state->platform_api->terminate();
    return;
  }

  if (state->keyboard->downedKeys[KB_W]) {
    state->seed = time(NULL) ^ (uint32_t) (dt * (state->seed));
    regenerate_cube_grid(state, CUBES_GRID_SIZE);
  }

  RenderingContext *ctx = &state->rendering_context;

  update_camera(state, dt);
  ctx->projection_mat = perspective_matrix(0.1, 1000, state->fov);

  Mat44 view_mat = Mat44::translate(0, 0, -15);
  ctx->view_mat = Mat44::rotate_y(-state->yRot) *
                  Mat44::rotate_x(-state->xRot) * view_mat;

  clear_buffer(ctx);
  clear_zbuffer(ctx);

  render_cubes(state, ctx);
}
