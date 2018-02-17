#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "fs.h"

int32_t macos_fs_size(char *filename)
{
    int fd = open(filename, O_RDONLY);
    if (!fd) {
        // TODO: Handle fopen failure
        return -1;
    }

    struct stat stbuf;
    if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
        // TODO: Not a regular file or fstat failed
        close(fd);
        return -1;
    }

    close(fd);
    return stbuf.st_size;
}

int32_t macos_fs_read(char *filename, void *memory, uint32_t size)
{
    int fd = open(filename, O_RDONLY);
    if (!fd) {
        // TODO: Handle fopen failure
        return -1;
    }

    struct stat stbuf;
    if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
        // TODO: Not a regular file or fstat failed
        return -1;
    }

    size_t bytes_left = size;
    size_t total_read = 0;
    void *buf = memory;

    while (true) {
        ssize_t bytes_read = read(fd, buf, bytes_left);

        if (bytes_read > 0) {
            bytes_left -= bytes_read;
            buf = (uint8_t *) buf + bytes_read;
            total_read += bytes_read;
        } else if (bytes_read < 0) {
            close(fd);
            return total_read;
        } else if (bytes_read == 0) {
            break;
        }
    }

    return total_read;
}

int32_t macos_file_open(MacosOpenFile *file, char *filename)
{
  int fd = open(filename, O_RDONLY);
  if (!fd) {
    // TODO: Handle fopen failure
    return -1;
  }

  file->fd = fd;
  return 0;
}

int32_t macos_file_read(MacosOpenFile *file, void *dst, uint32_t offset, uint32_t bytes)
{
  void *buf = dst;

  size_t bytes_left = bytes;
  size_t total_read = 0;

  lseek(file->fd, offset, SEEK_SET);

  while (total_read < bytes) {
    ssize_t bytes_read = read(file->fd, buf, bytes_left);

    if (bytes_read > 0) {
      bytes_left -= bytes_read;
      buf = (uint8_t *) buf + bytes_read;
      total_read += bytes_read;
    } else if (bytes_read < 0) {
      return total_read;
    } else if (bytes_read == 0) {
      break;
    }
  }

  return total_read;
}

static MacosDirectoryListingEntry directory_listing_entry;

bool macos_directory_listing_begin(MacosDirectoryListingIter *iter, char *directory)
{
  DIR *dp = opendir(directory);
  if (dp == NULL) {
    return false;
  }

  iter->dp = dp;
  return true;
}

MacosDirectoryListingEntry *macos_directory_listing_next_entry(MacosDirectoryListingIter *iter)
{
  struct dirent *de = readdir(iter->dp);
  if (de == NULL) {
    return NULL;
  }

  directory_listing_entry.name = de->d_name;
  directory_listing_entry.is_dir = (de->d_type == DT_DIR);

  return &directory_listing_entry;
}

void macos_directory_listing_end(MacosDirectoryListingIter *iter)
{
  closedir(iter->dp);
}
