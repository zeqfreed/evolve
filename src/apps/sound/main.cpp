#include "platform/platform.h"

#include "utils/math.cpp"
#include "utils/texture.cpp"
#include "utils/memory.cpp"
#include "utils/assets.cpp"
#include "utils/font.cpp"
#include "utils/ui.cpp"

#include "formats/tga.cpp"
#include "formats/wav.cpp"

#include "platform/sound.cpp"

#include "renderer/renderer.cpp"

#include <math.h>

typedef struct Envelope {
  float attack_time;
  float attack_level;
  float decay_time;
  float sustain_level;
  float release_time;
} Envelope;

typedef struct Voice {
  float note;
  Envelope envelope;
  float elapsed_time;
  float inactive_time;
  float peak_level;
  bool active;
} Voice;

typedef enum OscType {
  OSC_SINE,
  OSC_SQUARE,
  OSC_TRIANGLE,
  OSC_SAWTOOTH,
  OSC_NOISE
} OscType;

#define MAX_VOICES 8
#define MAX_KEYS 37

typedef struct KeyState {
  uint32_t octave;
  bool pressed;
} KeyState;

typedef struct SynthState {
  Voice voices[MAX_VOICES];
  uint32_t voices_count;
} SynthState;

typedef struct State {
  PlatformAPI *platform_api;
  KeyboardState *keyboard;
  MouseState *mouse;
  SoundBuffer sound_buffer;

  SoundMixer *sound_mixer;
  PlayingSound tracks[3];
  size_t tracks_count;

  float leftover_dt;
  SynthState synth_state;
  KeyState keys[MAX_KEYS];

  MemoryArena *main_arena;

  RenderingContext rendering_context;
  float screen_width;
  float screen_height;

  Font *font;
  UIContext ui;
} State;

static void enable_ortho(State *state, RenderingContext *ctx)
{
  ctx->viewport_mat = viewport_matrix((float) state->screen_width, (float) state->screen_height, false);
  ctx->projection_mat = orthographic_matrix(0.1f, 100.0f, 0.0f, (float) state->screen_height, (float) state->screen_width, 0);
  ctx->view_mat = Mat44::identity();
  ctx->model_mat = Mat44::translate(0, 0, 0);
  precalculate_matrices(ctx);
}

static Texture *load_tga_texture(State *state, char *filename)
{
  LoadedFile file = load_file(state->platform_api, state->main_arena, filename);
  if (!file.size) {
    printf("Failed to load texture: %s\n", filename);
    return NULL;
  }

  TgaImage image;
  image.read_header(file.contents, file.size);

  TgaHeader *header = &image.header;
  printf("TGA Image width: %d; height: %d; type: %d; bpp: %d\n",
         header->width, header->height, header->imageType, header->pixelDepth);

  printf("X offset: %d; Y offset: %d; FlipX: %d; FlipY: %d\n",
         header->xOffset, header->yOffset, image.flipX, image.flipY);

  Texture *texture = texture_create(state->main_arena, header->width, header->height);
  image.read_into_texture(file.contents, file.size, texture);
  return texture;
}

static Font *load_font(State *state, char *textureFilename)
{
  char ftdFilename[1024];
  if (snprintf(&ftdFilename[0], 1024, "%s.ftd", textureFilename) >= 1024) {
    return NULL;
  }

  LoadedFile file = load_file(state->platform_api, state->main_arena, ftdFilename);
  if (!file.size) {
    printf("Failed to load font: %s\n", ftdFilename);
    return NULL;
  }

  Font *font = (Font *) state->main_arena->allocate(sizeof(Font));
  font->texture = load_tga_texture(state, textureFilename);
  font_init(font, file.contents, file.size);

  return font;
}

static WavFile *load_wav(State *state, char *filename)
{
  LoadedFile file = load_file(state->platform_api, state->main_arena, filename);
  if (!file.size) {
    printf("Failed to load wav: %s\n", filename);
    return NULL;
  }

  WavFile *wav = (WavFile *) state->main_arena->allocate(sizeof(WavFile));
  if (wav_read(wav, file.contents, file.size)) {
    printf("Loaded WAV file %s\nChannels: %d\nSample rate: %d\nBits per sample: %d\nBlock align: %d\nTotal frames: %zu\n", filename, wav->channels, wav->samples_per_second, wav->bits_per_sample, wav->block_align, wav->frames);
    return wav;
  }

  return NULL;
}

static WavFile *load_wav_asset(State *state, char *filename)
{
  LoadedAsset asset = state->platform_api->load_asset(filename);

  WavFile *wav = (WavFile *) state->main_arena->allocate(sizeof(WavFile));
  if (wav_read(wav, asset.data, asset.size)) {
    printf("Loaded WAV file %s\nChannels: %d\nSample rate: %d\nBits per sample: %d\nBlock align: %d\n", filename, wav->channels, wav->samples_per_second, wav->bits_per_sample, wav->block_align);
    return wav;
  }

  return NULL;
}

float note_frequency(float n)
{
    return 440.0f * pow(2.0f, n / 12.0f);
}

static float apply_envelope(Envelope *env, float amplitude, float time_active, float time_released)
{
  if (time_released > env->release_time) {
    return 0.0f;
  }

  bool released = time_released > 0.0f;
  float t = released ? time_active - time_released : time_active;
  float level = env->sustain_level;

  if (t <= env->attack_time) {
    level = (t / env->attack_time) * env->attack_level;
  } else if (time_active - env->attack_time <= env->decay_time) {
    float tdecay = ((time_active - env->attack_time) / env->decay_time);
    level = env->attack_level + (env->sustain_level - env->attack_level) * tdecay;
  }

  if (released) {
    level *= (1.0f - (time_released / env->release_time));
  }

  return level * amplitude;
}

static float osc(OscType type, float frequency, float t, float mod_freq, float mod_amp)
{
  float mod = mod_amp * frequency * sin(2.0f * PI * mod_freq * t);
  float c = 2.0f * PI * frequency;

  switch (type) {
    case OSC_SINE:
      return sin(c * t + mod);
      break;

    case OSC_SQUARE: {
      float sine = sin(c * t + mod);
      return sine > 0.0f ? 1.0f : -1.0f;
    } break;

    case OSC_TRIANGLE: {
			return asin(sin(c * t + mod)) * (2.0 / PI);
    } break;

    case OSC_SAWTOOTH: {
			return (2.0 / PI) * (frequency * PI * fmod(t, 1.0f / frequency) - (PI / 2.0));
    } break;

    case OSC_NOISE: {
      return 2.0f * (float) rand() / (float) RAND_MAX - 1.0f;
    } break;

    default:
      return 0.0f;
  }
}

static void render_sound_buffer_state(State *state, SoundMixerDumpResult dump)
{
  RenderingContext *ctx = &state->rendering_context;

  float pixels_per_byte = 900.0f / state->sound_buffer.length;

  Vec3f top = {10.0, 10.0f, 0.0f};
  Vec3f bottom = {10.0, 50.0f, 0.0f};

  Vec3f p0 = bottom + Vec3f{dump.write_pos * pixels_per_byte, 0.0f, 0.0f};
  Vec3f p1 = top + Vec3f{dump.write_pos * pixels_per_byte, 0.0f, 0.0f};
  ctx->draw_line(ctx, p0, p1, {1.0f, 1.0f, 0.0f, 1.0f});

  p0 = bottom + Vec3f{dump.play_offset * pixels_per_byte, 0.0f, 0.0f};
  p1 = top + Vec3f{dump.play_offset * pixels_per_byte, 0.0f, 0.0f};
  ctx->draw_line(ctx, p0, p1, {0.0f, 1.0f, 0.0f, 1.0f});

  p0 = bottom + Vec3f{dump.write_offset * pixels_per_byte, 0.0f, 0.0f};
  p1 = top + Vec3f{dump.write_offset * pixels_per_byte, 0.0f, 0.0f};
  ctx->draw_line(ctx, p0, p1, {1.0f, 0.0f, 0.0f, 1.0f});

  p0 = Vec3f{10.0f, 52.0f, 0.0f};
  p1 = Vec3f{910.0f, 52.0f, 0.0f};
  ctx->draw_line(ctx, p0, p1, {0.5f, 0.5f, 0.5f, 1.0f});
}

static int32_t add_voice(SynthState *synth_state, Voice voice)
{
  int32_t found_idx = -1;

  if (synth_state->voices_count < MAX_VOICES) {
    found_idx = synth_state->voices_count;
    synth_state->voices[found_idx] = voice;
    synth_state->voices_count++;
    return found_idx;
  }

  // No more space. Replace voice that was inactive the longest time

  bool found_inactive = false;
  float max_inactive_time = 0.0f;

  for (size_t vi = 0; vi < MAX_VOICES; vi++) {
    Voice *v = &synth_state->voices[vi];
    if (!v->active) {
      if (!found_inactive) {
        found_idx = vi;
        found_inactive = true;
        continue;
      }
    }

    if (v->inactive_time > max_inactive_time) {
      found_idx = vi;
      max_inactive_time = v->inactive_time;
    }
  }

  if (found_idx > -1 && found_idx < MAX_VOICES) {
    synth_state->voices[found_idx] = voice;
    return found_idx;
  }

  return -1;
}

static Voice make_voice(float note)
{
  Voice result = {0};
  result.active = true;
  result.note = note;

  result.envelope.attack_time = 0.1f;
  result.envelope.attack_level = 1.0f;
  result.envelope.decay_time = 0.0f;
  result.envelope.sustain_level = 1.0f;
  result.envelope.release_time = 0.35f;

  return result;
}

static size_t playing_sound_synth_add_frames(PlayingSound *sound, float time, float *out, size_t frames_count)
{
  float seconds_per_sample = 1.0f / (float) PLATFORM_SAMPLE_RATE;

  SynthState *synth = (SynthState *) sound->data;

  for (size_t i = 0; i < frames_count; i++) {
    float value = 0.0f;

    for (size_t vi = 0; vi < synth->voices_count; vi++) {
      Voice *v = &synth->voices[vi];

      if (v->active || (v->elapsed_time > 0.0f && v->inactive_time < v->envelope.release_time)) {
        float frequency = note_frequency(v->note);

        float t = v->elapsed_time + (float) i * seconds_per_sample;
        float s = 0.75f * osc(OSC_SINE, frequency, t, 4.0f, 0.005f) +
                  0.25f * osc(OSC_TRIANGLE, frequency, t, 1.0f, 0.005f) +
                  0.15f * osc(OSC_SQUARE, frequency, t, 5.0f, 0.001f);

        float t2 = v->active ? 0.0f : v->inactive_time + (float) i * seconds_per_sample;
        value += apply_envelope(&v->envelope, s, t, t2) * 0.25f;
      }
    }

    for (size_t ch = 0; ch < PLATFORM_CHANNELS; ch++) {
      out[i * PLATFORM_CHANNELS + ch] += value * sound->volume[ch];
    }
  }

  return frames_count;
}

static bool playing_sound_synth_advance(PlayingSound *sound, float dt)
{
  sound->time += dt * sound->speedup;

  SynthState *synth_state = (SynthState *) sound->data;

  for (size_t vi = 0; vi < synth_state->voices_count; vi++) {
    synth_state->voices[vi].elapsed_time += dt;
    if (!synth_state->voices[vi].active) {
      synth_state->voices[vi].inactive_time += dt;
    }
  }

  return true;
}

static bool playing_sound_add_wav(State *state, WavFile *wav, bool loop, float speedup, float volume)
{
  if (state->tracks_count >= 3) {
    return false;
  }

  if (!wav) {
    return false;
  }

  size_t idx = state->tracks_count++;
  state->tracks[idx] = {0};
  state->tracks[idx].wav = wav;
  state->tracks[idx].loop = true;
  state->tracks[idx].time = 0.0f;
  state->tracks[idx].duration = (float) wav->frames / (float) wav->samples_per_second;
  state->tracks[idx].speedup = speedup;
  state->tracks[idx].volume[0] = volume;
  state->tracks[idx].volume[1] = volume;
  state->tracks[idx].add_frames = playing_sound_wav_add_frames;
  state->tracks[idx].advance = playing_sound_advance;

  return true;
}

static void initialize(State *state, DrawingBuffer *buffer)
{
  state->screen_width = (float) buffer->width;
  state->screen_height = (float) buffer->height;

  RenderingContext *ctx = &state->rendering_context;
  set_target(ctx, buffer);

  enable_ortho(state, ctx);
  renderer_set_flags(ctx, RENDER_BLENDING);
  renderer_set_blend_mode(ctx, BLEND_MODE_DECAL);

  ctx->zbuffer = (zval_t *) state->main_arena->allocate(buffer->width * buffer->height * sizeof(zval_t));

  state->font = load_font(state, (char *) "data/fonts/firasans.tga");
  ui_init(&state->ui, &state->rendering_context, state->font, state->keyboard, state->mouse);

  //WavFile *wav = load_wav(state, (char *) "data/misc/CHIPSHOP_Full120_002.wav");
  //WavFile *wav = load_wav(state, (char *) "data/misc/CS_FMArp A_140-E.wav");
  //WavFile *wav = load_wav(state, (char *) "data/misc/CS_Noise A-04.wav");
  //WavFile *wav = load_wav_asset(state, (char *) "Sound/Character/NightElf/NightElfFemale/NightElfFemaleChicken01.wav");
  //WavFile *wav = load_wav_asset(state, (char *) "Sound/Creature/Archer/ArcherYesAttack1.wav");

  playing_sound_add_wav(state, load_wav(state, (char *) "data/misc/alarm01.wav"), true, 1.0f, 1.0f);
  playing_sound_add_wav(state, load_wav_asset(state, (char *) "Sound/Character/NightElf/NightElfFemale/NightElfFemaleChicken01.wav"), true, 1.1f, 1.0f);

  if (state->tracks_count <= 2) {
    size_t idx = state->tracks_count++;
    state->tracks[idx] = {0};
    state->tracks[idx].data = (void *) &state->synth_state;
    state->tracks[idx].loop = false;
    state->tracks[idx].time = 0.0f;
    state->tracks[idx].duration = 0.0f;
    state->tracks[idx].speedup = 1.0f;
    state->tracks[idx].volume[0] = 0.6f;
    state->tracks[idx].volume[1] = 0.6f;
    state->tracks[idx].add_frames = playing_sound_synth_add_frames;
    state->tracks[idx].advance = playing_sound_synth_advance;
  }

  state->platform_api->sound_buffer_init(&state->sound_buffer, PLATFORM_CHANNELS, PLATFORM_SAMPLE_RATE, 3.0f);
  state->sound_mixer = sound_mixer_new(state->platform_api, &state->sound_buffer, 2.0f);

  sound_mixer_clear(state->sound_mixer);
  for (size_t i = 0; i < state->tracks_count; i++) {
    sound_mixer_mix_sound(state->sound_mixer, &state->tracks[i], 1.0f / 30.0f);
  }

  sound_mixer_dump_samples(state->sound_mixer);

  state->platform_api->sound_buffer_play(&state->sound_buffer);
}

static inline void clear_buffer(DrawingBuffer *buffer, Vec4f color)
{
  uint32_t iterCount = (buffer->width * buffer->height) / 4;

  __m128i value = _mm_set1_epi32(rgba_color(color));
  __m128i *p = (__m128i *) buffer->pixels;

  while (iterCount--) {
    *p++ = value;
  }
}

void update_keyboard(State *state)
{
  float notes[13] = {-9.0f, -8.0f, -7.0f, -6.0f, -5.0f, -4.0f, -3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f};
  kb_t keys[13] = {KB_A, KB_W, KB_S, KB_E, KB_D, KB_F, KB_T, KB_G, KB_Y, KB_H, KB_U, KB_J, KB_K};

  bool checked_voices[MAX_VOICES] = {false};
  int32_t key_offset = 12;

  SynthState *synth_state = &state->synth_state;

  for (size_t i = 0; i < 13; i++) {
    float note = notes[i];

    int32_t voice_idx = -1;
    for (size_t vi = 0; vi < synth_state->voices_count; vi++) {
      if (checked_voices[vi]) {
        continue;
      }

      if (synth_state->voices[vi].active && synth_state->voices[vi].note == note) {
        voice_idx = vi;
        break;
      }
    }

    bool pressed = KEY_IS_DOWN(state->keyboard, keys[i]);
    state->keys[i + key_offset].pressed = pressed;

    if (pressed) {
      if (voice_idx > -1) {
        checked_voices[voice_idx] = true;
      } else {
        int32_t idx = add_voice(synth_state, make_voice(note));
        if (idx >= 0 && idx < MAX_VOICES) {
          checked_voices[idx] = true;
        }
      }
    }
  }

  for (size_t vi = 0; vi < synth_state->voices_count; vi++) {
    if (!checked_voices[vi]) {
      synth_state->voices[vi].active = false;
    }
  }
}

void render_keyboard(State *state)
{
  RenderingContext *ctx = &state->rendering_context;
  UIContext *ui = &state->ui;

  renderer_set_flags(ctx, RENDER_BLENDING | RENDER_SHADING);
  renderer_set_blend_mode(ctx, BLEND_MODE_DECAL);

  ui_begin(ui, 10.0f, 10.0f);

  ui_group_begin(ui, (char *) "Keyboard");
  ui_layout_row_begin(ui, 600.0f, 30.0f);

  float pad = 1.0f;
  float x0 = 0.0f;
  float y0 = 600.0f;

  float wkc = (uint32_t) (MAX_KEYS * (7.0f / 12.0f)) + 1.0f;
  float wkw = (state->screen_width - wkc * pad) / wkc;
  float wkh = wkw * 4.25f;
  if (wkh > state->screen_height - y0) {
    wkh = state->screen_height - y0;
    wkw = wkh / 4.25f;
    x0 = (state->screen_width - (wkw + pad) * wkc) / 2.0f;
  }

  float bkw = wkw * 0.582f;
  float bkh = wkh * 0.66f;
  float boverlap = bkw * 0.437f;

  uint32_t first_octave = 4;

  bool blacks[12] =    {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
  size_t indices[12] = {0, 0, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6};

  float bro = wkw - bkw + boverlap;
  float bmo = wkw - (bkw / 2.0f);
  float blo = -boverlap;
  float offsets[12] = {0.0f, bro, 0.0f, blo, 0.0f, 0.0f, bro, 0.0f, bmo, 0.0f, blo, 0.0f};

  for (size_t b = 0; b < 2; b++) {
    for (size_t i = 0; i < MAX_KEYS; i++) {
      size_t key_idx = i % 12;
      size_t octave_idx = i / 12;
      // uint32_t key_octave = first_octave + octave_idx;
      bool pressed = state->keys[i].pressed;

      bool black = blacks[key_idx];
      if (black) {
        if (b == 1) {
          float x = x0 + (wkw + pad) * (indices[key_idx] + 7 * octave_idx) + offsets[key_idx];
          UIRect rect = {x, y0, x + bkw, y0 + bkh};
          Vec4f color = pressed ? Vec4f(0.57f, 0.44f, 0.86f, 1.0f) : Vec4f(0.1f, 0.1f, 0.1f, 1.0f);
          ui_rect(ui, rect, color);
        }
      } else {
        if (b == 0) {
          float x = x0 + (wkw + pad) * (indices[key_idx] + 7 * octave_idx);
          UIRect rect = {x, y0, x + wkw, y0 + wkh};
          Vec4f color = pressed ? Vec4f(0.9f, 0.9f, 0.98f, 1.0f) : Vec4f(1.0f, 1.0f, 0.94f, 1.0f);
          ui_rect(ui, rect, color);
        }
      }
    }
  }

  ui_layout_row_end(ui);
  ui_group_end(ui);
}

void render_ui(State *state)
{
  RenderingContext *ctx = &state->rendering_context;
  UIContext *ui = &state->ui;

  renderer_set_flags(ctx, RENDER_BLENDING | RENDER_SHADING);
  renderer_set_blend_mode(ctx, BLEND_MODE_DECAL);

  ui_begin(ui, 10.0f, 60.0f);

  char buf[255];
  snprintf(buf, 255, "Key pressed: %d, key octave: %d", state->keys[0].pressed, state->keys[0].octave);
  ui_layout_row_begin(ui, 0.0f, 30.0f);
  ui_label(ui, 0.0f, 10.0f, (uint8_t *) buf, UI_ALIGN_LEFT, UI_COLOR_NONE);
  ui_layout_row_end(ui);

  ui_group_begin(ui, (char *) "Keyboard");
  ui_layout_row_begin(ui, 600.0f, 30.0f);

  char names[8] = {'C', 'D', 'E', 'F', 'G', 'A', 'B', 'C'};
  float notes[8] = {-9.0f, -7.0f, -5.0f, -4.0f, -2.0f, 0.0f, 2.0f, 3.0f};
  kb_t keys[8] = {KB_A, KB_S, KB_D, KB_F, KB_G, KB_H, KB_J, KB_K};

  bool checked_voices[MAX_VOICES] = {false};

  SynthState *synth_state = &state->synth_state;

  for (size_t i = 0; i < 8; i++) {
    buf[0] = names[i];
    buf[1] = '0' + i;
    buf[2] = 0;

    float note = notes[i];

    int32_t voice_idx = -1;
    for (size_t vi = 0; vi < synth_state->voices_count; vi++) {
      if (checked_voices[vi]) {
        continue;
      }

      if (synth_state->voices[vi].active && synth_state->voices[vi].note == note) {
        voice_idx = vi;
        break;
      }
    }

    bool key_is_down = KEY_IS_DOWN(state->keyboard, keys[i]);
    if (key_is_down) {
      ui_set_active(ui, ui_hash(ui, UI_BUTTON_TYPE, (uint8_t *) buf));
    }

    if ((ui_button(ui, 30.0f, 30.0f, (uint8_t *) buf) == UI_BUTTON_RESULT_PRESSED) || key_is_down) {
      if (voice_idx > -1) {
        checked_voices[voice_idx] = true;
      } else {
        int32_t idx = add_voice(synth_state, make_voice(note));
        if (idx >= 0 && idx < MAX_VOICES) {
          checked_voices[idx] = true;
        }
      }
    }
  }

  for (size_t vi = 0; vi < synth_state->voices_count; vi++) {
    if (!checked_voices[vi]) {
      synth_state->voices[vi].active = false;
    }
  }

  ui_layout_row_end(ui);
  ui_group_end(ui);
}

static void render_mixer_buffer(State *state, SoundMixer *mixer, Vec3f origin, float width, float height)
{
  uint32_t samples_per_pixel = mixer->samples_count / width;

  RenderingContext *ctx = &state->rendering_context;

  ctx->draw_line(ctx, origin, {origin.x + width, origin.y, 0.0f}, {0.1f, 0.1f, 0.1f, 1.0f});
  ctx->draw_line(ctx, {origin.x, origin.y + height, 0.0f}, {origin.x + width, origin.y + height, 0.0f}, {0.1f, 0.1f, 0.1f, 1.0f});

  uint32_t sample_index = 0;
  for (size_t x = 0; x < width; x++) {
    float sample_avg = 0.0f;
    float sample_min = 0.0f;
    float sample_max = 0.0f;
    for (size_t si = 0; si < samples_per_pixel; si++) {
      float value = mixer->buffer[sample_index + si];
      sample_avg -= sample_avg / (float) samples_per_pixel;
      sample_avg += value / (float) samples_per_pixel;

      if (value < sample_min) {
        sample_min = value;
      }

      if (value > sample_max) {
        sample_max = value;
      }
    }

    sample_index += samples_per_pixel;

    float y0 = origin.y + height / 2.0f + height / 2.0f * sample_min;
    float y1 = origin.y + height / 2.0f + height / 2.0f * sample_max;
    Vec3f p0 = Vec3f{origin.x + (float) x, y0, 0.0f};
    Vec3f p1 = Vec3f{origin.x + (float) x, y1, 0.0f};
    ctx->draw_line(ctx, p0, p1, {0.745f, 0.498f, 0.898f, 1.0f});
  }
}

C_LINKAGE EXPORT void draw_frame(GlobalState *global_state, DrawingBuffer *drawing_buffer, float dt)
{
  State *state = (State *) global_state->state;

  if (!state) {
    MemoryArena *arena = MemoryArena::initialize(global_state->platform_api.allocate_memory(MB(64)), MB(64));

    state = (State *) arena->allocate(MB(1));
    global_state->state = state;

    state->main_arena = arena;

    state->platform_api = &global_state->platform_api;
    state->keyboard = global_state->keyboard;
    state->mouse = global_state->mouse;

    initialize(state, drawing_buffer);
  }

  if (KEY_WAS_PRESSED(state->keyboard, KB_ESCAPE)) {
    state->platform_api->terminate();
    return;
  }

  RenderingContext *ctx = &state->rendering_context;

  clear_buffer(drawing_buffer, {0.0f, 0.0f, 0.0f, 0.0f});
  //clear_zbuffer(ctx);

  update_keyboard(state);

  sound_mixer_clear(state->sound_mixer);
  for (size_t i = 0; i < state->tracks_count; i++) {
    sound_mixer_mix_sound(state->sound_mixer, &state->tracks[i], dt);
  }

  SoundMixerDumpResult dump_result = sound_mixer_dump_samples(state->sound_mixer);
  render_sound_buffer_state(state, dump_result);

  render_keyboard(state);
  render_mixer_buffer(state, state->sound_mixer, {0.0f, 300.0f, 0.0f}, state->screen_width, 300.0f);
  //render_ui(state);
}
