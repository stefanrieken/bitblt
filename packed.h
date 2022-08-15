#include <stdint.h>

#ifndef BITBLT_PACKED_H
#define BITBLT_PACKED_H

#include "image.h"
#include "planar.h"

PackedImage * new_packed_image(int width, int height, int depth);

/** wrap raw data in a structure */
PackedImage * to_packed_image(uint32_t * data, int width, int height, int depth);

PlanarImage * unpack(PackedImage * image);

/**
 * Project 2-bit coloured 'sprite' onto 'background' assuming colour '00' is transparent.
 * Copy sprite from top-left 'from' to bottom-right 'to', into background at position 'at'.
 */
void packed_bitblt(PackedImage * background, PackedImage * sprite, coords from, coords to, coords at, int transparent);

/** Convenience function that uses defaults for from, to (whole sprite) and from_plane (0). */
void packed_bitblt_full(PackedImage * background, PackedImage * sprite, coords at, int transparent);

#endif /* BITBLT_PACKED_H */
