#include "sound.h"

#include <string.h>
#include "platform.h"

static HWND windows_sound_hwnd = NULL;

// This is minimum amount of samples to cover 30 updates per second with 2 16-bit channels at 48kHz
#define MIN_SAMPLES_PER_WRITE 6400

bool windows_sound_buffer_init(SoundBuffer *sb, uint32_t channels, uint32_t sample_rate, float seconds) {

    LPDIRECTSOUND8 ds;
    HRESULT hresult = DirectSoundCreate8(NULL, &ds, NULL);
    if (FAILED(hresult)) {
        printf("Failed to initialize DirectSound: %d\n", hresult);
        return false;
    }

    IDirectSound_SetCooperativeLevel(ds, windows_sound_hwnd, DSSCL_PRIORITY);

    WAVEFORMATEX wave_format = {0};
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = channels;
    wave_format.nSamplesPerSec = sample_rate;
    wave_format.wBitsPerSample = BACKEND_BYTES_PER_SAMPLE * 8;
    wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
    sb->_private.wave_format = wave_format;

    size_t buffer_size = (size_t) (seconds * (float) sample_rate) * channels * BACKEND_BYTES_PER_SAMPLE;

    sb->channels = channels;
    sb->sample_rate = sample_rate;
    sb->length = buffer_size;

    memset(&sb->_private.dsb_desc, 0, sizeof(sb->_private.dsb_desc));
    sb->_private.dsb_desc.dwSize = sizeof(DSBUFFERDESC);
    sb->_private.dsb_desc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_STICKYFOCUS | DSBCAPS_GLOBALFOCUS;
    sb->_private.dsb_desc.dwBufferBytes = sb->length;
    sb->_private.dsb_desc.lpwfxFormat = &wave_format;

    LPDIRECTSOUNDBUFFER dsb = NULL;
    LPDIRECTSOUNDBUFFER8 buffer = NULL;

    hresult = IDirectSound_CreateSoundBuffer(ds, &sb->_private.dsb_desc, &dsb, NULL);
    if (FAILED(hresult)) {
        printf("Failed to create sound buffer\n");
        return false;
    }

    hresult = IDirectSound_QueryInterface(dsb, &IID_IDirectSoundBuffer8, (LPVOID *) &buffer);
    if (FAILED(hresult)) {
        printf("Failed to query buffer interface\n");
        return false;
    }

    sb->_private.dsb = dsb;
    sb->_private.buffer = buffer;

    memset(&sb->_private.play_info, 0, sizeof(sb->_private.play_info));

    return true;
}

void windows_sound_buffer_finalize(SoundBuffer *sb) {
    //
}

void windows_sound_buffer_play(SoundBuffer *sb) {
    if (!sb->_private.play_info.playing) {
        IDirectSoundBuffer8_Play(sb->_private.buffer, 0, 0, DSBPLAY_LOOPING);
    }
}

void windows_sound_buffer_stop(SoundBuffer *sb) {
    IDirectSoundBuffer8_Stop(sb->_private.buffer);
}

LockedSoundBufferRegion windows_sound_buffer_lock(SoundBuffer *sb) {
    LockedSoundBufferRegion result = {0};

    WindowsSoundBufferPlayInfo *pi = &sb->_private.play_info;

    uint32_t play_offset;
    uint32_t write_offset;
    HRESULT hresult = IDirectSoundBuffer_GetCurrentPosition(sb->_private.buffer, (LPDWORD) &play_offset, (LPDWORD) &write_offset);
    if (FAILED(hresult)) {
        // TODO: Disable / restart sound subsystem
        return result;
    }

    // Check if write_pos leads write_offset minding when both wrap around

    int32_t write_pos_lead = 0;
    if (write_offset < pi->last_write_offset) {
        if (pi->write_pos_wrapped) {
            write_pos_lead = (int32_t) pi->write_pos - (int32_t) write_offset;
        } else {
            write_pos_lead = -(int32_t)(sb->length - pi->write_pos + write_offset);
        }
        pi->write_pos_wrapped = false;
    } else {
        if (pi->write_pos_wrapped) {
            write_pos_lead = pi->write_pos + (sb->length - write_offset);
        } else {
            write_pos_lead = (int32_t) pi->write_pos - (int32_t) write_offset;
        }
    }

    if (write_pos_lead < 0) {
        printf("[WARN] write_pos warped by %d bytes to catch up with write_offset\n", -write_pos_lead + 500);
        pi->write_pos = (write_offset + 500) % sb->length;
        result.write_pos_lead = write_pos_lead - 500;
    } else {
        result.write_pos_lead = write_pos_lead;
    }

    result.play_offset = play_offset;
    result.write_offset = write_offset;
    result.write_pos = pi->write_pos;

    int32_t available_bytes = 0;
    if (pi->write_pos >= play_offset) {
        available_bytes = sb->length - pi->write_pos + play_offset;
    } else {
        available_bytes = play_offset - pi->write_pos;
    }

    if (play_offset >= pi->last_play_offset) {
        pi->total_played += (play_offset - pi->last_play_offset);
    } else {
        pi->total_played += (play_offset + sb->length - pi->last_play_offset);
    }

    pi->last_play_offset = play_offset;
    pi->last_write_offset = write_offset;
    pi->last_write_pos = pi->write_pos;

    if (available_bytes < MIN_SAMPLES_PER_WRITE) {
        // TODO: Disable / restart sound subsystem. This probably means that allocated buffer is not sufficient
        printf("[WARN] Only %d bytes available to write samples. At least %d expected.\n", available_bytes, MIN_SAMPLES_PER_WRITE);
        return result;
    }

    void *mem[2];
    uint32_t len[2];
    hresult = IDirectSoundBuffer8_Lock(sb->_private.buffer, pi->write_pos, available_bytes,
                                       (LPVOID *) &mem[0], (LPDWORD) (LPVOID *) &len[0], &mem[1], (LPDWORD) &len[1], 0);
    if (FAILED(hresult)) {
        // TODO: Disable / restart sound subsystem depending on code?
        printf("[WARN] Failed to lock sound buffer: %x\n", hresult);
        return result;
    }

    result.mem0 = mem[0];
    result.mem1 = mem[1];
    result.len0 = len[0];
    result.len1 = len[1];

    return result;
}

void windows_sound_buffer_unlock(SoundBuffer *sb, LockedSoundBufferRegion *locked_region, uint32_t advance_by) {
    HRESULT hresult = IDirectSoundBuffer8_Unlock(sb->_private.buffer, locked_region->mem0, locked_region->written0, locked_region->mem1, locked_region->written1);
    if (!SUCCEEDED(hresult)) {
        //printf("Failed to unlock buffer\n");
    }

    sb->_private.play_info.write_pos += advance_by;
    sb->_private.play_info.total_written += advance_by;

    if (sb->_private.play_info.write_pos >= sb->length) {
        sb->_private.play_info.write_pos -= sb->length;
        sb->_private.play_info.write_pos_wrapped = true;
    }
}
