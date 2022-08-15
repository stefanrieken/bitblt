#ifndef BITBLT_PLANAR_H
#define BITBLT_PLANAR_H

#include <stdint.h>
#include <stdbool.h>

#include "image.h"

/**
 * Planar images are multiple separate images,
 * each carrying one bit of color information
 * per pixel.
 */

/** Because it is an exercise in allocation, here's a convenience function for you. */
PlanarImage * new_planar_image(int width, int height, int depth);

/** Gather 'packed' pixel value from across planar images at location planar_pixel_index. */
uint8_t gather_pixel(PlanarImage * image, uint32_t planar_pixel_index);

/** Convert to packed image */
uint32_t * pack(PlanarImage * image);

/**
 * Place planar sprite at planar background on location 'at'.
 * 'From' allows you to blit a lower depth sprite onto a higher depth background, starting at plane 'from_plane'.
 * Coords 'from' and 'to' define the top left and bottom right corners of the sprite to copy.
 * Notice that this may only work well on word alignments. If you need a more precise cut-out,
 * you might want to try bitblt'ing to a smaller image first.
 */
void planar_bitblt(
   PlanarImage * background,
   PlanarImage * sprite,
   coords from,
   coords to,
   coords at,
   int from_plane,
   int transparent // select color; -1 is no transparency
  );

/** Convenience function that uses defaults for from, to (whole sprite) and from_plane (0). */
void planar_bitblt_full(PlanarImage * background, PlanarImage * sprite, coords at, int transparent);

/** As above, but for copying a single plane. */
void planar_bitblt_plane(PlanarImage * background, PlanarImage * sprite, coords at, int from_plane);

/** Copy all planes. Fits generic BitbltFunc signature. */
void planar_bitblt_all(
   PlanarImage * background,
   PlanarImage * sprite,
   coords from,
   coords to,
   coords at,
   int transparent
  );


#endif /* BITBLT_PLANAR_H */
