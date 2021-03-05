
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
#include "bitblt.h"

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
void bitblt(image * background, image * sprite, coords at, uint8_t bpp) {
  uint8_t pixels_per_word = 32 / bpp;
  // As one word of data contains a row of 16 pixels,
  // the sprite may fall word-misaligned.
  // Establish the exact offset
  uint8_t offset = at.x % pixels_per_word;

  // Forget about the offset until the very last moment
  // and focus on working per-word instead.
  // (That is what bit block transfer is all about.)
  at.x = (at.x-offset) / pixels_per_word; // For clarity. Could just write x / PX_PER_WORD
  // N.b. The y coordinate needs no such treatment.
  // (Assuming, as we do, that the images have a word-aligned width.)

  // Establish the bit masks required for the generic mask function.
  // This part is a bit awkward but it means we can remain generic for n bpp
  // while even allowing the mask function to be compiled inline.
  uint32_t * masks = (bpp == 2) ? masks2 : masks4;

  for(int i = 0; i < sprite->size.y; i++) {
    for (int j = 0; j < sprite->size.x / pixels_per_word; j++) {
      uint32_t spr_word = sprite->data[(i * (sprite->size.x/pixels_per_word)) + j];
      // Make up for any offset by shifting into two consecutive words; word 0 first
      uint32_t spr_word0 = spr_word >> (offset * bpp);
      // Merge sprite into background by ANDing with calculated mask and ORing with sprite data
      uint32_t * bg_word0 = &background->data[((at.y+i) * (background->size.x/pixels_per_word)) + (at.x+j)];
      *bg_word0 = (*bg_word0 & mask(spr_word0, masks, bpp)) | spr_word0;

      if (offset > 0) {
        // blt the spillover into the next word
        // TODO This is assuming we are well within background image boundaries!
        // need to do this conditionally because a shift with 32 is AND behaves 'undefined'
        uint32_t spr_word1 = spr_word << (32 - (offset * bpp)); // fills with 'transparent', so mask = OK
        uint32_t * bg_word1 = &background->data[((at.y+i) * (background->size.x/pixels_per_word)) + (at.x+j+1)];
        *bg_word1 = (*bg_word1 & mask(spr_word1, masks, bpp)) | spr_word1;
      }

    }
  }
}
