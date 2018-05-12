#pragma once

#include <mmreg.h>
#include <dsound.h>

typedef struct WindowsSoundBufferPlayInfo {
    bool playing;
    uint32_t write_pos;
    uint32_t last_play_offset;
    uint32_t total_written;
    uint32_t total_played;
    int32_t silence_countdown;
} WindowsSoundBufferPlayInfo;

typedef struct WindowsSoundBuffer {
    WAVEFORMATEX wave_format;
    DSBUFFERDESC dsb_desc;
    LPDIRECTSOUNDBUFFER dsb;
    LPDIRECTSOUNDBUFFER8 buffer;
    uint32_t length;
    WindowsSoundBufferPlayInfo play_info;
} WindowsSoundBuffer;
