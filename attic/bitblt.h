#include <stdint.h>

#ifndef BITBLT_H
#define BITBLT_H

typedef struct coords {
  uint32_t x;
  uint32_t y;
} coords;
typedef struct image {
  uint32_t * data;
  coords size;
} image;
typedef struct image8 {
  uint8_t * data;
  coords size;
} image8;

/**
 * Project 2-bit coloured 'sprite' onto 'background' assuming colour '00' is transparent.
 */
void bitblt(image * background, image * sprite, coords at, uint8_t bpp);

#endif /* BITBLT_H */
