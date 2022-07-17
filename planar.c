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

planar_image * new_planar_image(int width, int height, int depth) {
  planar_image * result = malloc(sizeof(planar_image));
  result->width = width;
  result->height = height;
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
  uint32_t * result = calloc(1,(sizeof(uint32_t) * image->width * image->height * image->depth) / 32);
  for (int i=0; i< image->height; i++) {
    for (int j=0; j<image->width; j++) {

      uint32_t planar_pixel_index = (i*image->width) + j;
      uint8_t pixel = gather_pixel(image, planar_pixel_index);

      // We have a pixel, now drop it at the right place
      uint32_t packed_pixel_index = planar_pixel_index * image->depth;
      uint32_t packed_word = packed_pixel_index / 32;
      uint32_t packed_bit = packed_pixel_index % 32; // 0, 2, .. 30
      //if (packed_bit != 0) packed_bit = 32-packed_bit;
      result[packed_word] |= pixel << ((32-image->depth)-packed_bit);
    }
  }

  return result;
}

void planar_bitblt(planar_image * background, planar_image * sprite, int at_x, int at_y, int from_plane, bool zero_transparent) {
  uint32_t offset = at_x % 32;


  for(int i=0; i<sprite->height;i++) {
    for(int j=0; j<sprite->width;j+=32) {
      uint32_t sprite_pixel_idx = (i*sprite->width) + j;
      uint32_t sprite_word_idx = sprite_pixel_idx / 32;

      uint32_t mask = 0;
      
      if (zero_transparent) {
        for(int n=0; n<sprite->depth; n++) {
          mask |= sprite->planes[n][sprite_word_idx];
        }
      } else {
        mask = 0xFFFFFFFF;
      }

      // Calculate background position
      uint32_t bg_pixel_idx = ((i+at_y)*background->width) + j+at_x;
      uint32_t bg_byte_idx = bg_pixel_idx / 32;

      // check for Y under- & overflow.
      // This is easily skipped because we only blit one row at the time.
      if (bg_byte_idx < 0) continue;
      if (bg_byte_idx > background->width / 32 * background->height) continue;

      for(int n=0; n<sprite->depth; n++) {
        // check for X out of bounds (under- or overflow)
        if(j+at_x>=0 && j+at_x<background->width){
	  background->planes[from_plane+n][bg_byte_idx] &= ~(mask >> offset); // clear required parts
	  background->planes[from_plane+n][bg_byte_idx] |= sprite->planes[n][sprite_word_idx] >> offset; // add sprite
	}
        // overflow any offset into next, unless X+1 out of bounds
        if (offset > 0 && j+at_x+32>=0 && j+at_x+32<background->width) {
         background->planes[from_plane+n][bg_byte_idx+1] &= ~(mask << (32 - offset)); // clear required parts
         background->planes[from_plane+n][bg_byte_idx+1] |= sprite->planes[n][sprite_word_idx] << (32-offset); // add sprite
        }
      }
    }
  }
}
