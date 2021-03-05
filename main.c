#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "bitmap.h"
#include "planar.h"

uint8_t circle[] = {
  0b00111100,
  0b01100110,
  0b11000011,
  0b10000001,
  0b10000001,
  0b11000011,
  0b01100110,
  0b00111100
};

uint8_t curve[] = {
  0b01100000,
  0b01100000,
  0b01100000,
  0b01100000,
  0b00110000,
  0b00111111,
  0b00001111,
  0b00000000
};

uint8_t none[] = {0,0,0,0,0,0,0,0};

/**
 * Make a simple 1-bit image.
 * Align into the first byte of the word.
 * Since we're little-endian, that's actually the bottom byte.
 */
image * make_img(uint8_t * rawdata) {

  image * image = malloc(sizeof(image));
  image->size.x = 32;
  image->size.y = 8;
  image->data = malloc(sizeof(uint32_t) * 8);
  for(int i = 0; i < 8; i++) {
    image->data[i] = rawdata[i];
  }
  return image;
}

int main (int argc, char ** argv) {
  coords at = { 0, 0};
  coords at1 = { 1, 0};

  image ** backgrounds = malloc(sizeof(image*) *4);
  backgrounds[0] = make_img(circle);
  backgrounds[1] = make_img(circle);
  backgrounds[2] = make_img(circle);
  backgrounds[3] = make_img(circle);
  image ** sprites = malloc(sizeof(image*) *4);
  sprites[0] = make_img(curve);
  sprites[1] = make_img(none);
  sprites[2] = make_img(none);
  sprites[3] = make_img(none);
  write_bitmap("circle.bmp", backgrounds[0], 1);
  write_bitmap("curve.bmp", sprites[0], 1);
  planar_bitblt(backgrounds, sprites, at, 4);
  write_bitmap("curve_before_circle.bmp", pack(backgrounds,4), 4);
  image ** mixed = malloc(sizeof(image*) *4);
  mixed[0] = make_img(circle);
  mixed[1] = make_img(curve);
  mixed[2] = make_img(circle);
  mixed[3] = make_img(curve);
  write_bitmap("mixed.bmp", pack(mixed,4), 4);
  return 0;
}
