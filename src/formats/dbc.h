#pragma once

#include <stdint.h>

typedef struct DBCHeader {
  uint32_t magic;
  uint32_t records_count;
  uint32_t fields_count;
  uint32_t record_size;
  uint32_t strings_size;
} DBCHeader;

typedef struct DBCRecord {
  uint32_t id;
  uint32_t name_offset;
} DBCRecord;

typedef struct DBCAnimationDataRecord {
  uint32_t id;
  uint32_t name_offset;
  uint32_t flags;
  uint32_t fallback[2];
  uint32_t behaviour_id;
  uint32_t behaviour_tier;
} DBCAnimationDataRecord;

typedef struct DBCCharSectionsRecord {
  uint32_t id;
  uint32_t race;
  uint32_t sex;
  uint32_t type;
  uint32_t variant;
  uint32_t color;
  uint32_t texture_names[3];
  uint32_t flags;


} DBCCharSectionsRecord;

typedef struct DBCFile {
  DBCHeader header;
  DBCRecord record;
} DBCFile;

#define DBC_RECORD(DBC, TYPE, IDX) ((TYPE *) ((uint8_t *) &DBC->record + IDX * sizeof(TYPE)))
#define DBC_STRING(DBC, OFFSET) (((char *) &DBC->record + DBC->header.record_size * DBC->header.records_count) + OFFSET)
