#include "platform/platform.h"

#include "renderer/memory.cpp"
#include "renderer/math.cpp"
#include "renderer/tga.cpp"
#include "renderer/renderer.cpp"

#include "model.cpp"

typedef struct State {
  PlatformAPI *platform_api;
  KeyboardState *keyboard;
  MemoryArena *main_arena;

  Model *model;

  RenderingContext rendering_context;
  IShader *modelShader;
  IShader *floorShader;

  float xRot;
  float yRot;
  float fov;
} State;

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

struct ModelShader : public IShader {
  Vec3f normals[3];
  Vec3f uvs[3];
  Vec3f pos[3];

  void vertex(RenderingContext *ctx, int idx, Vec3f position, Vec3f normal, Vec3f texture, Vec3f color)
  {
    pos[idx] = position;
    positions[idx] = position * ctx->modelview_mat * ctx->projection_mat;
    uvs[idx] = (Vec3f){texture.x, texture.y, 0};
    normals[idx] = (normal * ctx->normal_mat).normalized();
  }

  bool fragment(RenderingContext *ctx, float t0, float t1, float t2, Vec3f *color)
  {
    float intensity = 0.0;
    //Vec3f normal = (pos[2] - pos[1]).cross(pos[1] - pos[0]).normalized();
    Vec3f normal = normals[0] * t0 + normals[1] * t1 + normals[2] * t2;

    Vec3f uv = uvs[0] * t0 + uvs[1] * t1 + uvs[2] * t2;
    int texX = uv.x * ctx->diffuse->width;
    int texY = uv.y * ctx->diffuse->height;

    Vec3f tcolor = ctx->diffuse->pixels[texY*ctx->diffuse->width+texX];
    *color = (Vec3f){tcolor.r, tcolor.g, tcolor.b};

    if (1 && ctx->normal) {
      int nx = uv.x * ctx->normal->width;
      int ny = uv.y * ctx->normal->height;
      Vec3f ncolor = ctx->normal->pixels[ny*ctx->normal->width+nx];
      Vec3f tnormal = (Vec3f){2 * ncolor.r - 1, 2 * ncolor.g - 1, ncolor.b};

      Vec3f dp1 = pos[1] - pos[0];
      Vec3f dp2 = pos[2] - pos[1];
      Vec3f duv1 = uvs[1] - uvs[0];
      Vec3f duv2 = uvs[2] - uvs[1];
      float r = 1.0 / (duv1.x * duv2.y - duv1.y * duv2.x);
      Vec3f tangent = -(dp1 * duv2.y - dp2 * duv1.y) * r;
      tangent = (tangent - normal * normal.dot(tangent)).normalized();
      Vec3f bitangent = -((dp2 * duv1.x - dp1 * duv2.x) * r).normalized();

      Mat44 invTBN = {
        tangent.x, tangent.y, tangent.z, 0,
        bitangent.x, bitangent.y, bitangent.z, 0,
        normal.x, normal.y, normal.z, 0,
        0, 0, 0, 1
      };
      normal = (tnormal * invTBN).normalized();
    }

    Vec3f ambient = tcolor * 0.3;
    intensity = normal.dot(-ctx->light);
    if (intensity < 0.0) {
      intensity = 0.0;
    }

    *color = (ambient + tcolor * intensity).clamped();

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
    float intensity = 1.0; //normal.dot(ctx->light);

    Vec3f uvz = uvzs[0] * t0 + uvzs[1] * t1 + uvzs[2] * t2;
    float z = 1 / uvz.z;
    int texX = uvz.x * z * 5;
    int texY = uvz.y * z * 5;

    Vec3f tcolor;
    if (texX % 2 == texY % 2) {
      tcolor = (Vec3f){1, 1, 1};
    } else {
      tcolor = (Vec3f){0.5, 0.5, 0.5};
    }

    Vec3f vcolor = (Vec3f){colors[0].r * t0 + colors[1].r * t1 + colors[2].r * t2,
                           colors[0].g * t0 + colors[1].g * t1 + colors[2].g * t2,
                           colors[0].b * t0 + colors[1].b * t1 + colors[2].b * t2} * z;

    Vec3f pos = positions[0] * t0 + positions[1] * t1 + positions[2] * t2;
    Vec3f shadow = pos * ctx->shadow_mat;
    int shx = shadow.x;
    int shy = shadow.y;

    if (shadow.x >= 0 && shadow.y >= 0 &&
        shadow.x < ctx->shadowmap->width && shadow.y < ctx->shadowmap->height) {
      float shz = 1 - shadow.z;
      Vec3f shval = ctx->shadowmap->pixels[shy * ctx->shadowmap->width + shx];

      if (shz < shval.x) {
        intensity = 0.2;
      }
    }

    *color = ((Vec3f){vcolor.r * tcolor.r, vcolor.g * tcolor.g, vcolor.b * tcolor.b}).clamped() * intensity;

    return true;
  }
};

static Model *load_model(State *state, char *filename)
{
  FileContents contents = state->platform_api->read_file_contents(filename);
  if (contents.size <= 0) {
    printf("Failed to load model\n");
    return NULL;
  }

  printf("Read model file of %d bytes\n", contents.size);
  Model *model = (Model *) state->main_arena->allocate(sizeof(Model));

  model->parse(contents.bytes, contents.size, true);

  printf("Vertices: %d; Texture coords: %d; Normals: %d; Faces: %d\n",
         model->vcount, model->tcount, model->ncount, model->fcount);

  printf("Model bbox x in (%.2f .. %.2f); y in (%.2f .. %.2f); z in (%.2f .. %.2f)\n",
         model->min.x, model->max.x, model->min.y, model->max.y, model->min.z, model->max.z);

  model->vertices = (Vec3f *) state->main_arena->allocate(sizeof(Vec3f) * model->vcount);
  model->normals = (Vec3f *) state->main_arena->allocate(sizeof(Vec3f) * model->ncount);
  model->texture_coords = (Vec3f *) state->main_arena->allocate(sizeof(Vec3f) * model->tcount);
  model->faces = (ModelFace *) state->main_arena->allocate(sizeof(ModelFace) * model->fcount);

  model->parse(contents.bytes, contents.size);

  return model;
}

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

static void render_floor(State *state, RenderingContext *ctx)
{
  ctx->model_mat = Mat44::translate(0, 0, 0);

  Vec3f vertices[4][2] = {
    {{-0.5, 0, 0.5}, {0, 0, 0}},
    {{0.5, 0, 0.5}, {1, 0, 0}},
    {{0.5, 0, -0.5}, {1, 1, 0}},
    {{-0.5, 0, -0.5}, {0, 1, 0}}
  };
  Vec3f normal = {0, 1, 0};

  precalculate_matrices(ctx);

  state->floorShader->vertex(ctx, 0, vertices[0][0], normal, vertices[0][1], RED);
  state->floorShader->vertex(ctx, 1, vertices[1][0], normal, vertices[1][1], GREEN);
  state->floorShader->vertex(ctx, 2, vertices[2][0], normal, vertices[2][1], BLUE);
  draw_triangle(ctx, state->floorShader);

  state->floorShader->vertex(ctx, 0, vertices[0][0], normal, vertices[0][1], RED);
  state->floorShader->vertex(ctx, 1, vertices[2][0], normal, vertices[2][1], BLUE);
  state->floorShader->vertex(ctx, 2, vertices[3][0], normal, vertices[3][1], WHITE);
  draw_triangle(ctx, state->floorShader);
}

static void render_model(State *state, RenderingContext *ctx, Model *model, bool wireframe = false)
{
  Vec3f color = {1, 1, 1};

  ctx->model_mat = Mat44::translate(0, 0, 0);

  precalculate_matrices(ctx);

  for (int fi = 0; fi < model->fcount; fi++) {
    ModelFace face = model->faces[fi];

    for (int vi = 0; vi < 3; vi++) {
      Vec3f position = model->vertices[face.vi[vi]];
      Vec3f texture = model->texture_coords[face.ti[vi]];
      Vec3f normal = model->normals[face.ni[vi]];

      state->modelShader->vertex(ctx, vi, position, normal, texture, color);
    }

    draw_triangle(ctx, state->modelShader);
  }
}

static inline void render_line(RenderingContext *ctx, Vec3f start, Vec3f end, Vec3f color)
{
  float d0 = (Vec4f){start.x, start.y, start.z, 1}.dot(ctx->near_clip_plane);
  float d1 = (Vec4f){end.x, end.y, end.z, 1}.dot(ctx->near_clip_plane);

  if (d0 < 0 && d1 < 0) {
    return;
  }
  
  float c;
  if (d1 < 0) {
    c = d0 / (d0 - d1);
    end = start + (end - start) * c;
  } else if (d0 < 0) {
    c = d0 / (d1 - d0);
    start = start + (start - end) * c;
  }

  start = start * ctx->mvp_mat * ctx->viewport_mat;
  end = end * ctx->mvp_mat * ctx->viewport_mat;

  draw_line(ctx->target, start.x, start.y, end.x, end.y, color);
}

static void render_unit_axes(RenderingContext *ctx)
{
  ctx->model_mat = Mat44::identity();
  precalculate_matrices(ctx);

  Vec3f origin = {0, 0, 0};
  render_line(ctx, origin, (Vec3f){1, 0, 0}, RED);
  render_line(ctx, origin, (Vec3f){0, 1, 0}, GREEN);
  render_line(ctx, origin, (Vec3f){0, 0, 1}, BLUE);
  render_line(ctx, origin, ctx->light, WHITE);
}

static void initialize(State *state, DrawingBuffer *buffer)
{
  RenderingContext *ctx = &state->rendering_context;
  ctx->target = buffer;
  ctx->clear_color = BLACK;

  ctx->model_mat = Mat44::identity();
  ctx->viewport_mat = viewport_matrix(buffer->width, buffer->height);
  ctx->projection_mat = perspective_matrix(0.1, 10, 60);
  ctx->light = ((Vec3f){0, -1, 0}).normalized();

  ctx->zbuffer = (zval_t *) state->main_arena->allocate(buffer->width * buffer->height * sizeof(zval_t));

  // TODO: DON'T ALLOCATE IN DYLIB !!!
  state->floorShader = new FloorShader();
  state->modelShader = new ModelShader();

  char *basename = (char *) "misc/katarina";
  char model_filename[255];
  char diffuse_filename[255];
  char normal_filename[255];
  snprintf(model_filename, 255, (char *) "data/%s.obj", basename);
  snprintf(diffuse_filename, 255, (char *) "data/%s_D.tga", basename);
  snprintf(normal_filename, 255, (char *) "data/%s_N.tga", basename);

  state->model = load_model(state, model_filename);
  state->model->normalize();

  ctx->diffuse = load_texture(state, diffuse_filename);
  ctx->normal = load_texture(state, normal_filename);

  state->xRot = RAD(15.0);
  state->yRot = 0.0;
  state->fov = 60.0;
}

static void render_shadowmap(State *state, RenderingContext *ctx, Model *model, Texture *shadowmap)
{
  Vec3f color = {1, 1, 1};

  ModelShader shader;
  RenderingContext subctx = {};

  ASSERT(shadowmap->width == ctx->target->width);
  ASSERT(shadowmap->height == ctx->target->height);

  subctx.target = ctx->target;
  subctx.clear_color = BLACK;

  subctx.model_mat = Mat44::identity();
  subctx.viewport_mat = viewport_matrix(shadowmap->width, shadowmap->height);
  subctx.projection_mat = orthographic_matrix(0.1, 10, -1, -1, 1, 1);

  subctx.zbuffer = ctx->zbuffer;

  Mat44 view_mat = look_at_matrix(ctx->light, (Vec3f){0, 0, 0}, (Vec3f){0, 1, 0});
  subctx.view_mat = view_mat;

  precalculate_matrices(&subctx);
  ctx->shadow_mvp_mat = subctx.modelview_mat * subctx.projection_mat * subctx.viewport_mat;

  clear_zbuffer(&subctx);

  for (int fi = 0; fi < model->fcount; fi++) {
    ModelFace face = model->faces[fi];

    for (int vi = 0; vi < 3; vi++) {
      Vec3f position = model->vertices[face.vi[vi]];
      Vec3f texture = model->texture_coords[face.ti[vi]];
      Vec3f normal = model->normals[face.ni[vi]];

      shader.vertex(&subctx, vi, position, normal, texture, color);
    }

    draw_triangle(&subctx, &shader, true);
  }

  for  (int i = 0; i < shadowmap->width; i++) {
    for (int j = 0; j < shadowmap->height; j++) {
      zval_t zval = subctx.zbuffer[j * shadowmap->width + i];
      float v = (float) zval / ZBUFFER_MAX;
      shadowmap->pixels[j * shadowmap->width + i] = (Vec3f){v, v, v};
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
    MemoryArena *arena = MemoryArena::initialize(global_state->platform_api.allocate_memory(MB(64)), MB(64));
    // memset(state, 0, MB(64));

    state = (State *) arena->allocate(MB(1));
    global_state->state = state;

    state->main_arena = arena;
    state->platform_api = &global_state->platform_api;
    state->keyboard = global_state->keyboard;

    initialize(state, drawing_buffer);
  }

  if (state->keyboard->downedKeys[KB_ESCAPE]) {
    state->platform_api->terminate();
    return;
  }

  RenderingContext *ctx = &state->rendering_context;

  if (!ctx->shadowmap) {
    ctx->shadowmap = (Texture *) state->main_arena->allocate(sizeof(Texture));
    ctx->shadowmap->pixels = (Vec3f *) state->main_arena->allocate(ctx->target->width * ctx->target->height * sizeof(Vec3f));
    ctx->shadowmap->width = drawing_buffer->width;
    ctx->shadowmap->height = drawing_buffer->height;

    ctx->light = ((Vec3f){0.5, 1, 0.5}).normalized();

    render_shadowmap(state, ctx, state->model, ctx->shadowmap);
  }

  update_camera(state, dt);
  ctx->projection_mat = perspective_matrix(0.1, 10, state->fov);

  //Mat44 view_mat = look_at_matrix((Vec3f){0, 0, 1}, (Vec3f){0, 0.35, 0}, (Vec3f){0, 1, 0});
  Mat44 view_mat = Mat44::translate(0, 0, -1);
  ctx->view_mat = Mat44::rotate_y(-state->yRot) *
                  Mat44::translate(0, -0.5, 0) *
                  Mat44::rotate_x(-state->xRot) * view_mat;

  clear_buffer(ctx);
  clear_zbuffer(ctx);

  render_floor(state, ctx);
  render_model(state, ctx, state->model);
  // render_unit_axes(ctx);
}
