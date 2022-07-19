#include <stdint.h>

#ifndef PACKED_H
#define PACKED_H

#include "shared.h"
#include "planar.h"

typedef struct packed_image {
  WORD_T * data;
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
void packed_bitblt(packed_image * background, packed_image * sprite, coords at, bool zero_transparent);

#endif /* PACKED_H */
