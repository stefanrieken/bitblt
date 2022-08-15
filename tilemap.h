#ifndef BITBLT_TILEMAP_H
#define BITBLT_TILEMAP_H

#include "image.h"

typedef struct TileMap {
  Image * tileset; //contains size; can always change this pointer to swap out tile set
  int tile_size; // e.g. 8; might also support non-square?
  coords size;
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

#endif /*BITBLT_TILEMAP_H*/
