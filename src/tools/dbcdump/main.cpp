#include <cstdlib>
#include <cstdio>

#include <sys/mman.h>

struct DbcHeader {
  uint32_t magic;
  uint32_t recordsCount;
  uint32_t fieldsCount;
  uint32_t recordSize;
  uint32_t stringsSize;
};

struct DbcRecord {
  uint32_t id;
  uint32_t nameOffset;
};

void dbcdump(char *filename)
{
  FILE *file = fopen(filename, "r");
  if (!file) {
    return;
  }

  DbcHeader header;
  if (fread(&header, 1, sizeof(header), file) != sizeof(header)) {
    fclose(file);
    return;
  }

  size_t recordsSize = header.recordSize * header.recordsCount;
  size_t totalSize = recordsSize + header.stringsSize;
  char *buffer = (char *) malloc(totalSize);
  if (fread(buffer, 1, totalSize, file) != totalSize) {
    fclose(file);
    return;
  }

  char *strings = buffer + recordsSize;

  for (int i = 0; i < header.recordsCount; i++) {
    DbcRecord *record = (DbcRecord *) (buffer + i * header.recordSize);
    char *name = strings + record->nameOffset;
    printf("%d\t%s\n", record->id, name);
  }

  fclose(file);
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    exit(1);
  }

  dbcdump(argv[1]);
  return 0;
}
