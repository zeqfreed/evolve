#include "sound.h"

static SoundMixer *sound_mixer_new(PlatformAPI *papi, SoundBuffer *sound_buffer, float seconds)
{
    size_t buffer_size = (size_t) (seconds * (float) PLATFORM_SAMPLE_RATE) * PLATFORM_BYTES_PER_SAMPLE * PLATFORM_CHANNELS;
    SoundMixer *result = (SoundMixer *) papi->allocate_memory(sizeof(SoundMixer) + buffer_size);
    result->papi = papi;
    result->sound_buffer = sound_buffer;
    result->buffer = (sound_sample_t *) ((uint8_t *) result + sizeof(SoundMixer));
    result->samples_count = buffer_size / PLATFORM_BYTES_PER_SAMPLE;
    result->mixed_samples = 0;

    return result;
}

static void sound_mixer_free(SoundMixer *mixer)
{
    ASSERT(mixer);
    ASSERT(mixer->papi);
    mixer->papi->free_memory(mixer);
}

static void sound_mixer_clear(SoundMixer *mixer)
{
  if (!mixer) {
    return;
  }

  mixer->mixed_samples = 0;
  memset(mixer->buffer, 0, mixer->samples_count * sizeof(float));
}

static size_t playing_sound_wav_add_frames(PlayingSound *sound, float time, sound_sample_t *out, size_t frames_count)
{
  float frame_step = (float) sound->wav->samples_per_second * sound->speedup / (float) PLATFORM_SAMPLE_RATE;
  int32_t bytes_per_sample = sound->wav->bits_per_sample / 8;

  size_t generated_frames = 0;
  size_t out_len = frames_count * PLATFORM_BYTES_PER_SAMPLE * PLATFORM_CHANNELS;

  float current_frame = time * sound->wav->samples_per_second;

  while (generated_frames < frames_count) {
    int32_t source_frames = MAX(0, (int32_t) (sound->wav->frames - current_frame + 0.5));
    int32_t frames = MIN(source_frames / frame_step, frames_count - generated_frames);

    if (frames <= 0) {
      break;
    }

    uint8_t *pcm_data = (uint8_t *) sound->wav->pcm_data;
    for (size_t fi = 0; fi < frames; fi++) {
      size_t source_frame_index = (size_t) (current_frame + (float) fi * frame_step + 0.5);

      uint8_t *frame = pcm_data + source_frame_index * sound->wav->block_align;
      for (size_t ch = 0; ch < PLATFORM_CHANNELS; ch++) {
        uint8_t *sample_data = frame + ch * bytes_per_sample;

        uint32_t sample_value = 0;
        uint32_t shift = (sizeof(sample_value) - bytes_per_sample) * 8;
        for (size_t bi = 0; bi < bytes_per_sample; bi++) {
          sample_value |= (uint32_t)(sample_data[bi]) << shift;
          shift += 8;
        }

        out[fi * PLATFORM_CHANNELS + ch] += (sound_sample_t)((int32_t) sample_value / (sound_sample_t) UINT32_MAX) * sound->volume[ch];
      }
    }

    current_frame += frames * frame_step;
    if (sound->loop) {
      if (current_frame >= sound->wav->frames) {
        current_frame -= (float) sound->wav->frames;
      } else if (sound->wav->frames - current_frame < frame_step) {
        current_frame = frame_step - ((float) sound->wav->frames - current_frame);
      }
    }

    generated_frames += frames;
    out += frames * PLATFORM_CHANNELS;
    out_len -= (frames * PLATFORM_CHANNELS) * PLATFORM_SAMPLE_RATE;
  }

  return generated_frames;
}

static bool playing_sound_advance(PlayingSound *sound, float dt)
{
  sound->time += dt * sound->speedup;

  if (sound->time < sound->duration) {
    return true;
  } else if (sound->loop) {
    while (sound->time >= sound->duration) {
      sound->time -= sound->duration;
    }
    return true;
  }

  return false;
}

static void sound_mixer_mix_sound(SoundMixer *mixer, PlayingSound *sound, float dt)
{
  ASSERT(mixer);
  ASSERT(sound);

  if (!sound->wav) {
    return;
  }

  size_t target_sample_rate = 44100.0f;
  size_t target_channels = 2;
  float frame_step = (float) sound->wav->samples_per_second * sound->speedup / target_sample_rate;

  sound->add_frames(sound, sound->time, mixer->buffer, mixer->samples_count / target_channels);
  sound->advance(sound, dt);

  size_t dt_samples = (size_t) (dt * target_sample_rate) * target_channels; // round then multiply, because it must be a multiple of channel count
  mixer->mixed_samples = MAX(dt_samples, mixer->mixed_samples);
}

static SoundMixerDumpResult sound_mixer_dump_samples(SoundMixer *mixer)
{
  size_t overrun_samples = mixer->samples_count - mixer->mixed_samples;
  sound_sample_t *buf = mixer->buffer;

  SoundMixerDumpResult result = {0};

  LockedSoundBufferRegion locked = mixer->papi->sound_buffer_lock(mixer->sound_buffer);

  result.play_offset = locked.play_offset;
  result.write_offset = locked.write_offset;
  result.write_pos = locked.write_pos;
  result.write_pos_lead = locked.write_pos_lead;

  int32_t skip_samples = 0;
  if (locked.write_pos_lead < 0) {
    skip_samples = -locked.write_pos_lead / sizeof(float);
    printf("skipping %u samples to adjust for warped write position\n", skip_samples);
    buf = &buf[skip_samples];
  }

  if (skip_samples >= mixer->samples_count) {
    mixer->papi->sound_buffer_unlock(mixer->sound_buffer, &locked, 0);
    result.written_samples = mixer->samples_count - skip_samples;
    return result;
  }

  int32_t bytes_to_write = (mixer->samples_count - skip_samples) * sizeof(int16_t);
  int32_t bytes_written = 0;

  if (bytes_to_write > 0) {
    size_t count = MIN(bytes_to_write, locked.len0) / sizeof(int16_t);
    for (size_t i = 0; i < count; i++) {
      ((int16_t *) locked.mem0)[i] = (int16_t) INT16_MAX * buf[i];
    }

    bytes_written += count * sizeof(int16_t);
    locked.written0 = count * sizeof(int16_t);
    buf = &buf[count];

    if (bytes_written < bytes_to_write) {
      count = MIN(bytes_to_write - bytes_written, locked.len1) / sizeof(int16_t);
      for (size_t i = 0; i < count; i++) {
        ((int16_t *) locked.mem1)[i] = (int16_t) INT16_MAX * buf[i];
      }

      bytes_written += count * sizeof(int16_t);
      locked.written1 = count * sizeof(int16_t);
    }
  }

  int32_t overrun_bytes = MAX(0, bytes_written - ((mixer->samples_count - overrun_samples) * BACKEND_BYTES_PER_SAMPLE));
  mixer->papi->sound_buffer_unlock(mixer->sound_buffer, &locked, bytes_written - overrun_bytes);

  result.written_samples = (bytes_written - overrun_bytes) / BACKEND_BYTES_PER_SAMPLE + skip_samples;
  return result;
}