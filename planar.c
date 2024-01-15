/*
Planed blitting:

background: 0101 = blue, blue
00
11

sprite: 0010 = black, green
01
00

mask = ~(sprite.layer1 OR sprite.layer2)
10

AND (background.layer1, mask); AND (background.layer2, mask);
00
10

OR (background.layer1, sprite.layer1);
OR (background.layer2, sprite.layer2);
01
10
= blue, blue

*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "planar.h"

PlanarImage * new_planar_image(int width, int height, int depth) {
  PlanarImage * result = malloc(sizeof(PlanarImage));
  result->size.x = width;
  result->size.y = height;
  result->depth = depth;
  result->planes = malloc(sizeof(uint32_t**)*depth);

  int alloc_size = sizeof(uint32_t) * ((width+31) / 32) * height;

  for (int i=0;i<depth;i++)
  	result->planes[i]=calloc(1, alloc_size);

  return result;
}

uint8_t gather_pixel(PlanarImage * image, uint32_t planar_pixel_index) {
  uint8_t pixel = 0;
  uint32_t planar_word = planar_pixel_index / 32; // 32 pixels per word
  uint32_t planar_bit  = planar_pixel_index % 32; // 0 .. 31

  for(int n=0; n<image->depth; n++) {
    uint8_t bit = ((image->planes[n][planar_word]) >> (31-planar_bit)) & 0b1;
    pixel |= bit << n;
  }
  return pixel;
}


/**
 * Convert a set of planar images into a packed-pixel image.
 */
uint32_t * pack(PlanarImage * image) {

  uint32_t image_width_aligned = image_aligned_width(image->size.x, 1);

  uint32_t px_per_word = 32 / image->depth;
  uint32_t packed_width_aligned = image->size.x + ((image->size.x % px_per_word == 0) ? 0 : (px_per_word-(image->size.x % px_per_word)));

  uint32_t * result = calloc(1,(sizeof(uint32_t) * packed_width_aligned * image->size.y * image->depth) / 32);

  for (int i=0; i< image->size.y; i++) {
    for (int j=0; j<image->size.x; j++) {

      uint32_t planar_pixel_index = (i*image_width_aligned) + j;
      uint8_t pixel = gather_pixel(image, planar_pixel_index);

      // We have a pixel, now drop it at the right place
      uint32_t packed_pixel_index = (i*packed_width_aligned*image->depth) + j*image->depth;
      uint32_t packed_word = packed_pixel_index / 32;
      uint32_t packed_bit = packed_pixel_index % 32; // 0, 2, .. 30
      //if (packed_bit != 0) packed_bit = 32-packed_bit;
      result[packed_word] |= pixel << ((32-image->depth)-packed_bit);
    }
  }

  return result;
}

bool planar_bitblt(
   PlanarImage * background,
   PlanarImage * sprite,
   coords from,
   coords to,
   coords at,
   int from_plane,
   int transparent
  )
{
  bool collision = false;

  // This sanity check is a bit baby'ing,
  // but it might just make life easier somewhere
  if (to.x > sprite->size.x) to.x = sprite->size.x;
  if (to.y > sprite->size.y) to.y = sprite->size.y;

  // Take 'from' offset into account
  int fromx_offset = from.x % WORD_SIZE;

  // Say, offset is 7; we start drawing 7 bits earlier but mask off the end
  at.x -= fromx_offset;
  from.x -= fromx_offset;
  // Then mask should be 000000011111111...
  // either for the full word or until to.x, whichever comes first
  int mask_end = (to.x - from.x) < WORD_SIZE ? to.x - from.x : WORD_SIZE;
  if (background->size.x - at.x < mask_end) mask_end = background->size.x - at.x;
  WORD_T fromx_offset_mask = 0;
  for (int i=fromx_offset;i<mask_end;i++) {
    fromx_offset_mask |= 1 << ((WORD_SIZE-1)-i);
  }

  // This is the 'at' offset
  int offset = at.x % WORD_SIZE;
  if (offset < 0) offset +=WORD_SIZE;
  at.x = at.x - offset;

  uint32_t background_width_aligned = image_aligned_width(background->size.x, 1);
  uint32_t background_width_words = background_width_aligned / WORD_SIZE;
  uint32_t sprite_width_aligned = image_aligned_width(sprite->size.x, 1);

  // Calculate this value by progressive addition instead of repeated multiplication;
  // but since we don't start at zero, do pre-calculate the initial value
  int spr_word_idx_y = from.y * (sprite_width_aligned / WORD_SIZE);
  int bg_word_idx_y = at.y * (background_width_aligned / WORD_SIZE);
  int bg_word_end =  background->size.y * (background_width_aligned / WORD_SIZE);

  for(int i=from.y; i<to.y;i++) {
    // check for Y under- & overflow.
    // These are easily skipped because we only blit one row at the time.
    if (bg_word_idx_y < 0) goto continue_like_so;
    if (bg_word_idx_y >= bg_word_end) goto continue_like_so;

    int spr_word_idx = spr_word_idx_y + (from.x / WORD_SIZE);

    int bg_word_idx = bg_word_idx_y + (at.x / WORD_SIZE);

    // start at x=0 with 'from.x' mask; clean at end of for loop
    WORD_T mask = fromx_offset_mask;

    for(int j=from.x; j<to.x;j+=WORD_SIZE) {
      // TODO the fact that we reduce j to word units first thing,
      // suggests taht we may want to iterate in word units (as we do with packed)
      int bg_word_idx_x = ((j-from.x)+at.x) / WORD_SIZE;

      // cut off end alignment by removing relevant 1's from mask
      if (to.x -j < WORD_SIZE) {
        for (int n=to.x-j; n<WORD_SIZE;n++) {
          mask &= ~(1 << ((WORD_SIZE-1)-n));
        }
      }

      // refine mask based on transparent color
      if (transparent != -1) {
        WORD_T transparency_mask = ~0;
        for (int n = 0; n < sprite->depth;n++) {
          WORD_T expected_bits_for_transparent = ((transparent >> n) & 1) ? ~0 : 0;
          transparency_mask &= (sprite->planes[n][spr_word_idx] ^ ~expected_bits_for_transparent);
        }
        mask &= ~transparency_mask;
      }

      for(int n=0; n<sprite->depth; n++) {
        uint32_t sprite_word = sprite->planes[n][spr_word_idx];
        // check for X out of bounds (under- or overflow)

        if(bg_word_idx_x >= 0 && bg_word_idx_x < background_width_words) {
         uint32_t masked = background->planes[from_plane+n][bg_word_idx] & ~(mask >> offset);
         collision |= background->planes[from_plane+n][bg_word_idx] != masked;
          // TODO also subject sprite to mask in case of transparent
          background->planes[from_plane+n][bg_word_idx] = masked | ((sprite_word&mask) >> offset); // add sprite
        }
        // overflow any offset into next, unless X+1 out of bounds
        if(offset != 0 && bg_word_idx_x+1 >= 0 && bg_word_idx_x+1 < background_width_words) {
          uint32_t masked = background->planes[from_plane+n][bg_word_idx+1] & ~(mask << (WORD_SIZE-offset)); // clear required parts
          collision |= background->planes[from_plane+n][bg_word_idx+1] != masked; // clear required parts
          // TODO also subject sprite to mask in case of transparent
          background->planes[from_plane+n][bg_word_idx+1] = masked | ((sprite_word&mask) << (WORD_SIZE - offset)); // add sprite
        }
      }

      spr_word_idx++;
      bg_word_idx++;
      mask = ~0;
    }

continue_like_so:
    spr_word_idx_y += sprite_width_aligned / WORD_SIZE;
    bg_word_idx_y += background_width_aligned / WORD_SIZE;
  }

  return collision;
}

bool planar_bitblt_full(PlanarImage * background, PlanarImage * sprite, coords at, int transparent) {
  return planar_bitblt(background, sprite, (coords) {0,0}, sprite->size, at, 0, transparent);
}

bool planar_bitblt_plane(PlanarImage * background, PlanarImage * sprite, coords at, int from_plane) {
  return planar_bitblt(background, sprite, (coords) {0,0}, sprite->size, at, from_plane, -1);
}

bool planar_bitblt_all(
   PlanarImage * background,
   PlanarImage * sprite,
   coords from,
   coords to,
   coords at,
   int transparent
  )
{
  return planar_bitblt(background, sprite, from, to, at, 0, transparent);
}
