#include "sound.h"

static SoundMixer *sound_mixer_new(PlatformAPI *papi, SoundBuffer *sound_buffer, size_t max_sounds, float seconds)
{
    size_t buffer_size = (size_t) (seconds * (float) PLATFORM_SAMPLE_RATE) * PLATFORM_BYTES_PER_SAMPLE * PLATFORM_CHANNELS;
    size_t sounds_size = max_sounds * sizeof(MixedSound);

    SoundMixer *result = (SoundMixer *) papi->allocate_memory(sizeof(SoundMixer) + buffer_size + sounds_size);
    memset(result, 0, sizeof(SoundMixer) + buffer_size + sounds_size);

    result->papi = papi;
    result->sound_buffer = sound_buffer;

    result->samples = (sound_sample_t *) ((uint8_t *) result + sizeof(SoundMixer));
    result->samples_count = buffer_size / PLATFORM_BYTES_PER_SAMPLE;

    result->sounds = (MixedSound *) ((uint8_t *) result->samples + buffer_size);
    result->sounds_capacity = max_sounds;
    result->sounds_count = 0;

    return result;
}

static void sound_mixer_free(SoundMixer *mixer)
{
    ASSERT(mixer);
    ASSERT(mixer->papi);

    for (size_t i = 0; i < mixer->sounds_count; i++) {
      MixedSound *sound = &mixer->sounds[i];
      sound->mixed_sound.release(sound);
    }
    mixer->sounds_count = 0;

    mixer->papi->free_memory(mixer);
}

static size_t mixed_sound_wav_add_frames(MixedSound *sound, float time, sound_sample_t *out, size_t frames_count)
{
  WavFile *wav = (WavFile *) sound->data;

  float frame_step = (float) wav->samples_per_second * sound->speedup / (float) PLATFORM_SAMPLE_RATE;
  int32_t bytes_per_sample = wav->bits_per_sample / 8;

  size_t generated_frames = 0;
  size_t out_len = frames_count * PLATFORM_BYTES_PER_SAMPLE * PLATFORM_CHANNELS;

  float current_frame = time * wav->samples_per_second;

  while (generated_frames < frames_count) {
    int32_t source_frames = MAX(0, (int32_t) (wav->frames - current_frame + 0.5));
    int32_t frames = MIN(source_frames / frame_step, frames_count - generated_frames);

    if (frames <= 0) {
      break;
    }

    uint8_t *pcm_data = (uint8_t *) wav->pcm_data;
    for (size_t fi = 0; fi < frames; fi++) {
      size_t source_frame_index = (size_t) (current_frame + (float) fi * frame_step + 0.5);

      uint8_t *frame = pcm_data + source_frame_index * wav->block_align;
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
      if (current_frame >= wav->frames) {
        current_frame -= (float) wav->frames;
      } else if (wav->frames - current_frame < frame_step) {
        current_frame = frame_step - ((float) wav->frames - current_frame);
      }
    }

    generated_frames += frames;
    out += frames * PLATFORM_CHANNELS;
    out_len -= (frames * PLATFORM_CHANNELS) * PLATFORM_SAMPLE_RATE;
  }

  return generated_frames;
}

static bool mixed_sound_advance(MixedSound *sound, float dt)
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

static void mixed_sound_wav_release(MixedSound *sound)
{
    return;
}

static MixedSound *sound_mixer_add_sound(SoundMixer *mixer, void *data, MixedSoundInterface iface, float duration, bool loop, float speedup, float volume)
{
  if (mixer->sounds_count >= mixer->sounds_capacity) {
      return NULL;
  }

  MixedSound *sound = &mixer->sounds[mixer->sounds_count];
  mixer->sounds_count++;

  sound->data = data;
  sound->loop = loop;
  sound->time = 0.0f;
  sound->duration = duration;
  sound->speedup = speedup;
  sound->volume[0] = volume;
  sound->volume[1] = volume;
  sound->mixed_sound = iface;

  return sound;
}

static MixedSound *sound_mixer_add_wav_sound(SoundMixer *mixer, WavFile *wav, bool loop, float speedup, float volume)
{
    static MixedSoundInterface iface = {
        &mixed_sound_wav_add_frames,
        &mixed_sound_advance,
        &mixed_sound_wav_release
    };

    float duration = (float) wav->frames / (float) wav->samples_per_second;
    MixedSound *sound = sound_mixer_add_sound(mixer, (void *) wav, iface, duration, loop, speedup, volume);

    return sound;
}

static void sound_mixer_advance_sounds(SoundMixer *mixer, float dt)
{
  if (dt <= 0.0f) {
    return;
  }

  for (size_t i = 0; i < mixer->sounds_count; i++) {
    MixedSound *sound = &mixer->sounds[i];
    bool is_playing = mixer->sounds[i].mixed_sound.advance(sound, dt);
    if (!is_playing) {
        sound->mixed_sound.release(sound);
        if (mixer->sounds_count > 1) {
            *sound = mixer->sounds[mixer->sounds_count - 1];
        }
        mixer->sounds_count--;
    }
  }
}

static SoundMixerMixResult sound_mixer_mix(SoundMixer *mixer, float dt)
{
  memset(mixer->samples, 0, mixer->samples_count * sizeof(sound_sample_t));

  for (size_t i = 0; i < mixer->sounds_count; i++) {
    MixedSound *sound = &mixer->sounds[i];
    sound->mixed_sound.add_frames(sound, sound->time, mixer->samples, mixer->samples_count / PLATFORM_CHANNELS);
  }

  mixer->dt_avg -= mixer->dt_avg / 10.0f;
  mixer->dt_avg += dt / 10.0f;
  size_t mix_samples = mixer->dt_avg * (PLATFORM_SAMPLE_RATE * PLATFORM_CHANNELS);

  sound_sample_t *buf = mixer->samples;

  SoundMixerMixResult result = {0};

  LockedSoundBufferRegion locked = mixer->papi->sound_buffer_lock(mixer->sound_buffer);

  result.debug.play_offset = locked.play_offset;
  result.debug.write_offset = locked.write_offset;
  result.debug.write_pos = locked.write_pos;
  result.debug.write_pos_lead = locked.write_pos_lead;
  result.seconds_dumped = 0.0f;

  int32_t skip_samples = 0;
  if (locked.write_pos_lead < 0) {
    skip_samples = -locked.write_pos_lead / BACKEND_BYTES_PER_SAMPLE;
    printf("skipping %u samples to adjust for warped write position\n", skip_samples);
    buf = &buf[skip_samples];

    result.seconds_dumped = skip_samples / (float) (PLATFORM_SAMPLE_RATE * PLATFORM_CHANNELS);

    if (skip_samples >= mixer->samples_count) {
      mixer->papi->sound_buffer_unlock(mixer->sound_buffer, &locked, 0);
      result.seconds_dumped = mixer->samples_count / (float) (PLATFORM_SAMPLE_RATE * PLATFORM_CHANNELS);
      return result;
    }
  } else {
      float locked_seconds = locked.write_pos_lead / (float) (BACKEND_BYTES_PER_SAMPLE * mixer->sound_buffer->sample_rate);
      if (locked_seconds > mixer->dt_avg) {
          size_t drop_samples = (locked_seconds - mixer->dt_avg) * (float) (PLATFORM_SAMPLE_RATE * PLATFORM_CHANNELS);
          mix_samples = (drop_samples > mix_samples) ? 0 : mix_samples - drop_samples;
      }
  }

  result.seconds_dumped += mix_samples / (float) (PLATFORM_SAMPLE_RATE * PLATFORM_CHANNELS);

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

  int32_t overrun_bytes = MAX(0, bytes_written - mix_samples * BACKEND_BYTES_PER_SAMPLE);
  mixer->papi->sound_buffer_unlock(mixer->sound_buffer, &locked, bytes_written - overrun_bytes);

  sound_mixer_advance_sounds(mixer, result.seconds_dumped);

  return result;
}