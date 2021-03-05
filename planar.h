#ifndef PLANAR_H
#define PLANAR_H

#include <stdint.h>

typedef struct coords {
  uint32_t x;
  uint32_t y;
} coords;
typedef struct image {
  uint32_t * data;
  coords size;
} image;

uint8_t gather_pixel(image ** images, uint32_t planar_pixel_index, uint32_t num);
image * pack(image ** images, uint32_t num);
void planar_bitblt(image ** backgrounds, image ** sprites, coords at, int depth);
#endif
