
/*
Bit blitting

1. If required, translate all layers to the final colour model
2. Translate sprite to byte / word boundaries
3. Derive transparency bitmasks

Given the palette:
00 Transparent
01 Red
10 Green
11 Blue

Background: all red
Image: 2-pixel transparent+dot: 0010
Mask: 1100


Deriving the mask if the color data is spread over separate bytes:
a: 01
b: 00
mask = NOT (a OR b)

Since it is not:
0010
mask = ((image AND 1010) >> 1) OR (image AND 0101)
mask = ((0010) >> 1) OR (0000)
mask = (0001) OR 0000
Blitting:


(background AND mask) OR image
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "packed.h"


packed_image * to_packed_image(uint32_t * data, int width, int height, int depth) {
  packed_image * image = malloc(sizeof(packed_image));
  image->data = data;
  image->size.x = width;
  image->size.y = height;
  image->depth = depth;
  return image;
}

uint32_t packed_aligned_width(uint32_t width, int bpp) {
  uint32_t px_per_word = 32 / bpp;
  return width + ((width % px_per_word == 0) ? 0 : (px_per_word-(width % px_per_word)));
}

  
/**
 * Convert a packed-pixel image into a set of planar images.
 */
planar_image * unpack(packed_image * image) {

  uint32_t planar_width_aligned = planar_aligned_width(image->size.x);
  uint32_t packed_width_aligned = packed_aligned_width(image->size.x, image->depth);

  planar_image * result = new_planar_image(image->size.x, image->size.y, image->depth);

  for (int i=0; i< image->size.y; i++) {
    for (int j=0; j<image->size.x; j++) {
      uint32_t pixel_bit_idx = (i*packed_width_aligned*image->depth)+j*image->depth;
      uint32_t pixel_word = pixel_bit_idx / 32;
      uint32_t shift = pixel_bit_idx % 32;

      uint8_t packed_pixel = (image->data[pixel_word] >> (32-shift-image->depth)) & 0b1111; // <- TODO hardcoded 4-bit depth in mask

      uint32_t target_pixel_bit_idx = (i*planar_width_aligned)+j;
      uint32_t target_pixel_word = target_pixel_bit_idx / 32;
      uint32_t target_shift = target_pixel_bit_idx %32;

      for (int plane = 0; plane < image->depth; plane++) {
        uint32_t pixel = ((packed_pixel >> plane) & 0b1) << (31-target_shift);
        result->planes[plane][target_pixel_word] |= pixel;
      }
    }
  }

  return result;
}

/**
 * Generate a mask pattern that points out the leftmost pixel bits in a word for a given bpp.
 * So for instance: 2 bpp => 010101.....; 4 bpp => 000100010001; etc.
 */
uint32_t get_mask_pattern(uint8_t bpp) {
  uint32_t result = 0;
  for (int i=0; i<WORD_SIZE/bpp;i++) {
     result |= 1<<(i*bpp);
  }
  return result;
}

/**
 * Generate a transparency mask for one word of (pre-shifted) sprite data,
 * given a e.g. 2 bit color depth and 0b00 = transparent.
 * Test for a left bit (shifted right) OR a right bit to get 0b01.
 * Smear out to the left (0b11) and return the inverse (0b00, not transparent).
 * For input 0b00: OR to get 0b00, smear out for 0b00, inverse for 0b11, transparent.
 * Do this for a full 32-bit word at once.
 * ANDing the result with the background clears it wherever the sprite is not transparent.
 */
static inline uint32_t make_mask(uint32_t data, uint32_t mask_pattern, uint8_t bpp) {
  uint32_t result = 0;

  // If there's any data in any pixel nybble,
  // we set the lsb of that nybble in 'result'.
  for (uint8_t i = 0; i < bpp; i++) {
    result |= (data & (mask_pattern << i)) >> i;
  }
  // 'smear back': if lsb == 1, make all in nybble 1
  for (uint8_t i = 0; i < bpp-1; i++) {
    result |= result << i;
  }

  return result;
}

/**
 * Project 2-bit coloured 'sprite' onto 'background' assuming colour '00' is transparent.
 */
void packed_bitblt(packed_image * background, packed_image * sprite, coords at, bool zero_transparent) {
  uint8_t pixels_per_word = WORD_SIZE / background->depth;

  // offset to the right of the applicable word in pixels
  int offset = at.x % pixels_per_word;
  if (offset < 0) offset += pixels_per_word;

  // So for now, set at.x to a word boundary AND make it point to the word containing the pixel.
  at.x = (at.x - offset) / pixels_per_word;
  // also, pre-multiply offset with bpp
  offset *= background->depth;

  uint32_t background_width_aligned = packed_aligned_width(background->size.x, background->depth);
  uint32_t sprite_width_aligned = packed_aligned_width(sprite->size.x, sprite->depth);

  uint32_t mask_pattern = get_mask_pattern(sprite->depth);
  
  for(int i = 0; i < sprite->size.y; i++) {
    for (int j = 0; j < sprite_width_aligned / pixels_per_word; j++) {
      // if y out of bounds, we have nothing to do here
      if (at.y+i < 0) continue;
      if (at.y+i >= background->size.y) continue;

      int spr_word_idx = (i * (sprite_width_aligned/pixels_per_word)) + j;
      uint32_t spr_word = sprite->data[spr_word_idx];

      uint32_t mask;
      if (zero_transparent) {
        mask = make_mask(spr_word, mask_pattern, sprite->depth);
      } else {
        // opaque bitblt; still dynamically mask off (clip) width at image width
        uint32_t mask_end = sprite->size.x - (j*pixels_per_word) > WORD_SIZE ? WORD_SIZE : sprite->size.x - (j*pixels_per_word);
        for (int k = 0; k < mask_end;k++) {
          mask |= 1 << (WORD_SIZE-1)-k;
        }
      }

      // Merge sprite into background by ANDing with calculated mask and ORing with sprite data
      int bg_word_idx = ((at.y+i) * (background_width_aligned/pixels_per_word)) + (at.x+j);

      // Make up for any offset by shifting into two consecutive words; word 0 first
      if (at.x+j >= 0 && at.x+j < background_width_aligned / pixels_per_word) {
        uint32_t * bg_word0 = &background->data[bg_word_idx];
        *bg_word0 = (*bg_word0 & ~(mask >> offset)) | (spr_word >> offset);
      }

      // blt any spillover into the next word
      if (offset != 0 && at.x+j+1 >= 0 && at.x+j+1 < background_width_aligned / pixels_per_word) {
        uint32_t * bg_word1 = &background->data[bg_word_idx+1];
        *bg_word1 = (*bg_word1 & ~(mask << (WORD_SIZE-offset))) | spr_word << (WORD_SIZE-offset);
      }	
    }
  }
}

