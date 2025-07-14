#include "platform/platform.h"

#include "utils/math.cpp"
#include "utils/texture.cpp"
#include "utils/memory.cpp"
#include "utils/assets.cpp"
#include "utils/font.cpp"
#include "utils/ui.cpp"

#include "formats/tga.cpp"
#include "formats/blp.cpp"
#include "formats/dbc.cpp"
#include "formats/m2.cpp"

#include "renderer/renderer.cpp"

#include "dresser.cpp"
#include "model.cpp"

typedef struct RenderFlags {
  bool gouraud_shading;
  bool texture_mapping;
  bool normal_mapping;
  bool shadow_mapping;
  bool bilinear_filtering;
  bool lighting;
} RenderFlags;

typedef struct Animation {
  char *name;
  int32_t id;
  uint32_t dbc_id;
  uint32_t startFrame;
  uint32_t endFrame;
  float fps;
  float currentFrame;
} Animation;

static struct {
  const char *name;
  uint32_t display_id;
} creatures[] = {
  {"Auberdine Sentinel", 4845},
  {"\"Sea Wolf\" MacKinley", 1925},
  {"Stormind City Guard", 3167},
  {"Defias Smuggler", 4418},
  {"Defias Highwayman", 2342},
  {"Dalaran Wizard", 3585},
  {"Infernal", 169},
  {"Felhunter", 850},
  {"Murloc Flesheater", 506},
  {"Tarantula", 366},
  {"Young Goretusk", 8871},
  {"Kobold Geomancer", 163},
  {"Lady Sylvanas Windrunner", 11657},
  {"Onyxia", 8570},
  {"Wailing Banshee", 8782},
  {"Risen Construct", 12074},
  {"Scholomance Occultist", 11157},
  {"Beaten Corpse", 10004},
  {"Swine", 10015},
  {"Argent Officer Pureheart", 10211},
  {"Fallen Hero", 10460},
  {"Cyclone Warrior", 8716},
  {"Frostwolf Legionnaire", 12949},
  {"Riding Nightsaber", 9991},
  {"Alterac Yeti", 10946}
};

typedef struct State {
  PlatformAPI *platform_api;
  KeyboardState *keyboard;
  MouseState *mouse;
  MemoryArena *main_arena;
  MemoryArena *temp_arena;

  Texture *normalmap;
  Texture *shadowmap;

  Model *model;
  Texture *debugTexture;
  Texture *debugTexture2;
  RenderingContext rendering_context;
  Mat44 matShadowMVP;

  Font *font;
  UIContext ui;
  DBCFile *dbc_anim_data;

  AssetLoader loader;
  Dresser *dresser;
  uint32_t creature_index;
  DresserCreatureBase *creature;
  DresserCustomizationLimits appearance_limits;

  uint32_t screenWidth;
  uint32_t screenHeight;

  float xrot;
  float xrot_target;
  float yrot;
  float yrot_target;

  float fov;
  float camDistance;

  RenderFlags render_flags;
  Vec3f hsv;
  float sat_deg;
  uint32_t hair;
  float scale;
  float model_scale;

  DrawingBuffer *buffer;

  ModelBoneSet core_boneset;
  ModelBoneSet upper_boneset;
  Animation upperAnim;
  Animation lowerAnim;
  bool lower_anim_enabled;
  double currentGlobalFrame;

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
      float uvx = uv.x > 0.0f ? uv.x : 1.0f - uv.x;
      float uvy = uv.y > 0.0f ? uv.y : 1.0f - uv.y;

      // UV coordinates of the subpixel center
      float cuvx = (uvx + 0.5f / (float) texture->width) * (float) texture->width;
      float cuvy = (uvy + 0.5f / (float) texture->height) * (float) texture->height;

      // Distance of the subpixel center relative to the top-left pixel corner for lerping
      float dx = cuvx - (int) cuvx;
      float dy = cuvy - (int) cuvy;

      // Integer texture coorinates of the four texels we will lerp between
      int tx0 = (int)(cuvx) & (texture->width - 1);
      int ty0 = (int)(cuvy) & (texture->height - 1);
      int tx1 = ((int)(cuvx) + 1) & (texture->width - 1);
      int ty1 = ((int)(cuvy) + 1) & (texture->height - 1);

      // Fetch and lerp the values of the four texels
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
} DebugShaderData;

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

static Font *load_font(State *state, char *textureFilename)
{
  char ftdFilename[1024];
  if (snprintf(&ftdFilename[0], 1024, "%s.ftd", textureFilename) >= 1024) {
    return NULL;
  }

  LoadedFile file = load_file(state->platform_api, state->main_arena, ftdFilename);
  if (!file.size) {
    printf("Failed to load font: %s\n", ftdFilename);
    return NULL;
  }

  Font *font = (Font *) state->main_arena->allocate(sizeof(Font));
  font->texture = load_tga_texture(state, textureFilename);
  font_init(font, file.contents, file.size);

  return font;
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

  uint32_t texture_index = model->textureLookups[pass->textureId];
  shader_data.texture = model->textures[texture_index].texture;
  ASSERT(shader_data.texture != NULL);

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
  if (model == NULL) {
    return;
  }

  for (int rpi = 0; rpi < model->renderPassesCount; rpi++) {
    M2RenderPass *pass = &model->renderPasses[rpi];
    ModelSubmesh *submesh = &model->submeshes[pass->submesh];
    if (!submesh->enabled) {
      continue;
    }

    M2RenderFlag *rf = &model->renderFlags[pass->renderFlagIndex];

    if ((rf->flags & 0x04) != 0x04) {
      renderer_enable(ctx, RENDER_CULLING);
    } else {
      renderer_disable(ctx, RENDER_CULLING);
    }

    switch (mode) {
      case RENDER_MODE_OPAQUE:
        if (rf->blendingMode != 0) {
          continue;
        }
        renderer_disable(ctx, RENDER_BLENDING);
        break;

      case RENDER_MODE_TRANSPARENT:
        if (rf->blendingMode > 0) {
          renderer_enable(ctx, RENDER_BLENDING);
          renderer_set_blend_mode(ctx, map_blending_mode(rf->blendingMode));
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

static void enable_ortho(State *state, RenderingContext *ctx)
{
  ctx->viewport_mat = viewport_matrix((float) state->screenWidth, (float) state->screenHeight, false);
  ctx->projection_mat = orthographic_matrix(0.1f, 100.0f, 0.0f, (float) state->screenHeight, (float) state->screenWidth, 0);
  ctx->view_mat = Mat44::identity();
  ctx->model_mat = Mat44::translate(0, 0, 0);
  precalculate_matrices(ctx);
}

static void render_debug_texture(State *state, RenderingContext *ctx, Texture *texture, float x, float y, float width)
{
  const Vec3f texture_coords[4] = {
    {0, (float) texture->height, 0.0f},
    {(float) texture->width, (float) texture->height, 0.0f},
    {(float) texture->width, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f}
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
  ASSERT(shader_data.texture != NULL);

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
  if (state->creature == NULL) {
    return;
  }

  M2Model *model = state->creature->model;
  if (model == NULL) {
    return;
  }

  m2_reset_bones(model);

  if (state->creature->type == DCT_CHARACTER) {
    if (state->lower_anim_enabled && (state->upperAnim.id != state->lowerAnim.id)) {
      m2_calc_bones(model, &state->core_boneset, state->lowerAnim.id, state->lowerAnim.currentFrame, state->currentGlobalFrame);
      m2_calc_bones(model, &state->upper_boneset, state->upperAnim.id, state->upperAnim.currentFrame, state->currentGlobalFrame);
      m2_calc_bones(model, NULL, state->lowerAnim.id, state->lowerAnim.currentFrame, state->currentGlobalFrame);
    } else {
      m2_calc_bones(model, NULL, state->upperAnim.id, state->upperAnim.currentFrame, state->currentGlobalFrame);
    }
  } else {
    m2_calc_bones(model, NULL, state->upperAnim.id, state->upperAnim.currentFrame, state->currentGlobalFrame);
  }

  m2_animate_vertices(model);

  state->modelChanged = true;
}

static void set_animation(State *state, Animation *anim, uint32_t newId)
{
  if (state->creature == NULL) {
    return;
  }

  M2Model *model = state->creature->model;

  anim->id = newId;
  anim->startFrame = model->animations[newId].startFrame;
  anim->endFrame = model->animations[newId].endFrame;
  anim->fps = 1000.0;
  anim->currentFrame = anim->startFrame;
  anim->name = (char *) "N/A";

  int16_t dbId = model->animations[newId].id;
  if (dbId >= 0) {
    DBCRecord *anim_record = dbc_get_record(state->dbc_anim_data, dbId);
    if (anim_record) {
      anim->name = DBC_STRING(state->dbc_anim_data, anim_record->name_offset);
      anim->dbc_id = dbId;
    }
  }

  printf("Switched animation to: %s (%d)\n", anim->name, anim->id);

  animate_model(state);
}

static void switch_animation(State *state, Animation *anim, int32_t inc)
{
  M2Model *model = state->creature->model;
  uint32_t newId = CLAMP_CYCLE((int32_t) anim->id + inc, (int32_t) 0, (int32_t) model->animationsCount - 1);
  set_animation(state, anim, newId);
}

static void set_creature(State *state, DresserCreatureBase *creature)
{
  if (creature == NULL) {
    return;
  }

  state->creature = creature;

  M2Model *model = creature->model;
  if (model->bounding_radius > 3.0f) {
    state->model_scale = 2.8f / model->bounding_radius;
  } else {
    state->model_scale = 1.0f;
  }

  state->debugTexture = model->textures[0].texture;

  if (creature->type == DCT_CHARACTER) {
    DresserCharacter *character = (DresserCharacter *) creature;

    state->appearance_limits = dresser_get_customization_limits(state->dresser, character->race, character->sex);

    state->core_boneset = m2_character_core_boneset(creature->model);
    state->upper_boneset = m2_character_upper_body_boneset(creature->model);

    if (model->animationLookups[state->upperAnim.dbc_id] > -1) {
      set_animation(state, &state->upperAnim, model->animationLookups[state->upperAnim.dbc_id]);
    } else {
      set_animation(state, &state->upperAnim, 0);
    }

    if (model->animationLookups[state->lowerAnim.dbc_id] > -1) {
      set_animation(state, &state->lowerAnim, model->animationLookups[state->lowerAnim.dbc_id]);
    } else {
      set_animation(state, &state->lowerAnim, 0);
    }
  } else {
    if (model->animationLookups[state->upperAnim.dbc_id] > -1) {
      set_animation(state, &state->upperAnim, model->animationLookups[state->upperAnim.dbc_id]);
    } else {
      set_animation(state, &state->upperAnim, 0);
    }
  }

  animate_model(state);
}

static void initialize(State *state, DrawingBuffer *buffer)
{
  RenderingContext *ctx = &state->rendering_context;
  set_target(ctx, buffer);
  ctx->clear_color = BLACK;

  ctx->model_mat = Mat44::identity();
  ctx->viewport_mat = viewport_matrix((float) buffer->width, (float) buffer->height, true);
  ctx->projection_mat = perspective_matrix(0.1f, 10.0f, 60.0f);

  ctx->light = Vec3f(0.5f, 1.0f, 0.5f).normalized();

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

  // LoadedAsset asset = state->platform_api->load_asset((char *) "Spells/Intellect_128.blp");
  // if (asset.data != NULL) {
  //   printf("Loaded asset of size: %zu\n", asset.size);

  //   BlpImage image;
  //   image.read_header(asset.data, asset.size);

  //   BlpHeader *header = &image.header;
  //   printf("BLP Image width: %d; height: %d; compression: %d\nAlpha depth: %d, alpha type: %d\n",
  //          header->width, header->height, header->compression, header->alphaDepth, header->alphaType);

  //   Texture *texture = texture_create(state->main_arena, header->width, header->height);
  //   image.read_into_texture(asset.data, asset.size, texture);

  //   state->debugTexture = texture;
  //   state->platform_api->release_asset(&asset);
  // }

  asset_loader_init(&state->loader, state->platform_api);
  Asset *dbc_asset = asset_loader_get_dbc(&state->loader, (char *) "DBFilesClient/AnimationData.dbc");
  if (dbc_asset != NULL) {
    state->dbc_anim_data = dbc_asset->dbc;
  }

  // Asset *asset = asset_loader_get_texture(&state->loader, (char *) "Character/Troll/Female/TrollFemaleSkin00_108.blp");
  // Asset *asset = asset_loader_get_texture(&state->loader, (char *) "Character/Troll/Female/TrollFemaleFaceLower00_05.blp");
  // Asset *asset = asset_loader_get_texture(&state->loader, (char *) "Character/Troll/ScalpLowerHair02_00.blp");
  // Asset *asset = asset_loader_get_texture(&state->loader, (char *) "Character/NightElf/FacialUpperHair01_00.blp");
  // Asset *asset = asset_loader_get_texture(&state->loader, (char *) "Character/Scourge/FacialUpperHair03_00.blp");
  // Asset *asset = asset_loader_get_texture(&state->loader, (char *) "Item/TextureComponents/LegLowerTexture/Robe_C_03BlackBlack_Pant_LL_U.blp");
  // state->debugTexture = asset->texture;

  state->dresser = (Dresser *) state->main_arena->allocate(sizeof(Dresser));
  dresser_init(state->dresser, &state->loader);

  state->creature_index = 0;
  set_creature(state, dresser_load_creature(state->dresser, creatures[state->creature_index].display_id));
  switch_animation(state, &state->lowerAnim, 0);
  switch_animation(state, &state->upperAnim, 0);

  #if 0
  if (state->creature && state->creature->type == DCT_CHARACTER) {
    DresserCharacterEquipment equip = {};

    equip.slots[DEIS_HEAD] = {35612}; // Redemption Headpiece
    //equip.slots[DEIS_HEAD] = {15912}; // Silk Wizard Hat
    //equip.slots[DEIS_HEAD] = {17277};
    equip.slots[DEIS_WRISTS] = {35619}; // Redemption Wristguards
    equip.slots[DEIS_SHOULDERS] = {35617}; // Redemption Spaulders
    equip.slots[DEIS_LEGS] = {35616}; // Redemption Legguards
    equip.slots[DEIS_FEET] = {35613}; // Redemption Boots
    equip.slots[DEIS_CHEST] = {35618}; // Redemption Tunic
    //equip.slots[DEIS_CHEST] = {12702}; // Silver Dress Robes
    equip.slots[DEIS_HANDS] = {35615}; // Redemption Handguards
    equip.slots[DEIS_WAIST] = {35614}; // Redemption Girdle
    equip.slots[DEIS_BACK] = {35408}; // Veil of Eclipse

    dresser_set_character_equipment(state->dresser, (DresserCharacter *) state->creature, &equip);
  }
  #endif

  animate_model(state);

  //set_character_model(state, 0, 0);

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

  state->font = load_font(state, (char *) "data/fonts/firasans.tga");

  ui_init(&state->ui, &state->rendering_context, state->font, state->keyboard, state->mouse);

  state->xrot = RAD(15.0f);
  state->xrot_target = state->xrot;
  state->yrot = 0.0f;
  state->yrot_target = state->yrot;

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

  // set_animation(state, &state->upperAnim, 0);
  // set_animation(state, &state->lowerAnim, 0);
  state->lower_anim_enabled = false;

  state->temp_arena->discard();
}

static void render_shadowmap(State *state, RenderingContext *ctx, Texture *shadowmap)
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

  if (state->creature != NULL) {
    float scale = state->scale * state->model_scale;
    subctx.model_mat = Mat44::rotate_y(-RAD(90)) * Mat44::scale(scale, scale, scale);
    precalculate_matrices(&subctx);

    render_m2_model(state, &subctx, state->creature->model);
  }

  for (int i = 0; i < shadowmap->width; i++) {
    for (int j = 0; j < shadowmap->height; j++) {
      zval_t zval = subctx.zbuffer[j * shadowmap->width + i];
      float v = (float) zval / ZBUFFER_MAX;
      shadowmap->pixels[j * shadowmap->width + i] = { v, v, v, 1.0 };
    }
  }
}

#if 0
static void render_codepoint(State *state, RenderingContext *ctx, uint32_t codepoint, float x, float y)
{
  FontShaderData data = {};
  data.texture = state->font->texture;
  data.clampu = data.texture->width - 1;
  data.clampv = data.texture->height - 1;

  FontQuad q = *font_codepoint_quad(state->font, codepoint);
  // printf("quad: %.03f %.03f %.03f %.03f\n      %.03f %.03f %.03f %.03f\n", q.s0, q.t0, q.s1, q.t1, q.x0, q.y0, q.x1, q.y1);

  Vec3f tex[4] = {};
  tex[0] = {q.s0, q.t0, 0.0f};
  tex[1] = {q.s1, q.t0, 0.0f};
  tex[2] = {q.s1, q.t1, 0.0f};
  tex[3] = {q.s0, q.t1, 0.0f};

  Vec3f pos[4] = {};
  pos[0] = Vec3f{q.x0 + x, q.y0 + y, 0.0f} * ctx->mvp_mat;
  pos[1] = Vec3f{q.x1 + x, q.y0 + y, 0.0f} * ctx->mvp_mat;
  pos[2] = Vec3f{q.x1 + x, q.y1 + y, 0.0f} * ctx->mvp_mat;
  pos[3] = Vec3f{q.x0 + x, q.y1 + y, 0.0f} * ctx->mvp_mat;

  data.uv0 = tex[0];
  data.duv[0] = tex[1] - data.uv0;
  data.duv[1] = tex[2] - data.uv0;
  ctx->draw_triangle(ctx, &fragment_text, (void *) &data, pos[0], pos[1], pos[2]);

  data.duv[0] = tex[2] - data.uv0;
  data.duv[1] = tex[3] - data.uv0;
  ctx->draw_triangle(ctx, &fragment_text, (void *) &data, pos[0], pos[2], pos[3]);
}
#endif

#define Y_RAD_PER_SEC RAD(120.0f)
#define X_RAD_PER_SEC RAD(120.0f)
#define FOV_DEG_PER_SEC 30.0f
#define CAM_DIST_PER_SEC 1.0f
#define HUE_PER_SEC 120.0f
#define SATURATION_PER_SEC RAD(120.0f)
#define MOUSE_SENSE_X 0.8f
#define MOUSE_SENSE_Y 0.8f

#define GLOBAL_ANIM_FPS 1000.0

static void update_animation(Animation *animation, float dt)
{
  animation->currentFrame += dt * animation->fps;
  if (animation->currentFrame > animation->endFrame) {
    animation->currentFrame = animation->startFrame;
  }
}

static void update_animations(State *state, float dt)
{
  float dframe = dt * GLOBAL_ANIM_FPS;
  state->currentGlobalFrame += dframe;

  update_animation(&state->upperAnim, dt);
  update_animation(&state->lowerAnim, dt);

  animate_model(state);
}

static float clamp_angle(float rad)
{
  if (rad > 2*PI) {
    return rad - 2 * PI * (int) (rad / (2*PI));
  } else if (rad < -2*PI) {
    return rad - 2 * PI * (int) (rad / (2*PI));
  }

  return rad;
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

  state->yrot_target += da;

  da = 0.0f;
  if (KEY_IS_DOWN(state->keyboard, KB_UP_ARROW)) {
    da += X_RAD_PER_SEC * dt;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_DOWN_ARROW)) {
    da -= X_RAD_PER_SEC * dt;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_LEFT_SHIFT)) {
    da *= 3.0;
  }

  state->xrot_target += da;

  if (MOUSE_BUTTON_IS_DOWN(state->mouse, MB_LEFT)) {
    state->xrot_target += state->mouse->frameDy * RAD(360.0f) / state->screenHeight * MOUSE_SENSE_Y;
    state->yrot_target -= state->mouse->frameDx * RAD(720.0f) / state->screenWidth * MOUSE_SENSE_X;
  }

  float clamped = clamp_angle(state->xrot_target);
  state->xrot += clamped - state->xrot_target;
  state->xrot_target = clamped;

  clamped = clamp_angle(state->yrot_target);
  state->yrot += clamped - state->yrot_target;
  state->yrot_target = clamped;

  float t = dt / 0.1f;
  state->xrot += (state->xrot_target - state->xrot) * t;
  state->yrot += (state->yrot_target - state->yrot) * t;

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
  float step = 1.0f;
  bool loop = false;

  if (KEY_IS_DOWN(state->keyboard, KB_LEFT_SHIFT)) {
    step = 15.0f;
    loop = true;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_Z)) {
    state->upperAnim.currentFrame -= step;
    if (state->upperAnim.currentFrame < state->upperAnim.startFrame) {
      if (loop) {
        state->upperAnim.currentFrame = state->upperAnim.endFrame;
      } else {
        state->upperAnim.currentFrame = state->upperAnim.startFrame;
      }
    }
    update_anim = true;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_X)) {
    state->upperAnim.currentFrame += step;
    if (state->upperAnim.currentFrame > state->upperAnim.endFrame) {
      if (loop) {
        state->upperAnim.currentFrame = state->upperAnim.startFrame;
      } else {
        state->upperAnim.currentFrame = state->upperAnim.endFrame;
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
    switch_animation(state, &state->upperAnim, -1);
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_W)) {
    switch_animation(state, &state->upperAnim, 1);
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

#if 0
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

  if (KEY_WAS_PRESSED(state->keyboard, KB_5)) {
    state->hairIdx--;
    if (state->hairIdx < MIN_HAIR_IDX) state->hairIdx = MAX_HAIR_IDX;
    appearanceChanged = true;
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_6)) {
    state->hairIdx++;
    if (state->hairIdx > MAX_HAIR_IDX) state->hairIdx = MIN_HAIR_IDX;
    appearanceChanged = true;
  }

  if (appearanceChanged) {
    set_character_appearance(state, state->appearance);
  }
#endif
}

#ifdef __ARCH_X86__

static inline void clear_buffer(DrawingBuffer *buffer, Vec4f color)
{
  uint32_t iterCount = (buffer->width * buffer->height) / 4;

  __m128i value = _mm_set1_epi32(rgba_color(color));
  __m128i *p = (__m128i *) buffer->pixels;

  while (iterCount--) {
    *p++ = value;
  }
}

#else

static inline void clear_buffer(DrawingBuffer *buffer, Vec4f color)
{
  uint32_t iterCount = buffer->width * buffer->height;
  Vec4f *p = (Vec4f *) buffer->pixels;

  while (iterCount--) {
    *p++ = color;
  }
}

#endif

#include <stdlib.h>

#define RANDOM(a, b) (b > 0 ? (rand() % b + a) : a)

DresserCharacterAppearance random_appearance(DresserCustomizationLimits lims)
{
  DresserCharacterAppearance result = {};
  result.skin_color = RANDOM(lims.skin_color[0], lims.skin_color[1]);
  result.face = RANDOM(lims.face[0], lims.face[1]);
  result.hair_color = RANDOM(lims.hair_color[0], lims.hair_color[1]);
  result.hair = RANDOM(lims.hair[0], lims.hair[1]);
  result.facial = RANDOM(lims.facial[0], lims.facial[1]);

  return result;
}

bool render_appearance_switcher(UIContext *ui, uint32_t *value, uint32_t min, uint32_t max, char *fmt)
{
  float h = 30.0f;

  ui_group_begin(ui, fmt);
  ui_layout_row_begin(ui, 215.0f, h);

  bool result = false;

  if (ui_button(ui, h, h, (uint8_t *) "<") == UI_BUTTON_RESULT_CLICKED) {
    *value = CLAMP_CYCLE((int32_t) *value - 1, (int32_t) min, (int32_t) max);
    result = true;
  };

  //ui_layout_pull_right(ui);
  if (ui_button(ui, h, h, (uint8_t *) ">") == UI_BUTTON_RESULT_CLICKED) {
    *value = CLAMP_CYCLE(*value + 1, min, max);
    result = true;
  }

  ui_layout_fill(ui);
  char buf[255];
  snprintf(buf, sizeof(buf) * sizeof(buf[0]), fmt, *value - min + 1);
  ui_label(ui, 0, h, (uint8_t *) &buf);

  ui_layout_row_end(ui);
  ui_group_end(ui);

  return result;
}

void render_character_ui(State *state, DresserCharacter *character)
{
  RenderingContext *ctx = &state->rendering_context;
  UIContext *ui = &state->ui;

  char buf[255] = {};

  uint32_t race = character->race;
  uint32_t sex = character->sex;

  ui_layout_row_begin(ui, 0.0f, 30.0f);
  if (ui_button(ui, 30.0f, 30.0f, (uint8_t *) "<") == UI_BUTTON_RESULT_CLICKED) {
    race = CLAMP_CYCLE((int32_t) race - 1, 0, (int32_t) state->dresser->races_count - 1);
  }
  if (ui_button(ui, 30.0f, 30.0f, (uint8_t *) ">") == UI_BUTTON_RESULT_CLICKED) {
    race = CLAMP_CYCLE(race + 1, 0, state->dresser->races_count - 1);
  }
  ui_label(ui, 139.0f, 30.0f, (uint8_t *) state->dresser->race_options[race].name);
  if (ui_button(ui, 100.0f, 30.0f, sex == 0 ? (uint8_t *) "Male" : (uint8_t *) "Female") == UI_BUTTON_RESULT_CLICKED) {
    sex = !sex;
  }
  ui_layout_row_end(ui);

  ui_layout_spacer(ui, 0.0f, 10.0f);

  // Upper body animation controls

  ui_group_begin(ui, (char *) "UBAC");
  ui_layout_row_begin(ui, 600.0f, 30.0f);

  ui_label(ui, 150.0f, 30.0f, (uint8_t *) "Upper body", UI_ALIGN_LEFT, UI_COLOR_NONE);

  if (ui_button(ui, 30.0f, 30.0f, (uint8_t *) "<") == UI_BUTTON_RESULT_CLICKED) {
    switch_animation(state, &state->upperAnim, -1);
  };

  if (ui_button(ui, 30.0f, 30.0f, (uint8_t *) ">") == UI_BUTTON_RESULT_CLICKED) {
    switch_animation(state, &state->upperAnim, 1);
  }

  ui_layout_pull_right(ui);

  if (ui_button(ui, 100.0f, 30.0f, (uint8_t *) (state->playing ? "Pause" : "Play")) == UI_BUTTON_RESULT_CLICKED) {
    state->playing = !state->playing;
  }

  ui_layout_fill(ui);
  snprintf(buf, sizeof(buf) * sizeof(buf[0]), "%s (%d)", state->upperAnim.name, state->upperAnim.id);
  ui_label(ui, 0, 30.0f, (uint8_t *) &buf, UI_ALIGN_CENTER);

  ui_layout_row_end(ui);
  ui_group_end(ui);

  // Lower body animation controls

  ui_group_begin(ui, (char *) "LBAC");
  ui_layout_row_begin(ui, 600.0f, 30.0f);

  ui_label(ui, 150.0f, 30.0f, (uint8_t *) "Lower body", UI_ALIGN_LEFT, UI_COLOR_NONE);

  ui_set_disabled(ui, !state->lower_anim_enabled);

  if (ui_button(ui, 30.0f, 30.0f, (uint8_t *) "<") == UI_BUTTON_RESULT_CLICKED) {
    switch_animation(state, &state->lowerAnim, -1);
  };

  if (ui_button(ui, 30.0f, 30.0f, (uint8_t *) ">") == UI_BUTTON_RESULT_CLICKED) {
    switch_animation(state, &state->lowerAnim, 1);
  }

  ui_layout_pull_right(ui);

  ui_set_disabled(ui, false);
  if (ui_button(ui, 100.0f, 30.0f, (uint8_t *) (state->lower_anim_enabled ? "Disable" : "Enable")) == UI_BUTTON_RESULT_CLICKED) {
    state->lower_anim_enabled = !state->lower_anim_enabled;
  }

  ui_set_disabled(ui, !state->lower_anim_enabled);

  ui_layout_fill(ui);
  snprintf(buf, sizeof(buf) * sizeof(buf[0]), "%s (%d)", state->lowerAnim.name, state->lowerAnim.id);
  ui_label(ui, 0, 30.0f, (uint8_t *) &buf, UI_ALIGN_CENTER);

  ui_set_disabled(ui, false);

  ui_layout_row_end(ui);
  ui_group_end(ui);

  ui_layout_spacer(ui, 0.0f, 10.0f);

  // Appearance controls

  bool changed = false;
  DresserCustomizationLimits *lims = &state->appearance_limits;
  DresserCharacterAppearance *app = &character->appearance;

  changed |= render_appearance_switcher(ui, &app->face, lims->face[0], lims->face[1], (char *) "Face %d");
  changed |= render_appearance_switcher(ui, &app->skin_color, lims->skin_color[0], lims->skin_color[1], (char *) "Skin %d");

  snprintf(buf, 255, "%s %%d", state->dresser->race_options[character->race].feature_names[character->sex][0]);
  changed |= render_appearance_switcher(ui, &app->hair, lims->hair[0], lims->hair[1], buf);

  snprintf(buf, 255, "%s color %%d", state->dresser->race_options[character->race].feature_names[character->sex][0]);
  changed |= render_appearance_switcher(ui, &app->hair_color, lims->hair_color[0], lims->hair_color[1], buf);

  snprintf(buf, 255, "%s %%d", state->dresser->race_options[character->race].feature_names[character->sex][1]);
  changed |= render_appearance_switcher(ui, &app->facial, lims->facial[0], lims->facial[1], buf);

  // Randomize button

  ui_layout_row_begin(ui, 215.0f, 30.0f);

  ui_layout_fill(ui);
  if (ui_button(ui, 0, 30.0f, (uint8_t *) "Randomize") == UI_BUTTON_RESULT_CLICKED) {
    character->appearance = random_appearance(state->appearance_limits);
    changed = true;
  };

  ui_layout_row_end(ui);

  ui_layout_row_begin(ui, 200.0f, 30.0f);
  snprintf(buf, 255, "%.3f MB", (float) state->loader.allocator.total_allocated / 1048576.0f);
  ui_label(ui, 0.0f, 30.0f, (uint8_t *) "Asset memory usage: ", UI_ALIGN_LEFT, UI_COLOR_NONE);
  ui_label(ui, 0.0f, 30.0f, (uint8_t *) buf);
  ui_layout_row_end(ui);

  ui_end(ui);

  if (race != character->race || sex != character->sex) {
    DresserCharacter *new_character = dresser_load_character(state->dresser, race, sex);
    if (new_character != NULL) {
      DresserCharacterAppearance new_appearance = dresser_get_default_character_appearance(state->dresser, new_character);
      dresser_set_character_appearance(state->dresser, new_character, &new_appearance);
      dresser_set_character_equipment(state->dresser, new_character, &character->equipment);

      set_creature(state, (DresserCreatureBase *) new_character);
    }
  } else if (changed) {
    dresser_set_character_appearance(state->dresser, character, &character->appearance);
    dresser_set_character_equipment(state->dresser, character, &character->equipment);
    animate_model(state);
  }
}

static void render_creature_ui(State *state, DresserCreature *creature)
{
  RenderingContext *ctx = &state->rendering_context;
  UIContext *ui = &state->ui;

  char buf[255] = {};

  // Upper body animation controls

  ui_group_begin(ui, (char *) "UBAC");
  ui_layout_row_begin(ui, 600.0f, 30.0f);

  ui_label(ui, 150.0f, 30.0f, (uint8_t *) "Upper body", UI_ALIGN_LEFT, UI_COLOR_NONE);

  if (ui_button(ui, 30.0f, 30.0f, (uint8_t *) "<") == UI_BUTTON_RESULT_CLICKED) {
    switch_animation(state, &state->upperAnim, -1);
  };

  if (ui_button(ui, 30.0f, 30.0f, (uint8_t *) ">") == UI_BUTTON_RESULT_CLICKED) {
    switch_animation(state, &state->upperAnim, 1);
  }

  ui_layout_pull_right(ui);

  if (ui_button(ui, 100.0f, 30.0f, (uint8_t *) (state->playing ? "Pause" : "Play")) == UI_BUTTON_RESULT_CLICKED) {
    state->playing = !state->playing;
  }

  ui_layout_fill(ui);
  snprintf(buf, sizeof(buf) * sizeof(buf[0]), "%s (%d)", state->upperAnim.name, state->upperAnim.id);
  ui_label(ui, 0, 30.0f, (uint8_t *) &buf, UI_ALIGN_CENTER);

  ui_layout_row_end(ui);
  ui_group_end(ui);
}

void render_ui(State *state)
{
  RenderingContext *ctx = &state->rendering_context;
  UIContext *ui = &state->ui;

  renderer_set_flags(ctx, RENDER_BLENDING | RENDER_SHADING);
  renderer_set_blend_mode(ctx, BLEND_MODE_DECAL);

  ui_begin(ui, 10.0f, 10.0f);

  char buf[255];
  snprintf(buf, 255, "X: %.03f, Y: %.03f", state->mouse->windowX, state->mouse->windowY);
  ui_layout_row_begin(ui, 0.0f, 30.0f);
  ui_label(ui, 0.0f, 30.0f, (uint8_t *) buf, UI_ALIGN_LEFT, UI_COLOR_NONE);
  ui_layout_row_end(ui);

  ui_group_begin(ui, (char *) "Creature");
  ui_layout_row_begin(ui, 600.0f, 30.0f);

  int32_t creature_idx = (int32_t) state->creature_index;

  if (ui_button(ui, 30.0f, 30.0f, (uint8_t *) "<") == UI_BUTTON_RESULT_CLICKED) {
    creature_idx = CLAMP_CYCLE((int32_t) creature_idx - 1, (int32_t) 0, (int32_t) (sizeof(creatures) / sizeof(creatures[0])) - 1);
  };
  if (ui_button(ui, 30.0f, 30.0f, (uint8_t *) ">") == UI_BUTTON_RESULT_CLICKED) {
    creature_idx = CLAMP_CYCLE(creature_idx + 1, 0, sizeof(creatures) / sizeof(creatures[0]) - 1);
  }

  ui_layout_fill(ui);
  ui_label(ui, 0, 30.0f, (uint8_t *) creatures[creature_idx].name, UI_ALIGN_CENTER);

  ui_layout_row_end(ui);
  ui_group_end(ui);

  if ((uint32_t) creature_idx != state->creature_index) {
    state->creature_index = creature_idx;
    set_creature(state, dresser_load_creature(state->dresser, creatures[state->creature_index].display_id));
  }

  if (state->creature == NULL) {
    return;
  }

  ui_layout_spacer(ui, 0.0f, 10.0f);

  if (state->creature->type == DCT_CHARACTER) {
    render_character_ui(state, (DresserCharacter *) state->creature);
  } else {
    render_creature_ui(state, (DresserCreature *) state->creature);
  }
}

static void render_creature(State *state, DresserCreatureBase *creature, RenderingContext *ctx)
{
  if (creature == NULL) {
    return;
  }

  float scale = state->scale * state->model_scale;
  Mat44 parent_mat = Mat44::rotate_y(-RAD(90)) * Mat44::scale(scale, scale, scale);
  ctx->model_mat = parent_mat;
  precalculate_matrices(ctx);

  render_m2_model(state, ctx, creature->model, RENDER_MODE_OPAQUE);
  render_m2_model(state, ctx, creature->model, RENDER_MODE_TRANSPARENT);

  for (size_t ii = 0; ii < creature->items_count; ii++) {
    M2Model *item = creature->items[ii];
    M2Attachment *att = creature->attachments[ii];

    if (item != NULL && att != NULL) {
      ModelBone bone = creature->model->bones[att->bone];
      Mat44 mat = Mat44::translate(att->offset.x, att->offset.y, att->offset.z) * bone.matrix;

      ctx->model_mat = mat * parent_mat;
      precalculate_matrices(ctx);

      render_m2_model(state, ctx, item, RENDER_MODE_OPAQUE);
      render_m2_model(state, ctx, item, RENDER_MODE_TRANSPARENT);
    }
  }

  if (state->showBones) {
    render_m2_model_bones(state, ctx, creature->model);
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
    state->mouse = global_state->mouse;

    initialize(state, drawing_buffer);
    animate_model(state);
  }

  if (state->keyboard->downedKeys[KB_ESCAPE]) {
    state->platform_api->terminate();
    return;
  }

  RenderingContext *ctx = &state->rendering_context;

  if (!state->shadowmap) {
    state->shadowmap = texture_create(state->main_arena, 512, 512);
    render_shadowmap(state, ctx, state->shadowmap);
  }

  if (state->playing) {
    update_animations(state, dt);
  }

  if (state->modelChanged) {
    if (state->render_flags.shadow_mapping) {
      render_shadowmap(state, ctx, state->shadowmap);
    }

    state->modelChanged = false;
  }

  update_camera(state, dt);
  handle_input(state);

  ctx->viewport_mat = viewport_matrix((float) state->screenWidth, (float) state->screenHeight, true);
  ctx->projection_mat = perspective_matrix(0.1f, 10.0f, state->fov);

  //ctx->view_mat = look_at_matrix((Vec3f){0, 0, 1}, (Vec3f){0, 0.35, 0}, (Vec3f){0, 1, 0});
  ctx->view_mat = Mat44::rotate_y(-state->yrot) *
                  Mat44::translate(0.0f, -0.5f, 0.0f) *
                  Mat44::rotate_x(-state->xrot) *
                  Mat44::translate(0.0f, 0.0f, -state->camDistance);

  set_target(ctx, state->buffer);
  renderer_enable(ctx, RENDER_ZTEST);

  clear_buffer(state->buffer, Vec4f(ctx->clear_color, 0.0f));
  clear_zbuffer(ctx);

  render_floor(state, ctx);

  render_creature(state, state->creature, ctx);

  if (state->showUnitAxes) {
    render_unit_axes(ctx);
  }

  // Render 2D elements
  enable_ortho(state, ctx);

  // if (state->render_flags.shadow_mapping) {
  //   render_debug_texture(state, ctx, state->shadowmap, 10, 10, 400);
  // }

  if (state->debugTexture) {
    renderer_set_flags(ctx, RENDER_BLENDING);
    renderer_set_blend_mode(ctx, BLEND_MODE_DECAL);

    render_debug_texture(state, ctx, state->debugTexture, 10, state->ui.y + 8, 400);
  }

  if (state->debugTexture2) {
    renderer_set_flags(ctx, RENDER_BLENDING);
    renderer_set_blend_mode(ctx, BLEND_MODE_DECAL);

    render_debug_texture(state, ctx, state->debugTexture2, 1180, 410, 400);
  }

  render_ui(state);
}

// #ifdef PLATFORM_WINDOWS

// BOOLEAN WINAPI DllMain(IN HINSTANCE hDllHandle,
//                        IN DWORD     nReason,
//                        IN LPVOID    Reserved)
// {
//   switch (nReason) {
//     case DLL_PROCESS_ATTACH:
//       DisableThreadLibraryCalls(hDllHandle);
//       break;

//     case DLL_PROCESS_DETACH:
//       break;
//   }

//   return true;
// }
// #endif
