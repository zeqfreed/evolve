#include <cstdio>
#include <cstdint>

#include "m2.h"

static void m2_dump_header(M2Header *header, void *bytes)
{
  printf("%c%c%c%c (version %d)\n", header->magic[0], header->magic[1], header->magic[2], header->magic[3], header->version);

  char *name = &((char *) bytes)[header->nameOffset]; // TODO: use snprintf to avoid overflow
  printf("Model name: %s\n", name);

  const char *entries[] = {
    "GlobalSequences", "Animations", "AnimationLookups", "PlayableAnimLookups",
    "Bones", "KeyBoneLookups", "Vertices", "Views", "Colors"
  };

  uint32_t *h = (uint32_t *) &header->globalSequencesCount;
  for (int i = 0; i < 9; i++) {
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

  printf("Translation:\n\tinterpolation: %d, global seq: %d\n\t%d timestamps (0x%x), %d keyframes (0x%x)\n",
         trans->interpolationType, trans->globalSequence,
         trans->timestampsCount, trans->timestampsOffset,
         trans->keyframesCount, trans->keyframesOffset);

  printf("Rotation:\n\tinterpolation: %d, global seq: %d\n\t%d timestamps (0x%x), %d keyframes (0x%x)\n",
         rot->interpolationType, rot->globalSequence,
         rot->timestampsCount, rot->timestampsOffset,
         rot->keyframesCount, rot->keyframesOffset);

  printf("Scaling:\n\tinterpolation: %d, global seq: %d\n\t%d timestamps (0x%x), %d keyframes (0x%x)\n",
         scale->interpolationType, scale->globalSequence,
         scale->timestampsCount, scale->timestampsOffset,
         scale->keyframesCount, scale->keyframesOffset);                                                                                                                                  

  printf("\n");
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

M2Model *m2_load(void *bytes, size_t size, MemoryArena *arena)
{
  M2Header *header = (M2Header *) bytes;
  m2_dump_header(header, bytes);

  M2Model *model = (M2Model *) arena->allocate(sizeof(M2Model));
  model->verticesCount = header->verticesCount;
  model->positions = (Vec3f *) arena->allocate(sizeof(Vec3f) * header->verticesCount);
  model->normals = (Vec3f *) arena->allocate(sizeof(Vec3f) * header->verticesCount);
  model->textureCoords = (Vec3f *) arena->allocate(sizeof(Vec3f) * header->verticesCount);

  Mat44 worldMat = Mat44::identity();
  worldMat.f = 0;
  worldMat.j = 1;
  worldMat.k = 0;
  worldMat.g = -1;

  M2Vertex *vertices = (M2Vertex *) ((uint8_t *) bytes + header->verticesOffset);
  for(int i = 0; i < header->verticesCount; i++) {
    model->positions[i] = vertices[i].pos * worldMat;
    model->normals[i] = -vertices[i].normal * worldMat;
    model->textureCoords[i] = (Vec3f){vertices[i].texcoords[0], vertices[i].texcoords[1]};
  }

  M2View *view = (M2View *) ((uint8_t *) bytes + header->viewsOffset);
  m2_dump_view(view);

  M2Geoset *geosets = (M2Geoset *) ((uint8_t *) bytes + view->submeshesOffset);
  model->partsCount = view->submeshesCount;
  model->parts = (ModelPart *) arena->allocate(sizeof(ModelPart) * view->submeshesCount);
  for (int i = 0; i < view->submeshesCount; i++) {
    //m2_dump_geoset(&geosets[i]);
    ModelPart part;
    part.id = geosets[i].id;
    part.facesStart = geosets[i].indicesStart / 3;
    part.facesCount = geosets[i].indicesCount / 3;
    model->parts[i] = part;
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

  // M2Animation *animations = (M2Animation *) ((uint8_t *) bytes + header->animationsOffset);
  // for (int i = 0; i < header->animationsCount; i++) {
  //   m2_dump_animation(&animations[i]);
  // }

  // int16_t *animationLookups = (int16_t *) ((uint8_t *) bytes + header->animationLookupsOffset);
  // for (int i = 0; i < 139; i++) {
  //   printf("Anim %d = %d\n", i, animationLookups[i]);
  // }

  model->bonesCount = header->bonesCount;
  model->bones = (ModelBone *) arena->allocate(sizeof(ModelBone) * header->bonesCount);

  M2Bone *bones = (M2Bone *) ((uint8_t *) bytes + header->bonesOffset);
  for (int i = 0; i < header->bonesCount; i++) {
    //m2_dump_bone(&bones[i]);
    ModelBone bone;
    bone.pivot = bones[i].pivot * worldMat;
    bone.parent = bones[i].parent;

    model->bones[i] = bone;
  }

  return model;
}
