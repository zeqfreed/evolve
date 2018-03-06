#pragma once

#include "utils/assets.h"

typedef struct DresserCustomizationLimits {
  uint32_t skin_color[2]; // min/max
  uint32_t face[2];
  uint32_t hair_color[2];
  uint32_t hair[2];
  uint32_t facial[2];
} DresserCustomizationOptions;

typedef struct DresserCharacterAppearance {
  uint32_t skin_color;
  uint32_t face;
  uint32_t hair_color;
  uint32_t hair;
  uint32_t facial;
} DresserCharacterAppearance;

typedef enum DresserFeatureType {
  DFT_NONE = 0,
  DFT_NORMAL,
  DFT_FEATURES,
  DFT_MARKINGS,
  DFT_PIERCINGS,
  DFT_EARRINGS,
  DFT_TUSKS,
  DFT_HORNS
} DresserFeatureType;

typedef struct DresserRaceOptions {
  uint32_t id;
  char *name;
  char short_name[2];

  // Sex-specific options: 0 - male, 1 - female
  uint32_t indices[2]; // first record index for this race/sex
  DresserCustomizationLimits limits[2];
  uint32_t display_ids[2];
  DresserFeatureType feature_types[2][2]; // [sex][hair/facial]
  char *feature_names[2][2];
} DresserRaceOptions;

#define DRESSER_MAX_RACES 16

typedef enum DresserSectionType {
  DST_SKIN = 0,
  DST_FACE,
  DST_FACIAL,
  DST_HAIR,
  DST_UNDERWEAR
} DresserSectionType;

typedef struct DresserCharacterTextures {
  Texture *skin;
  Texture *skin_extra;
  Texture *hair;
} DresserCharacterTextures;

typedef enum DresserEquipmentTextureSlot {
  DETS_ARM_UPPER = 0,
  DETS_ARM_LOWER,
  DETS_HAND,
  DETS_TORSO_UPPER,
  DETS_TORSO_LOWER,
  DETS_LEG_UPPER,
  DETS_LEG_LOWER,
  DETS_FOOT,

  DETS_SLOTS_COUNT
} DresserEquipmentTextureSlot;

typedef enum DresserEquipmentItemSlot {
  DEIS_HEAD = 0,
  DEIS_WRISTS,
  DEIS_SHOULDERS,
  DEIS_LEGS,
  DEIS_FEET,
  DEIS_SHIRT,
  DEIS_CHEST,
  DEIS_HANDS,
  DEIS_WAIST,
  DEIS_BACK,
  DEIS_TABARD,
  DEIS_WEARABLE_SLOTS_COUNT
} DresserEquipmentItemSlot;

typedef struct DresserEquipmentItem {
  uint32_t display_id;
} DresserEquipmentItem;

typedef struct DresserCharacterEquipment {
  DresserEquipmentItem slots[DEIS_WEARABLE_SLOTS_COUNT];
} DresserCharacterEquipment;

#define DRESSER_GEOSETS_COUNT 21

typedef struct DresserEquipmentSetDisplayInfo {
  Texture *textures[DETS_SLOTS_COUNT];
  uint32_t geosets[DRESSER_GEOSETS_COUNT];
  M2Model *helm;
} DresserEquipmentSetDisplayInfo;

typedef enum DresserCreatureType {
  DCT_CREATURE = 0,
  DCT_CHARACTER
} DresserCreatureType;

typedef struct DresserCreatureBase {
  DresserCreatureType type;
  M2Model *model;
  uint32_t items_count;
  M2Model *items[10];
  M2Attachment *attachments[10];
} DresserCreatureBase;

typedef struct DresserCreature {
  DresserCreatureBase base;
  Texture *textures[3];
} DresserCreature;

typedef struct DresserCharacter {
  DresserCreatureBase base;
  uint32_t race;
  uint32_t sex;
  DresserCharacterAppearance appearance;
  DresserCharacterEquipment equipment;
  DresserCharacterTextures textures;
} DresserCharacter;

typedef struct Dresser {
  AssetLoader *loader;

  bool prefer_male_armor;
  uint32_t races_count;
  DresserRaceOptions race_options[DRESSER_MAX_RACES];
  DresserCharacterAppearance appearance;
} Dresser;
