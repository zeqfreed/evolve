
#include <stdarg.h>
#include "dresser.h"

static void dresser_init(Dresser *dresser, AssetLoader *loader)
{
  ASSERT(dresser != NULL);
  dresser->loader = loader;
}

static void texture_blit(Texture *dst, Texture *src, uint32_t dstX, uint32_t dstY, uint32_t srcX, uint32_t srcY, uint32_t width, uint32_t height)
{
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
  if (dst == NULL || src == NULL) {
    return NULL;
  }

  texture_blit(dst, src, 128, 0, 0, 0, src->width, src->height);
  return dst;
}

static inline Texture *blit_legs_upper(Texture *dst, Texture *src)
{
  if (dst == NULL || src == NULL) {
    return NULL;
  }

  texture_blit(dst, src, 128, 160 - src->height, 0, 0, src->width, src->height);
  return dst;
}

static inline Texture *blit_face_lower(Texture *dst, Texture *src)
{
  if (dst == NULL || src == NULL) {
    return NULL;
  }

  texture_blit(dst, src, 0, 192, 0, 0, src->width, src->height);
  return dst;
}

static inline Texture *blit_face_upper(Texture *dst, Texture *src)
{
  if (dst == NULL || src == NULL) {
    return NULL;
  }

  texture_blit(dst, src, 0, 160, 0, 0, src->width, src->height);
  return dst;
}

static Texture *load_parameterized_blp_texture(AssetLoader *loader, char *filename, ...)
{
  char buf[1024];
  va_list args;
  va_start(args, filename);
  int n = vsnprintf(buf, sizeof(buf), filename, args) < sizeof(buf);
  va_end(args);

  if (n >= sizeof(buf)) {
    return NULL;
  }

  Asset *asset = asset_loader_get_texture(loader, buf);
  if (asset == NULL) {
    return NULL;
  }

  return asset->texture;
}

static Texture *dresser_get_character_texture(Dresser *dresser, CharAppearance appearance)
{
  Texture *skin = load_parameterized_blp_texture(
    dresser->loader,
    (char *) "Character/NightElf/Female/NightElfFemaleSkin00_%02d.blp",
    appearance.skinIdx);

  Texture *pelvis = load_parameterized_blp_texture(
    dresser->loader,
    (char *) "Character/NightElf/Female/NightElfFemaleNakedPelvisSkin00_%02d.blp",
    appearance.skinIdx);

  Texture *torso = load_parameterized_blp_texture(
    dresser->loader,
    (char *) "Character/NightElf/Female/NightElfFemaleNakedTorsoSkin00_%02d.blp",
    appearance.skinIdx);

  Texture *faceLower = load_parameterized_blp_texture(
    dresser->loader,
    (char *) "Character/NightElf/Female/NightElfFemaleFaceLower%02d_%02d.blp",
    appearance.faceIdx,
    appearance.skinIdx);

  Texture *faceUpper = load_parameterized_blp_texture(
    dresser->loader,
    (char *) "Character/NightElf/Female/NightElfFemaleFaceUpper%02d_%02d.blp",
    appearance.faceIdx,
    appearance.skinIdx);

  Texture *scalpLower = load_parameterized_blp_texture(
    dresser->loader,
    (char *) "Character/NightElf/ScalpLowerHair00_%02d.blp",
    appearance.hairColorIdx,
    appearance.skinIdx);

  Texture *scalpUpper = load_parameterized_blp_texture(
    dresser->loader,
    (char *) "Character/NightElf/ScalpUpperHair00_%02d.blp",
    appearance.hairColorIdx,
    appearance.skinIdx);

  Texture *facialLower = load_parameterized_blp_texture(
    dresser->loader,
    (char *) "Character/NightElf/FacialLowerHair%02d_%02d.blp",
    appearance.facialDetailIdx,
    appearance.hairColorIdx);

  Texture *facialUpper = load_parameterized_blp_texture(
    dresser->loader,
    (char *) "Character/NightElf/FacialUpperHair%02d_%02d.blp",
    appearance.facialDetailIdx,
    appearance.hairColorIdx);

  blit_torso_upper(skin, torso);
  blit_legs_upper(skin, pelvis);
  blit_face_lower(skin, faceLower);
  blit_face_upper(skin, faceUpper);
  blit_face_lower(skin, facialLower);
  blit_face_upper(skin, facialUpper);
  blit_face_lower(skin, scalpLower);
  blit_face_upper(skin, scalpUpper);

  return skin;
}

static Texture *dresser_get_hair_texture(Dresser *dresser, CharAppearance appearance)
{
  Texture *hair = load_parameterized_blp_texture(
    dresser->loader,
    (char *) "Character/NightElf/Hair00_%02d.blp",
    appearance.hairColorIdx);

  return hair;
}
