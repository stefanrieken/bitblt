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

#include "planar.h"

planar_image * new_planar_image(int width, int height, int depth) {
  planar_image * result = malloc(sizeof(planar_image));
  result->size.x = width;
  result->size.y = height;
  result->depth = depth;
  result->planes = malloc(sizeof(uint32_t**)*depth);

  int alloc_size = sizeof(uint32_t) * ((width+31) / 32) * height;

  for (int i=0;i<depth;i++)
  	result->planes[i]=calloc(1, alloc_size);

  return result;
}

uint8_t gather_pixel(planar_image * image, uint32_t planar_pixel_index) {
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
uint32_t * pack(planar_image * image) {

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

void planar_bitblt(
   planar_image * background,
   planar_image * sprite,
   coords from,
   coords to,
   coords at,
   int from_plane,
   int transparent
  )
{
  // This sanity check is a bit baby'ing,
  // but it might just make life easier somewhere
  if (to.x > sprite->size.x) to.x = sprite->size.x;
  if (to.y > sprite->size.y) to.y = sprite->size.y;

  int offset = at.x % WORD_SIZE;
  if (offset < 0) offset +=WORD_SIZE;

  uint32_t background_width_aligned = image_aligned_width(background->size.x, 1);
  uint32_t sprite_width_aligned = image_aligned_width(sprite->size.x, 1);

  for(int i=from.y; i<to.y;i++) {
    for(int j=from.x; j<to.x;j+=WORD_SIZE) {
      uint32_t sprite_pixel_idx = (i*sprite_width_aligned) + j;
      uint32_t sprite_word_idx = sprite_pixel_idx / WORD_SIZE;

      WORD_T mask = 0;

      // basic mask for cutting off alignment
      uint32_t mask_end = sprite->size.x - j > WORD_SIZE ? WORD_SIZE : sprite->size.x - j;
      for (int k = 0; k < mask_end;k++) {
        mask |= 1 << ((WORD_SIZE-1)-k);
      }

      // refine mask based on transparent color
      if (transparent != -1) {
        WORD_T transparency_mask = ~0;
        for (int n = 0; n < sprite->depth;n++) {
          WORD_T expected_bits_for_transparent = ((transparent >> n) & 1) ? ~0 : 0;
          transparency_mask &= (sprite->planes[n][sprite_word_idx] ^ ~expected_bits_for_transparent);
        }
        mask &= ~transparency_mask;
      }

      // Calculate background position
      uint32_t bg_pixel_idx = ((i+at.y)*background_width_aligned) + j+at.x;
      uint32_t bg_byte_idx = bg_pixel_idx / WORD_SIZE;


      // check for Y under- & overflow.
      // These are easily skipped because we only blit one row at the time.
      if (bg_byte_idx < 0) continue;
      if (bg_byte_idx > background_width_aligned * background->size.y / WORD_SIZE) continue;


      for(int n=0; n<sprite->depth; n++) {
        uint32_t sprite_word = sprite->planes[n][sprite_word_idx];
        // check for X out of bounds (under- or overflow)

        if(j+at.x>=0 && j+at.x-offset<background->size.x) {
          background->planes[from_plane+n][bg_byte_idx] &= ~(mask >> offset); // clear required parts
          // TODO also subject sprite to mask in case of transparent
          background->planes[from_plane+n][bg_byte_idx] |= (sprite_word&mask) >> offset; // add sprite
        }
        // overflow any offset into next, unless X+1 out of bounds
        if (offset != 0 && j+at.x+WORD_SIZE>=0 && j+at.x+WORD_SIZE-offset<background->size.x) {
         background->planes[from_plane+n][bg_byte_idx+1] &= ~(mask << (WORD_SIZE-offset)); // clear required parts
         // TODO also subject sprite to mask in case of transparent
         background->planes[from_plane+n][bg_byte_idx+1] |= (sprite_word&mask) << (WORD_SIZE - offset); // add sprite
        }
      }
    }
  }
}

void planar_bitblt_full(planar_image * background, planar_image * sprite, coords at, int transparent) {
  planar_bitblt(background, sprite, (coords) {0,0}, sprite->size, at, 0, transparent);
}

void planar_bitblt_plane(planar_image * background, planar_image * sprite, coords at, int from_plane) {
  planar_bitblt(background, sprite, (coords) {0,0}, sprite->size, at, from_plane, -1);
}
