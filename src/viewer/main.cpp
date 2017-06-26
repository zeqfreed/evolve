#include "platform/platform.h"

#include "renderer/math.cpp"
#include "renderer/tga.cpp"
#include "renderer/renderer.cpp"
#include "utils/memory.cpp"
#include "utils/assets.cpp"

#include "model.cpp"
#include "m2.cpp"

typedef struct RenderFlags {
  bool gouraud_shading;
  bool texture_mapping;
  bool normal_mapping;
  bool shadow_mapping;
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
  RenderingContext rendering_context;
  Mat44 matShadowMVP;

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

  uint32_t animId;
  uint32_t startFrame;
  uint32_t endFrame;
  uint32_t currentFrame;
  double currentFracFrame;
  double currentAnimFPS;
  bool playing;
  bool showBones;
  bool modelChanged;
} State;

static Vec3f hsv_to_rgb(Vec3f hsv)
{
  float h = hsv.x;
  float s = hsv.y;
  float v = hsv.z;

  h = fmod(h / 60.0, 6);
  float c = v * s;
  float x = c * (1 - fabs(fmod(h, 2) - 1));  
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

  return (Vec3f){r, g, b};
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
  int texX = (int)((uv.x * texture->width)) & (texture->width - 1);
  int texY = (int)((uv.y * texture->height)) & (texture->height - 1);

  Vec3f tcolor;
  if (f->texture_mapping) {
    tcolor = texture->pixels[texY*texture->width+texX];
  } else {
    tcolor = d->color;
  }

  if (f->normal_mapping && d->normalmap) {
    int nx = (int)(uv.x * d->normalmap->width) & (d->normalmap->width - 1);
    int ny = (int)(uv.y * d->normalmap->height) & (d->normalmap->height - 1);
    Vec3f ncolor = d->normalmap->pixels[ny*d->normalmap->width+nx];
    Vec3f tnormal = (Vec3f){2 * ncolor.r - 1, 2 * ncolor.g - 1, 2 * ncolor.b};

    Vec3f dp1 = d->pos[1] - d->pos[0];
    Vec3f dp2 = d->pos[2] - d->pos[1];
    Vec3f duv1 = d->uvs[1] - d->uvs[0];
    Vec3f duv2 = d->uvs[2] - d->uvs[1];
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

  if (f->lighting) {
    intensity = normal.dot(-ctx->light);
  } else {
    intensity = 1.0;
  }

  if (intensity < 0.0) {
    intensity = 0.0;
  }

  Vec3f ambient = tcolor * 0.3;
  *color = (ambient + tcolor * intensity).clamped();

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
  int texX = uvz.x * z * 5;
  int texY = uvz.y * z * 5;

  Vec3f tcolor;
  if (texX % 2 == texY % 2) {
    tcolor = (Vec3f){1, 1, 1};
  } else {
    tcolor = (Vec3f){0.5, 0.5, 0.5};
  }

  Vec3f vcolor = (Vec3f){d->colors[0].r * t0 + d->colors[1].r * t1 + d->colors[2].r * t2,
                         d->colors[0].g * t0 + d->colors[1].g * t1 + d->colors[2].g * t2,
                         d->colors[0].b * t0 + d->colors[1].b * t1 + d->colors[2].b * t2} * z;

  if (f->shadow_mapping) {
    Vec3f pos = d->pos[0] * t0 + d->pos[1] * t1 + d->pos[2] * t2;
    Vec3f shadow = pos * d->matShadow;
    int shx = shadow.x;
    int shy = shadow.y;

    if (shadow.x >= 0 && shadow.y >= 0 &&
        shadow.x < shadowmap->width && shadow.y < shadowmap->height) {
      float shz = 1 - shadow.z;
      Vec3f shval = shadowmap->pixels[shy * shadowmap->width + shx];

      if (shval.x > 0) {
        intensity = 0.2;
      }
    }
  }

  *color = ((Vec3f){vcolor.r * tcolor.r, vcolor.g * tcolor.g, vcolor.b * tcolor.b}).clamped() * intensity;

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
  Vec3f tcolor = d->texture->pixels[v * d->texture->width + u];

  *color = (Vec3f){tcolor.r, tcolor.g, tcolor.b};

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
  
  Texture *texture = (Texture *) state->main_arena->allocate(sizeof(Texture));
  texture->width = header->width;
  texture->height = header->height;
  texture->pixels = (Vec3f *) state->main_arena->allocate(sizeof(Vec3f) * texture->width * texture->height);

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
  shader_data.normal = (Vec3f){0, 1, 0};

  Vec3f positions[3];

  for (int tri = 0; tri < 2; tri++) {
    for (int i = 0; i < 3; i++) {
      int idx = triangles[tri][i];

      Vec3f cam_pos = vertices[idx][0] * ctx->modelview_mat;
      shader_data.pos[i] = cam_pos * ctx->projection_mat;
      positions[i] = vertices[idx][0] * ctx->mvp_mat;

      float iz = 1 / cam_pos.z;
      Vec3f tex = vertices[idx][1];
      shader_data.uvzs[i] = (Vec3f){tex.x * iz, tex.y * iz, iz};
      shader_data.colors[i] = colors[tri][i] * iz;
    }

    draw_triangle(ctx, &fragment_floor, (void *) &shader_data, positions[0], positions[1], positions[2]);
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
      shader_data.uvs[vi] = (Vec3f){texture.x, texture.y, 0};
      shader_data.normals[vi] = (normal * ctx->normal_mat).normalized();

      positions[vi] = position * ctx->mvp_mat;
    }

    draw_triangle(ctx, &fragment_model, (void *) &shader_data, positions[0], positions[1], positions[2]);
  }
}

static void render_m2_model(State *state, RenderingContext *ctx, M2Model *model)
{
  ctx->model_mat = Mat44::rotate_y(-RAD(90)) * Mat44::scale(state->scale, state->scale, state->scale);
  precalculate_matrices(ctx);

  ModelShaderData shader_data = {};
  shader_data.color = hsv_to_rgb(state->hsv);
  shader_data.flags = &state->render_flags;
  Vec3f positions[3];

  for (int rpi = 0; rpi < model->renderPassesCount; rpi++) {
    M2RenderPass pass = model->renderPasses[rpi];
    ModelSubmesh submesh = model->submeshes[pass.submesh];
    if (!submesh.enabled) {
      continue;
    }

    shader_data.texture = state->textures[pass.textureId];

    uint32_t faceStart = submesh.facesStart;
    uint32_t faceEnd = faceStart + submesh.facesCount;

    for (int fi = faceStart; fi < faceEnd; fi++) {
      M2Face face = model->faces[fi];

      for (int vi = 0; vi < 3; vi++) {
        Vec3f position = model->animatedPositions[face.indices[vi]];
        Vec3f normal = model->animatedNormals[face.indices[vi]];
        Vec3f texture = model->textureCoords[face.indices[vi]];

        shader_data.pos[vi] = position * ctx->model_mat;
        shader_data.uvs[vi] = (Vec3f){texture.x, texture.y, 0};
        shader_data.normals[vi] = (normal * ctx->normal_mat).normalized();

        positions[vi] = position * ctx->mvp_mat;
      }

      draw_triangle(ctx, &fragment_model, (void *) &shader_data, positions[0], positions[1], positions[2]);
    }
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

    Vec3f color = GREEN;

    if (bone.parent >= 0) {
      ModelBone parentBone = model->bones[bone.parent];
      Vec3f p1 = bone.pivot * bone.matrix;
      Vec3f p2 = parentBone.pivot * parentBone.matrix;
      render_line(ctx, p1, p2, color);
    } else {
      Vec3f pos = bone.pivot * bone.matrix * mat;
      set_pixel(ctx->target, pos.x, pos.y, color);
    }
  }
}

static void render_debug_texture(State *state, RenderingContext *ctx, Texture *texture, float x, float y, float width)
{
  ctx->viewport_mat = viewport_matrix(state->screenWidth, state->screenHeight, false);
  ctx->projection_mat = orthographic_matrix(0.1, 100, 0, state->screenHeight, state->screenWidth, 0);
  ctx->view_mat = Mat44::identity();
  ctx->model_mat = Mat44::translate(0, 0, 0);
  precalculate_matrices(ctx);

  const Vec3f texture_coords[4] = {
    {0, 0, 0},
    {texture->width, 0, 0},
    {texture->width, texture->height, 0},
    {0, texture->height, 0}
  };

  float height = width * ((float) texture->height / (float) texture->width);

  Vec3f positions[4];
  positions[0] = (Vec3f){x, y + height, 0.0} * ctx->mvp_mat;
  positions[1] = (Vec3f){x + width, y + height, 0.0} * ctx->mvp_mat;
  positions[2] = (Vec3f){x + width, y, 0.0} * ctx->mvp_mat;
  positions[3] = (Vec3f){x, y, 0.0} * ctx->mvp_mat;

  DebugShaderData shader_data = {};
  shader_data.clampu = texture->width - 1;
  shader_data.clampv = texture->height - 1;
  shader_data.texture = texture;

  shader_data.uv0 = texture_coords[0];
  shader_data.duv[0] = texture_coords[1] - shader_data.uv0;
  shader_data.duv[1] = texture_coords[2] - shader_data.uv0;
  draw_triangle(ctx, &fragment_debug, (void *) &shader_data, positions[0], positions[1], positions[2]);

  shader_data.duv[0] = texture_coords[2] - shader_data.uv0;
  shader_data.duv[1] = texture_coords[3] - shader_data.uv0;
  draw_triangle(ctx, &fragment_debug, (void *) &shader_data, positions[0], positions[2], positions[3]);
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

static void animate_model(State *state)
{
  m2_animate_vertices(state->m2model, state->animId, state->currentFrame);
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
    1101, // Default pants
    1301, // Default pants
    1501 // Default back/cloak
  };

  for (int si = 0; si < model->submeshesCount; si++) {
    for (int esi = 0; esi < sizeof(enabledSubmeshes) / sizeof(enabledSubmeshes[0]); esi++) {
      if (model->submeshes[si].id == enabledSubmeshes[esi]) {
        model->submeshes[si].enabled = true;
      }
    }

    if (si == 34) {
      // Hack to disable eye glow submesh; Remove when alpha blending is supported
      model->submeshes[si].enabled = false;
    }
  }
}

static void initialize(State *state, DrawingBuffer *buffer)
{
  RenderingContext *ctx = &state->rendering_context;
  ctx->target = buffer;
  ctx->clear_color = BLACK;

  ctx->model_mat = Mat44::identity();
  ctx->viewport_mat = viewport_matrix(buffer->width, buffer->height, true);
  ctx->projection_mat = perspective_matrix(0.1, 10, 60);
  ctx->light = ((Vec3f){0, -1, 0}).normalized();

  ctx->zbuffer = (zval_t *) state->main_arena->allocate(buffer->width * buffer->height * sizeof(zval_t));

  char *basename = (char *) "misc/man";
  char model_filename[255];
  char diffuse_filename[255];
  char normal_filename[255];
  snprintf(model_filename, 255, (char *) "data/%s.obj", basename);
  snprintf(diffuse_filename, 255, (char *) "data/%s_D.tga", basename);
  snprintf(normal_filename, 255, (char *) "data/%s_N.tga", basename);

  state->screenWidth = buffer->width;
  state->screenHeight = buffer->height;

  load_m2_model(state, (char *) "data/misc/nelf.m2");
  state->textures[0] = load_texture(state, (char *) "data/misc/nelf_0.tga");
  state->textures[1] = load_texture(state, (char *) "data/misc/nelf_1.tga");
  state->textures[2] = load_texture(state, (char *) "data/misc/cape.tga");
  state->textures[3] = load_texture(state, (char *) "data/misc/nelf_3.tga");

  // load_m2_model(state, (char *) "data/misc/dwarf.m2");
  // state->textures[0] = load_texture(state, (char *) "data/misc/dwarf_0.tga");
  // state->textures[1] = load_texture(state, (char *) "data/misc/dwarf_1.tga");
  // state->textures[2] = load_texture(state, (char *) "data/misc/nelf_cape.tga");
  // state->textures[3] = load_texture(state, (char *) "data/misc/nelf_eye_glow.tga");  

  // load_m2_model(state, (char *) "data/misc/diablo.m2");
  // state->textures[0] = load_texture(state, (char *) "data/misc/diablo_D.tga");

  // load_m2_model(state, (char *) "data/misc/cat.m2");
  // state->textures[0] = load_texture(state, (char *) "data/misc/cat_0.tga");

  // load_m2_model(state, (char *) "data/misc/riding_horse.m2");
  // state->textures[0] = load_texture(state, (char *) "data/misc/riding_horse_0.tga");
  // state->textures[1] = load_texture(state, (char *) "data/misc/riding_horse_1.tga");

  state->xRot = RAD(15.0);
  state->yRot = 0.0;
  state->fov = 60.0;
  state->camDistance = 1.0;

  memset(&state->render_flags, 1, sizeof(state->render_flags)); // Set all flags
  state->hsv = (Vec3f){260.0, 0.33, 1.0};
  state->sat_deg = RAD(109);
  
  state->hair = 6;
  state->scale = 0.4;

  state->playing = true;
  state->showBones = false;
  state->modelChanged = true;
  switch_animation(state, 24);
  enable_submeshes(state);

  state->temp_arena->discard();
}

static void render_shadowmap(State *state, RenderingContext *ctx, Model *model, Texture *shadowmap)
{
  DrawingBuffer dummyTarget = *ctx->target;
  dummyTarget.width = shadowmap->width;
  dummyTarget.height = shadowmap->height;

  RenderingContext subctx = {};

  subctx.target = &dummyTarget;
  subctx.clear_color = BLACK;

  subctx.model_mat = Mat44::identity();
  subctx.viewport_mat = viewport_matrix(shadowmap->width, shadowmap->height, true);
  subctx.projection_mat = orthographic_matrix(0.1, 10, -1, -1, 1, 1);

  ASSERT(shadowmap->width * shadowmap->height < ctx->target->width * ctx->target->height); // Check we won't overflow zbuffer
  subctx.zbuffer = ctx->zbuffer;

  // HACK: Multiplication by 5 so camera doesn't end up inside geometry
  subctx.view_mat = look_at_matrix(ctx->light * 5, (Vec3f){0, 0, 0}, (Vec3f){0, 1, 0});
  precalculate_matrices(&subctx);

  state->matShadowMVP = subctx.modelview_mat * subctx.projection_mat * subctx.viewport_mat;

  clear_zbuffer(&subctx);
  render_m2_model(state, &subctx, state->m2model);

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
#define CAM_DIST_PER_SEC 1.0
#define HUE_PER_SEC 120.0
#define SATURATION_PER_SEC RAD(120.0)

static bool update_animation(State *state, float dt)
{
  state->currentFracFrame += dt * state->currentAnimFPS;

  bool result = false;

  uint32_t frame = (uint32_t) state->currentFracFrame;
  if (frame != state->currentFrame) {
    result = true;
    state->currentFrame = frame;
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
    state->scale = CLAMP(state->scale, 0.1, 5.0);
  } else {
    state->camDistance += da;
    state->camDistance = CLAMP(state->camDistance, 1.0, 3.0);
  }

  if (KEY_IS_DOWN(state->keyboard, KB_H)) {
    state->hsv.x += HUE_PER_SEC * dt;
  }

  if (state->hsv.x > 360.0) {
    state->hsv.x -= 360.0;
  }

  if (KEY_IS_DOWN(state->keyboard, KB_C)) {
    state->sat_deg += SATURATION_PER_SEC * dt;
  }

  if (state->sat_deg > RAD(360.0)) {
    state->sat_deg -= RAD(360.0);
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

  state->hsv.y = cos(state->sat_deg) * 0.5 + 0.5; // map to 0..1
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
}

C_LINKAGE void draw_frame(GlobalState *global_state, DrawingBuffer *drawing_buffer, float dt)
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
    state->shadowmap = (Texture *) state->main_arena->allocate(sizeof(Texture));
    state->shadowmap->width = 512;
    state->shadowmap->height = 512;
    state->shadowmap->pixels = (Vec3f *) state->main_arena->allocate(sizeof(Vec3f) * state->shadowmap->width * state->shadowmap->height);

    ctx->light = ((Vec3f){0.5, 1, 0.5}).normalized();
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

  ctx->viewport_mat = viewport_matrix(state->screenWidth, state->screenHeight, true);
  ctx->projection_mat = perspective_matrix(0.1, 10, state->fov);

  //ctx->view_mat = look_at_matrix((Vec3f){0, 0, 1}, (Vec3f){0, 0.35, 0}, (Vec3f){0, 1, 0});
  ctx->view_mat = Mat44::rotate_y(-state->yRot) *
                  Mat44::translate(0, -0.5, 0) *
                  Mat44::rotate_x(-state->xRot) *
                  Mat44::translate(0, 0, -state->camDistance);

  Quaternion q1 = Quaternion::axisAngle((Vec3f){0, 1, 0}, state->yRot);
  //Vec3f newx = q1 * (Vec3f){1, 0, 0} * q1.conjugate();
  Quaternion q2 = Quaternion::axisAngle((Vec3f){1, 0, 0}, state->xRot);
  Quaternion q3 = q1 * q2;
  Mat44 mat = Mat44::from_quaternion(q3);
  Vec3f p1 = (Vec3f){0, 0, 1} * mat;
  Vec3f p2 = (q2 * (Vec3f){0, 0, 1} * q2.conjugate());
  Vec3f p3 = (q3 * (Vec3f){0, 0, 1} * q3.conjugate());
  //ctx->view_mat = look_at_matrix((Vec3f){0, 1, 1}, (Vec3f){0, 0, 0}, (Vec3f){0, 1, 0});

  clear_buffer(ctx);
  clear_zbuffer(ctx);

  render_floor(state, ctx);
  //render_model(state, ctx, state->model);

  render_m2_model(state, ctx, state->m2model);

  if (state->showBones) {
   render_m2_model_bones(state, ctx, state->m2model);
  }
  // render_unit_axes(ctx);

  //render_line(ctx, p1, (Vec3f){0, 0, 0}, BLUE);
  // render_line(ctx, p2, (Vec3f){0, 0, 0}, RED);
  // render_line(ctx, p3, (Vec3f){0, 0, 0}, WHITE);

  if (state->render_flags.shadow_mapping) {
    render_debug_texture(state, ctx, state->shadowmap, 10, 10, 400);
  }
}
