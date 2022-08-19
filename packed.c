
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
#include "shared.h"
#include "planar.h"

PackedImage * to_packed_image(uint32_t * data, int width, int height, int depth) {
  PackedImage * image = malloc(sizeof(PackedImage));
  image->data = data;
  image->size.x = width;
  image->size.y = height;
  image->depth = depth;
  return image;
}

PackedImage * new_packed_image(int width, int height, int depth) {
  uint32_t * data = malloc(sizeof(uint32_t) * image_aligned_width(width, depth) * height);
  return to_packed_image(data, width, height, depth);
}

/**
 * Convert a packed-pixel image into a set of planar images.
 */
PlanarImage * unpack(PackedImage * image) {
  if (image == NULL) return NULL;

  uint32_t planar_width_aligned = image_aligned_width(image->size.x, 1);
  uint32_t packed_width_aligned = image_aligned_width(image->size.x, image->depth);

  PlanarImage * result = new_planar_image(image->size.x, image->size.y, image->depth);

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
uint32_t repeat_pattern(uint32_t pattern, uint8_t bpp) {
  uint32_t result = 0;
  for (int i=0; i<WORD_SIZE/bpp;i++) {
     result |= pattern<<(i*bpp);
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
void packed_bitblt(
    PackedImage * background,
    PackedImage * sprite,
    coords from,
    coords to,
    coords at,
    int transparent
  )
{
  uint8_t pixels_per_word = WORD_SIZE / background->depth;

  // Take 'from' offset into account
  // See planar implementation for reference
  int fromx_offset = (from.x * pixels_per_word) % WORD_SIZE;
  at.x -= fromx_offset / pixels_per_word;
  from.x -= fromx_offset / pixels_per_word;
  int mask_end = ((to.x - from.x) * pixels_per_word) < WORD_SIZE ? ((to.x - from.x) * pixels_per_word) : WORD_SIZE;
  WORD_T fromx_offset_mask = 0;
  for (int i=fromx_offset;i<mask_end;i++) {
    fromx_offset_mask |= 1 << ((WORD_SIZE-1)-i);
  }

  // offset to the right of the applicable word in pixels
  // this is the 'at' offset
  int offset = at.x % pixels_per_word;
  if (offset < 0) offset += pixels_per_word;

  // So for now, set at.x to a word boundary AND make it point to the word containing the pixel.
  at.x = at.x - offset;
  // also, pre-multiply offset with bpp
  offset *= background->depth;

  uint32_t background_width_aligned = image_aligned_width(background->size.x, background->depth);
  uint32_t sprite_width_aligned = image_aligned_width(sprite->size.x, sprite->depth);

  uint32_t mask_pattern = repeat_pattern(1, sprite->depth);
  uint32_t transparent_pattern = transparent == -1 ? 0 : repeat_pattern(transparent, sprite->depth);

  for(int i = from.y; i < to.y; i++) {

    // start at x=0 with 'from.x' mask; clean at end of for loop
    WORD_T mask = fromx_offset_mask;
    int k = fromx_offset;

    for (int j = from.x; j < to.x; j+=pixels_per_word) {
      // if y out of bounds, we have nothing to do here
      if ((at.y-from.y)+i < 0) continue;
      if ((at.y-from.y)+i >= background->size.y) continue;

      int spr_word_idx = ((i * sprite_width_aligned) + j)/pixels_per_word;
      WORD_T spr_word = sprite->data[spr_word_idx];

      if (transparent != -1) {
        mask = make_mask(mask & (spr_word ^ transparent_pattern), mask_pattern, sprite->depth);
        spr_word &= mask;
      } else {
        // opaque bitblt; still dynamically mask off (clip) width at image width
        uint32_t mask_end = ((to.x - j)*pixels_per_word) > WORD_SIZE ? WORD_SIZE : ((to.x - j)*pixels_per_word);
        for (; k < mask_end;k++) {
          mask |= 1 << ((WORD_SIZE-1)-k);
        }
        k=0; // only start at 'fromx_offset' in first word of line
      }

      // Merge sprite into background by ANDing with calculated mask and ORing with sprite data
      int bg_word_idx = ((((at.y-from.y)+i) * background_width_aligned) + ((at.x-from.x)+j))/pixels_per_word;

      // Make up for any offset by shifting into two consecutive words; word 0 first
      if ((at.x-from.x)+j >= 0 && (at.x-from.x)+j < background_width_aligned) {
        uint32_t * bg_word0 = &background->data[bg_word_idx];
        *bg_word0 = (*bg_word0 & ~(mask >> offset)) | (spr_word >> offset);
      }

      // blt any spillover into the next word
      if (offset != 0 && (at.x-from.x)+j+pixels_per_word >= 0 && (at.x-from.x)+j+pixels_per_word < background_width_aligned) {
        uint32_t * bg_word1 = &background->data[bg_word_idx+1];
        *bg_word1 = (*bg_word1 & ~(mask << (WORD_SIZE-offset))) | spr_word << (WORD_SIZE-offset);
      }

      mask = 0xFFFFFFFF;
    }
  }
}

void packed_bitblt_full(PackedImage * background, PackedImage * sprite, coords at, int transparent) {
  packed_bitblt(background, sprite, (coords) {0,0}, sprite->size, at, transparent);
}
