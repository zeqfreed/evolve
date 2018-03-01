#pragma once

#include "math.h"
#include "memory.h"

PACK_START(1);

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
  uint32_t texturesCount;
  uint32_t texturesOffset;
  uint32_t transparenciesCount;
  uint32_t transparenciesOffset;
  uint32_t reservedCount;
  uint32_t reservedOffset;
  uint32_t textureAnimationsCount;
  uint32_t textureAnimationsOffset;
  uint32_t textureReplacementsCount;
  uint32_t textureReplacementsOffset;
  uint32_t renderFlagsCount;
  uint32_t renderFlagsOffset;
  uint32_t boneLookupsCount;
  uint32_t boneLookupsOffset;
  uint32_t textureLookupsCount;
  uint32_t textureLookupsOffset;
  uint32_t textureUnitLookupsCount;
  uint32_t textureUnitLookupsOffset;
} PACKED M2Header;

typedef struct M2Vertex {
  Vec3f pos;
  uint8_t weights[4];
  uint8_t bones[4];
  Vec3f normal;
  float texcoords[2];
  uint32_t reserved1;
  uint32_t reserved2;
} PACKED M2Vertex;

typedef struct M2Texture {
  uint32_t type;
  uint32_t flags;
  uint32_t filenameLength;
  uint32_t filenameOffset;
} PACKED M2Texture;

typedef struct M2RenderFlag {
  uint16_t flags;
  uint16_t blendingMode;
} PACKED M2RenderFlag;

typedef struct M2RenderPass {
  uint16_t flags;
  uint16_t shaderFlags;
  uint16_t submesh;
  uint16_t _unknown1;
  uint16_t colorIndex;
  uint16_t renderFlagIndex;
  uint16_t textureUnitIndex;
  uint16_t mode;
  uint16_t textureId;
  uint16_t _unknown2;
  uint16_t transparencyIndex;
  uint16_t textureAnimationIndex;
} PACKED M2RenderPass;

typedef struct M2View {
  uint32_t indicesCount;
  uint32_t indicesOffset;
  uint32_t facesCount;
  uint32_t facesOffset;
  uint32_t propsCount;
  uint32_t propsOffset;
  uint32_t submeshesCount;
  uint32_t submeshesOffset;
  uint32_t renderPassesCount;
  uint32_t renderPassesOffset;
	int32_t lod;
} PACKED M2View;

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
} PACKED M2Geoset;

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
} PACKED M2Animation;

typedef struct M2AnimationBlock {
  uint16_t interpolationType;
  int16_t globalSequence;
  uint32_t lookupsCount;
  uint32_t lookupsOffset;
  uint32_t timestampsCount;
  uint32_t timestampsOffset;
  uint32_t keyframesCount;
  uint32_t keyframesOffset;
} PACKED M2AnimationBlock;

typedef struct M2Bone {
  int32_t keybone;
  uint32_t flags;
  int16_t parent;
  int16_t geoid;
  M2AnimationBlock translation;
  M2AnimationBlock rotation;
  M2AnimationBlock scaling;
  Vec3f pivot;
} PACKED M2Bone;

PACK_END();

typedef struct ModelSubmesh {
  uint32_t id;
  uint32_t facesStart;
  uint32_t facesCount;
  uint32_t verticesStart;
  uint32_t verticesCount;
  bool enabled;
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
  bool isGlobal;
  ModelInterpolationType interpolationType;
  uint32_t animationsCount;
  ModelAnimationRange *animationRanges;
  uint32_t keyframesCount;
  uint32_t *timestamps;
  void *data;
} ModelAnimationData;

typedef enum ModelTextureType {
  MTT_INLINE = 0,
  MTT_SKIN,
  MTT_CHAR_HAIR = 6,
  MTT_SKIN_EXTRA = 8
} ModelTextureType;

typedef struct ModelTexture {
  uint32_t type;
  char *name; // for MTT_INLINE only
} ModelTexture;

typedef struct ModelBone {
  int32_t parent;
  int32_t keybone;
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
  int32_t id;
  uint32_t startFrame;
  uint32_t endFrame;
  float speed;
} ModelAnimation;

typedef struct ModelVertexWeight {
  uint32_t bone;
  float weight;
} ModelVertexWeight;

typedef struct M2Model {
  uint32_t verticesCount;
  uint32_t weightsPerVertex;
  Vec3f *positions;
  Vec3f *animatedPositions;
  Vec3f *normals;
  Vec3f *animatedNormals;
  Vec3f *textureCoords;
  uint32_t texturesCount;
  ModelTexture *textures;
  uint32_t textureLookupsCount;
  uint32_t *textureLookups;
  ModelVertexWeight *weights;
  uint32_t facesCount;
  M2Face *faces;
  uint32_t submeshesCount;
  ModelSubmesh *submeshes;
  uint32_t bonesCount;
  ModelBone *bones;
  uint32_t keybonesCount;
  int16_t *keybones;
  uint32_t animationLookupsCount;
  int16_t *animationLookups;
  uint32_t animationsCount;
  ModelAnimation *animations;
  uint32_t renderFlagsCount;
  M2RenderFlag *renderFlags;
  uint32_t renderPassesCount;
  M2RenderPass *renderPasses;
} M2Model;

typedef enum M2Keybone {
  M2_KEYBONE_ARM_L = 0,
  M2_KEYBONE_ARM_R,
  M2_KEYBONE_SHOULDER_L,
  M2_KEYBONE_SHOULDER_R,
  M2_KEYBONE_STOMACH,
  M2_KEYBONE_WAIST,
  M2_KEYBONE_HEAD,
  M2_KEYBONE_JAW,
  M2_KEYBONE_FINGER1_R,
  M2_KEYBONE_FINGER2_R,
  M2_KEYBONE_FINGER3_R,
  M2_KEYBONE_FINGER4_R,
  M2_KEYBONE_THUMB_R,
  M2_KEYBONE_FINGER1_L,
  M2_KEYBONE_FINGER2_L,
  M2_KEYBONE_FINGER3_L,
  M2_KEYBONE_FINGER4_L,
  M2_KEYBONE_THUMB_L,
  M2_KEYBONE_BTH,
  M2_KEYBONE_CSR,
  M2_KEYBONE_CSL,
  M2_KEYBONE_BREATH,
  M2_KEYBONE_NAME,
  M2_KEYBONE_NAMEMOUNT,
  M2_KEYBONE_CHD,
  M2_KEYBONE_CCH,
  M2_KEYBONE_ROOT
} M2Keybone;

typedef struct ModelBoneSet {
  uint32_t set[8];
} BoneSet;

#define MODEL_BONESET_SET(BS, IDX) (BS.set[IDX / 32] |= (1 << (IDX % 32)))
#define MODEL_PBONESET_ISSET(PBS, IDX) ((PBS->set[IDX / 32] & (1 << (IDX % 32))) > 0)

M2Model *m2_load(MemoryAllocator *allocator, void *bytes, size_t size);
