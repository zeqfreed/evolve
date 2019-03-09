#pragma once

#define WAVE_FORMAT_PCM 1

PACK_START(1);

typedef struct WavFmtChunk
{
    uint16_t tag;
    uint16_t channels;
    uint32_t samples_per_second;
    uint32_t avg_bytes_per_second;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint16_t ext_size;
    uint16_t valid_bits_per_sample;
    uint32_t channel_mask;
    uint8_t guid[16];
} PACKED WavFmtChunk;

PACK_END();

typedef struct WavFile {
    uint32_t samples_per_second;
    uint16_t channels;
    uint16_t bits_per_sample;
    uint16_t block_align;
    size_t frames;
    void *pcm_data;
    size_t pcm_length;
} WavFile;
