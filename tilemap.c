#include <stdio.h>

#include "tilemap.h"
#include "packed.h"

/**
 * Apply a map verbatim, ignoring any construction instructions it may contain.
 *
 * TODO: Momentarily focuses on 'packed', but if we can supply a 'planar'
 * bitblt function as a callback, it should work as well.
 *
 * NOTE: 'from' and 'size' are in tiles; 'at' is in pixels!
 */
void apply_plain_tile_map(TileMap * map, PackedImage * bg, coords from, coords size, BitbltFunc * blit, coords at, int transparent) {
   coords img_from;
   coords img_to;

   int n=1;
   for (int i=from.y; i<from.y+size.y; i++) {
     for (int j=from.x; j<from.x+size.x; j += n) {
       uint8_t tile = map->map[i*map->map_size.x+j] & map->mask;

       // e.g. tile = 2; img is 16x16; x,y is 0,8
       // e.g. tile = 3; img is 16x16; x,y is 8,8
       img_from.x = (tile * map->tile_size.x) % map->tileset->size.x;
       img_from.y = ((tile * map->tile_size.x) / map->tileset->size.x) * map->tile_size.y;

       n=1;
       while (
         // next map entry at x is next tile in sequence...
         j+n < map->map_size.x &&
         (map->map[i*map->map_size.x+j+n] & map->mask) == tile+n &&
         // and sequence does not cross edge of image
         map->tileset->size.x != ((j+n)*map->tile_size.x)
       ) {
         // optimize: include next x entry in the blit.
         n++;
       }

       img_to.x = img_from.x + n*map->tile_size.x;
       img_to.y = img_from.y + map->tile_size.y;

       blit(bg, map->tileset, img_from, img_to, (coords) {at.x+((j-from.x)*map->tile_size.x), at.y+((i-from.y)*map->tile_size.y)}, transparent);
     }
   }
}

/**
 * Perform square collision detection with tilemap.
 * Assumes tile MSB value of 1 indicates tile is solid.
 *
 * Returns the tile number of the first collision found, or -1 if no collision.
 */
int overlaps_solid_tile(TileMap * map, coords at, area a) {
  // normalize offset and divide by tile size
  a.from.x = (a.from.x - at.x) / map->tile_size.x;
  a.from.y = (a.from.y - at.y) / map->tile_size.y;
  // add e.g. 7 to round up division by 8 to properly include last tile
  // (that is, pending any exclusive-to / inclusive-to errors we already have anyway)
  a.to.x = (a.to.x - at.x + (map->tile_size.x-1)) / map->tile_size.x;
  a.to.y = (a.to.y - at.y + (map->tile_size.y-1)) / map->tile_size.y;

  // keep search within map's range
  if (a.from.x < 0) a.from.x = 0;
  if (a.from.y < 0) a.from.y = 0;
  if (a.to.x >= map->map_size.x) a.to.x = map->map_size.x-1;
  if (a.to.y >= map->map_size.y) a.to.y = map->map_size.y-1;

  // use counter to avoid repeated multiplications;
  int index = 0; // use counter to avoid repeated multiplications

  // TODO we might do away with the double for loop as well,
  // but we have to ensure that we still do an inclusive-to search
  for (int i=a.from.y; i<=a.to.y;i++) {
    for (int j=a.from.x; j<=a.to.x; j++) {
      index += 1;
      if ((map->map[index] >> 7) == 1) {
        return index;
      }
    }
  }

  return -1;
}
