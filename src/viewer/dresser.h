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

typedef struct Dresser {
  AssetLoader *loader;

  uint32_t races_count;
  DresserRaceOptions race_options[DRESSER_MAX_RACES];
  DresserCharacterAppearance appearance;
} Dresser;
