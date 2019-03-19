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

struct MixedSound;
typedef size_t (* MixedSoundAddFramesFunc)(struct MixedSound *sound, float time, sound_sample_t *out, size_t frames_count);
typedef bool (* MixedSoundAdvanceFunc)(struct MixedSound *sound, float dt);
typedef void (* MixedSoundReleaseFunc)(struct MixedSound *sound);

typedef struct MixedSoundInterface {
  MixedSoundAddFramesFunc add_frames;
  MixedSoundAdvanceFunc advance;
  MixedSoundReleaseFunc release;
} MixedSoundInterface;

typedef struct MixedSound {
  bool loop;
  float time;
  float duration;
  float speedup;
  float volume[2];
  void *data;
  MixedSoundInterface mixed_sound;
} MixedSound;

typedef struct SoundMixer {
  struct PlatformAPI *papi;
  SoundBuffer *sound_buffer;

  sound_sample_t *samples;
  size_t samples_count;

  MixedSound *sounds;
  size_t sounds_capacity;
  size_t sounds_count;

  float dt_avg;
} SoundMixer;

typedef struct SoundMixerMixResult {
  float seconds_dumped;

  struct {
    size_t play_offset;
    size_t write_offset;
    size_t write_pos;
    int32_t write_pos_lead;
  } debug;
} SoundMixerMixResult;
