#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "game.h"

C_LINKAGE FileContents macos_read_file_contents(char *filename)
{
    FileContents result = {0};

    int fd = open(filename, O_RDONLY);
    if (!fd) {
        // TODO: Handle fopen failure
        return result;
    }

    struct stat stbuf;
    if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
        // TODO: Not a regular file or fstat failed
        return result;
    }

    void *buffer = malloc(stbuf.st_size);
    size_t bytes_read = read(fd, buffer, stbuf.st_size);
    if (bytes_read < stbuf.st_size) {
        // TODO: Could not read all file
        free(buffer);
        return result;
    }

    close(fd);

    result.size = stbuf.st_size;
    result.bytes = buffer;

    return result;
}
