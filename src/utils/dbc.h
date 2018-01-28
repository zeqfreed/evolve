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
  char *name;
} DBCRecord;

typedef struct DBCData {
  uint32_t count;
  DBCRecord *records;
  char *strings;
} DBCData;
