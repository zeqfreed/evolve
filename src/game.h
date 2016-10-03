
#include <stdint.h>

#ifdef __cplusplus
    #define C_LINKAGE extern "C"
#else
    #define C_LINKAGE extern
#endif

C_LINKAGE void draw_frame(uint8_t *pixels, int width, int height, int pitch, int bpp);
