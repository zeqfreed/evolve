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
MPQRegistry MPQ_REGISTRY = {};

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

    if (!entry->is_dir && string_ends_with(entry->name, (char *) ".mpq")) {
      char buf[2048];
      snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s%c%s", directory, DIRECTORY_SEPARATOR, entry->name);

      MPQArchive *archive = &registry->archives[registry->archives_count++];
      mpq_archive_init(archive, buf);
    }
  }

  PLATFORM_API.directory_listing_end(&iter);
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
  OpenFile file = {};
  if (PAPI_ERROR(PLATFORM_API.file_open(&file, filename))) {
    return false;
  }

  if (PAPI_ERROR(PLATFORM_API.file_read(&file, (void *) &archive->header, 0, sizeof(MPQHeader)))) {
    return false;
  }

  printf("Read archive %s: %d\n", filename, archive->header.archive_size);

  archive->file = file;

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

static MPQHashEntry *mpq_archive_find_entry(MPQArchive *archive, char *name)
{
  uint32_t hash = mpq_string_hash(name, MPQ_HASH_TYPE_OFFSET);
  uint32_t check1 = mpq_string_hash(name, MPQ_HASH_TYPE_CHECK1);
  uint32_t check2 = mpq_string_hash(name, MPQ_HASH_TYPE_CHECK2);
  uint32_t start_idx = hash & (archive->header.hash_table_size - 1);

  MPQHashEntry *table = archive->hash_table;
  MPQHashEntry *result = NULL;

  uint32_t locale = 0;
  uint32_t platform = 0;

  uint32_t idx = start_idx;
  uint32_t tries = 10;

  while (table[idx].block_index != MPQ_HASH_ENTRY_EMPTY && tries) {
    MPQHashEntry *entry = &table[idx];

    if (entry->block_index != MPQ_HASH_ENTRY_DELETED) {
      if (entry->check1 == check1 && entry->check2 == check2) {
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
  MPQFile result = {};

  MPQArchive *matchedArchive = NULL;
  MPQHashEntry *matchedEntry = NULL;

  for (size_t i = 0; i < registry->archives_count; i++) {
    MPQHashEntry *entry = mpq_archive_find_entry(&registry->archives[i], name);
    if (entry != NULL) {
      matchedArchive = &registry->archives[i];
      matchedEntry = entry;
      break;
    }
  }

  if (matchedArchive == NULL || matchedEntry == NULL) {
    return result;
  }

  MPQBlockEntry *block = &matchedArchive->block_table[matchedEntry->block_index];
  if ((block->flags & MPQ_BLOCK_IS_FILE) != MPQ_BLOCK_IS_FILE) {
    return result;
  }

  size_t uncompressed_sector_size = 512 << matchedArchive->header.sector_size_shift;
  void *uncompressed_data = PLATFORM_API.allocate_memory(block->file_size);

  uint8_t buf[4096] = {};
  if (PAPI_ERROR(PLATFORM_API.file_read(&matchedArchive->file, (void *) &buf[0], block->offset, 4))) {
    return result;
  }

  int32_t data_offset = ((int32_t *) buf)[0];
  size_t sectors_count = data_offset / sizeof(int32_t);

  int32_t *sectors = new int32_t[sectors_count];
  PLATFORM_API.file_read(&matchedArchive->file, sectors, block->offset, data_offset);

  for (size_t i = 0; i < sectors_count - 1; i++) {
    size_t sector_size = sectors[i + 1] - sectors[i] - 1;
    PLATFORM_API.file_read(&matchedArchive->file, (void *) &buf, block->offset + sectors[i], sector_size);

    void *sector_data = (void *) &buf[0];

    if ((block->flags & MPQ_BLOCK_IS_COMPRESSED) == MPQ_BLOCK_IS_COMPRESSED) {
      // First byte denotes the compression method used
      uint8_t compression = buf[0];
      sector_data = (void *) &buf[1];

      inflate(sector_data,
              sector_size,
              (void *) ((uint8_t *) uncompressed_data + i * uncompressed_sector_size),
              uncompressed_sector_size);
    }
  }

  delete[] sectors;

  result.data = uncompressed_data;
  result.size = block->file_size;

  return result;
}

void mpq_release_file(MPQRegistry *registry, MPQFile *file)
{
  PLATFORM_API.free_memory(file->data);
}
