
#include <stdarg.h>
#include "dresser.h"

static DresserFeatureType _parse_feature_type(char *name)
{
  DresserFeatureType result = DFT_NONE;

  struct {const char *name; uint32_t len; DresserFeatureType type;} mapping[] = {
    {"NORMAL", 6, DFT_NORMAL},
    {"FEATURES", 8, DFT_FEATURES},
    {"MARKINGS", 8, DFT_MARKINGS},
    {"PIERCINGS", 9, DFT_PIERCINGS},
    {"EARRINGS", 8, DFT_EARRINGS},
    {"TUSKS", 5, DFT_TUSKS},
    {"HORNS", 5, DFT_HORNS}
  };

  for (size_t i = 0; i < sizeof(mapping) / sizeof(mapping[0]); i++) {
    if (strncmp(mapping[i].name, name, mapping[i].len) == 0) {
      result = mapping[i].type;
      break;
    }
  }

  return result;
}

static void _dresser_load_limits(Dresser *dresser)
{
  Asset *asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/CharSections.dbc");
  if (asset == NULL) {
    return;
  }
  DBCFile *char_sections = asset->dbc;

  for (size_t i = 0; i < char_sections->header.records_count; i++) {
    DBCCharSectionsRecord *rec = DBC_RECORD(char_sections, DBCCharSectionsRecord, i);
    if (rec->flags & 1) {
      continue;
    }

    uint32_t race = rec->race - 1;
    if (race >= dresser->races_count) {
      continue;
    }

    DresserCustomizationLimits *lims = &dresser->race_options[race].limits[rec->sex];
    dresser->race_options[race].indices[rec->sex] = MIN(dresser->race_options[race].indices[rec->sex], i);

    switch (rec->type) {
    case DST_SKIN:
      lims->skin_color[0] = MIN(lims->skin_color[0], rec->color);
      lims->skin_color[1] = MAX(lims->skin_color[1], rec->color);
      break;

    case DST_FACE:
      lims->face[0] = MIN(lims->face[0], rec->variant);
      lims->face[1] = MAX(lims->face[1], rec->variant);
      break;

    case DST_HAIR:
      lims->hair[0] = MIN(lims->hair[0], rec->variant);
      lims->hair[1] = MAX(lims->hair[1], rec->variant);
      lims->hair_color[0] = MIN(lims->hair_color[0], rec->color);
      lims->hair_color[1] = MAX(lims->hair_color[1], rec->color);
      break;

    case DST_FACIAL:
      lims->facial[0] = MIN(lims->facial[0], rec->variant);
      lims->facial[1] = MAX(lims->facial[1], rec->variant);
      break;
    }
  }

  asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/CharacterFacialHairStyles.dbc");
  if (asset != NULL) {
    for (size_t i = 0; i < asset->dbc->header.records_count; i++) {
      DBCCharacterFacialHairStylesRecord *rec = DBC_RECORD(asset->dbc, DBCCharacterFacialHairStylesRecord, i);

      DresserCustomizationLimits *lims = &dresser->race_options[rec->race - 1].limits[rec->sex];
      lims->facial[0] = MIN(lims->facial[0], rec->variant);
      lims->facial[1] = MAX(lims->facial[1], rec->variant);
    }
  }

  for (size_t ri = 0; ri < dresser->races_count; ri++) {
    for (size_t si = 0; si < 2; si++) {
      DresserCustomizationOptions *lims = &dresser->race_options[ri].limits[si];
      printf("race %zu sex %zu: skin %u..%u, face %u..%u, hair %u..%u, hair color: %u..%u, facial %u..%u\n",
             ri, si, lims->skin_color[0], lims->skin_color[1], lims->face[0], lims->face[1],
             lims->hair[0], lims->hair[1], lims->hair_color[0], lims->hair_color[1], lims->facial[0], lims->facial[1]);

    }
  }
}

static void _dresser_load_races(Dresser *dresser)
{
  Asset *asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/ChrRaces.dbc");
  if (asset == NULL) {
    return;
  }
  DBCFile *chr_races = asset->dbc;

  dresser->races_count = MIN(DRESSER_MAX_RACES, chr_races->header.records_count);

  for (size_t ri = 0; ri < dresser->races_count; ri++) {
    DBCCharRacesRecord *rec = DBC_RECORD(chr_races, DBCCharRacesRecord, ri);
    char *features[3];
    features[0] = DBC_STRING(chr_races, rec->features[0]);
    features[1] = DBC_STRING(chr_races, rec->features[1]);
    features[2] = DBC_STRING(chr_races, rec->features[2]);
    printf("race %zu: id %u flags 0x%x %s %s %s\n", ri, rec->id, rec->flags, features[0], features[1], features[2]);

    DresserRaceOptions *opts = &dresser->race_options[ri];
    opts->id = rec->id;
    opts->display_ids[0] = rec->male_display_id;
    opts->display_ids[1] = rec->female_display_id;
    opts->indices[0] = (uint32_t) -1;
    opts->indices[1] = (uint32_t) -1;

    char *name = DBC_STRING(chr_races, rec->i18n_name.enUS);
    size_t len = strlen(name);
    opts->name = (char *) ALLOCATE_SIZE(&dresser->loader->allocator, len + 1);
    memcpy(opts->name, name, len);

    static const char *feature_names[] = {"Feature", "Feature", "Feature", "Markings", "Piercings", "Earrings", "Tusks"};

    for (size_t si = 0; si < 2; si++) {
      DresserFeatureType hair_type = _parse_feature_type(features[2]);
      if (hair_type == DFT_HORNS) {
        opts->feature_names[si][0] = (char *) "Horns";
      } else {
        opts->feature_names[si][0] = (char *) "Hair";
      }

      DresserFeatureType feature_type = _parse_feature_type(features[si]);
      opts->feature_types[si][1] = feature_type;
      opts->feature_names[si][1] = (char *) feature_names[feature_type];
    }
  }
}

static void dresser_init(Dresser *dresser, AssetLoader *loader)
{
  ASSERT(dresser != NULL);
  dresser->loader = loader;

  _dresser_load_races(dresser);
  _dresser_load_limits(dresser);
}

static M2Model *dresser_load_character_model(Dresser *dresser, uint32_t race, uint32_t sex)
{
  M2Model *result = NULL;

  if (race >= dresser->races_count) {
    return result;
  }

  if (sex > 1) {
    return result;
  }

  DresserRaceOptions *opts = &dresser->race_options[race];
  uint32_t display_id = opts->display_ids[sex];
  printf("Display id: %u\n", display_id);

  Asset *asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/CreatureDisplayInfo.dbc");
  if (asset == NULL) {
    return result;
  }
  DBCFile *display_info = asset->dbc;

  uint32_t model_id = 0;
  for (size_t i = 0; i < display_info->header.records_count; i++) {
    DBCCreatureDisplayInfoRecord *rec = DBC_RECORD(display_info, DBCCreatureDisplayInfoRecord, i);
    if (rec->id == display_id) {
      model_id = rec->model_id;
    }
  }

  printf("Model id: %u\n", model_id);

  asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/CreatureModelData.dbc");
  if (asset == NULL) {
    return result;
  }
  DBCFile *model_data = asset->dbc;

  char filename[255] = {};

  for (size_t i = 0; i < model_data->header.records_count; i++) {
    DBCCreatureModelDataRecord *rec = DBC_RECORD(model_data, DBCCreatureModelDataRecord, i);
    if (rec->id == model_id) {
      char *model_name = DBC_STRING(model_data, rec->model_name);
      char *s = model_name;
      char *d = filename;
      char *dot = NULL;

      do {
        *d++ = *s++;
        if (*s == '.') {
          dot = d;
        }
      } while (*s && (d - filename < 255));

      if (dot != NULL) {
        dot++;
        *dot++ = 'm';
        *dot++ = '2';
        *dot = 0;
      }

      break;
    }
  };

  printf("Loading character model: %s\n", filename);

  if (filename[0] == 0) {
    return result;
  }

  asset = asset_loader_get_model(dresser->loader, (char *) filename);
  if (asset == NULL) {
    return result;
  }

  result = asset->model;
  return result;
}

static DresserCustomizationLimits dresser_get_customization_limits(Dresser *dresser, uint32_t race, uint32_t sex)
{
  DresserCustomizationLimits result = {};

  if (race >= dresser->races_count) {
    return result;
  }

  if (sex > 1) {
    return result;
  }

  result = dresser->race_options[race].limits[sex];
  return result;
}

static DresserCharacterAppearance dresser_get_character_appearance(Dresser *dresser, uint32_t race, uint32_t sex)
{
  DresserCharacterAppearance result = {};

  if (race >= dresser->races_count) {
    return result;
  }

  if (sex > 1) {
    return result;
  }

  DresserCustomizationLimits lims = dresser->race_options[race].limits[sex];
  result.skin_color = lims.skin_color[0];
  result.face = lims.face[0];
  result.hair_color = lims.hair_color[0];
  result.hair = lims.hair[0];
  result.facial = lims.facial[0];

  return result;
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

static Texture *_dresser_load_texture(Dresser *dresser, DBCFile *dbc, DBCCharSectionsRecord *rec, uint32_t texture_index)
{
  char *texture_name = DBC_STRING(dbc, rec->texture_names[texture_index]);
  printf("Texture #%u: %s\n", texture_index, texture_name);
  if (texture_name == NULL || *texture_name == '\0') {
    return NULL;
  }

  Asset *asset = asset_loader_get_texture(dresser->loader, texture_name);
  if (asset == NULL) {
    return NULL;
  }

  return asset->texture;
}

static DresserCharacterTextures dresser_load_character_textures(Dresser *dresser, uint32_t race, uint32_t sex,
                                                                  DresserCharacterAppearance appearance)
{
  DresserCharacterTextures result = {};
  if (sex > 1) {
    return result;
  }

  Asset *asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/CharSections.dbc");
  if (asset == NULL) {
    return result;
  }
  DBCFile *dbc = asset->dbc;

  Texture *skin[2] = {};
  Texture *face[2] = {};
  Texture *features[2] = {};
  Texture *hair = {};
  Texture *scalp[2] = {};
  Texture *underwear[2] = {};

  DresserRaceOptions *opts = &dresser->race_options[race];

  printf("Prepare textures for race %u, sex %u\n", race, sex);

  for (size_t i = opts->indices[sex]; i < dbc->header.records_count; i++) {
    DBCCharSectionsRecord *rec = DBC_RECORD(dbc, DBCCharSectionsRecord, i);
    if (rec->race - 1 == race && rec->sex == sex) {
      if (rec->flags & 1) {
        continue;
      }

      switch (rec->type) {
      case DST_SKIN:
        if (rec->color == appearance.skin_color) {
          printf("Use skin (%u): variant %u color %u\n", rec->type, rec->variant, rec->color);
          skin[0] = _dresser_load_texture(dresser, dbc, rec, 0);
          skin[1] = _dresser_load_texture(dresser, dbc, rec, 1);
        }
        break;
      case DST_FACE:
        if (rec->color == appearance.skin_color && rec->variant == appearance.face) {
          printf("Use face (%u): variant %u color %u\n", rec->type, rec->variant, rec->color);
          face[0] = _dresser_load_texture(dresser, dbc, rec, 0);
          face[1] = _dresser_load_texture(dresser, dbc, rec, 1);
        }
        break;
      case DST_FACIAL:
        if (rec->color == appearance.hair_color && rec->variant == appearance.facial) {
          printf("Use feature (%u): variant %u color %u\n", rec->type, rec->variant, rec->color);
          features[0] = _dresser_load_texture(dresser, dbc, rec, 0);
          features[1] = _dresser_load_texture(dresser, dbc, rec, 1);
        }
        break;
      case DST_HAIR:
        if (rec->color == appearance.hair_color && rec->variant == appearance.hair) {
          printf("Use hair (%u): variant %u color %u\n", rec->type, rec->variant, rec->color);
          hair = _dresser_load_texture(dresser, dbc, rec, 0);
          scalp[0] = _dresser_load_texture(dresser, dbc, rec, 1);
          scalp[1] = _dresser_load_texture(dresser, dbc, rec, 2);
        } else if (rec->color == appearance.hair_color && hair == NULL) {
          // If no hair matched, grab first variant for selected color. Required for beard on bald Orc
          printf("Fallback to hair (%u): variant %u color %u", rec->type, rec->variant, rec->color);
          hair = _dresser_load_texture(dresser, dbc, rec, 0);
        }
        break;
      case DST_UNDERWEAR:
        if (rec->color == appearance.skin_color) {
          printf("Use underwear (%u): variant %u color %u\n", rec->type, rec->variant, rec->color);
          underwear[0] = _dresser_load_texture(dresser, dbc, rec, 0);
          underwear[1] = _dresser_load_texture(dresser, dbc, rec, 1);
        }
        break;
      }
    } else {
      break;
    }
  }

  blit_legs_upper(skin[0], underwear[0]);
  blit_torso_upper(skin[0], underwear[1]);

  blit_face_lower(skin[0], face[0]);
  blit_face_upper(skin[0], face[1]);
  blit_face_lower(skin[0], features[0]);
  blit_face_upper(skin[0], features[1]);
  blit_face_lower(skin[0], scalp[0]);
  blit_face_upper(skin[0], scalp[1]);

  result.skin = skin[0];
  result.skin_extra = skin[1];
  result.hair = hair;

  return result;
}
