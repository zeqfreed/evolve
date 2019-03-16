#pragma once
#include <stdint.h>

#include "platform/platform.h"
#include "formats/wav.h"

typedef float sound_sample_t;

#define PLATFORM_CHANNELS 2
#define PLATFORM_BYTES_PER_SAMPLE (sizeof(sound_sample_t))
#define PLATFORM_SAMPLE_RATE 44100

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

struct PlayingSound;
typedef size_t (* PlayingSoundAddFramesFunc)(struct PlayingSound *sound, float time, sound_sample_t *out, size_t frames_count);
typedef bool (* PlayingSoundAdvanceFunc)(struct PlayingSound *sound, float dt);

typedef struct PlayingSound {
  bool loop;
  float time;
  float duration;
  float speedup;
  float volume[2];
  union {
    WavFile *wav;
    void *data;
  };
  PlayingSoundAddFramesFunc add_frames;
  PlayingSoundAdvanceFunc advance;
} PlayingSound;

typedef struct SoundMixer {
  struct PlatformAPI *papi;
  SoundBuffer *sound_buffer;
  sound_sample_t *buffer;
  size_t samples_count;
  size_t mixed_samples;
  float dt_avg;
} SoundMixer;

typedef struct SoundMixerDumpResult {
  size_t play_offset;
  size_t write_offset;
  size_t write_pos;
  int32_t write_pos_lead;
  float seconds_dumped;
} SoundMixerDumpResult;
