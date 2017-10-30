#include "platform/platform.h"

#include "utils/math.cpp"
#include "utils/tga.cpp"
#include "utils/blp.cpp"
#include "utils/texture.cpp"
#include "utils/memory.cpp"
#include "utils/assets.cpp"

#include "renderer/renderer.cpp"

#include "dresser.cpp"
#include "model.cpp"
#include "m2.cpp"

typedef struct RenderFlags {
  bool gouraud_shading;
  bool texture_mapping;
  bool normal_mapping;
  bool shadow_mapping;
  bool bilinear_filtering;
  bool lighting;
} RenderFlags;

typedef struct State {
  PlatformAPI *platform_api;
  KeyboardState *keyboard;
  MemoryArena *main_arena;
  MemoryArena *temp_arena;

  Texture *normalmap;
  Texture *shadowmap;

  Model *model;
  M2Model *m2model;
  Texture *textures[5];
  Texture *debugTexture;
  RenderingContext rendering_context;
  Mat44 matShadowMVP;

  Dresser *dresser;
  CharAppearance appearance;

  uint32_t screenWidth;
  uint32_t screenHeight;

  float xRot;
  float yRot;
  float fov;
  float camDistance;

  RenderFlags render_flags;
  Vec3f hsv;
  float sat_deg;
  uint32_t hair;
  float scale;

  DrawingBuffer *buffer;

  uint32_t animId;
  uint32_t startFrame;
  uint32_t endFrame;
  uint32_t currentFrame;
  double currentFracFrame;
  double currentAnimFPS;
  uint32_t currentGlobalFrame;
  double currentGlobalFracFrame;
  bool playing;
  bool showBones;
  bool showUnitAxes;
  bool modelChanged;
} State;

static Vec3f hsv_to_rgb(Vec3f hsv)
{
  float h = hsv.x;
  float s = hsv.y;
  float v = hsv.z;

  h = fmod(h / 60.0f, 6.0f);
  float c = v * s;
  float x = c * (1.0f - fabs(fmod(h, 2.0f) - 1.0f));
  float m = v - c;

  int hh = (int) h;

  float r = m;
  float g = m;
  float b = m;

  if (hh == 0) {
    r += c;
    g += x;
  } else if (hh == 1) {
    r += x;
    g += c;
  } else if (hh == 2) {
    g += c;
    b += x;
  } else if (hh == 3) {
    g += x;
    b += c;
  } else if (hh == 4) {
    r += x;
    b += c;
  } else if (hh == 5) {
    r += c;
    b += x;
  }

  return {r, g, b};
}

typedef struct ModelShaderData {
  Vec3f pos[3];
  Vec3f normals[3];
  Vec3f uvs[3];
  Vec3f color;
  Texture *normalmap;
  Texture *texture;
  RenderFlags *flags;
} ModelShaderData;

FRAGMENT_FUNC(fragment_model)
{
  ModelShaderData *d = (ModelShaderData *) shader_data;
  RenderFlags *f = d->flags;
  Texture *texture = d->texture;

  float intensity = 0.0;

  Vec3f normal;
  if (f->gouraud_shading) {
    normal = d->normals[0] * t0 + d->normals[1] * t1 + d->normals[2] * t2;
  } else {
    normal = (d->pos[2] - d->pos[1]).cross(d->pos[2] - d->pos[0]).normalized();
  }

  Vec3f uv = d->uvs[0] * t0 + d->uvs[1] * t1 + d->uvs[2] * t2;

  Vec3f texel;
  Vec4f texel4;
  if (f->texture_mapping) {
    if (f->bilinear_filtering) {
      Vec3f muv;
      muv.x = uv.x * texture->width;
      muv.y = uv.y * texture->height;

      int tx0 = (int)(muv.x) & (texture->width - 1);
      int ty0 = (int)(muv.y) & (texture->height - 1);
      int tx1 = ((int)(muv.x + 1)) & (texture->width - 1);
      int ty1 = ((int)(muv.y + 1)) & (texture->height - 1);

      float dx = muv.x - tx0;
      float dy = muv.y - ty0;
      Vec4f ta = TEXEL4F(texture, tx0, ty0);
      Vec4f tb = TEXEL4F(texture, tx1, ty0);
      Vec4f tc = TEXEL4F(texture, tx0, ty1);
      Vec4f td = TEXEL4F(texture, tx1, ty1);
      texel4 = lerp(lerp(ta, tb, dx), lerp(tc, td, dx), dy);
      texel = texel4.xyz;
    } else {
      int tx0 = (int)(uv.x * texture->width) & (texture->width - 1);
      int ty0 = (int)(uv.y * texture->height) & (texture->height - 1);
      texel4 = TEXEL4F(texture, tx0, ty0);
      texel = texel4.xyz;
    }
  } else {
    texel4 = {0.0f, 0.0f, 0.0f, 1.0f};
    texel = d->color;
  }

#if 0
  if (f->normal_mapping && d->normalmap) {
    int nx = (int)(uv.x * d->normalmap->width) & (d->normalmap->width - 1);
    int ny = (int)(uv.y * d->normalmap->height) & (d->normalmap->height - 1);
    Vec3f ncolor = TEXEL3F(d->normalmap, nx, ny);
    Vec3f tnormal = {2 * ncolor.r - 1, 2 * ncolor.g - 1, 2 * ncolor.b};

    Vec3f dp1 = d->pos[1] - d->pos[0];
    Vec3f dp2 = d->pos[2] - d->pos[1];
    Vec3f duv1 = d->uvs[1] - d->uvs[0];
    Vec3f duv2 = d->uvs[2] - d->uvs[1];
    float r = 1.0f / (duv1.x * duv2.y - duv1.y * duv2.x);
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
#endif

  if (f->lighting) {
    intensity = normal.dot(-ctx->light);
  } else {
    intensity = 1.0f;
  }

  if (intensity < 0.0f) {
    intensity = 0.0f;
  }

  Vec3f ambient = texel * 0.3f;
  *color = Vec4f((ambient + texel * intensity).clamped(), texel4.a);

  return true;
}

typedef struct FloorShaderData {
  Vec3f pos[3];
  Vec3f uvzs[3];
  Vec3f colors[3];
  Vec3f normal;
  Texture *shadowmap;
  RenderFlags *flags;
  Mat44 matShadow;
} FloorShaderData;

FRAGMENT_FUNC(fragment_floor)
{
  FloorShaderData *d = (FloorShaderData *) shader_data;
  Texture *shadowmap = d->shadowmap;
  RenderFlags *f = d->flags;

  Vec3f normal = d->normal;
  float intensity = 1.0; //normal.dot(ctx->light);

  Vec3f uvz = d->uvzs[0] * t0 + d->uvzs[1] * t1 + d->uvzs[2] * t2;
  float z = 1 / uvz.z;
  int32_t texX = (int32_t) (uvz.x * z * 5.0f);
  int32_t texY = (int32_t) (uvz.y * z * 5.0f);

  Vec3f tcolor;
  if (texX % 2 == texY % 2) {
    tcolor = { 1.0f, 1.0f, 1.0f };
  } else {
    tcolor = { 0.5f, 0.5f, 0.5f };
  }

  Vec3f vcolor = { d->colors[0].r * t0 + d->colors[1].r * t1 + d->colors[2].r * t2,
                   d->colors[0].g * t0 + d->colors[1].g * t1 + d->colors[2].g * t2,
                   d->colors[0].b * t0 + d->colors[1].b * t1 + d->colors[2].b * t2 };
  vcolor = vcolor * z;

  if (f->shadow_mapping) {
    Vec3f pos = d->pos[0] * t0 + d->pos[1] * t1 + d->pos[2] * t2;
    Vec3f shadow = pos * d->matShadow;
    int32_t shx = (int32_t) shadow.x;
    int32_t shy = (int32_t) shadow.y;

    if (shadow.x >= 0 && shadow.y >= 0 &&
        shadow.x < shadowmap->width && shadow.y < shadowmap->height) {
      Vec3f shval = TEXEL3F(shadowmap, shx, shy);

      if (shval.x > 0.0f) {
        intensity = 0.2f;
      }
    }
  }

  Vec3f rcolor = { vcolor.r * tcolor.r, vcolor.g * tcolor.g, vcolor.b * tcolor.b };
  *color = Vec4f((rcolor * intensity).clamped(), 1.0f);

  return true;
}

typedef struct DebugShaderData {
  Vec3f pos[3];
  Vec3f uv0;
  Vec3f duv[2];
  uint32_t clampu;
  uint32_t clampv;
  Texture *texture;
} FontShaderData;

FRAGMENT_FUNC(fragment_debug)
{
  DebugShaderData *d = (DebugShaderData *) shader_data;

  float fu = d->uv0.x + t1 * d->duv[0].x + t2 * d->duv[1].x;
  float fv = d->uv0.y + t1 * d->duv[0].y + t2 * d->duv[1].y;

  int u = ((int) fu) & d->clampu;
  int v = ((int) fv) & d->clampv;
  *color = TEXEL4F(d->texture, u, v);

  return true;
}

static Model *load_model(State *state, char *filename)
{
  LoadedFile file = load_file(state->platform_api, state->temp_arena, filename);
  if (!file.size) {
    printf("Failed to load model: %s\n", filename);
    return NULL;
  }

  Model *model = (Model *) state->main_arena->allocate(sizeof(Model));
  model->parse(file.contents, file.size, true);

  printf("Vertices: %d; Texture coords: %d; Normals: %d; Faces: %d\n",
         model->vcount, model->tcount, model->ncount, model->fcount);

  printf("Model bbox x in (%.2f .. %.2f); y in (%.2f .. %.2f); z in (%.2f .. %.2f)\n",
         model->min.x, model->max.x, model->min.y, model->max.y, model->min.z, model->max.z);

  model->vertices = (Vec3f *) state->main_arena->allocate(sizeof(Vec3f) * model->vcount);
  model->normals = (Vec3f *) state->main_arena->allocate(sizeof(Vec3f) * model->ncount);
  model->texture_coords = (Vec3f *) state->main_arena->allocate(sizeof(Vec3f) * model->tcount);
  model->faces = (ModelFace *) state->main_arena->allocate(sizeof(ModelFace) * model->fcount);

  model->parse(file.contents, file.size);

  return model;
}

static void load_m2_model(State *state, char *filename)
{
  LoadedFile file = load_file(state->platform_api, state->temp_arena, filename);
  if (!file.size) {
    printf("Failed to load file: %s\n", filename);
    return;
  }

  state->m2model = m2_load(file.contents, file.size, state->main_arena);
}

static Texture *load_tga_texture(State *state, char *filename)
{
  LoadedFile file = load_file(state->platform_api, state->temp_arena, filename);
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

static void render_floor(State *state, RenderingContext *ctx)
{
  ctx->model_mat = Mat44::identity();
  precalculate_matrices(ctx);

  Vec3f vertices[4][2] = {
    {{-0.5, 0, 0.5}, {0, 0, 0}},
    {{0.5, 0, 0.5}, {1, 0, 0}},
    {{0.5, 0, -0.5}, {1, 1, 0}},
    {{-0.5, 0, -0.5}, {0, 1, 0}}
  };

  int triangles[2][3] = {
    {0, 1, 2},
    {0, 2, 3}
  };

  Vec3f colors[2][3] = {
    {RED, GREEN, BLUE},
    {RED, BLUE, WHITE}
  };

  FloorShaderData shader_data = {};
  shader_data.matShadow = ctx->mvp_mat.inverse() * state->matShadowMVP;
  shader_data.shadowmap = state->shadowmap;
  shader_data.flags = &state->render_flags;
  shader_data.normal = {0, 1, 0};

  Vec3f positions[3];

  for (int tri = 0; tri < 2; tri++) {
    for (int i = 0; i < 3; i++) {
      int idx = triangles[tri][i];

      Vec3f cam_pos = vertices[idx][0] * ctx->modelview_mat;
      shader_data.pos[i] = cam_pos * ctx->projection_mat;
      positions[i] = vertices[idx][0] * ctx->mvp_mat;

      float iz = 1 / cam_pos.z;
      Vec3f tex = vertices[idx][1];
      shader_data.uvzs[i] = {tex.x * iz, tex.y * iz, iz};
      shader_data.colors[i] = colors[tri][i] * iz;
    }

    ctx->draw_triangle(ctx, &fragment_floor, (void *) &shader_data, positions[0], positions[1], positions[2]);
  }
}

static void render_model(State *state, RenderingContext *ctx, Model *model)
{
  ctx->model_mat = Mat44::translate(0, 0, 0);
  precalculate_matrices(ctx);

  ModelShaderData shader_data = {};
  shader_data.color = hsv_to_rgb(state->hsv);
  shader_data.flags = &state->render_flags;
  Vec3f positions[3];

  for (int fi = 0; fi < model->fcount; fi++) {
    ModelFace face = model->faces[fi];

    for (int vi = 0; vi < 3; vi++) {
      Vec3f position = model->vertices[face.vi[vi]];
      Vec3f texture = model->texture_coords[face.ti[vi]];
      Vec3f normal = model->normals[face.ni[vi]];

      shader_data.pos[vi] = position;
      shader_data.uvs[vi] = {texture.x, texture.y, 0};
      shader_data.normals[vi] = (normal * ctx->normal_mat).normalized();

      positions[vi] = position * ctx->mvp_mat;
    }

    ctx->draw_triangle(ctx, &fragment_model, (void *) &shader_data, positions[0], positions[1], positions[2]);
  }
}

static inline void render_m2_pass(State *state, RenderingContext *ctx, M2Model *model, M2RenderPass *pass, ModelSubmesh *submesh)
{
  ModelShaderData shader_data = {};
  shader_data.color = hsv_to_rgb(state->hsv);
  shader_data.flags = &state->render_flags;
  Vec3f positions[3];

  shader_data.texture = state->textures[pass->textureId];

  uint32_t faceStart = submesh->facesStart;
  uint32_t faceEnd = faceStart + submesh->facesCount;

  for (int fi = faceStart; fi < faceEnd; fi++) {
    M2Face face = model->faces[fi];

    for (int vi = 0; vi < 3; vi++) {
      Vec3f position = model->animatedPositions[face.indices[vi]];
      Vec3f normal = model->animatedNormals[face.indices[vi]];
      Vec3f texture = model->textureCoords[face.indices[vi]];

      shader_data.pos[vi] = position * ctx->model_mat;
      shader_data.uvs[vi] = {texture.x, texture.y, 0};
      shader_data.normals[vi] = (normal * ctx->normal_mat).normalized();

      positions[vi] = position * ctx->mvp_mat;
    }

    ctx->draw_triangle(ctx, &fragment_model, (void *) &shader_data, positions[0], positions[1], positions[2]);
  }
}

typedef enum {
  RENDER_MODE_ANY,
  RENDER_MODE_OPAQUE,
  RENDER_MODE_TRANSPARENT
} RenderMode;

static BlendMode map_blending_mode(uint16_t blendingMode)
{
  switch (blendingMode) {
    case 1:
      // return BLEND_MODE_SRC_COPY;
      return BLEND_MODE_DECAL; // This works better
    case 2:
      return BLEND_MODE_DECAL;
    case 4:
      return BLEND_MODE_SRC_ALPHA_ONE;
    default:
      return BLEND_MODE_SRC_COPY;
  }
}

static void render_m2_model(State *state, RenderingContext *ctx, M2Model *model, RenderMode mode = RENDER_MODE_ANY)
{
  ctx->model_mat = Mat44::rotate_y(-RAD(90)) * Mat44::scale(state->scale, state->scale, state->scale);
  precalculate_matrices(ctx);

  for (int rpi = 0; rpi < model->renderPassesCount; rpi++) {
    M2RenderPass *pass = &model->renderPasses[rpi];
    ModelSubmesh *submesh = &model->submeshes[pass->submesh];
    if (!submesh->enabled) {
      continue;
    }

    M2RenderFlag *rf = &model->renderFlags[pass->renderFlagIndex];

    set_culling(ctx, (rf->flags & 0x04) != 0x04);

    switch (mode) {
      case RENDER_MODE_OPAQUE:
        if (rf->blendingMode != 0) {
          continue;
        }
        set_blending(ctx, false);
        break;

      case RENDER_MODE_TRANSPARENT:
        if (rf->blendingMode > 0) {
          set_blending(ctx, true);
          set_blend_mode(ctx, map_blending_mode(rf->blendingMode));
        } else {
          continue;
        }
        break;

      default:; // No filtering
    }

    render_m2_pass(state, ctx, model, pass, submesh);
  }
}

static void render_m2_model_bones(State *state, RenderingContext *ctx, M2Model *model)
{
  ctx->model_mat = Mat44::rotate_y(-RAD(90)) * Mat44::scale(state->scale, state->scale, state->scale);
  precalculate_matrices(ctx);

  Mat44 mat = ctx->mvp_mat * ctx->viewport_mat;

  for (int i = 0; i < model->bonesCount; i++) {
    ModelBone bone = model->bones[i];
    if (!bone.calculated) {
      continue;
    }

    Vec4f color = {0.0f, 1.0f, 0.0f, 1.0f};

    if (bone.parent >= 0) {
      ModelBone parentBone = model->bones[bone.parent];
      Vec3f p1 = bone.pivot * bone.matrix;
      Vec3f p2 = parentBone.pivot * parentBone.matrix;
      ctx->draw_line(ctx, p1, p2, color);
    } else {
      Vec3f pos = bone.pivot * bone.matrix * mat;
      // set_pixel_safe(ctx->target, (uint32_t) pos.x, (uint32_t) pos.y, color);
    }
  }
}

static void render_debug_texture(State *state, RenderingContext *ctx, Texture *texture, float x, float y, float width)
{
  ctx->viewport_mat = viewport_matrix((float) state->screenWidth, (float) state->screenHeight, false);
  ctx->projection_mat = orthographic_matrix(0.1f, 100.0f, 0.0f, (float) state->screenHeight, (float) state->screenWidth, 0);
  ctx->view_mat = Mat44::identity();
  ctx->model_mat = Mat44::translate(0, 0, 0);
  precalculate_matrices(ctx);

  const Vec3f texture_coords[4] = {
    {0.0f, 0.0f, 0.0f},
    {(float) texture->width, 0.0f, 0.0f},
    {(float) texture->width, (float) texture->height, 0.0f},
    {0, (float) texture->height, 0.0f}
  };

  float height = width * ((float) texture->height / (float) texture->width);

  Vec3f positions[4] = {
    {x, y + height, 0.0},
    {x + width, y + height, 0.0},
    {x + width, y, 0.0},
    {x, y, 0.0}
  };

  for (uint32_t i = 0; i < 4; i++) {
    positions[i] = positions[i] * ctx->mvp_mat;
  }

  DebugShaderData shader_data = {};
  shader_data.clampu = texture->width - 1;
  shader_data.clampv = texture->height - 1;
  shader_data.texture = texture;

  shader_data.uv0 = texture_coords[0];
  shader_data.duv[0] = texture_coords[1] - shader_data.uv0;
  shader_data.duv[1] = texture_coords[2] - shader_data.uv0;
  ctx->draw_triangle(ctx, &fragment_debug, (void *) &shader_data, positions[0], positions[1], positions[2]);

  shader_data.duv[0] = texture_coords[2] - shader_data.uv0;
  shader_data.duv[1] = texture_coords[3] - shader_data.uv0;
  ctx->draw_triangle(ctx, &fragment_debug, (void *) &shader_data, positions[0], positions[2], positions[3]);
}

static void render_unit_axes(RenderingContext *ctx)
{
  ctx->model_mat = Mat44::identity();
  precalculate_matrices(ctx);

  Vec3f origin = {0, 0, 0};
  ctx->draw_line(ctx, origin, {1, 0, 0}, {1.0f, 0.0f, 0.0f, 1.0f});
  ctx->draw_line(ctx, origin, {0, 1, 0}, {0.0f, 1.0f, 0.0f, 1.0f});
  ctx->draw_line(ctx, origin, {0, 0, 1}, {0.0f, 0.0f, 1.0f, 1.0f});
}

static void animate_model(State *state)
{
  m2_animate_vertices(state->m2model, state->animId, state->currentFrame, state->currentGlobalFrame);
  state->modelChanged = true;
}

static void switch_animation(State *state, int32_t animId)
{
  if (animId < 0) {
    animId = state->m2model->animationsCount - 1;
  }

  if (animId >= state->m2model->animationsCount) {
    animId = 0;
  }

  state->animId = animId;
  ModelAnimation animation = state->m2model->animations[state->animId];
  state->startFrame = animation.startFrame;
  state->endFrame = animation.endFrame;
  state->currentFrame = animation.startFrame;
  state->currentFracFrame = (double) state->currentFrame;
  state->currentAnimFPS = 1000.0;

  animate_model(state);

  printf("Switched animation to: %d\n", state->animId);
}

static void enable_submeshes(State *state)
{
  M2Model *model = state->m2model;

  uint16_t enabledSubmeshes[] = {
    5, // Hair style
    401, // Default gloves
    501, // Default boots
    702, // Ears
    1101, // Default pants
    1301, // Default pants
    1501, // Default back/cloak
    1701 // Eyeglow
  };

  for (int si = 0; si < model->submeshesCount; si++) {
    for (int esi = 0; esi < sizeof(enabledSubmeshes) / sizeof(enabledSubmeshes[0]); esi++) {
      if (model->submeshes[si].id == enabledSubmeshes[esi]) {
        model->submeshes[si].enabled = true;
      }
    }
  }
}

static void set_character_appearance(State *state, CharAppearance appearance)
{
  state->dresser->mainArena->discard();
  state->textures[0] = dresser_get_character_texture(state->dresser, appearance);
  state->textures[1] = dresser_get_hair_texture(state->dresser, appearance);
}

static void initialize(State *state, DrawingBuffer *buffer)
{
  RenderingContext *ctx = &state->rendering_context;
  set_target(ctx, buffer);
  ctx->clear_color = BLACK;

  ctx->model_mat = Mat44::identity();
  ctx->viewport_mat = viewport_matrix((float) buffer->width, (float) buffer->height, true);
  ctx->projection_mat = perspective_matrix(0.1f, 10.0f, 60.0f);

  ctx->light = { 0, -1, 0 };
  ctx->light = ctx->light.normalized();

  ctx->zbuffer = (zval_t *) state->main_arena->allocate(buffer->width * buffer->height * sizeof(zval_t));

  char *basename = (char *) "misc/man";
  char model_filename[255];
  char diffuse_filename[255];
  char normal_filename[255];
  snprintf(model_filename, 255, (char *) "data/%s.obj", basename);
  snprintf(diffuse_filename, 255, (char *) "data/%s_D.tga", basename);
  snprintf(normal_filename, 255, (char *) "data/%s_N.tga", basename);

  state->buffer = buffer;
  state->screenWidth = buffer->width;
  state->screenHeight = buffer->height;

  state->dresser = (Dresser *) state->main_arena->allocate(sizeof(Dresser));
  dresser_init(state->dresser, state->platform_api, state->main_arena);

  load_m2_model(state, (char *) "data/misc/nelf-patched.m2");
  // state->textures[0] = load_tga_texture(state, (char *) "data/misc/nelf_0.tga");
  // state->textures[1] = load_tga_texture(state, (char *) "data/misc/nelf_1.tga");
  state->textures[2] = load_tga_texture(state, (char *) "data/misc/cape.tga");
  state->textures[3] = load_tga_texture(state, (char *) "data/misc/nelf_3.tga");

  state->appearance = {0};
  state->appearance.facialDetailIdx = MIN_FACIAL_DETAIL_IDX;
  set_character_appearance(state, state->appearance);

  // load_m2_model(state, (char *) "data/misc/dwarf.m2");
  // state->textures[0] = load_tga_texture(state, (char *) "data/misc/dwarf_0.tga");
  // state->textures[1] = load_tga_texture(state, (char *) "data/misc/dwarf_1.tga");
  // state->textures[2] = load_tga_texture(state, (char *) "data/misc/cape.tga");
  // state->textures[3] = load_tga_texture(state, (char *) "data/misc/nelf_3.tga");

  // load_m2_model(state, (char *) "data/misc/diablo.m2");
  // state->textures[0] = load_tga_texture(state, (char *) "data/misc/diablo_D.tga");

  // load_m2_model(state, (char *) "data/misc/hydra.m2");
  // state->textures[0] = load_tga_texture(state, (char *) "data/misc/hydra_0.tga");

  // load_m2_model(state, (char *) "data/misc/gnoll_caster.m2");
  // state->textures[0] = load_tga_texture(state, (char *) "data/misc/gnoll_caster_0.tga");
  // state->textures[1] = load_tga_texture(state, (char *) "data/misc/nelf_3.tga");

  // load_m2_model(state, (char *) "data/misc/ent.m2");
  // state->textures[0] = load_tga_texture(state, (char *) "data/misc/ent_0.tga");
  // state->textures[1] = state->textures[0];

  // load_m2_model(state, (char *) "data/misc/riding_horse.m2");
  // state->textures[0] = load_tga_texture(state, (char *) "data/misc/riding_horse_0.tga");
  // state->textures[1] = load_tga_texture(state, (char *) "data/misc/riding_horse_1.tga");

  // load_m2_model(state, (char *) "data/misc/murloc.m2");
  // state->textures[0] = load_tga_texture(state, (char *) "data/misc/murloc_0.tga");
  // state->textures[1] = state->textures[0];

  //state->textures[4] = load_blp_texture(state, (char *) "data/misc/test2.blp");

  state->xRot = RAD(15.0f);
  state->yRot = 0.0f;
  state->fov = 60.0f;
  state->camDistance = 1.0f;

  memset(&state->render_flags, 1, sizeof(state->render_flags)); // Set all flags
  state->hsv = { 260.0f, 0.33f, 1.0f };
  state->sat_deg = RAD(109.0f);

  state->hair = 6;
  state->scale = 0.4f;

  state->playing = true;
  state->showBones = false;
  state->showUnitAxes = false;
  state->modelChanged = true;

  state->debugTexture = NULL; // load_blp_texture(state->platform_api, state->main_arena, state->temp_arena, (char *) "data/mpq/Character/NightElf/ScalpLowerHair00_04.blp");

  switch_animation(state, 24);
  enable_submeshes(state);

  state->temp_arena->discard();
}

static void render_shadowmap(State *state, RenderingContext *ctx, Model *model, Texture *shadowmap)
{
  RenderingContext subctx = {};

  set_target(&subctx, shadowmap);

  subctx.model_mat = Mat44::identity();
  subctx.viewport_mat = viewport_matrix((float) shadowmap->width, (float) shadowmap->height, true);
  subctx.projection_mat = orthographic_matrix(0.1f, 10.0f, -1.0f, -1.0f, 1.0f, 1.0f);

  ASSERT(shadowmap->width * shadowmap->height < ctx->target_width * ctx->target_height); // Check we won't overflow zbuffer
  subctx.zbuffer = ctx->zbuffer;

  // HACK: Multiplication by 5 so camera doesn't end up inside geometry
  subctx.view_mat = look_at_matrix(ctx->light * 5, {0, 0, 0}, {0, 1, 0});
  precalculate_matrices(&subctx);

  state->matShadowMVP = subctx.modelview_mat * subctx.projection_mat * subctx.viewport_mat;

  clear_zbuffer(&subctx);
  render_m2_model(state, &subctx, state->m2model);

  for (int i = 0; i < shadowmap->width; i++) {
    for (int j = 0; j < shadowmap->height; j++) {
      zval_t zval = subctx.zbuffer[j * shadowmap->width + i];
      float v = (float) zval / ZBUFFER_MAX;
      shadowmap->pixels[j * shadowmap->width + i] = { v, v, v, 1.0 };
    }
  }
}

#define Y_RAD_PER_SEC RAD(120.0f)
#define X_RAD_PER_SEC RAD(120.0f)
#define FOV_DEG_PER_SEC 30.0f
#define CAM_DIST_PER_SEC 1.0f
#define HUE_PER_SEC 120.0f
#define SATURATION_PER_SEC RAD(120.0f)

static bool update_animation(State *state, float dt)
{
  float dframe = dt * state->currentAnimFPS;
  state->currentFracFrame += dframe;
  state->currentGlobalFracFrame += dframe;

  bool result = false;

  uint32_t globalFrame = (uint32_t) state->currentGlobalFracFrame;
  if (globalFrame != state->currentGlobalFrame) {
    state->currentGlobalFrame = globalFrame;
    result = true;
  }

  uint32_t frame = (uint32_t) state->currentFracFrame;
  if (frame != state->currentFrame) {
    result = true;
    state->currentFrame = frame;
  }

  if (result) {
    animate_model(state);
  }

  if (state->currentFracFrame > state->endFrame) {
    state->currentFracFrame = state->startFrame;
  }

  return result;
}

static void update_camera(State *state, float dt)
{
  float da = 0.0;
  if (KEY_IS_DOWN(state->keyboard, KB_LEFT_ARROW)) {
    da -= Y_RAD_PER_SEC * dt;
  }
  if (KEY_IS_DOWN(state->keyboard, KB_RIGHT_ARROW)) {
    da += Y_RAD_PER_SEC * dt;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_LEFT_SHIFT)) {
    da *= 3.0;
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
  //state->xRot = CLAMP(state->xRot, -PI, PI);
  if (state->xRot > 2*PI) {
    state->xRot -= 2*PI;
  }

  da = 0.0;
  if (KEY_IS_DOWN(state->keyboard, KB_PLUS)) {
    da -= CAM_DIST_PER_SEC * dt;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_MINUS)) {
    da += CAM_DIST_PER_SEC * dt;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_LEFT_ALT)) {
    state->scale -= da;
    state->scale = CLAMP(state->scale, 0.1f, 5.0f);
  } else {
    state->camDistance += da;
    state->camDistance = CLAMP(state->camDistance, 0.7f, 3.0f);
  }

  if (KEY_IS_DOWN(state->keyboard, KB_H)) {
    state->hsv.x += HUE_PER_SEC * dt;
  }

  if (state->hsv.x > 360.0f) {
    state->hsv.x -= 360.0f;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_C)) {
    state->sat_deg += SATURATION_PER_SEC * dt;
  }

  if (state->sat_deg > RAD(360.0f)) {
    state->sat_deg -= RAD(360.0f);
  }

  bool update_anim = false;
  int step = 1;
  bool loop = false;

  if (KEY_IS_DOWN(state->keyboard, KB_LEFT_SHIFT)) {
    step = 15;
    loop = true;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_Z)) {
    state->currentFrame -= step;
    if (state->currentFrame < state->startFrame) {
      if (loop) {
        state->currentFrame = state->endFrame;
      } else {
        state->currentFrame = state->startFrame;
      }
    }
    update_anim = true;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_X)) {
    state->currentFrame += step;
    if (state->currentFrame > state->endFrame) {
      if (loop) {
        state->currentFrame = state->startFrame;
      } else {
        state->currentFrame = state->endFrame;
      }
    }
    update_anim = true;
  }

  if (update_anim) {
    animate_model(state);
  }

  state->hsv.y = cos(state->sat_deg) * 0.5f + 0.5f; // map to 0..1
}

static void handle_input(State *state)
{
  if (KEY_WAS_PRESSED(state->keyboard, KB_T)) {
    state->render_flags.texture_mapping = !state->render_flags.texture_mapping;
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_G)) {
    state->render_flags.gouraud_shading = !state->render_flags.gouraud_shading;
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_S)) {
    state->render_flags.shadow_mapping = !state->render_flags.shadow_mapping;
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_N)) {
    state->render_flags.normal_mapping = !state->render_flags.normal_mapping;
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_L)) {
    state->render_flags.lighting = !state->render_flags.lighting;
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_F)) {
    state->render_flags.bilinear_filtering = !state->render_flags.bilinear_filtering;
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_SPACE)) {
    state->hair++;
    if (state->hair > 7) {
      state->hair = 1;
    }
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_Q)) {
    switch_animation(state, state->animId - 1);
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_W)) {
    switch_animation(state, state->animId + 1);
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_P)) {
    state->playing = !state->playing;
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_B)) {
    state->showBones = !state->showBones;
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_U)) {
    state->showUnitAxes = !state->showUnitAxes;
  }

  bool appearanceChanged = false;
  bool shift = KEY_IS_DOWN(state->keyboard, KB_LEFT_SHIFT);

  if (KEY_WAS_PRESSED(state->keyboard, KB_9)) {
    appearanceChanged = true;

    if (shift) {
      state->appearance.faceIdx = CLAMP_CYCLE(state->appearance.faceIdx - 1, 0, MAX_FACE_IDX);
    } else {
      state->appearance.skinIdx = CLAMP_CYCLE(state->appearance.skinIdx - 1, 0, MAX_SKIN_IDX);
    }
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_0)) {
    appearanceChanged = true;

    if (shift) {
      state->appearance.faceIdx = CLAMP_CYCLE(state->appearance.faceIdx + 1, 0, MAX_FACE_IDX);
    } else {
      state->appearance.skinIdx = CLAMP_CYCLE(state->appearance.skinIdx + 1, 0, MAX_SKIN_IDX);
    }
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_7)) {
    appearanceChanged = true;

    if (shift) {
      state->appearance.facialDetailIdx = CLAMP_CYCLE(state->appearance.facialDetailIdx - 1, MIN_FACIAL_DETAIL_IDX, MAX_FACIAL_DETAIL_IDX);
    } else {
      state->appearance.hairColorIdx = CLAMP_CYCLE(state->appearance.hairColorIdx - 1, 0, MAX_HAIR_COLOR_IDX);
    }
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_8)) {
    appearanceChanged = true;

    if (shift) {
      state->appearance.facialDetailIdx = CLAMP_CYCLE(state->appearance.facialDetailIdx + 1, MIN_FACIAL_DETAIL_IDX, MAX_FACIAL_DETAIL_IDX);
    } else {
      state->appearance.hairColorIdx = CLAMP_CYCLE(state->appearance.hairColorIdx + 1, 0, MAX_HAIR_COLOR_IDX);
    }
  }

  if (appearanceChanged) {
    set_character_appearance(state, state->appearance);
  }
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
    MemoryArena *arena = MemoryArena::initialize(global_state->platform_api.allocate_memory(MB(128)), MB(128));
    // memset(state, 0, MB(64));

    state = (State *) arena->allocate(MB(1));
    global_state->state = state;

    state->main_arena = arena;
    state->temp_arena = arena->subarena(MB(32));
    state->platform_api = &global_state->platform_api;
    state->keyboard = global_state->keyboard;

    initialize(state, drawing_buffer);
    animate_model(state);
  }

  if (state->keyboard->downedKeys[KB_ESCAPE]) {
    state->platform_api->terminate();
    return;
  }

  RenderingContext *ctx = &state->rendering_context;

  if (!state->shadowmap) {
    ctx->light = { 0.5, 1, 0.5 };
    ctx->light = ctx->light.normalized();

    state->shadowmap = texture_create(state->main_arena, 512, 512);
    render_shadowmap(state, ctx, state->model, state->shadowmap);
  }

  if (state->playing) {
    update_animation(state, dt);
  }

  if (state->modelChanged) {
    if (state->render_flags.shadow_mapping) {
      render_shadowmap(state, ctx, state->model, state->shadowmap);
    }

    state->modelChanged = false;
  }

  update_camera(state, dt);
  handle_input(state);

  ctx->viewport_mat = viewport_matrix((float) state->screenWidth, (float) state->screenHeight, true);
  ctx->projection_mat = perspective_matrix(0.1f, 10.0f, state->fov);

  //ctx->view_mat = look_at_matrix((Vec3f){0, 0, 1}, (Vec3f){0, 0.35, 0}, (Vec3f){0, 1, 0});
  ctx->view_mat = Mat44::rotate_y(-state->yRot) *
                  Mat44::translate(0.0f, -0.5f, 0.0f) *
                  Mat44::rotate_x(-state->xRot) *
                  Mat44::translate(0.0f, 0.0f, -state->camDistance);

  set_target(ctx, state->buffer);
  clear_buffer(state->buffer, {0.0f, 0.0f, 0.0f, 0.0f});
  clear_zbuffer(ctx);

  render_floor(state, ctx);
  //render_model(state, ctx, state->model);

  render_m2_model(state, ctx, state->m2model, RENDER_MODE_OPAQUE);
  render_m2_model(state, ctx, state->m2model, RENDER_MODE_TRANSPARENT);

  if (state->showBones) {
   render_m2_model_bones(state, ctx, state->m2model);
  }

  if (state->showUnitAxes) {
    render_unit_axes(ctx);
  }

  if (state->render_flags.shadow_mapping) {
    render_debug_texture(state, ctx, state->shadowmap, 10, 10, 400);
  }

  set_blending(ctx, true);
  set_blend_mode(ctx, BLEND_MODE_DECAL);

  if (state->debugTexture) {
    render_debug_texture(state, ctx, state->debugTexture, 10, 410, 400);
  }
}

#ifdef _WIN32
#include "Windows.h"

BOOLEAN WINAPI DllMain(IN HINSTANCE hDllHandle,
                       IN DWORD     nReason,
                       IN LPVOID    Reserved)
{
  switch (nReason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hDllHandle);
      break;

    case DLL_PROCESS_DETACH:
      break;
  }

  return true;
}
#endif
