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

Map to packed-pixel / color index ('on the serial fly'):
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "planar.h"

uint8_t gather_pixel(image ** images, uint32_t planar_pixel_index, uint32_t depth) {
  uint8_t pixel = 0;
  uint32_t planar_word = planar_pixel_index / 32; // 32 pixels per word
  uint32_t planar_bit  = planar_pixel_index % 32; // 0 .. 31

  for(int n=0; n<depth; n++) {
    uint8_t bit = ((images[n]->data[planar_word]) >> (31-planar_bit)) & 0b1;
    pixel |= bit << n;
  }
  return pixel;
}

/**
 * Convert a set of planar images into a packed-pixel image.
 */
image * pack(image ** images, uint32_t num) {
  image * result = malloc(sizeof(image)*4);
  result->size.x = images[0]->size.x;
  result->size.y = images[0]->size.y;
  result->data = calloc(1,(sizeof(uint32_t) * result->size.x * result->size.y * num) / 32);
  for (int i=0; i< result->size.y; i++) {
    for (int j=0; j<result->size.x; j++) {

      uint32_t planar_pixel_index = (i*images[0]->size.x) + j;
      uint8_t pixel = gather_pixel(images, planar_pixel_index, num);

      // We have a pixel, now drop it at the right place
      uint32_t packed_pixel_index = planar_pixel_index * num; // num == bpp
      uint32_t packed_word = packed_pixel_index / 32;
      uint32_t packed_bit = packed_pixel_index % 32; // 0, 2, .. 30
      //if (packed_bit != 0) packed_bit = 32-packed_bit;
      result->data[packed_word] |= pixel << ((32-num)-packed_bit);
    }
  }

  return result;
}

void planar_bitblt(image ** backgrounds, image ** sprites, coords at, int depth) {
  uint32_t offset = at.x % 32;

  for(int i=0; i<sprites[0]->size.y;i++) {
    for(int j=0; j<sprites[0]->size.x;j+=32) {
      uint32_t sprite_pixel_idx = (i*sprites[0]->size.x) + j;
      uint32_t sprite_word_idx = sprite_pixel_idx / 32;

      uint32_t mask = 0;
      for(int n=0; n<depth; n++) {
        mask |= sprites[n]->data[sprite_word_idx];
      }

      // Calculate background position
      uint32_t bg_pixel_idx = ((i+at.y)*backgrounds[0]->size.x) + j+at.x;
      uint32_t bg_byte_idx = bg_pixel_idx / 32;

      for(int n=0; n<depth; n++) {
        backgrounds[n]->data[bg_byte_idx] &= ~(mask >> offset); // clear required parts
        backgrounds[n]->data[bg_byte_idx] |= sprites[n]->data[sprite_word_idx] >> offset; // add sprite
        // overflow any offset into next
        if (offset > 0) {
         backgrounds[n]->data[bg_byte_idx+1] &= ~(mask << (32 - offset)); // clear required parts
         backgrounds[n]->data[bg_byte_idx+1] |= sprites[n]->data[sprite_word_idx] << (32-offset); // add sprite
        }
      }
    }
  }
}
