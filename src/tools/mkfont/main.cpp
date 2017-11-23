#include <cstdlib>
#include <cstdio>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define countof(a) (sizeof(a) / sizeof(a[0]))

typedef struct TgaHeader
{
  uint8_t idLength;
  uint8_t colorMapType;
  uint8_t imageType;
  uint16_t colorMapStart;
  uint16_t colorMapLength;
  uint8_t colorMapDepth;
  uint16_t xOffset;
  uint16_t yOffset;
  uint16_t width;
  uint16_t height;
  uint8_t pixelDepth;
  uint8_t imageDescriptor;
} __attribute__((packed)) TgaHeader;

void *read_file(char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    return NULL;
  }

  fseek(file, 0L, SEEK_END);
  long size = ftell(file);
  fseek(file, 0L, SEEK_SET);

  printf("size: %zu\n", size);

  void *result = malloc(size);
  uint8_t *p = (uint8_t*) result;
  size_t block = 4096;

  while (true) {
    size_t n = fread(p, 1, block, file);
    p += n;
    if (n == block) {
      continue;
    }

    if (feof(file)) {
      break;
    } else if (ferror(file)) {
      perror("Failed to read file:");
      free(result);
      return NULL;
    }
  }

  return result;
}

bool write_tga(char *outfile, uint8_t *alpha, size_t w, size_t h)
{
  FILE *file = fopen(outfile, "wb");
  if (!file) {
    return false;
  }

  TgaHeader header = {};
  header.imageType = 2;
  header.width = w;
  header.height = h;
  header.pixelDepth = 32;
  header.imageDescriptor = 8 | (1 << 5); // 32 bit + Bottom-up

  int n = fwrite(&header, 1, sizeof(TgaHeader), file);
  if (n != sizeof(TgaHeader)) {
    return false;
  }

  size_t bufsize = w * h * sizeof(uint32_t);
  uint32_t *buf = (uint32_t *) malloc(bufsize);

  for (int i = 0; i < w*h; i++) {
    uint8_t a = alpha[i];
    buf[i] = (a << 24) | 0xFFFFFF;
  }

  n = fwrite(buf, 1, bufsize, file);
  if (n != bufsize) {
    free(buf);
    return false;
  }

  free(buf);
  return true;
}

typedef struct FTDHeader {
  char magic[4];
  float fontSize;
  uint32_t lineHeight;
  uint32_t codepointsCount;
  uint32_t rangesCount;
  uint32_t rangesOffset;
  uint32_t quadsCount;
  uint32_t quadsOffset;
  uint32_t kernPairsCount;
  uint32_t kernPairsOffset;
} __attribute__((packed)) FTDHeader;

bool write_ftd(char *ftdfile, stbtt_fontinfo *font, int size, size_t w, size_t h, stbtt_pack_range *packRanges, size_t numRanges)
{
  FILE *file = fopen(ftdfile, "wb");
  if (!file) {
    return false;
  }

  FTDHeader header = {};
  header.magic[0] = 'F';
  header.magic[1] = 'T';
  header.magic[2] = 'D';
  header.magic[3] = '0';

  uint32_t *ranges = (uint32_t *) calloc(sizeof(uint32_t) * 2 * numRanges, 1);
  size_t numCodepoints = 0;

  for (size_t i = 0; i < numRanges; i++) {
    uint32_t first = packRanges[i].first_unicode_codepoint_in_range;
    uint32_t onePastLast = first + packRanges[i].num_chars;

    ranges[i*2] = first;
    ranges[i*2+1] = onePastLast;
    numCodepoints += packRanges[i].num_chars;
  }

  float *quads = (float *) calloc(sizeof(float) * 8 * numCodepoints, 1);
  size_t qidx = 0;

  for (size_t i = 0; i < numRanges; i++) {
    uint32_t firstCodepoint = packRanges[i].first_unicode_codepoint_in_range;

    for (size_t j = 0; j < packRanges[i].num_chars; j++) {
      stbtt_aligned_quad q;
      float x = 0.0f;
      float y = 0.0f;
      stbtt_GetPackedQuad(packRanges[i].chardata_for_range, w, h, j, &x, &y, &q, 0);

      quads[qidx*8] = q.x0;
      quads[qidx*8+1] = q.y0;
      quads[qidx*8+2] = q.x1;
      quads[qidx*8+3] = q.y1;

      quads[qidx*8+4] = q.s0 * w;
      quads[qidx*8+5] = h - q.t0 * h;
      quads[qidx*8+6] = q.s1 * w;
      quads[qidx*8+7] = h - q.t1 * h;

      qidx++;
    }
  }

  size_t numKernPairs = numCodepoints * numCodepoints;
  float *kernPairs = (float *) calloc(sizeof(float) * numKernPairs, 1);
  float scale = stbtt_ScaleForPixelHeight(font, size);

  size_t cpIdx0 = 0;

  for (size_t i0 = 0; i0 < numRanges; i0++) {
    uint32_t firstCp0 = packRanges[i0].first_unicode_codepoint_in_range;

    for (size_t j0 = 0; j0 < packRanges[i0].num_chars; j0++) {
      uint32_t cp0 = firstCp0 + j0;

      int advance0, lsb0;
      stbtt_GetCodepointHMetrics(font, cp0, &advance0, &lsb0);

      size_t cpIdx1 = 0;

      for (size_t i1 = 0; i1 < numRanges; i1++) {
        uint32_t firstCp1 = packRanges[i1].first_unicode_codepoint_in_range;

        for (size_t j1 = 0; j1 < packRanges[i1].num_chars; j1++) {
          uint32_t cp1 = firstCp1 + j1;
          float kerning = stbtt_GetCodepointKernAdvance(font, cp0, cp1);
          kernPairs[cpIdx0 * numCodepoints + cpIdx1] = scale * (advance0 + kerning);
          cpIdx1++;
        }
      }

      cpIdx0++;
    }
  }

  size_t rangesSize = sizeof(uint32_t) * 2 * numRanges;
  size_t quadsSize = sizeof(float) * 8 * numCodepoints;
  size_t kernPairsSize = sizeof(float) * numKernPairs;

  printf("num ranges: %zu; ranges size: %zu\n", numRanges, rangesSize);
  printf("num codepoints: %zu; quads size: %zu\n", numCodepoints, quadsSize);
  printf("num kern pairs: %zu; kern pairs size: %zu\n", numKernPairs, kernPairsSize);

  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(font, &ascent, &descent, &lineGap);
  header.fontSize = size;
  header.lineHeight = scale * (ascent - descent + lineGap);

  header.codepointsCount = numCodepoints;
  header.rangesCount = numRanges;
  header.rangesOffset = sizeof(FTDHeader);
  header.quadsCount = numCodepoints;
  header.quadsOffset = header.rangesOffset + rangesSize;
  header.kernPairsCount = numKernPairs;
  header.kernPairsOffset = header.quadsOffset + quadsSize;

  fwrite(&header, 1, sizeof(header), file);
  fwrite(ranges, 1, rangesSize, file);
  fwrite(quads, 1, quadsSize, file);
  fwrite(kernPairs, 1, kernPairsSize, file);

  fclose(file);

  free(quads);
  free(ranges);
  free(kernPairs);
  return true;
}

#define FONT_SIZE 32

int mkfont(char *fontfile, char *outfile) {
  uint8_t *data = (uint8_t *) read_file(fontfile);
  if (!data) {
    return 1;
  }

  stbtt_fontinfo font;
  stbtt_InitFont(&font, data, stbtt_GetFontOffsetForIndex(data, 0));

  size_t pw = 512;
  size_t ph = 512;
  uint8_t *pixels = (uint8_t *) calloc(pw * ph * sizeof(uint8_t), 1);

  uint32_t charRanges[][2] = {
    {0xFFFD, 1}, // Replacement character
    {0x0020, 95}, // Basic latin
    {0x0410, 64}, // Russian cyrillic
    {0x0401, 1}, // Ё
    {0x0451, 1}, // ё
    {0x1F4A9, 1}, // Pile of poo
    {0x1F44D, 2}, // Thumbs up/down
    {0x1F47B, 1}, // Ghost
    {0x1F47E, 2}, // Alien, Imp, Skull
    {0x1F600, 5} // Emoticons
  };

  stbtt_pack_range *packRanges = (stbtt_pack_range *) calloc(countof(charRanges) * sizeof(stbtt_pack_range), 1);
  for (size_t i = 0; i < countof(charRanges); i++) {
    packRanges[i].font_size = FONT_SIZE;
    packRanges[i].first_unicode_codepoint_in_range = charRanges[i][0];
    packRanges[i].num_chars = charRanges[i][1];
    packRanges[i].chardata_for_range = (stbtt_packedchar *) malloc(charRanges[i][1] * sizeof(stbtt_packedchar));
  }

  stbtt_pack_context pctx;
  if (!stbtt_PackBegin(&pctx, pixels, pw, ph, 0, 1, NULL)) {
    printf("stbtt_PackBegin failed\n");
  }

  stbtt_PackSetOversampling(&pctx, 2, 2);

  if (!stbtt_PackFontRanges(&pctx, data, 0, packRanges, countof(charRanges))) {
    printf("stbtt_PackFontRanges failed\n");
  }
  stbtt_PackEnd(&pctx);

  char ftdfile[1024] = {};
  snprintf(&ftdfile[0], sizeof(ftdfile), "%s.ftd", outfile);
  write_ftd(ftdfile, &font, FONT_SIZE, pw, ph, packRanges, countof(charRanges));

  write_tga(outfile, pixels, pw, ph);

  free(pixels);
  for (size_t i = 0; i < countof(charRanges); i++) {
    free(packRanges[i].chardata_for_range);
  }
  free(packRanges);

  return 0;
}

int main(int argc, char **argv)
{
  if (argc < 3) {
    exit(1);
  }

  int ret = mkfont(argv[1], argv[2]);
  return ret;
}
