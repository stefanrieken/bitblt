#ifndef PLANAR_H
#define PLANAR_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Planar images are multiple separate images,
 * each carrying one bit of color information
 * per pixel.
 */


/**
 * Administrative entity for a planar image.
 * Remember that the raw image arrays may still be stored anywhere.
 */
typedef struct planar_image {
  uint32_t ** planes;
  int width;
  int height;
  int depth;
} planar_image;


/** Because it is an exercise in allocation, here's a convenience function for you. */
planar_image * new_planar_image(int width, int height, int depth);

/** Calculate the width after alignment. */
uint32_t planar_aligned_width(uint32_t width);

/** Gather 'packed' pixel value from across planar images at location planar_pixel_index. */
uint8_t gather_pixel(planar_image * image, uint32_t planar_pixel_index);

/** Convert to packed image */
uint32_t * pack(planar_image * image);

/**
 * Place planar sprite at planar background on location 'at'.
 * 'From' allows you to blit a lower depth sprite onto a higher depth background, starting at plane 'from'.
 */
void planar_bitblt(planar_image * background, planar_image * sprite, int at_x, int at_y, int from_plane, bool zero_transparent);
#endif
