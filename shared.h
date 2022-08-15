#ifndef BITBLT_SHARED_H
#define BITBLT_SHARED_H

#include "display/display.h"

/**
 * Common demo data,structs and callbacks.
 */

typedef void DrawEventCallback(coords from, coords to);

 UserEventCallback * delegate_draw_callback(DrawEventCallback * draw_cb);

 /**
  * Catty paletty! (as well as some badly defined colors)
  */
extern uint8_t palette[];

/**
 * Make a simple 1-bit image.
 * Align into the first byte of the word.
 * Since we're little-endian, that's actually the bottom byte.
 */
PlanarImage * make_img(uint16_t * rawdata, int height);
void draw_text(PlanarImage * on, char * text, int line, bool fixedWidth, bool fixedHeight);

#endif /* BITBLT_SHARED_H */
