#include "sound.h"

#include <string.h>

#define BYTES_PER_SAMPLE 2
#define SAMPLES_PER_SECOND 44100

static HWND windows_sound_hwnd = NULL;

bool windows_sound_buffer_init(WindowsSoundBuffer *sb) {
    
    LPDIRECTSOUND8 ds; 
    HRESULT hresult = DirectSoundCreate8(NULL, &ds, NULL);
    if (FAILED(hresult)) {
        printf("Failed to initialize DirectSound: %d\n", hresult);
        return false;
    }

    IDirectSound_SetCooperativeLevel(ds, windows_sound_hwnd, DSSCL_PRIORITY);

    WAVEFORMATEX wave_format = {0};
    wave_format.wFormatTag = WAVE_FORMAT_PCM; 
    wave_format.nChannels = 1;
    wave_format.nSamplesPerSec = SAMPLES_PER_SECOND;
    wave_format.wBitsPerSample = BYTES_PER_SAMPLE * 8;
    wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
    sb->wave_format = wave_format;

    sb->length = 3 * wave_format.nAvgBytesPerSec;

    memset(&sb->dsb_desc, 0, sizeof(sb->dsb_desc));
    sb->dsb_desc.dwSize = sizeof(DSBUFFERDESC);
    sb->dsb_desc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_GLOBALFOCUS;
    sb->dsb_desc.dwBufferBytes = sb->length;
    sb->dsb_desc.lpwfxFormat = &wave_format;    

    LPDIRECTSOUNDBUFFER dsb = NULL;
    LPDIRECTSOUNDBUFFER8 buffer = NULL;

    hresult = IDirectSound_CreateSoundBuffer(ds, &sb->dsb_desc, &dsb, NULL);
    if (FAILED(hresult)) {
        printf("Failed to create sound buffer\n");
        return false;
    }

    hresult = IDirectSound_QueryInterface(dsb, &IID_IDirectSoundBuffer8, (LPVOID *) &buffer);
    if (FAILED(hresult)) {
        printf("Failed to query buffer interface\n");
        return false;
    }

    sb->dsb = dsb;
    sb->buffer = buffer;

    memset(&sb->play_info, 0, sizeof(sb->play_info));

    return true;
}

void windows_sound_buffer_finalize(WindowsSoundBuffer *sb) {
    //
}

void windows_sound_buffer_play(WindowsSoundBuffer *sb) {
    if (!sb->play_info.playing) {
        IDirectSoundBuffer8_Play(sb->buffer, 0, 0, DSBPLAY_LOOPING);
    }
}

void windows_sound_buffer_stop(WindowsSoundBuffer *sb) {
    IDirectSoundBuffer8_Stop(sb->buffer);
}

LockedSoundBufferRegion windows_sound_buffer_lock(WindowsSoundBuffer *sb) {
    LockedSoundBufferRegion result = {0};

    WindowsSoundBufferPlayInfo *pi = &sb->play_info;

    uint32_t play_offset;
    uint32_t write_offset;
    HRESULT hresult = IDirectSoundBuffer_GetCurrentPosition(sb->buffer, (LPDWORD) &play_offset, (LPDWORD) &write_offset);
    if (SUCCEEDED(hresult)) {
        printf("Current play offset: %d; write offset: %d\n", play_offset, write_offset);
    } else {
        printf("Failed to get current buffer position\n");
        return result;
    }

    int32_t available_bytes = 0;
    if (play_offset > write_offset) {
        available_bytes = play_offset - pi->write_pos;
    } else {
        if (pi->write_pos >= write_offset) {
            available_bytes = sb->length - pi->write_pos + play_offset;
        } else {
            available_bytes = sb->length - write_offset + play_offset;
        }
    }

    printf("Available bytes: %d; pos: %d\n", available_bytes, pi->write_pos);

    if (play_offset > pi->last_play_offset) {
        pi->total_played += play_offset - pi->last_play_offset;
    } else if (play_offset < pi->last_play_offset) {
        pi->total_played += (play_offset + sb->length - pi->last_play_offset);
    }
    pi->last_play_offset = play_offset;

    // Lock only when there's at least this amount of bytes available for writing
    if (available_bytes < 128) {
        return result;
    }

    if (pi->write_pos >= sb->length) {
        pi->write_pos -= sb->length;
    }
    
    printf("Locking at %u for %u (write offset = %u)\n", pi->write_pos, available_bytes, write_offset);
    hresult = IDirectSoundBuffer8_Lock(sb->buffer, pi->write_pos, available_bytes, &result.mem0, (LPDWORD) &result.len0, &result.mem1, (LPDWORD) &result.len1, 0);
    if (SUCCEEDED(hresult)) {
        printf("Locked: %p (%u), %p (%u)\n", result.mem0, result.len0, result.mem1, result.len1);
    } else {
        printf("Failed to lock buffer: %x\n", hresult);
        return (LockedSoundBufferRegion){0};
    }

    if (result.len0 > available_bytes) {
        result.len0 = available_bytes;
    }

    if (result.len1 > available_bytes - result.len0) {
        result.len1 = available_bytes - result.len0;
    }

    return result;
}

void windows_sound_buffer_unlock(WindowsSoundBuffer *sb, LockedSoundBufferRegion *locked_region) {
    HRESULT hresult = IDirectSoundBuffer8_Unlock(sb->buffer, locked_region->mem0, locked_region->written0, locked_region->mem1, locked_region->written1);
    if (!SUCCEEDED(hresult)) {
        printf("Failed to unlock buffer\n");
    }

    uint32_t written = locked_region->written0 + locked_region->written1;
    sb->play_info.write_pos += written;
    sb->play_info.total_written += written;
}
