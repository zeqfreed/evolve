#include <string.h>

#include "dbc.h"

DBCData *dbc_load_animation_data(AssetLoader *loader, char *filename)
{
  LoadedFile file = load_file_temporary(loader, filename);
  if (!file.size) {
    printf("Failed to load DBC file: %s\n", filename);
    return NULL;
  }

  DBCData *result = (DBCData *) loader->dataArena->allocate(file.size);

  DBCHeader *header = (DBCHeader *) file.contents;
  uint8_t *data = (uint8_t *) file.contents + sizeof(DBCHeader);
  char *strings = (char *) data + header->record_size * header->records_count;

  size_t strings_size = (intptr_t) strings - (intptr_t) header;

  result->count = header->records_count;
  result->records = (DBCRecord *) loader->dataArena->allocate(sizeof(DBCRecord) * header->records_count);
  result->strings = (char *) loader->dataArena->allocate(file.size - strings_size);

  memcpy(result->strings, strings, strings_size);

  for (size_t i = 0; i < header->records_count; i++) {
    uint32_t *rec = (uint32_t *) (data + i * header->record_size);
    result->records[i].id = rec[0];
    result->records[i].name = &result->strings[rec[1]];
  }

  for (size_t i = 0; i < result->count; i++) {
    printf("%d\t%s\n", result->records[i].id, result->records[i].name);
  }

  return result;
}

DBCRecord *dbc_get_record(DBCData *data, uint32_t id)
{
  ASSERT(data != NULL);
  DBCRecord *result = NULL;

  for (size_t i = 0; i < data->count; i++) {
    if (data->records[i].id == id) {
      result = &data->records[i];
      break;
    }
  }

  return result;
}
