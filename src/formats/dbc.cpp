#include <string.h>

#include "dbc.h"

DBCRecord *dbc_get_record(DBCFile *dbc, uint32_t id)
{
  for (size_t i = 0; i < dbc->header.records_count; i++) {
    DBCRecord *rec = (DBCRecord *) ((uint8_t *) &dbc->record + i * dbc->header.record_size);
    if (rec->id == id) {
      return rec;
    }
  }

  return NULL;
}

void dbc_dump_header(DBCFile *dbc)
{
  DBCHeader *h = &dbc->header;
  printf("records: %u, fields: %u, record size: %u, strings size: %u\n",
         h->records_count, h->fields_count, h->record_size, h->strings_size);
}
