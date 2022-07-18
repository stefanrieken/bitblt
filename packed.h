#include <stdint.h>

#ifndef PACKED_H
#define PACKED_H

#include "planar.h"

typedef struct coords {
  uint32_t x;
  uint32_t y;
} coords;

typedef struct packed_image {
  uint32_t * data;
  coords size;
  int depth;
} packed_image;


/** wrap raw data in a structure */
packed_image * to_packed_image(uint32_t * data, int width, int height, int depth);

planar_image * unpack(packed_image * image);

/** Calculate the width after alignment. */
uint32_t packed_aligned_width(uint32_t width, int bpp);

/**
 * Project 2-bit coloured 'sprite' onto 'background' assuming colour '00' is transparent.
 */
void packed_bitblt(packed_image * background, packed_image * sprite, int at_x, int at_y);

#endif /* PACKED_H */
