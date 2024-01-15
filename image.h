#ifndef BITBLT_IMAGE_H
#define BITBLT_IMAGE_H

#include <stdint.h>
#include <stdbool.h>

#define WORD_SIZE 32
#define WORD_T uint32_t

/**
 * Image functionality shared / common between packed and planar
 */

/**
 * for calculating division and remainders by bitshifting and masking
 */
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
typedef struct Image {
  union {
    WORD_T ** planes; // for planar images
    WORD_T * data; // for packed images
  };
  coords size;
  int depth;
} Image, PlanarImage, PackedImage;

/**
 * Calculate with of image line after word alignment.
 * Use 1bpp to calculate for a planar image plane.
 */
uint32_t image_aligned_width(uint32_t width, int bpp);

typedef bool BitbltFunc(Image * background, Image * sprite, coords from, coords to, coords at, int transparent);

#endif /* BITBLT_IMAGE_H */
