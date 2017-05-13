#pragma once

#include "math.h"
#include "memory.h"

#pragma pack(push)

typedef struct M2Header {
  char magic[4];
  uint32_t version;
  uint32_t nameLength;
  uint32_t nameOffset;
  uint32_t flags;

  uint32_t globalSequencesCount;
  uint32_t globalSequencesOffset;
  uint32_t animationsCount;
  uint32_t animationsOffset;
  uint32_t animationLookupsCount;
  uint32_t animationLookupsOffset;
  uint32_t playableAnimationLookupsCount;
  uint32_t playableAnimationLookupsOffset;
  uint32_t bonesCount;
  uint32_t bonesOffset;
  uint32_t keyBoneLookupsCount;
  uint32_t keyBoneLookupsOffset;
  uint32_t verticesCount;
  uint32_t verticesOffset;
  uint32_t viewsCount;
  uint32_t viewsOffset;
  uint32_t colorsCount;
  uint32_t colorsOffset;
} M2Header;

typedef struct M2Vertex {
	Vec3f pos;
	uint8_t weights[4];
	uint8_t bones[4];
	Vec3f normal;
	float texcoords[2];
  uint32_t reserved1;
  uint32_t reserved2;
} M2Vertex;

typedef struct M2View {
  uint32_t indicesCount;
  uint32_t indicesOffset;
  uint32_t facesCount;
  uint32_t facesOffset;
  uint32_t propsCount;
  uint32_t propsOffset;
  uint32_t submeshesCount;
  uint32_t submeshesOffset;
  uint32_t texturesCount;
  uint32_t texturesOffset;
	int32_t lod;
} M2View;

typedef struct M2Geoset {
	uint32_t id;
	uint16_t verticesStart;
	uint16_t verticesCount;
	uint16_t indicesStart;
	uint16_t indicesCount;
	uint16_t skinnedBonesCount;
	uint16_t bonesStart;
	uint16_t rootBone;
	uint16_t bonesCount;
	Vec3f boundingBox;
	//float radius;
  //uint16_t _padding;
  //uint16_t unknown[6]; // 12 bytes
} M2Geoset;

typedef struct M2Animation {
	uint32_t id;
	uint32_t timeStart;
	uint32_t timeEnd;
	float moveSpeed;
	uint32_t flags;
	int16_t probability;
	uint16_t _padding;
	uint32_t replayMin;
	uint32_t replayMax;
	uint32_t blendTime;
	Vec3f minBounds;
  Vec3f maxBounds;
  float radius;
	int16_t nextAnimation;
	uint16_t aliasedAnimation;
} M2Animation;

typedef struct M2AnimationBlock {
	uint16_t interpolationType;
	int16_t globalSequence;
  uint32_t lookupsCount;
  uint32_t lookupsOffset;
	uint32_t timestampsCount;
	uint32_t timestampsOffset;
	uint32_t keyframesCount;
	uint32_t keyframesOffset;
} M2AnimationBlock;

typedef struct M2Bone {
	int32_t keybone;
	uint32_t flags;
	int16_t parent;
	int16_t geoid;
  // int16_t _unknown1;
  // int16_t _unknown2;
	M2AnimationBlock translation;
	M2AnimationBlock rotation;
	M2AnimationBlock scaling;
	Vec3f pivot;
} M2Bone;

typedef struct ModelPart {
  uint32_t id;
  uint32_t facesStart;
  uint32_t facesCount;
} ModelPart;

typedef struct ModelAnimationRange {
  uint32_t start;
  uint32_t end;
} ModelAnimationRange;

typedef enum ModelInterpolationType {
  MODEL_INTERPOLATION_NONE = 0,
  MODEL_INTERPOLATION_LINEAR,
  MODEL_INTERPOLATION_HERMITE,
  MODEL_INTERPOLATION_BEZIER
} ModelInterpolationType;

typedef struct ModelAnimationData {
  ModelInterpolationType interpolationType;
  uint32_t animationsCount;
  ModelAnimationRange *animationRanges;
  uint32_t keyframesCount;
  uint32_t *timestamps;
  void *data;
} ModelAnimationData;

typedef struct ModelBone {
  int32_t parent;
  int16_t keybone;
  Vec3f pivot;
  ModelAnimationData translations;
  ModelAnimationData rotations;
  ModelAnimationData scalings;
  Mat44 matrix;
  Mat44 normal_matrix;
  bool calculated;
} ModelBone;

typedef struct M2Face {
  uint32_t indices[3];
} M2Face;

typedef struct ModelAnimation {
  uint32_t startFrame;
  uint32_t endFrame;
  uint32_t speed;
} ModelAnimation;

typedef struct ModelVertexWeight {
  uint32_t bone;
  float weight;
} ModelVertexWeight;

typedef struct M2Model {
  uint32_t verticesCount;
  uint32_t weightsPerVertex;
  Vec3f *positions;
  Vec3f *normals;
  Vec3f *textureCoords;
  ModelVertexWeight *weights;
  uint32_t facesCount;
  M2Face *faces;
  uint32_t partsCount;
  ModelPart *parts;
  uint32_t bonesCount;
  ModelBone *bones;
  uint32_t animationsCount;
  ModelAnimation *animations;
} M2Model;

#pragma pack(pop)

M2Model *m2_load(void *bytes, size_t size, MemoryArena *arena);
