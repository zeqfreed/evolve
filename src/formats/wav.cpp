#include "platform/platform.h"
#include "wav.h"

#define WAV_FILE_SUPPORT

#define IS_RIFF(x) ((x)[0] == 'R' && (x)[1] == 'I' && (x)[2] == 'F' && (x)[3] == 'F')
#define IS_WAVE(x) ((x)[0] == 'W' && (x)[1] == 'A' && (x)[2] == 'V' && (x)[3] == 'E')
#define IS_FMT(x) ((x)[0] == 'f' && (x)[1] == 'm' && (x)[2] == 't' && (x)[3] == ' ')
#define IS_DATA(x) ((x)[0] == 'd' && (x)[1] == 'a' && (x)[2] == 't' && (x)[3] == 'a')

bool wav_read(WavFile *wav, void *bytes, size_t len)
{
    ASSERT(bytes != NULL);
    ASSERT(len > 16);

    if (!IS_RIFF((char *) bytes)) {
        return false;
    }

    size_t size = ((uint32_t *) bytes)[1];
    if (size + 8 > len) {
        return false;
    }

    if (!IS_WAVE((char *) bytes + 8)) {
        return false;
    }

    WavFmtChunk *fmt_chunk = NULL;
    uint32_t fmt_chunk_size = 0;
    void *data_chunk = NULL;
    uint32_t data_chunk_size = 0;

    char *data = (char *) bytes + 12;
    while (data + 8 < (char *) bytes + len) {
        char *subchunk_name = data;
        uint32_t subchunk_size = *((uint32_t *)(data + 4));

        if (IS_FMT(subchunk_name)) {
            fmt_chunk = (WavFmtChunk *) (data + 8);
            fmt_chunk_size = subchunk_size;
        } else if (IS_DATA(subchunk_name)) {
            data_chunk = (void *) (data + 8);
            data_chunk_size = subchunk_size;
        }

        data += (8 + subchunk_size);
    }

    if (!fmt_chunk) {
        return false;
    }

    if (!fmt_chunk->tag == WAVE_FORMAT_PCM) {
        return false;
    }

    if (!data_chunk) {
        return false;
    }

    wav->channels = fmt_chunk->channels;
    wav->samples_per_second = fmt_chunk->samples_per_second;
    wav->bits_per_sample = fmt_chunk->bits_per_sample;
    wav->block_align = fmt_chunk->block_align;
    wav->pcm_data = (void *) data_chunk;
    wav->pcm_length = data_chunk_size;
    wav->frames = data_chunk_size / wav->block_align;

    return true;
}
