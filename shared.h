#ifndef BITBLT_SHARED_H
#define BITBLT_SHARED_H

#include <stdint.h>

#define WORD_SIZE 32
#define WORD_T uint32_t

// for calculating division and remainders by bitshifting and masking
// e.g. word size=8; division by 8 = shift right by
#define WORD(x) ((x) >> 5)
#define BIT(x)  ((x) & (WORD_SIZE-1))
typedef struct coords {
  int x;
  int y;
} coords;

typedef struct area {
  coords from;
  coords to;
} area;

/**
 * 'Abstract class' for image,
 * as well as their 'implementation' aliases,
 * combined using a union.
 */
typedef struct image {
  union {
    WORD_T ** planes; // for planar images
    WORD_T * data; // for packed images
  };
  coords size;
  int depth;
} image, planar_image, packed_image;

/**
 * Calculate with of image line after word alignment.
 * Use 1bpp to calculate for a planar image plane.
 */
uint32_t image_aligned_width(uint32_t width, int bpp);

#endif /* BITBLT_SHARED_H */
