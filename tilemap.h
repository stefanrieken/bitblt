#ifndef BITBLT_TILEMAP_H
#define BITBLT_TILEMAP_H

#include "image.h"

typedef struct TileMap {
  Image * tileset; //contains size; can always change this pointer to swap out tile set
  coords tile_size; // e.g. 8x8
  coords map_size;
  uint8_t * map;
  uint8_t mask; // which portion of the tile is a tile, and which is instructional data?
} TileMap;

/**
 * Apply a map verbatim, ignoring any construction instructions it may contain.
 *
 * TODO: Momentarily focuses on 'packed', but if we can supply a 'planar'
 * bitblt function as a callback, it should work as well.
 *
 * NOTE: 'from' and 'size' are in tiles; 'at' is in pixels!
 */
void apply_plain_tile_map(TileMap * map, PackedImage * bg, coords from, coords size, BitbltFunc * blit, coords at, int transparent);

/**
 * Perform square collision detection with tilemap.
 * Assumes tile MSB value of 1 indicates tile is solid.
 *
 * Returns the tile number of the first collision found, or -1 if no collision.
 */
int overlaps_solid_tile(TileMap * map, coords at, area a);

#endif /*BITBLT_TILEMAP_H*/
