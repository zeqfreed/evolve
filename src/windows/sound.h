#pragma once

#include <mmreg.h>
#include <dsound.h>

#define BACKEND_BYTES_PER_SAMPLE 2

typedef struct WindowsSoundBufferPlayInfo {
    bool playing;
    uint32_t write_pos;
    uint32_t last_play_offset;
    uint32_t last_write_offset;
    uint32_t last_write_pos;
    bool write_pos_wrapped; // Indicates that write_pos wrapped around while write_offset is still on previous "circle"
    uint32_t total_written;
    uint32_t total_played;
    int32_t silence_countdown;
} WindowsSoundBufferPlayInfo;

typedef struct WindowsSoundBuffer {
    WAVEFORMATEX wave_format;
    DSBUFFERDESC dsb_desc;
    LPDIRECTSOUNDBUFFER dsb;
    LPDIRECTSOUNDBUFFER8 buffer;
    WindowsSoundBufferPlayInfo play_info;
} WindowsSoundBuffer;
