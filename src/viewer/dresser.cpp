
#include <stdarg.h>
#include "dresser.h"

static Texture *load_blp_texture(PlatformAPI *papi, MemoryArena *arena, MemoryArena *tempArena, char *filename)
{
  LoadedFile file = load_file(papi, tempArena, filename);
  if (!file.size) {
    printf("Failed to load texture: %s\n", filename);
    return NULL;
  }

  BlpImage image;
  image.read_header(file.contents, file.size);

  BlpHeader *header = &image.header;
  printf("BLP Image width: %d; height: %d; compression: %d\nAlpha depth: %d, alpha type: %d\n",
         header->width, header->height, header->compression, header->alphaDepth, header->alphaType);

  Texture *texture = texture_create(arena, header->width, header->height);
  image.read_into_texture(file.contents, file.size, texture);
  return texture;
}

static void dresser_init(Dresser *dresser, PlatformAPI *platformAPI, MemoryArena *arena)
{
  ASSERT(dresser != NULL);

  dresser->platformAPI = platformAPI;
  dresser->mainArena = arena->subarena(MB(16));
  dresser->tempArena = arena->subarena(MB(16));
}

static void texture_blit(Texture *dst, Texture *src, uint32_t dstX, uint32_t dstY, uint32_t srcX, uint32_t srcY, uint32_t width, uint32_t height)
{
  if (dst == NULL || src == NULL) {
    return;
  }

  if (srcX >= src->width || srcY >= src->height) {
    return;
  }

  width = MIN3(dst->width, src->width - srcX, dstX + width);
  height = MIN3(dst->height, src->height - srcY, dstY + height);

  for (uint32_t j = 0; j < height; j++) {
    for (uint32_t i = 0; i < width; i++) {
      uint32_t dstIdx = (dstY + j) * dst->width + (dstX + i);
      uint32_t srcIdx = (srcY + j) * src->width + (srcX + i);
      dst->pixels[dstIdx] = blend_decal(src->pixels[srcIdx], dst->pixels[dstIdx]);
    }
  }
}

static inline Texture *blit_torso_upper(Texture *dst, Texture *src)
{
  texture_blit(dst, src, 128, 0, 0, 0, src->width, src->height);
  return dst;
}

static inline Texture *blit_legs_upper(Texture *dst, Texture *src)
{
  texture_blit(dst, src, 128, 160 - src->height, 0, 0, src->width, src->height);
  return dst;
}

static inline Texture *blit_face_lower(Texture *dst, Texture *src)
{
  texture_blit(dst, src, 0, 192, 0, 0, src->width, src->height);
  return dst;
}

static inline Texture *blit_face_upper(Texture *dst, Texture *src)
{
  texture_blit(dst, src, 0, 160, 0, 0, src->width, src->height);
  return dst;
}

static Texture *load_parameterized_blp_texture(PlatformAPI *papi, MemoryArena *mainArena, MemoryArena *tempArena, char *filename, ...)
{
  char buf[1024];
  va_list args;
  va_start(args, filename);
  int n = vsnprintf(buf, sizeof(buf), filename, args) < sizeof(buf);
  va_end(args);

  if (n >= sizeof(buf)) {
    return NULL;
  }

  return load_blp_texture(papi, mainArena, tempArena, buf);
}

static Texture *dresser_get_character_texture(Dresser *dresser, CharAppearance appearance)
{
  Texture *skin = load_parameterized_blp_texture(dresser->platformAPI, dresser->mainArena, dresser->tempArena,
                                                 (char *) "data/mpq/Character/NightElf/Female/NightElfFemaleSkin00_%02d.blp", appearance.skinIdx);
  Texture *pelvis = load_parameterized_blp_texture(dresser->platformAPI, dresser->tempArena, dresser->tempArena,
                                                   (char *) "data/mpq/Character/NightElf/Female/NightElfFemaleNakedPelvisSkin00_%02d.blp", appearance.skinIdx);
  Texture *torso = load_parameterized_blp_texture(dresser->platformAPI, dresser->tempArena, dresser->tempArena,
                                                  (char *) "data/mpq/Character/NightElf/Female/NightElfFemaleNakedTorsoSkin00_%02d.blp", appearance.skinIdx);

  Texture *faceLower = load_parameterized_blp_texture(dresser->platformAPI, dresser->tempArena, dresser->tempArena,
                                                      (char *) "data/mpq/Character/NightElf/Female/NightElfFemaleFaceLower%02d_%02d.blp", appearance.faceIdx, appearance.skinIdx);
  Texture *faceUpper = load_parameterized_blp_texture(dresser->platformAPI, dresser->tempArena, dresser->tempArena,
                                                      (char *) "data/mpq/Character/NightElf/Female/NightElfFemaleFaceUpper%02d_%02d.blp", appearance.faceIdx, appearance.skinIdx);

  Texture *scalpLower = load_parameterized_blp_texture(dresser->platformAPI, dresser->tempArena, dresser->tempArena,
                                                      (char *) "data/mpq/Character/NightElf/ScalpLowerHair00_%02d.blp", appearance.hairColorIdx, appearance.skinIdx);
  Texture *scalpUpper = load_parameterized_blp_texture(dresser->platformAPI, dresser->tempArena, dresser->tempArena,
                                                      (char *) "data/mpq/Character/NightElf/ScalpUpperHair00_%02d.blp", appearance.hairColorIdx, appearance.skinIdx);

  Texture *facialLower = load_parameterized_blp_texture(dresser->platformAPI, dresser->tempArena, dresser->tempArena,
                                                        (char *) "data/mpq/Character/NightElf/FacialLowerHair%02d_%02d.blp", appearance.facialDetailIdx, appearance.hairColorIdx);
  Texture *facialUpper = load_parameterized_blp_texture(dresser->platformAPI, dresser->tempArena, dresser->tempArena,
                                                        (char *) "data/mpq/Character/NightElf/FacialUpperHair%02d_%02d.blp", appearance.facialDetailIdx, appearance.hairColorIdx);

  blit_torso_upper(skin, torso);
  blit_legs_upper(skin, pelvis);
  blit_face_lower(skin, faceLower);
  blit_face_upper(skin, faceUpper);
  blit_face_lower(skin, facialLower);
  blit_face_upper(skin, facialUpper);
  blit_face_lower(skin, scalpLower);
  blit_face_upper(skin, scalpUpper);

  dresser->tempArena->discard();

  return skin;
}

static Texture *dresser_get_hair_texture(Dresser *dresser, CharAppearance appearance)
{
    Texture *hair = load_parameterized_blp_texture(dresser->platformAPI, dresser->tempArena, dresser->tempArena,
                                                        (char *) "data/mpq/Character/NightElf/Hair00_%02d.blp", appearance.hairColorIdx);
    dresser->tempArena->discard();
    return hair;
}
