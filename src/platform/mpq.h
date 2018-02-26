#pragma once

#include "defs.h"

typedef struct MPQHeader {
  char magic[4];
  uint32_t header_size;
  uint32_t archive_size;
  uint16_t version;
  uint16_t sector_size_shift;
  uint32_t hash_table_offset;
  uint32_t block_table_offset;
  uint32_t hash_table_size;
  uint32_t block_table_size;
} MPQHeader;

typedef struct MPQHashEntry {
  uint32_t check1;
  uint32_t check2;
  uint16_t locale;
  uint16_t platform;
  uint32_t block_index;
} MPQHashEntry;

typedef struct MPQBlockEntry {
  uint32_t offset;
  uint32_t block_size;
  uint32_t file_size;
  uint32_t flags;
} MPQBlockEntry;

typedef struct MPQArchive {
  OpenFile file;
  MPQHeader header;
  MPQHashEntry *hash_table;
  MPQBlockEntry *block_table;
} MPQArchive;

#define MPQ_MAX_ARCHIVES 32

typedef struct MPQRegistry {
  size_t archives_count;
  MPQArchive archives[MPQ_MAX_ARCHIVES];
} MPQRegistry;

typedef struct MPQFileId {
  uint32_t hash;
  uint32_t check1;
  uint32_t check2;
} MPQFileId;

typedef struct MPQFile {
  MPQFileId id;
  void *data;
  size_t size;
} MPQFile;

#define MPQ_BLOCK_IS_FILE 0x80000000
#define MPQ_BLOCK_IS_SINGLE_UNIT 0x01000000
#define MPQ_BLOCK_ADJUSTED_KEY 0x00020000
#define MPQ_BLOCK_IS_ENCRYPTED 0x00010000
#define MPQ_BLOCK_IS_COMPRESSED 0x00000200
#define MPQ_BLOCK_IS_IMPLODED 0x00000100

#define MPQ_COMPRESSION_DEFLATE 0x02

extern MPQRegistry MPQ_REGISTRY;

C_LINKAGE void mpq_registry_init(MPQRegistry *registry, char *directory);
