#include "platform.h"
#include "mpq.h"

#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ZLIB_APIS
#define MINIZ_NO_MALLOC
#include "../libs/miniz.c"

#define MPQ_HASH_TYPE_OFFSET 0
#define MPQ_HASH_TYPE_CHECK1 1
#define MPQ_HASH_TYPE_CHECK2 2
#define MPQ_HASH_TYPE_FILE_KEY 3

#define MPQ_HASH_ENTRY_EMPTY 0xFFFFFFFF
#define MPQ_HASH_ENTRY_DELETED 0xFFFFFFFE

static uint32_t CRYPT_TABLE[0x500];
MPQRegistry MPQ_REGISTRY = {0};

static void mpq_init_crypt_table()
{
  uint32_t seed   = 0x00100001;
  uint32_t index1 = 0;
  uint32_t index2 = 0;
  int i;

  for (index1 = 0; index1 < 0x100; index1++) {
    for (index2 = index1, i = 0; i < 5; i++, index2 += 0x100){
      uint32_t temp1;
      uint32_t temp2;

      seed  = (seed * 125 + 3) % 0x2AAAAB;
      temp1 = (seed & 0xFFFF) << 0x10;

      seed  = (seed * 125 + 3) % 0x2AAAAB;
      temp2 = (seed & 0xFFFF);

      CRYPT_TABLE[index2] = (temp1 | temp2);
    }
  }
}

static size_t string_length(char *string)
{
  size_t result = 0;
  char *s = string;
  while (*s++) {
    result++;
  }

  return result;
}

static bool string_ends_with(char *string, char *what)
{
  size_t string_len = string_length(string);
  size_t what_len = string_length(what);

  if (string_len < what_len) {
    return false;
  }

  char *s = string + (string_len - what_len);
  char *w = what;
  size_t matched = 0;

  while (*s && *w && *s == *w) {
    matched++;
    s++;
    w++;
  }

  return matched == what_len;
}

static bool mpq_archive_init(MPQArchive *archive, char *filename);

typedef int (* QSortCompareFunc)(const void *, const void *);

static int32_t _mpq_archive_compare(MPQArchive *a, MPQArchive *b)
{
  if (a->priority > b->priority) {
    return -1;
  } else if (a->priority < b->priority) {
    return 1;
  }

  return 0;
}

static inline char downcase(char c)
{
  if (c >= 'A' && c <= 'Z') {
    return c + 32;
  }

  return c;
}

static int32_t string_equal_ni(char *a, char *b, size_t len)
{
  while (len--) {
    if (*a && *b) {
      if (downcase(*a) > downcase(*b)) {
        return 1;
      } else if (downcase(*a) < downcase(*b)) {
        return -1;
      }

      a++;
      b++;
    } else {
      break;
    }
  }

  return 0;
}

void mpq_registry_init(MPQRegistry *registry, char *directory)
{
  mpq_init_crypt_table();

  DirectoryListingIter iter;
  DirectoryListingEntry *entry;

  if (!PLATFORM_API.directory_listing_begin(&iter, directory)) {
    return;
  }

  while ((entry = PLATFORM_API.directory_listing_next_entry(&iter)) != NULL) {
    if (registry->archives_count >= MPQ_MAX_ARCHIVES) {
      continue;
    }

    char *p = entry->name;
    do {
      *p = downcase(*p);
    } while (*p++);

    if (!entry->is_dir && string_ends_with(entry->name, (char *) ".mpq")) {
      uint32_t priority = 0;
      if (strncmp(entry->name, "patch", 5) == 0) {
        if (entry->name[5] == '.') {
          priority = 1;
        } else if (entry->name[5] == '-') {
          priority = atoi(&entry->name[6]);
        }
      }

      char buf[2048] = {0};
      snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s", directory);
      buf[strlen(directory)] = DIRECTORY_SEPARATOR;
      snprintf(&buf[strlen(directory) + 1], 255, "%s", entry->name);

      MPQArchive *archive = &registry->archives[registry->archives_count++];
      archive->priority = priority;

      mpq_archive_init(archive, buf);
    }
  }

  PLATFORM_API.directory_listing_end(&iter);

  qsort(registry->archives, registry->archives_count, sizeof(MPQArchive), (QSortCompareFunc) _mpq_archive_compare);

  for (size_t i = 0; i < registry->archives_count; i++) {
    MPQArchive *archive = &registry->archives[i];
    printf("Archive %s: priority %u, size %u\n", archive->filename, archive->priority, archive->header.archive_size);
  }
}

static void mpq_decrypt_data(void *data, size_t size, uint32_t key)
{
  uint32_t *buf = (uint32_t *) data;
  uint32_t seed = 0xEEEEEEEE;
  size_t count = size / sizeof(uint32_t);

  while(count--) {
    seed += CRYPT_TABLE[0x400 + (key & 0xFF)];
    uint32_t ch = *buf ^ (key + seed);
    key = ((~key << 0x15) + 0x11111111L) | (key >> 0x0B);
    seed = ch + seed + (seed << 5) + 3;
    *buf++ = ch;
  }
}

static char mpq_normalize_char(char ch)
{
  if (ch >= 'a' && ch <= 'z') {
    return ch - 32;
  }

  if (ch == '/') {
    return '\\';
  }

  return ch;
}

static uint32_t mpq_string_hash(char *string, uint32_t type)
{
  uint32_t seed1 = 0x7FED7FED;
  uint32_t seed2 = 0xEEEEEEEE;

  char *p = string;
  while(*p != 0) {
    uint32_t ch = mpq_normalize_char(*p++);
    seed1 = CRYPT_TABLE[(type << 8) + ch] ^ (seed1 + seed2);
    seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
  }

  return seed1;
}

static bool mpq_archive_init(MPQArchive *archive, char *filename)
{
  PlatformFile file = {0};
  if (PAPI_ERROR(PLATFORM_API.file_open(&file, filename))) {
    return false;
  }

  if (PAPI_ERROR(PLATFORM_API.file_read(&file, (void *) &archive->header, 0, sizeof(MPQHeader)))) {
    return false;
  }

  archive->file = file;

  uint32_t len = strlen(filename);
  archive->filename = (char *) PLATFORM_API.allocate_memory(len + 1);
  memcpy(archive->filename, filename, len);

  size_t hash_table_size = archive->header.hash_table_size * sizeof(MPQHashEntry);
  archive->hash_table = (MPQHashEntry *) PLATFORM_API.allocate_memory(hash_table_size);
  if (PAPI_ERROR(PLATFORM_API.file_read(&file, (void *) archive->hash_table, archive->header.hash_table_offset, hash_table_size))) {
    return false;
  }

  uint32_t htkey = mpq_string_hash((char *) "(hash table)", MPQ_HASH_TYPE_FILE_KEY);
  mpq_decrypt_data((void *) archive->hash_table, hash_table_size, htkey);

  size_t block_table_size = archive->header.block_table_size * sizeof(MPQHashEntry);
  archive->block_table = (MPQBlockEntry *) PLATFORM_API.allocate_memory(block_table_size);
  if (PAPI_ERROR(PLATFORM_API.file_read(&file, (void *) archive->block_table, archive->header.block_table_offset, block_table_size))) {
    return false;
  }

  uint32_t btkey = mpq_string_hash((char *) "(block table)", MPQ_HASH_TYPE_FILE_KEY);
  mpq_decrypt_data((void *) archive->block_table, block_table_size, btkey);

  return true;
}

static MPQFileId mpq_file_id(char *name)
{
  MPQFileId result = {0};
  result.hash = mpq_string_hash(name, MPQ_HASH_TYPE_OFFSET);
  result.check1 = mpq_string_hash(name, MPQ_HASH_TYPE_CHECK1);
  result.check2 = mpq_string_hash(name, MPQ_HASH_TYPE_CHECK2);

  return result;
}

static MPQHashEntry *mpq_archive_find_entry(MPQArchive *archive, MPQFileId file_id)
{
  uint32_t start_idx = file_id.hash & (archive->header.hash_table_size - 1);

  MPQHashEntry *table = archive->hash_table;
  MPQHashEntry *result = NULL;

  uint32_t locale = 0;
  uint32_t platform = 0;

  uint32_t idx = start_idx;
  uint32_t tries = 100;

  while (table[idx].block_index != MPQ_HASH_ENTRY_EMPTY && tries) {
    MPQHashEntry *entry = &table[idx];

    if (entry->block_index != MPQ_HASH_ENTRY_DELETED) {
      if (entry->check1 == file_id.check1 && entry->check2 == file_id.check2) {
        result = entry;
        break;
      }
    }

    idx = (idx + 1) & (archive->header.hash_table_size - 1);
    tries--;
  }

  return result;
}

int32_t inflate(void *src, size_t src_size, void *dst, size_t dst_size)
{
  tinfl_decompressor decompressor;
  tinfl_init(&decompressor);

  static mz_uint8 outbuf[TINFL_LZ_DICT_SIZE];

  size_t in_bytes = src_size;
  size_t out_bytes = dst_size;
  int32_t result = tinfl_decompress(&decompressor, (const mz_uint8 *) src, &in_bytes, (mz_uint8 *) dst, (mz_uint8 *) dst, &out_bytes, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF | TINFL_FLAG_PARSE_ZLIB_HEADER);

  return result;
}

MPQFile mpq_load_file(MPQRegistry *registry, char *name)
{
  MPQFile result = {0};

  MPQArchive *matchedArchive = NULL;
  MPQHashEntry *matchedEntry = NULL;

  MPQFileId file_id = mpq_file_id(name);

  for (size_t i = 0; i < registry->archives_count; i++) {
    MPQHashEntry *entry = mpq_archive_find_entry(&registry->archives[i], file_id);
    if (entry != NULL) {
      matchedArchive = &registry->archives[i];
      matchedEntry = entry;
      break;
    }
  }

  if (matchedArchive == NULL || matchedEntry == NULL) {
    printf("[ERROR] No archive contained %s\n", name);
    return result;
  }

  printf("Found %s in archive %s\n", name, matchedArchive->filename);

  MPQBlockEntry *block = &matchedArchive->block_table[matchedEntry->block_index];
  if ((block->flags & MPQ_BLOCK_IS_FILE) != MPQ_BLOCK_IS_FILE) {
    return result;
  }

  size_t uncompressed_sector_size = 512 << matchedArchive->header.sector_size_shift;
  void *uncompressed_data = PLATFORM_API.allocate_memory(block->file_size);

  uint8_t buf[4096] = {0};
  if (PAPI_ERROR(PLATFORM_API.file_read(&matchedArchive->file, (void *) &buf[0], block->offset, 4))) {
    return result;
  }

  int32_t data_offset = ((int32_t *) buf)[0];
  size_t sectors_count = data_offset / sizeof(int32_t);

  int32_t *sectors = (int32_t *) PLATFORM_API.allocate_memory(sectors_count * sizeof(int32_t));
  PLATFORM_API.file_read(&matchedArchive->file, sectors, block->offset, data_offset);

  for (size_t i = 0; i < sectors_count - 1; i++) {
    size_t sector_size = sectors[i + 1] - sectors[i] - 1;
    PLATFORM_API.file_read(&matchedArchive->file, (void *) &buf, block->offset + sectors[i], sector_size);

    void *sector_data = (void *) &buf[0];

    if ((block->flags & MPQ_BLOCK_IS_COMPRESSED) == MPQ_BLOCK_IS_COMPRESSED) {
      // First byte denotes the compression method used
      uint8_t compression = buf[0];
      sector_data = (void *) &buf[1];

      ASSERT(compression == 2);

      inflate(sector_data,
              sector_size,
              (void *) ((uint8_t *) uncompressed_data + i * uncompressed_sector_size),
              uncompressed_sector_size);
    }
  }

  PLATFORM_API.free_memory(sectors);

  result.data = uncompressed_data;
  result.size = block->file_size;

  return result;
}

void mpq_release_file(MPQRegistry *registry, MPQFile *file)
{
  PLATFORM_API.free_memory(file->data);
}
