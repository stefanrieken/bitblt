#include <stdio.h>

#include "tilemap.h"
#include "packed.h"

/**
 * Apply a map verbatim, ignoring any construction instructions it may contain.
 *
 * TODO: Momentarily focuses on 'packed', but if we can supply a 'planar'
 * bitblt function as a callback, it should work as well.
 *
 * NOTE: 'from' and 'to' are in tiles; 'at' is in pixels!
 */
void apply_plain_tile_map(TileMap * map, PackedImage * bg, coords from, coords size, coords at, int transparent) {
   coords img_from;
   coords img_to;

   int n=1;
   for (int i=from.y; i<from.y+size.y; i++) {
     for (int j=from.x; j<from.x+size.x; j += n) {

       uint8_t tile = map->map[i*map->size.x+j] & map->mask;

       // e.g. tile = 2; img is 16x16; x,y is 0,8
       // e.g. tile = 3; img is 16x16; x,y is 8,8
       img_from.x = (tile * map->tile_size) % map->tileset->size.x;
       img_from.y = ((tile * map->tile_size) / map->tileset->size.x) * map->tile_size;

       n=1;
       while (
         // next map entry at x is next tile in sequence...
         j+n < map->size.x &&
         (map->map[i*map->size.x+j+n] & map->mask) == tile+n &&
         // and sequence does not cross edge of image
         map->tileset->size.x != ((j+n)*map->tile_size)
       ) {
         // optimize: include next x entry in the blit.
         n++;
       }

       img_to.x = img_from.x + n*map->tile_size;
       img_to.y = img_from.y + map->tile_size;

       packed_bitblt(bg, map->tileset, img_from, img_to, at, transparent);
     }
   }
}
