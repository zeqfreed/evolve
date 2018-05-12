#include <stdint.h>

typedef struct LockedSoundBufferRegion {
    void *mem0;
    void *mem1;
    uint32_t len0;
    uint32_t len1;
    uint32_t written0;
    uint32_t written1;
} LockedSoundBufferRegion;
