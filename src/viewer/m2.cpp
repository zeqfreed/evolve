#include <cstdio>
#include <cstdint>

#include "m2.h"

typedef void (ModelLoadAnimValueFunc)(void *, void *);
typedef void (ModelDumpAnimValueFunc)(uint32_t, void *);

static void m2_dump_header(M2Header *header, void *bytes)
{
  printf("%c%c%c%c (version %d)\n", header->magic[0], header->magic[1], header->magic[2], header->magic[3], header->version);

  char *name = &((char *) bytes)[header->nameOffset]; // TODO: use snprintf to avoid overflow
  printf("Model name: %s\n", name);

  const char *entries[] = {
    "GlobalSequences", "Animations", "AnimationLookups", "PlayableAnimLookups",
    "Bones", "KeyBoneLookups", "Vertices", "Views", "Colors", "Textures", "Transparencies",
    "Reserved", "TextureAnimations", "TextureReplacements", "RenderFlags", "BoneLookups", "TextureLookups",
    "TextureUnitLookups"
  };

  uint32_t *h = (uint32_t *) &header->globalSequencesCount;
  for (int i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
    uint32_t count = h[i * 2];
    size_t offset = h[i * 2 + 1];
    printf("%s: %d entries at %p\n", entries[i], count, (void *) offset);
  }
}

static void m2_dump_view(M2View *view)
{
  printf("View %p\n", view);

  const char *entries[] = {
    "Indices", "Faces", "Props", "Submeshes", "Textures"
  };

  uint32_t *v = (uint32_t *) &view->indicesCount;
  for (int i = 0; i < 5; i++) {
    uint32_t count = v[i * 2];
    size_t offset = v[i * 2 + 1];
    printf("%s: %d entries at %p\n", entries[i], count, (void *) offset);
  }
}

static void m2_dump_texture(M2Texture *tex, void *bytes)
{
  printf("Texture %p\n", tex);
  printf("Type: %d\n", tex->type);
  printf("Flags: %x\n", tex->flags);

  char *filename = (char *) ((uint8_t *) bytes + tex->filenameOffset);
  printf("Filename length: %d\n", tex->filenameLength);
  printf("Filename: %s\n", filename);
}

static void m2_dump_render_pass(M2RenderPass *pass, void *bytes, M2Header *header)
{
  // uint16_t *textureLookups = (uint16_t *) ((uint8_t *) bytes + header->textureLookupsOffset);
  // ASSERT(pass->textureId < header->texureLookupsCount);

  printf("Render pass %p\n", pass);
  printf("Flags: 0x%x; Shader flags: 0x%x\n", pass->flags, pass->shaderFlags);
  printf("Submesh: %d\n", pass->submesh);
  printf("Texture ID: %d\n", pass->textureId);
  printf("Tex. unit: %d, transp.: %d, tex. anim: %d\n", pass->textureUnitIndex, pass->transparencyIndex, pass->textureAnimationIndex);
}

static void m2_dump_geoset(M2Geoset *geoset)
{
  printf("Geoset %d\n", geoset->id);

  const char *fields[] = {
    "verticesStart", "verticesCount", "indicesStart",
    "indicesCount", "skinnedBonesCount", "bonesStart",
    "rootBone", "bonesCount"
  };

  uint16_t *f = (uint16_t *) &geoset->verticesStart;
  for (int i = 0; i < 8; i++) {
    uint16_t value = f[i];
    printf("%s: %d\n", fields[i], value);
  }

  printf("BBox (?): %.3f %.3f %.3f\n", geoset->boundingBox.x, geoset->boundingBox.y, geoset->boundingBox.z);
  printf("\n");
}

static void m2_dump_bone(M2Bone *bone)
{
  printf("Bone %p\n", bone);
  printf("Keybone: %d\nParent: %d\nFlags: %d\nGeoset ID: %d\n", bone->keybone, bone->parent, bone->flags, bone->geoid);
  printf("Pivot: %.3f %.3f %.3f\n", bone->pivot.x, bone->pivot.y, bone->pivot.z);

  M2AnimationBlock *trans = &bone->translation;
  M2AnimationBlock *rot = &bone->rotation;
  M2AnimationBlock *scale = &bone->scaling;

  printf("Translation:\n"
          "\tinterpolation: %d, global seq: %d; lookups count: %d, offset: 0x%x\n"
          "\t%d timestamps (0x%x), %d keyframes (0x%x)\n",
         trans->interpolationType, trans->globalSequence,
         trans->lookupsCount, trans->lookupsOffset,
         trans->timestampsCount, trans->timestampsOffset,
         trans->keyframesCount, trans->keyframesOffset);

  printf("Rotation:\n"
         "\tinterpolation: %d, global seq: %d; lookups count: %d, offset: 0x%x\n"
         "\t%d timestamps (0x%x), %d keyframes (0x%x)\n",
         rot->interpolationType, rot->globalSequence,
         rot->lookupsCount, rot->lookupsOffset,
         rot->timestampsCount, rot->timestampsOffset,
         rot->keyframesCount, rot->keyframesOffset);

  printf("Scaling:\n"
         "\tinterpolation: %d, global seq: %d; count: %d, offset: 0x%x\n"
         "\t%d timestamps (0x%x), %d keyframes (0x%x)\n",
         scale->interpolationType, scale->globalSequence,
         scale->lookupsCount, scale->lookupsOffset,
         scale->timestampsCount, scale->timestampsOffset,
         scale->keyframesCount, scale->keyframesOffset);
}

static void m2_dump_animation_block(M2Header *header, void *bytes, M2AnimationBlock *block, uint32_t elSize, ModelDumpAnimValueFunc *func)
{
  printf("Animation block %p\n", block);

  uint32_t *timestamps = (uint32_t *) ((uint8_t *) bytes + block->timestampsOffset);
  void *data = (void *) ((uint8_t *) bytes + block->keyframesOffset);

  for (uint32_t i = 0; i < block->keyframesCount; i++) {
    uint32_t timestamp = timestamps[i];
    func(timestamp, (uint8_t *) data + elSize * i);
  }
}

static void m2_dump_animation(M2Animation *animation)
{
  printf("Animation %p (id: %d)\n", animation, animation->id);
  printf("Start: %d; End: %d\n", animation->timeStart, animation->timeEnd);
  printf("Move speed: %.3f\n", animation->moveSpeed);
  printf("Flags: %d\n", animation->flags);
  printf("Radius: %.3f\n", animation->radius);
  printf("Next: %d; Alias of: %d\n\n", animation->nextAnimation, animation->aliasedAnimation);
}

void m2_load_translation(void *src, void *dst)
{
  static Mat44 worldMat = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  Vec3f *srcVal = (Vec3f *) src;
  Vec3f *dstVal = (Vec3f *) dst;
  *dstVal = *srcVal * worldMat;
}

void m2_load_rotation(void *src, void *dst)
{
  static Mat44 worldMat = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  Quaternion *srcVal = (Quaternion *) src;
  Quaternion *dstVal = (Quaternion *) dst;

  dstVal->v = srcVal->v * worldMat;
  dstVal->w = srcVal->w;
}

void m2_load_scaling(void *src, void *dst)
{
  static Mat44 worldMat = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  Vec3f *srcVal = (Vec3f *) src;
  Vec3f *dstVal = (Vec3f *) dst;
  *dstVal = *srcVal * worldMat;
}

void m2_dump_anim_vec3f(uint32_t timestamp, void *value)
{
  Vec3f *vec = (Vec3f *) value;
  printf("%d\t%.03f %.03f %.03f\n", timestamp, vec->x, vec->y, vec->z);
}

void m2_dump_anim_quaternion(uint32_t timestamp, void *value)
{
  Quaternion *q = (Quaternion *) value;
  printf("%d\t%.03f %.03f %.03f %.03f\n", timestamp, q->x, q->y, q->z, q->w);
}

void m2_load_animation_data(void *bytes, M2Header *header, MemoryArena *arena, ModelAnimationData *data, M2AnimationBlock *block, uint32_t elSize, ModelLoadAnimValueFunc *func)
{
    ASSERT(block->timestampsCount == block->keyframesCount);

    if (block->timestampsCount == 0) {
      data->animationsCount = 0;
      data->keyframesCount = 0;
      return;
    }

    data->interpolationType = (ModelInterpolationType) block->interpolationType;
    ASSERT(data->interpolationType < 2); // TODO: Support Hermite and Bezier interpolations

    if (block->globalSequence >= 0) {
      uint32_t *globalSequences = (uint32_t *) ((uint8_t *) bytes + header->globalSequencesOffset);

      data->isGlobal = true;
      data->animationsCount = 0;
      data->animationRanges = (ModelAnimationRange *) arena->allocate(sizeof(ModelAnimationRange));
      data->animationRanges[0].start = 0;
      data->animationRanges[0].end = globalSequences[block->globalSequence];

    } else if (block->lookupsCount > 0) {
      uint32_t *ranges = (uint32_t *) ((uint8_t *) bytes + block->lookupsOffset);

      data->isGlobal = false;
      data->animationsCount = block->lookupsCount;
      data->animationRanges = (ModelAnimationRange *) arena->allocate(sizeof(ModelAnimationRange) * block->lookupsCount);

      for (uint32_t li = 0; li < block->lookupsCount; li++) {
        ModelAnimationRange range;
        range.start = ranges[li * 2];
        range.end = ranges[li * 2 + 1];
        data->animationRanges[li] = range;
      }
    }

    uint32_t *timestamps = (uint32_t *) ((uint8_t *) bytes + block->timestampsOffset);
    void *values = (void *) ((uint8_t *) bytes + block->keyframesOffset);

    data->keyframesCount = block->keyframesCount;
    data->timestamps = (uint32_t *) arena->allocate(sizeof(uint32_t) * block->keyframesCount);
    data->data = arena->allocate(elSize * block->keyframesCount);

    for (uint32_t ki = 0; ki < block->keyframesCount; ki++) {
      uint32_t ts = timestamps[ki];
      data->timestamps[ki] = ts;

      func((uint8_t *) values + elSize * ki, (uint8_t *) data->data + elSize * ki);
    }
}

M2Model *m2_load(void *bytes, size_t size, MemoryArena *arena)
{
  M2Header *header = (M2Header *) bytes;
  m2_dump_header(header, bytes);

  // uint16_t *texLookups = (uint16_t *) ((uint8_t *) bytes + header->textureLookupsOffset);
  // for (int ti = 0; ti < header->textureLookupsCount; ti++) {
  //   printf("Texture lookup %d: %d\n", ti, texLookups[ti]);
  // }

  // uint16_t *texUnitLookups = (uint16_t *) ((uint8_t *) bytes + header->textureUnitLookupsOffset);
  // for (int tui = 0; tui < header->textureUnitLookupsCount; tui++) {
  //   printf("Texture unit lookup %d: %d\n", tui, texUnitLookups[tui]);
  // }

  M2Model *model = (M2Model *) arena->allocate(sizeof(M2Model));
  model->verticesCount = header->verticesCount;
  model->positions = (Vec3f *) arena->allocate(sizeof(Vec3f) * header->verticesCount);
  model->animatedPositions = (Vec3f *) arena->allocate(sizeof(Vec3f) * header->verticesCount);
  model->normals = (Vec3f *) arena->allocate(sizeof(Vec3f) * header->verticesCount);
  model->animatedNormals = (Vec3f *) arena->allocate(sizeof(Vec3f) * header->verticesCount);
  model->textureCoords = (Vec3f *) arena->allocate(sizeof(Vec3f) * header->verticesCount);

  model->weightsPerVertex = 4;
  model->weights = (ModelVertexWeight *) arena->allocate(sizeof(ModelVertexWeight) * model->weightsPerVertex * header->verticesCount);

  static Mat44 worldMat = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  M2Vertex *vertices = (M2Vertex *) ((uint8_t *) bytes + header->verticesOffset);
  for(int i = 0; i < header->verticesCount; i++) {
    model->positions[i] = vertices[i].pos * worldMat;
    model->animatedPositions[i] = model->positions[i];
    model->normals[i] = -(vertices[i].normal * worldMat);
    model->animatedNormals[i] = model->normals[i];
    model->textureCoords[i] = {vertices[i].texcoords[0], vertices[i].texcoords[1], 0.0f};

    for (int wi = 0; wi < model->weightsPerVertex; wi++) {
      ModelVertexWeight vw = {};
      vw.bone = vertices[i].bones[wi];
      vw.weight = (float) vertices[i].weights[wi] / 255.0f;
      model->weights[i * 4 + wi] = vw;
    }
  }

  M2View *view = (M2View *) ((uint8_t *) bytes + header->viewsOffset);
  m2_dump_view(view);

  model->renderPassesCount = view->renderPassesCount;
  model->renderPasses = (M2RenderPass *) arena->allocate(sizeof(M2RenderPass) * view->renderPassesCount);

  model->renderFlagsCount = header->renderFlagsCount;
  model->renderFlags = (M2RenderFlag *) arena->allocate(sizeof(M2RenderFlag) * header->renderFlagsCount);
  M2RenderFlag *renderFlags = (M2RenderFlag *) ((uint8_t *) bytes + header->renderFlagsOffset);
  for (int rfi = 0; rfi < header->renderFlagsCount; rfi++) {
    model->renderFlags[rfi] = renderFlags[rfi];
    // printf("Render flag %d: f = %d; b = %d\n", rfi, renderFlags[rfi].flags, renderFlags[rfi].blendingMode);
  }

  M2RenderPass *renderPasses = (M2RenderPass *) ((uint8_t *) bytes + view->renderPassesOffset);
  for (int rpi = 0; rpi < view->renderPassesCount; rpi++) {
    //m2_dump_render_pass(&renderPasses[rpi], bytes, header);
    model->renderPasses[rpi] = renderPasses[rpi];
    // printf("Render pass %d: flags: %d; submesh: %d, render flags index = %d\n", rpi, renderPasses[rpi].flags, renderPasses[rpi].submesh, renderPasses[rpi].renderFlagIndex);
  }

  // M2Texture *textures = (M2Texture *) ((uint8_t *) bytes + header->texturesOffset);
  // for (int ti = 0; ti < header->texturesCount; ti++) {
  //   m2_dump_texture(&textures[ti], bytes);
  // }

  M2Geoset *geosets = (M2Geoset *) ((uint8_t *) bytes + view->submeshesOffset);
  model->submeshesCount = view->submeshesCount;
  model->submeshes = (ModelSubmesh *) arena->allocate(sizeof(ModelSubmesh) * view->submeshesCount);
  for (int i = 0; i < view->submeshesCount; i++) {
    //m2_dump_geoset(&geosets[i]);
    ModelSubmesh submesh;
    submesh.id = geosets[i].id;
    submesh.verticesStart = geosets[i].verticesStart;
    submesh.verticesCount = geosets[i].verticesCount;
    submesh.facesStart = geosets[i].indicesStart / 3;
    submesh.facesCount = geosets[i].indicesCount / 3;
    submesh.enabled = (submesh.id == 0); // By default enable only parts without alternatives

    model->submeshes[i] = submesh;
  }

  uint16_t *indicesLookup = (uint16_t *) ((uint8_t *) bytes + view->indicesOffset);
  uint16_t *faces = (uint16_t *) ((uint8_t *) bytes + view->facesOffset);

  uint32_t facesCount = view->facesCount / 3;
  model->facesCount = facesCount;
  model->faces = (M2Face *) arena->allocate(sizeof(M2Face) * facesCount);
  for (int i = 0; i < facesCount; i++) {
    M2Face face;

    face.indices[0] = indicesLookup[faces[i * 3]];
    face.indices[1] = indicesLookup[faces[i * 3 + 1]];
    face.indices[2] = indicesLookup[faces[i * 3 + 2]];

    model->faces[i] = face;
  }

  M2Animation *animations = (M2Animation *) ((uint8_t *) bytes + header->animationsOffset);
  model->animationsCount = header->animationsCount;
  model->animations = (ModelAnimation *) arena->allocate(sizeof(ModelAnimation) * header->animationsCount);
  for (int ai = 0; ai < header->animationsCount; ai++) {
    model->animations[ai].id = -1;
    model->animations[ai].startFrame = animations[ai].timeStart;
    model->animations[ai].endFrame = animations[ai].timeEnd;
    model->animations[ai].speed = animations[ai].moveSpeed;
  }

  int16_t *animLookups = (int16_t *) ((uint8_t *) bytes + header->animationLookupsOffset);
  model->animationLookupsCount = header->animationLookupsCount;
  model->animationLookups = (int16_t *) arena->allocate(sizeof(int32_t) * model->animationLookupsCount);
  for (size_t ali = 0; ali < model->animationLookupsCount; ali++) {
    int16_t anim_id = animLookups[ali];
    model->animationLookups[ali] = anim_id;

    if (anim_id >= 0 && anim_id < model->animationsCount) {
      model->animations[anim_id].id = ali; // Reverse lookup
    }
  }

  model->bonesCount = header->bonesCount;
  model->bones = (ModelBone *) arena->allocate(sizeof(ModelBone) * header->bonesCount);

  M2Bone *bones = (M2Bone *) ((uint8_t *) bytes + header->bonesOffset);
  for (int i = 0; i < header->bonesCount; i++) {
    ModelBone bone = {};
    bone.pivot = bones[i].pivot * worldMat;
    bone.parent = bones[i].parent;
    bone.keybone = bones[i].keybone;

    m2_load_animation_data(bytes, header, arena, &bone.translations, &bones[i].translation, sizeof(Vec3f), m2_load_translation);
    m2_load_animation_data(bytes, header, arena, &bone.rotations, &bones[i].rotation, sizeof(Quaternion), m2_load_rotation);
    m2_load_animation_data(bytes, header, arena, &bone.scalings, &bones[i].scaling, sizeof(Vec3f), m2_load_scaling);

    model->bones[i] = bone;
  }

  model->keybonesCount = header->keyBoneLookupsCount;
  model->keybones = (int16_t *) arena->allocate(sizeof(int16_t) * header->keyBoneLookupsCount);

  int16_t *keybones = (int16_t *) ((uint8_t *) bytes + header->keyBoneLookupsOffset);
  for (size_t bli = 0; bli < header->keyBoneLookupsCount; bli++) {
    model->keybones[bli] = keybones[bli];
  }

  return model;
}

inline void m2_reset_bones(M2Model *model)
{
  for (int i = 0; i < model->bonesCount; i++) {
    model->bones[i].calculated = false;
  }
}

inline void m2_anim_get_frame(ModelAnimationData *data, ModelAnimationRange range, uint32_t frame, uint32_t globalFrame, uint32_t *idx0, uint32_t *idx1, float *t)
{
  if (range.start == range.end) {
    *idx0 = range.start;
    *idx1 = range.start;
    *t = 0;
    return;
  }

  if (data->isGlobal) {
    frame = range.start + globalFrame % (range.end - range.start);
  }

  uint32_t tsMax = data->timestamps[range.end];
  if (frame > tsMax) {
    *idx0 = range.end;
    *idx1 = range.end;
    *t = 1;
    return;
  }

  for (int i = range.start; i < range.end; i++) {
    uint32_t ts0 = data->timestamps[i];
    uint32_t ts1 = data->timestamps[i + 1];

    if (frame >= ts0 && frame <= ts1) {
      *idx0 = i;
      *idx1 = i + 1;
      *t = (float) (frame - ts0) / (float)(ts1 - ts0);
      return;
    }
  }

  *idx0 = range.start;
  *idx1 = range.start + 1;
  *t = 0;
}

void m2_calc_bone(M2Model *model, ModelBone *bone, uint32_t animId, uint32_t frame, uint32_t globalFrame)
{
  if (bone->calculated) {
    return;
  }

  Mat44 mat = Mat44::identity();
  Mat44 nmat = mat;

  ModelAnimationRange trRange = {};
  bool translate = false;
  ModelAnimationRange rotRange = {};
  bool rotate = false;
  ModelAnimationRange scaleRange = {};
  bool scale = false;

  if (bone->translations.isGlobal) {
    trRange = bone->translations.animationRanges[0];
    translate = true;
  } else if (animId < bone->translations.animationsCount) {
    trRange = bone->translations.animationRanges[animId];
    translate = bone->translations.keyframesCount > 0;
  }

  if (bone->rotations.isGlobal) {
    rotRange = bone->rotations.animationRanges[0];
    rotate = true;
  } else if (animId < bone->rotations.animationsCount) {
    rotRange = bone->rotations.animationRanges[animId];
    rotate = bone->rotations.keyframesCount > 0;
  }

  if (bone->scalings.isGlobal) {
    scaleRange = bone->scalings.animationRanges[0];
    scale = true;
  } else if (animId < bone->scalings.animationsCount) {
    scaleRange = bone->scalings.animationRanges[animId];
    scale = bone->scalings.keyframesCount > 0;
  }

  bool animated = translate || rotate || scale;
  uint32_t idx0;
  uint32_t idx1;
  float t;

  if (animated) {
    mat = Mat44::translate(-bone->pivot.x, -bone->pivot.y, -bone->pivot.z);

    if (scale) {
      m2_anim_get_frame(&bone->scalings, scaleRange, frame, globalFrame, &idx0, &idx1, &t);
      Vec3f s0 = ((Vec3f *) bone->scalings.data)[idx0];
      Vec3f s1 = ((Vec3f *) bone->scalings.data)[idx1];
      Vec3f s = lerp(s0, s1, t);
      mat = mat * Mat44::scale(s.x, s.y, s.z);
    }

    if (rotate) {
      m2_anim_get_frame(&bone->rotations, rotRange, frame, globalFrame, &idx0, &idx1, &t);
      Quaternion q0 = ((Quaternion *) bone->rotations.data)[idx0];
      Quaternion q;

      switch (bone->rotations.interpolationType) {
        case MODEL_INTERPOLATION_NONE:
          q = q0;
          break;

        case MODEL_INTERPOLATION_LINEAR: {
          Quaternion q1 = ((Quaternion *) bone->rotations.data)[idx1];
          q = lerp(q0, q1, t);
        } break;

        default:
          q = q0;
          break;
      }

      Mat44 rotmat = Mat44::from_quaternion(q);
      mat = mat * rotmat;
      nmat = rotmat;
    }

    if (translate) {
      m2_anim_get_frame(&bone->translations, trRange, frame, globalFrame, &idx0, &idx1, &t);
      Vec3f tr0 = ((Vec3f *) bone->translations.data)[idx0];
      Vec3f tr;

      switch (bone->translations.interpolationType) {
        case MODEL_INTERPOLATION_NONE:
          tr = tr0;
          break;

        case MODEL_INTERPOLATION_LINEAR: {
          Vec3f tr1 = ((Vec3f *) bone->translations.data)[idx1];
          tr = lerp(tr0, tr1, t);
        } break;

        default:
          tr = tr0;
          break;
      }

      mat = mat * Mat44::translate(tr.x, tr.y, tr.z);
    }

    mat = mat * Mat44::translate(bone->pivot.x, bone->pivot.y, bone->pivot.z);
  }

  if (bone->parent > -1) {
    m2_calc_bone(model, &model->bones[bone->parent], animId, frame, globalFrame);
    mat = mat * model->bones[bone->parent].matrix;
    nmat = nmat * model->bones[bone->parent].normal_matrix;
  }

  bone->matrix = mat;
  bone->normal_matrix = nmat;
  bone->calculated = true;
}

void m2_calc_bones(M2Model *model, ModelBoneSet *boneset, uint32_t animId, uint32_t frame, uint32_t globalFrame)
{
  if (boneset != NULL) {
    for (size_t i = 0; i < sizeof(boneset->set) / sizeof(boneset->set[0]); i++) {
      uint32_t boneIdx = 32 * i;
      uint32_t set = boneset->set[i];

      while (set > 0) {
        if (set & 0x1) {
          m2_calc_bone(model, &model->bones[boneIdx], animId, frame, globalFrame);
        }

        boneIdx++;
        set = set >> 1;
      }
    }
  } else {
    for (size_t i = 0; i < model->bonesCount; i++) {
      m2_calc_bone(model, &model->bones[i], animId, frame, globalFrame);
    }
  }
}

void m2_animate_vertices(M2Model *model)
{
  for (int rpi = 0; rpi < model->renderPassesCount; rpi++) {
    M2RenderPass pass = model->renderPasses[rpi];
    ModelSubmesh submesh = model->submeshes[pass.submesh];

    if (!submesh.enabled) {
      continue;
    }

    uint32_t vstart = submesh.verticesStart;
    uint32_t vend = submesh.verticesStart + submesh.verticesCount;

    for (int vi = vstart; vi < vend; vi++) {
      Vec3f pos = {};
      Vec3f normal = {};
      ModelVertexWeight *weights = &model->weights[vi * model->weightsPerVertex];

      for (int wi = 0; wi < model->weightsPerVertex; wi++) {
        if (weights[wi].weight > 0) {
          Vec3f v = model->positions[vi] * model->bones[weights[wi].bone].matrix;
          Vec3f n = model->normals[vi] * model->bones[weights[wi].bone].normal_matrix;
          pos = pos + v * weights[wi].weight;
          normal = normal + n * weights[wi].weight;
        }
      }

      model->animatedPositions[vi] = pos;
      model->animatedNormals[vi] = normal;
    }
  }
}

ModelBoneSet m2_character_core_boneset(M2Model *model)
{
  ModelBoneSet result = {};

  if (model->keybones[M2_KEYBONE_ROOT] > -1) {
    for (size_t i = 0; i <= model->keybones[M2_KEYBONE_ROOT]; i++) {
      MODEL_BONESET_SET(result, i);
    }
  }

  return result;
}

ModelBoneSet m2_character_upper_body_boneset(M2Model *model)
{
  ModelBoneSet result = {};

  for (size_t i = 0; i < M2_KEYBONE_WAIST; i++) {
    if (model->keybones[i] > -1) {
      MODEL_BONESET_SET(result, model->keybones[i]);
    }
  }

  if (model->keybones[M2_KEYBONE_HEAD] > -1) {
    MODEL_BONESET_SET(result, model->keybones[M2_KEYBONE_HEAD]);
  }

  if (model->keybones[M2_KEYBONE_JAW] > -1) {
    MODEL_BONESET_SET(result, model->keybones[M2_KEYBONE_JAW]);
  }

  for (size_t i = M2_KEYBONE_BTH; i <= M2_KEYBONE_ROOT; i++) {
    if (model->keybones[i] > -1) {
      MODEL_BONESET_SET(result, model->keybones[i]);
    }
  }

  for (size_t i = 0; i < 5; i++) {
    if (model->keybones[M2_KEYBONE_FINGER1_R + i] > -1) {
      MODEL_BONESET_SET(result, model->keybones[M2_KEYBONE_FINGER1_L + i]);
    }

    if (model->keybones[M2_KEYBONE_FINGER1_L + i] > -1) {
      MODEL_BONESET_SET(result, model->keybones[M2_KEYBONE_FINGER1_R + i]);
    }
  }

  return result;
}
