#pragma once

#include <stdint.h>

typedef uint32_t DBCString;

PACK_START(1);

typedef struct DBCHeader {
  uint32_t magic;
  uint32_t records_count;
  uint32_t fields_count;
  uint32_t record_size;
  uint32_t strings_size;
} PACKED DBCHeader;

typedef struct DBCRecord {
  uint32_t id;
  uint32_t name_offset;
} PACKED DBCRecord;

typedef struct DBCAnimationDataRecord {
  uint32_t id;
  uint32_t name_offset;
  uint32_t flags;
  uint32_t fallback[2];
  uint32_t behaviour_id;
  uint32_t behaviour_tier;
} PACKED DBCAnimationDataRecord;

typedef struct DBCCharSectionsRecord {
  uint32_t id;
  uint32_t race;
  uint32_t sex;
  uint32_t type;
  uint32_t variant;
  uint32_t color;
  uint32_t texture_names[3];
  uint32_t flags;
} PACKED DBCCharSectionsRecord;

typedef struct DBCLocalizedString {
  uint32_t enUS;
  uint32_t koKR;
  uint32_t frFR;
  uint32_t deDE;
  uint32_t enCN;
  uint32_t enTW;
  uint32_t esES;
  uint32_t esMX;
  uint32_t flags;
} PACKED DBCLocalizedString;

typedef struct DBCCharRacesRecord {
  uint32_t id;
  uint32_t flags;
  uint32_t faction_id;
  uint32_t exploration_sound_id;
  uint32_t male_display_id;
  uint32_t female_display_id;
  uint32_t client_prefix;
  float mount_scale;
  uint32_t language;
  uint32_t creature_type;
  uint32_t login_effect_spell_id;
  uint32_t combat_stun_spell_id;
  uint32_t res_sickness_spell_id;
  uint32_t splash_sound_id;
  uint32_t starting_taxi_node;
  uint32_t file;
  uint32_t cinematic_sequence_id;
  DBCLocalizedString i18n_name;
  DBCString features[3];
} PACKED DBCCharRacesRecord;

typedef struct DBCCreatureDisplayInfoRecord {
  uint32_t id;
  uint32_t model;
  uint32_t sound;
  uint32_t extra_id;
  float model_scale;
  uint32_t alpha;
  DBCString texture_variants[3];
  uint32_t size_class;
  uint32_t blood_id;
  uint32_t npc_sound_id;
} PACKED DBCCreatureDisplayInfoRecord;

typedef struct DBCCreatureDisplayInfoExtraRecord {
  uint32_t id;
  uint32_t race;
  uint32_t sex;
  uint32_t skin_color;
  uint32_t face;
  uint32_t hair;
  uint32_t hair_color;
  uint32_t feature;
  uint32_t items[10];
  DBCString texture;
} DBCCreatureDisplayInfoExtraRecord;

typedef struct DBCCreatureModelDataRecord {
  uint32_t id;
  uint32_t flags;
  DBCString model_name;
  uint32_t size_class;
  float model_scale;
  uint32_t blood_id;
  uint32_t footprint_tex_id;
  float footprint_tex_dims[2];
  float footprint_tex_scale;
  uint32_t material;
  uint32_t footstep_shake;
  uint32_t death_thud_shake;
  float collision_dims[2];
  float mount_height;
} PACKED DBCCreatureModelDataRecord;

typedef struct DBCCharHairGeosetRecord {
  uint32_t id;
  uint32_t race;
  uint32_t sex;
  uint32_t variant;
  uint32_t geoset;
  uint32_t is_bald;
} PACKED DBCCharHairGeosetRecord;

typedef struct DBCCharacterFacialHairStylesRecord {
  uint32_t race;
  uint32_t sex;
  uint32_t variant;
  uint32_t geosets[6];
} PACKED DBCCharacterFacialHairStylesRecord;

typedef struct DBCItemDisplayInfoRecord {
  uint32_t id;
  DBCString models[2];
  DBCString model_textures[2];
  DBCString icon;
  uint32_t geosets[3];
  DBCString ground_model;
  uint32_t spell_id;
  uint32_t sound;
  uint32_t helm_vis_ids[2];
  DBCString textures[8];
  uint32_t visual;
} DBCItemDisplayInfoRecord;

typedef struct DBCItemSetRecord {
  uint32_t id;
  DBCLocalizedString i18n_name;
  uint32_t items[17];
  uint32_t spell_ids[8];
  uint32_t thresholds[8];
  uint32_t skill;
  uint32_t rank;
} DBCItemSetRecord;

typedef struct DBCHelmetGeosetVisDataRecord {
  uint32_t id;
  uint32_t hide_masks[5];
} DBCHelmetGeosetVisDataRecord;

PACK_END();

typedef struct DBCFile {
  DBCHeader header;
  DBCRecord record;
} DBCFile;

#define DBC_RECORD(DBC, TYPE, IDX) ((TYPE *) ((uint8_t *) &DBC->record + IDX * sizeof(TYPE)))
#define DBC_STRING(DBC, OFFSET) (((char *) &DBC->record + DBC->header.record_size * DBC->header.records_count) + OFFSET)
