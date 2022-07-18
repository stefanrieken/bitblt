
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

#define MASK_LEFT 0xAAAAAAAA
#define MASK_RIGHT 0x55555555
#define PX_PER_WORD_2B 16
#define BPP_2B 2

#define MASK_3RD 0x88888888
#define MASK_2ND 0x44444444
#define MASK_1ST 0x22222222
#define MASK_0TH 0x11111111
#define PX_PER_WORD_4B 8
#define BPP_4B 4

uint32_t masks2[] = { MASK_RIGHT, MASK_LEFT, 0, 0 };
uint32_t masks4[] = { MASK_0TH, MASK_1ST, MASK_2ND, MASK_3RD };

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
 * Generate a transparency mask for one word of (pre-shifted) sprite data,
 * given a e.g. 2 bit color depth and 0b00 = transparent.
 * Test for a left bit (shifted right) OR a right bit to get 0b01.
 * Smear out to the left (0b11) and return the inverse (0b00, not transparent).
 * For input 0b00: OR to get 0b00, smear out for 0b00, inverse for 0b11, transparent.
 * Do this for a full 32-bit word at once.
 * ANDing the result with the background clears it wherever the sprite is not transparent.
 */
static inline uint32_t mask(uint32_t data, uint32_t * masks, uint8_t bpp) {
  // uint32_t result = data & masks[0];
  // result |= (data & masks[1]) >> 1;
  // result |= (data & masks[2]) >> 2;
  // result |= (data & masks[3]) >> 3;
  uint32_t result = 0;
  for (uint8_t i = 0; i < bpp; i++) {
    result |= (data & masks[i]) >> i;
  }
  // smear back
  for (uint8_t i = 0; i < bpp; i++) {
    result |= result << i;
  }

  // and invert
  return ~result;
}

static inline uint32_t mask_2(uint32_t data) {
  uint32_t result = ((data & MASK_LEFT) >> 1) | (data & MASK_RIGHT);
  return ~(result | (result << 1));
}

/**
 * Project 2-bit coloured 'sprite' onto 'background' assuming colour '00' is transparent.
 */
void packed_bitblt(packed_image * background, packed_image * sprite, int at_x, int at_y) {
  uint8_t pixels_per_word = 32 / background->depth;

  // As one word of data contains a row of 32/bpp pixels,
  // the sprite may fall word-misaligned.
  // Establish the exact offset
  uint8_t offset = at_x % pixels_per_word;

  // Forget about the offset until the very last moment
  // and focus on working per-word instead.
  // (That is what bit block transfer is all about.)
  at_x = (at_x-offset) / pixels_per_word; // For clarity. Could just write x / pixels_per_word

  // N.b. The y coordinate needs no such treatment.
  // (Assuming, as we do, that the images have a word-aligned width.)

  // Establish the bit masks required for the generic mask function.
  // This part is a bit awkward but it means we can remain generic for n bpp
  // while even allowing the mask function to be compiled inline.
  uint32_t * masks = (background->depth == 2) ? masks2 : masks4;

  uint32_t background_width_aligned = packed_aligned_width(background->size.x, background->depth);
  uint32_t sprite_width_aligned = packed_aligned_width(sprite->size.x, sprite->depth);

  for(int i = 0; i < sprite->size.y; i++) {
    for (int j = 0; j < sprite->size.x / pixels_per_word; j++) {
      uint32_t spr_word = sprite->data[(i * (sprite_width_aligned/pixels_per_word)) + j];
      // Make up for any offset by shifting into two consecutive words; word 0 first
      uint32_t spr_word0 = spr_word >> (offset * background->depth);
      // Merge sprite into background by ANDing with calculated mask and ORing with sprite data
      uint32_t * bg_word0 = &background->data[((at_y+i) * (background_width_aligned/pixels_per_word)) + (at_x+j)];
      *bg_word0 = (*bg_word0 & mask(spr_word0, masks, background->depth)) | spr_word0;

      if (offset > 0) {
        // blt the spillover into the next word
        // TODO This is assuming we are well within background image boundaries!
        // need to do this conditionally because a shift with 32 is AND behaves 'undefined'
        uint32_t spr_word1 = spr_word << (32 - (offset * background->depth)); // fills with 'transparent', so mask = OK
        uint32_t * bg_word1 = &background->data[((at_y+i) * (background_width_aligned/pixels_per_word)) + (at_x+j+1)];
        *bg_word1 = (*bg_word1 & mask(spr_word1, masks, background->depth)) | spr_word1;
      }

    }
  }
}
