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
