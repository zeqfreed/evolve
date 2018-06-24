#pragma once
#include <stdint.h>

typedef struct LockedSoundBufferRegion {
    void *mem0;
    void *mem1;
    uint32_t len0;
    uint32_t len1;
    uint32_t written0;
    uint32_t written1;
    uint32_t play_offset;
    uint32_t write_offset;
    uint32_t write_pos;
    int32_t write_pos_lead;
} LockedSoundBufferRegion;
