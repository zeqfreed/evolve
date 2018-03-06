
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

    char *prefix = DBC_STRING(chr_races, rec->client_prefix);
    opts->short_name[0] = prefix[0];
    opts->short_name[1] = prefix[1];

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
  dresser->prefer_male_armor = true;

  _dresser_load_races(dresser);
  _dresser_load_limits(dresser);
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

static DresserCharacterAppearance dresser_get_default_character_appearance(Dresser *dresser, DresserCharacter *character)
{
  DresserCharacterAppearance result = {};

  if (character->race >= dresser->races_count) {
    return result;
  }

  if (character->sex > 1) {
    return result;
  }

  DresserCustomizationLimits lims = dresser->race_options[character->race].limits[character->sex];
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

static inline Texture *blit_torso_lower(Texture *dst, Texture *src)
{
  if (dst == NULL || src == NULL) {
    return NULL;
  }

  texture_blit(dst, src, 128, 96 - src->height, 0, 0, src->width, src->height);
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

static inline Texture *blit_legs_lower(Texture *dst, Texture *src)
{
  if (dst == NULL || src == NULL) {
    return NULL;
  }

  texture_blit(dst, src, 128, 160, 0, 0, src->width, src->height);
  return dst;
}

static inline Texture *blit_arms_upper(Texture *dst, Texture *src)
{
  if (dst == NULL || src == NULL) {
    return NULL;
  }

  texture_blit(dst, src, 0, 0, 0, 0, src->width, src->height);
  return dst;
}

static inline Texture *blit_arms_lower(Texture *dst, Texture *src)
{
  if (dst == NULL || src == NULL) {
    return NULL;
  }

  texture_blit(dst, src, 0, 128 - src->height, 0, 0, src->width, src->height);
  return dst;
}

static inline Texture *blit_hands(Texture *dst, Texture *src)
{
  if (dst == NULL || src == NULL) {
    return NULL;
  }

  texture_blit(dst, src, 0, 160 - src->height, 0, 0, src->width, src->height);
  return dst;
}

static inline Texture *blit_feet(Texture *dst, Texture *src)
{
  if (dst == NULL || src == NULL) {
    return NULL;
  }

  texture_blit(dst, src, 128, 256 - src->height, 0, 0, src->width, src->height);
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

static Texture *_dresser_load_texture(Dresser *dresser, char *name)
{
  Asset *asset = asset_loader_get_texture(dresser->loader, name);
  if (asset == NULL) {
      asset = asset_loader_get_texture(dresser->loader, (char *) "World/Scale/1_Null.blp");
      if (asset == NULL) {
        return NULL;
      } else {
        return asset->texture;
      }
  }

  return asset->texture;
}

static Texture *_dresser_load_char_section_texture(Dresser *dresser, DBCFile *dbc, DBCCharSectionsRecord *rec, uint32_t texture_index)
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

static DresserCharacterTextures _dresser_load_character_textures(Dresser *dresser, uint32_t race, uint32_t sex,
                                                                 DresserCharacterAppearance appearance)
{
  printf("!!!! skin: %u\n", appearance.skin_color);
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
          skin[0] = _dresser_load_char_section_texture(dresser, dbc, rec, 0);
          skin[1] = _dresser_load_char_section_texture(dresser, dbc, rec, 1);
        }
        break;
      case DST_FACE:
        if (rec->color == appearance.skin_color && rec->variant == appearance.face) {
          printf("Use face (%u): variant %u color %u\n", rec->type, rec->variant, rec->color);
          face[0] = _dresser_load_char_section_texture(dresser, dbc, rec, 0);
          face[1] = _dresser_load_char_section_texture(dresser, dbc, rec, 1);
        }
        break;
      case DST_FACIAL:
        if (rec->color == appearance.hair_color && rec->variant == appearance.facial) {
          printf("Use feature (%u): variant %u color %u\n", rec->type, rec->variant, rec->color);
          features[0] = _dresser_load_char_section_texture(dresser, dbc, rec, 0);
          features[1] = _dresser_load_char_section_texture(dresser, dbc, rec, 1);
        }
        break;
      case DST_HAIR:
        if (rec->color == appearance.hair_color && rec->variant == appearance.hair) {
          printf("Use hair (%u): variant %u color %u\n", rec->type, rec->variant, rec->color);
          hair = _dresser_load_char_section_texture(dresser, dbc, rec, 0);
          scalp[0] = _dresser_load_char_section_texture(dresser, dbc, rec, 1);
          scalp[1] = _dresser_load_char_section_texture(dresser, dbc, rec, 2);
        } else if (rec->color == appearance.hair_color && hair == NULL) {
          // If no hair matched, grab first variant for selected color. Required for beard on bald Orc
          printf("Fallback to hair (%u): variant %u color %u\n", rec->type, rec->variant, rec->color);
          hair = _dresser_load_char_section_texture(dresser, dbc, rec, 0);
        }
        break;
      case DST_UNDERWEAR:
        if (rec->color == appearance.skin_color) {
          printf("Use underwear (%u): variant %u color %u\n", rec->type, rec->variant, rec->color);
          underwear[0] = _dresser_load_char_section_texture(dresser, dbc, rec, 0);
          underwear[1] = _dresser_load_char_section_texture(dresser, dbc, rec, 1);
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

static Texture *_dresser_load_item_slot_texture(Dresser *dresser, char *name, DresserEquipmentTextureSlot slot, uint32_t sex)
{
  Texture *result = NULL;

  const char *prefixes[] = {
    "Item/TextureComponents/ArmUpperTexture",
    "Item/TextureComponents/ArmLowerTexture",
    "Item/TextureComponents/HandTexture",
    "Item/TextureComponents/TorsoUpperTexture",
    "Item/TextureComponents/TorsoLowerTexture",
    "Item/TextureComponents/LegUpperTexture",
    "Item/TextureComponents/LegLowerTexture",
    "Item/TextureComponents/FootTexture"
  };

  char buf[255];
  char s = (sex == 0 || dresser->prefer_male_armor) ? 'M' : 'F';
  int32_t len = snprintf(buf, sizeof(buf), "%s/%s_%c.blp", prefixes[slot], name, s);
  if (len > sizeof(buf)) {
    return NULL;
  }

  Asset *asset = asset_loader_get_texture(dresser->loader, buf);
  if (asset == NULL) {
    buf[len - 5] = 'U'; // Try to find unisex version
    return _dresser_load_texture(dresser, buf);
  }

  return asset->texture;
}

static char *_dresser_get_model_name(Dresser *dresser, char *name)
{
  static char buf[1024] = {};
  uint32_t dotpos = 0;
  char *n = name;

  while (*n++) {
    dotpos++;
    if (*n == '.') {
      break;
    }
  }

  snprintf(buf, sizeof(buf), "%.*s.m2", dotpos, name);
  return buf;
}

static char *_dresser_get_component_model_name(Dresser *dresser, DresserEquipmentItemSlot slot,
                                               char *name, uint32_t race, uint32_t sex)
{
  if (race >= dresser->races_count) {
    return NULL;
  }

  static char buf[1024] = {};
  uint32_t dotpos = 0;
  char *n = name;

  while (*n++) {
    dotpos++;
    if (*n == '.') {
      break;
    }
  }

  char *short_name = dresser->race_options[race].short_name;
  char sexchar = sex == 0 ? 'M' : 'F';

  switch (slot) {
  case DEIS_HEAD:
    snprintf(buf, sizeof(buf), "Item/ObjectComponents/Head/%.*s_%c%c%c.m2", dotpos, name, short_name[0], short_name[1], sexchar);
    break;

  case DEIS_SHOULDERS:
    snprintf(buf, sizeof(buf), "Item/ObjectComponents/Shoulder/%.*s.m2", dotpos, name);
    break;

  default:
    buf[0] = '\0';
    break;
  }

  return buf;
}

static char *_dresser_get_component_texture_name(Dresser *dresser, DresserEquipmentItemSlot slot,
                                                 char *name, uint32_t race, uint32_t sex)
{
  const char *prefixes[] = {
    "Item/ObjectComponents/Head",
    NULL,
    "Item/ObjectComponents/Shoulder",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "Item/ObjectComponents/Cape"
  };

  static char buf[1024] = {};
  snprintf(buf, sizeof(buf), "%s/%s.blp", prefixes[slot], name);
  return buf;
}

static char *_dresser_get_creature_texture_name(Dresser *dresser, char *creature_name, char *texture_name)
{
  static char buf[1024] = {};
  uint32_t dotpos = 0;
  char *n = texture_name;

  while (*n++) {
    dotpos++;
    if (*n == '.') {
      break;
    }
  }

  snprintf(buf, sizeof(buf), "Creature/%s/%.*s.blp", creature_name, dotpos, texture_name);
  return buf;
}

static M2Model *_dresser_load_item(Dresser *dresser, DresserEquipmentItemSlot slot,
                                   char *name, char *tex0, char *tex1, uint32_t race, uint32_t sex)
{
  char *model_name = _dresser_get_component_model_name(dresser, slot, name, race, sex);
  printf("Item model: %s\n", model_name);
  Asset *asset = asset_loader_get_model(dresser->loader, model_name);
  if (asset == NULL) {
    return NULL;
  }

  M2Model *model = asset->model;

  asset = asset_loader_get_texture(dresser->loader, (char *) "World/Scale/1_Null.blp");
  Texture *placeholder = asset->texture; // TODO: Generate procedurally

  for (size_t i = 0; i < model->texturesCount; i++) {
    printf("Item texture #%zu: type %u name %s\n", i, model->textures[i].type, model->textures[i].name);

    ModelTexture *model_texture = &model->textures[i];
    Texture *texture = NULL;

    switch (model_texture->type) {
    case MTT_INLINE: {
      Asset *texture_asset = asset_loader_get_texture(dresser->loader, model_texture->name);
      if (texture_asset != NULL) {
        texture = texture_asset->texture;
      }
    } break;
    case MTT_OBJECT_SKIN: {
      char *texture_name = _dresser_get_component_texture_name(dresser, slot, tex0, race, sex);
      Asset *texture_asset = asset_loader_get_texture(dresser->loader, texture_name);
      if (texture_asset != NULL) {
        texture = texture_asset->texture;
      }
    } break;
    }

    if (texture != NULL) {
      model->textures[i].texture = texture;
    } else {
      model->textures[i].texture = placeholder;
    }
  }

  return model;
}

static void _dresser_blit_equipment_textures(Dresser *dresser, Texture *skin, Texture **textures)
{
  if (skin == NULL) {
    return;
  }

  for (size_t i = 0; i < DETS_SLOTS_COUNT; i++) {
    Texture *texture = textures[i];
    if (texture == NULL) {
      continue;
    }

    switch ((DresserEquipmentTextureSlot) i) {
    case DETS_ARM_UPPER:
      blit_arms_upper(skin, texture);
      break;

    case DETS_ARM_LOWER:
      blit_arms_lower(skin, texture);
      break;

    case DETS_HAND:
      blit_hands(skin, texture);
      break;

    case DETS_TORSO_UPPER:
      blit_torso_upper(skin, texture);
      break;

    case DETS_TORSO_LOWER:
      blit_torso_lower(skin, texture);
      break;

    case DETS_LEG_UPPER:
      blit_legs_upper(skin, texture);
      break;

    case DETS_LEG_LOWER:
      blit_legs_lower(skin, texture);
      break;

    case DETS_FOOT:
      blit_feet(skin, texture);
      break;

    default:
      break;
    }
  }
}

static void dresser_set_character_appearance(Dresser *dresser, DresserCharacter *character,
                                             DresserCharacterAppearance *appearance)
{
  character->appearance = *appearance;

  M2Model *model = character->base.model;
  character->textures = _dresser_load_character_textures(dresser, character->race, character->sex, *appearance);

  for (size_t i = 0; i < model->texturesCount; i++) {
    printf("Model texture #%zu: type %u name %s\n", i, model->textures[i].type, model->textures[i].name);

    ModelTexture *model_texture = &model->textures[i];
    Texture *texture = NULL;

    switch (model_texture->type) {
    case MTT_INLINE: {
      texture = _dresser_load_texture(dresser, model_texture->name);
    } break;
    case MTT_SKIN:
      texture = character->textures.skin;
      break;
    case MTT_CHAR_HAIR:
      texture = character->textures.hair;
      break;
    case MTT_SKIN_EXTRA:
      texture = character->textures.skin_extra;
      break;
    }

    if (texture != NULL) {
      model->textures[i].texture = texture;
    } else {
      model->textures[i].texture = _dresser_load_texture(dresser, (char *) "World/Scale/1_Null.blp");
    }
  }
}

static void dresser_set_creature_appearance(Dresser *dresser, DresserCreature *creature)
{
  M2Model *model = creature->base.model;

  for (size_t i = 0; i < model->texturesCount; i++) {
    printf("Creature model texture #%zu: type %u name %s\n", i, model->textures[i].type, model->textures[i].name);

    ModelTexture *model_texture = &model->textures[i];
    Texture *texture = NULL;

    switch (model_texture->type) {
    case MTT_INLINE: {
      texture = _dresser_load_texture(dresser, model_texture->name);
    } break;
    case MTT_MONSTER_0:
    case MTT_MONSTER_1:
    case MTT_MONSTER_2: {
      uint32_t idx = model_texture->type - MTT_MONSTER_0;
      texture = creature->textures[idx];
    } break;
    default:
      break;
    }

    if (texture != NULL) {
      model->textures[i].texture = texture;
    } else {
      model->textures[i].texture = _dresser_load_texture(dresser, (char *) "World/Scale/1_Null.blp");
    }
  }
}

static void _dresser_apply_character_geosets(Dresser *dresser, DresserCharacter *character,
                                             uint32_t *helm_vis_ids, uint32_t *geosets)
{
  M2Model *model = character->base.model;

  DresserCharacterAppearance *appearance = &character->appearance;

  geosets[0] = 1;
  geosets[1] = 101;
  geosets[2] = 201;
  geosets[3] = 301;

  Asset *asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/CharHairGeosets.dbc");
  if (asset != NULL) {
    for (size_t i = 0; i < asset->dbc->header.records_count; i++) {
      DBCCharHairGeosetRecord *rec = DBC_RECORD(asset->dbc, DBCCharHairGeosetRecord, i);

      if (rec->race == character->race + 1 && rec->sex == character->sex && rec->variant == appearance->hair) {
        if (rec->is_bald || rec->geoset == 0) {
          geosets[0] = 1; // Default hair option
        } else {
          geosets[0] = rec->geoset;
        }
        break;
      }
    }
  }

  asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/CharacterFacialHairStyles.dbc");
  if (asset != NULL) {
    for (size_t i = 0; i < asset->dbc->header.records_count; i++) {
      DBCCharacterFacialHairStylesRecord *rec = DBC_RECORD(asset->dbc, DBCCharacterFacialHairStylesRecord, i);

      if (rec->race == character->race + 1 && rec->sex == character->sex && rec->variant == appearance->facial) {
        if (rec->geosets[3] > 0) { geosets[1] = rec->geosets[3] + 100; }
        if (rec->geosets[4] > 0) { geosets[3] = rec->geosets[4] + 300; } // SIC! 4th geoset is group 3
        if (rec->geosets[5] > 0) { geosets[2] = rec->geosets[5] + 200; }
        break;
      }
    }
  }

  uint32_t hide_geosets[5][2] = {
    {0, 1},
    {1, 101},
    {3, 301},
    {2, 201},
    {7, 701}
  };

  printf("Head geosets (before helm): %u %u %u %u\n", geosets[0], geosets[1], geosets[2], geosets[3]);

  asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/HelmetGeosetVisData.dbc");
  if (asset != NULL) {
    for (size_t i = 0; i < asset->dbc->header.records_count; i++) {
      DBCHelmetGeosetVisDataRecord *rec = DBC_RECORD(asset->dbc, DBCHelmetGeosetVisDataRecord, i);
      if (rec->id == helm_vis_ids[0] || rec->id == helm_vis_ids[1]) {
        for (size_t mi = 0; mi < sizeof(hide_geosets) / sizeof(hide_geosets[0]); mi++) {
          if (rec->hide_masks[mi] & (1 << (character->race + 1))) {
            geosets[hide_geosets[mi][0]] = hide_geosets[mi][1];
          }
        }
      }
    }
  }

  printf("Head geosets (after helm): %u %u %u %u\n", geosets[0], geosets[1], geosets[2], geosets[3]);

  for (int si = 0; si < model->submeshesCount; si++) {
    if (model->submeshes[si].id == 0) {
      model->submeshes[si].enabled = true;
      continue;
    }

    uint32_t group = model->submeshes[si].id / 100;
    model->submeshes[si].enabled = (model->submeshes[si].id == geosets[group]);
  }
}

static void dresser_set_character_equipment(Dresser *dresser, DresserCharacter *character,
                                            DresserCharacterEquipment *equipment)
{
  Asset *asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/ItemDisplayInfo.dbc");
  DBCFile *dbc = asset->dbc;

  // Bitmask of textures to be applied to skin for particular slot
  static uint32_t slot_textures[] = {
    0b00000000, // DEIS_HEAD
    0b00000110, // DEIS_WRISTS
    0b00000000, // DEIS_SHOULDERS
    0b11100000, // DEIS_LEGS
    0b11000000, // DEIS_FEET
    0b01111011, // DEIS_SHIRT
    0b01111011, // DEIS_CHEST
    0b00000110, // DEIS_HANDS
    0b00100000, // DEIS_WAIST
    0b00000000, // DEIS_BACK
  };

  M2Model *model = character->base.model;

  Texture *textures[DETS_SLOTS_COUNT] = {};
  uint32_t geosets[DRESSER_GEOSETS_COUNT];

  uint32_t items_count = 0;
  M2Model *items[5] = {};
  M2Attachment *attachments[5] = {};

  uint32_t helm_vis_ids[2] = {};

  for (size_t gi = 0; gi < DRESSER_GEOSETS_COUNT; gi++) {
    geosets[gi] = gi * 100 + 1;
  }
  geosets[7] = 702; // Enable ears

  for (size_t i = 0; i < (size_t) DEIS_WEARABLE_SLOTS_COUNT; i++) {
    DresserEquipmentItemSlot slot = (DresserEquipmentItemSlot) i;
    if (equipment->slots[i].display_id == 0) {
      continue;
    }

    DBCItemDisplayInfoRecord *rec = (DBCItemDisplayInfoRecord *) dbc_get_record(dbc, equipment->slots[i].display_id);
    if (rec == NULL) {
      printf("Equipment with display_id %u not found\n", equipment->slots[i].display_id);
      continue;
    }

    printf("\nEquipment slot %zu:\n", i);
    printf("Item %u: visual %u geosets %u %u %u\n", rec->id, rec->visual, rec->geosets[0], rec->geosets[1], rec->geosets[2]);
    printf("Spell %u sound %u helm %u %u\n", rec->spell_id, rec->sound, rec->helm_vis_ids[0], rec->helm_vis_ids[1]);
    printf("Models: %s %s\n", DBC_STRING(dbc, rec->models[0]), DBC_STRING(dbc, rec->models[1]));
    printf("Model textures: %s %s\n", DBC_STRING(dbc, rec->model_textures[0]), DBC_STRING(dbc, rec->model_textures[1]));
    printf("Icon: %s\n", DBC_STRING(dbc, rec->icon));
    printf("Ground model: %s\n", DBC_STRING(dbc, rec->ground_model));

    M2Model *item = NULL;

    switch (slot) {
    case DEIS_HEAD: {
      char *model_name = DBC_STRING(dbc, rec->models[0]);
      char *texture0 = DBC_STRING(dbc, rec->model_textures[0]);
      char *texture1 = DBC_STRING(dbc, rec->model_textures[1]);
      item = _dresser_load_item(dresser, slot, model_name, texture0, texture1, character->race, character->sex);
      if (item != NULL) {
        helm_vis_ids[0] = rec->helm_vis_ids[0];
        helm_vis_ids[1] = rec->helm_vis_ids[1];

        uint32_t idx = items_count++;
        items[idx] = item;
        int16_t att_idx = character->base.model->attachmentLookups[M2_AT_HELM];
        if (att_idx > -1) {
          attachments[idx] = &character->base.model->attachments[att_idx];
        }
      }
    } break;

    case DEIS_SHOULDERS: {
      char *model_name = DBC_STRING(dbc, rec->models[0]);
      char *texture0 = DBC_STRING(dbc, rec->model_textures[0]);
      char *texture1 = DBC_STRING(dbc, rec->model_textures[1]);

      item = _dresser_load_item(dresser, slot, model_name, texture0, texture1, character->race, character->sex);
      if (item != NULL) {
        uint32_t idx = items_count++;
        items[idx] = item;
        int16_t att_idx = character->base.model->attachmentLookups[M2_AT_LSHOULDER];
        if (att_idx > -1) {
          attachments[idx] = &character->base.model->attachments[att_idx];
        }
      }

      model_name = DBC_STRING(dbc, rec->models[1]);
      item = _dresser_load_item(dresser, slot, model_name, texture0, texture1, character->race, character->sex);
      if (item != NULL) {
        uint32_t idx = items_count++;
        items[idx] = item;
        int16_t att_idx = character->base.model->attachmentLookups[M2_AT_RSHOULDER];
        if (att_idx > -1) {
          attachments[idx] = &character->base.model->attachments[att_idx];
        }
      }
    } break;

    case DEIS_HANDS: {
      geosets[4] = 401 + rec->geosets[0];
      if (rec->geosets[0] > 0) {
        geosets[8] = 800; // Gloves > Sleeves
      }
    } break;

    case DEIS_FEET: {
      if (rec->geosets[0] > 0) {
        geosets[5] = 501 + rec->geosets[0];
      }
    } break;

    case DEIS_LEGS: {
      if (rec->geosets[2] > 0) {
        geosets[13] = 1301 + rec->geosets[2];
        geosets[5] = 500;
        geosets[11] = 1100;
      }
    } break;

    case DEIS_CHEST: {
      geosets[8] = 801 + rec->geosets[0];
      geosets[10] = 1001 + rec->geosets[1];
      if (rec->geosets[2] > 0) {
        geosets[5] = 500; // Lap > Boots
        geosets[11] = 1100;
        geosets[13] = 1301 + rec->geosets[2];
      }
    } break;

    case DEIS_BACK: {
      for (size_t ti = 0; ti < model->texturesCount; ti++) {
        if (model->textures[ti].type == MTT_OBJECT_SKIN) {
          char *texture0 = DBC_STRING(dbc, rec->model_textures[0]);
          char *texture_name = _dresser_get_component_texture_name(dresser, slot, texture0, character->race, character->sex);
          model->textures[ti].texture = _dresser_load_texture(dresser, texture_name);
        }
      }

      geosets[15] = 1501 + rec->geosets[0];
    } break;

    default:
      break;
    }

    for (size_t si = 0; si < 8; si++) {
      if (!(slot_textures[slot] & (1 << si))) {
        continue;
      }

      char *texture_name = DBC_STRING(dbc, rec->textures[si]);
      if (*texture_name == '\0') {
        continue;
      }

      printf("Item texture %zu: %s\n", si, texture_name);

      DresserEquipmentTextureSlot tex_slot = (DresserEquipmentTextureSlot) si;
      Texture *texture = _dresser_load_item_slot_texture(dresser, texture_name, tex_slot, character->sex);
      if (texture == NULL) {
        continue;
      }

      if (textures[si] != NULL) {
        texture_blit(textures[si], texture, 0, 0, 0, 0, texture->width, texture->height);
      } else {
        textures[si] = texture;
      }
    }
  }

  character->equipment = *equipment;

  character->base.items_count = items_count;
  for (size_t i = 0; i < items_count; i++) {
    character->base.items[i] = items[i];
    character->base.attachments[i] = attachments[i];
  }

  _dresser_apply_character_geosets(dresser, character, helm_vis_ids, geosets);
  _dresser_blit_equipment_textures(dresser, character->textures.skin, textures);
}

static char *_dresser_get_creature_model_name(Dresser *dresser, uint32_t model)
{
  Asset *asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/CreatureModelData.dbc");
  if (asset == NULL) {
    return NULL;
  }

  DBCCreatureModelDataRecord *rec = (DBCCreatureModelDataRecord *) dbc_get_record(asset->dbc, model);
  if (rec == NULL) {
    return NULL;
  }

  return _dresser_get_model_name(dresser, DBC_STRING(asset->dbc, rec->model_name));
}

static DresserCharacterEquipment _dresser_load_character_equipment(Dresser *dresser, uint32_t *items)
{
  DresserCharacterEquipment result = {};

  static DresserEquipmentItemSlot slot_map[] = {
    DEIS_HEAD,
    DEIS_SHOULDERS,
    DEIS_SHIRT,
    DEIS_CHEST,
    DEIS_WAIST,
    DEIS_LEGS,
    DEIS_FEET,
    DEIS_WRISTS,
    DEIS_HANDS,
    DEIS_TABARD
  };

  for (size_t ii = 0; ii < 10; ii++) {
    if (items[ii] == 0) {
      continue;
    }

    result.slots[slot_map[ii]] = {items[ii]};
  }

  return result;
}

static DresserCharacter *dresser_load_character(Dresser *dresser, uint32_t race, uint32_t sex)
{
  DresserCharacter *result = NULL;

  if (race >= dresser->races_count) {
    return result;
  }

  if (sex > 1) {
    return result;
  }

  DresserRaceOptions *opts = &dresser->race_options[race];
  uint32_t display_id = opts->display_ids[sex];

  Asset *asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/CreatureDisplayInfo.dbc");
  if (asset == NULL) {
    return result;
  }

  DBCCreatureDisplayInfoRecord *display_rec = (DBCCreatureDisplayInfoRecord *) dbc_get_record(asset->dbc, display_id);
  char *model_name = _dresser_get_creature_model_name(dresser, display_rec->model);
  if (model_name == NULL || *model_name == '\0') {
    return result;
  }

  asset = asset_loader_get_model(dresser->loader, (char *) model_name);
  if (asset == NULL) {
    return NULL;
  }

  result = ALLOCATE_ONE(&dresser->loader->allocator, DresserCharacter);
  result->base.type = DCT_CHARACTER;
  result->base.model = asset->model;

  result->race = race;
  result->sex = sex;

  return result;
}

static DresserCreatureBase *dresser_load_creature(Dresser *dresser, uint32_t id)
{
  DresserCreatureBase *result = NULL;

  Asset *asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/CreatureDisplayInfo.dbc");
  if (asset == NULL) {
    return result;
  }

  DBCFile *display_dbc = asset->dbc;
  DBCCreatureDisplayInfoRecord *display_rec = (DBCCreatureDisplayInfoRecord *) dbc_get_record(asset->dbc, id);
  if (display_rec == NULL) {
    return result;
  }

  asset = asset_loader_get_dbc(dresser->loader, (char *) "DBFilesClient/CreatureDisplayInfoExtra.dbc");
  if (asset == NULL) {
    return result;
  }

  DBCCreatureDisplayInfoExtraRecord *extra_rec = (DBCCreatureDisplayInfoExtraRecord *) dbc_get_record(asset->dbc, display_rec->extra_id);
  if (extra_rec != NULL) {
    DresserCharacter *character = dresser_load_character(dresser, extra_rec->race - 1, extra_rec->sex);
    result = (DresserCreatureBase *) character;

    character->appearance.skin_color = extra_rec->skin_color;
    character->appearance.face = extra_rec->face;
    character->appearance.hair = extra_rec->hair;
    character->appearance.hair_color = extra_rec->hair_color;
    character->appearance.facial = extra_rec->feature;
    dresser_set_character_appearance(dresser, character, &character->appearance);

    DresserCharacterEquipment equip = _dresser_load_character_equipment(dresser, extra_rec->items);
    dresser_set_character_equipment(dresser, character, &equip);

  } else {
    char *model_name = _dresser_get_creature_model_name(dresser, display_rec->model);
    if (model_name == NULL || *model_name == '\0') {
      return result;
    }

    printf("Creature model name: %s\n", model_name);

    asset = asset_loader_get_model(dresser->loader, (char *) model_name);
    if (asset == NULL) {
      return NULL;
    }

    DresserCreature *creature = ALLOCATE_ONE(&dresser->loader->allocator, DresserCreature);
    result = (DresserCreatureBase *) creature;

    creature->base.type = DCT_CREATURE;
    creature->base.model = asset->model;

    uint32_t dirlen = 0;
    char *p = model_name;
    do {
      if (*p == '/' || *p == '\\') {
        dirlen = p - model_name;
      }
    } while (*p++);

    char buf[1024] = {};

    for (size_t ti = 0; ti < 3; ti++) {
      char *texture_name = DBC_STRING(display_dbc, display_rec->texture_variants[ti]);
      if (*texture_name == '\0') {
        continue;
      }

      uint32_t dotpos = 0;
      p = texture_name;
      do {
        dotpos++;
      } while (*p != '.' && *p++);

      snprintf(buf, sizeof(buf), "%.*s/%.*s.blp", dirlen, model_name, dotpos, texture_name);
      creature->textures[ti] = _dresser_load_texture(dresser, buf);
    }

    dresser_set_creature_appearance(dresser, creature);
  }

  return result;
}
