#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#include "bitmap.h"
#include "planar.h"
#include "display/display.h"
#include "font.h"

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

// same as in write bitmap, not very sanitized
uint8_t palette[] = {
	0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF,

	0x00, 0xFF, 0x00,
	0x00, 0x00, 0xFF,
	0xFF, 0x00, 0x00,

	0x00, 0x88, 0x00,
	0x00, 0x00, 0x88,
	0x88, 0x00, 0x00,

	0x88, 0x88, 0x88,

	0x88, 0x88, 0x00,
	0x00, 0x88, 0x88,
	0x88, 0x00, 0x88,

	0x88, 0x88, 0x88,
	0x88, 0x88, 0x00,
	0x00, 0x88, 0x88,
	0x88, 0x00, 0x88,
};

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

image * make_canvas(int width, int height) {
  image * image = malloc(sizeof(image));
  image->size.x = width;
  image->size.y = height;
  image->data = calloc(1, sizeof(uint32_t) * (width / 32) * height);
  return image;
}

void * worker_func(void * args) {
  image ** backgrounds = (image **) args;

  sleep(1);

  coords at = { 0, 0};
  coords at1 = { 1, 0};

  image ** sprites = malloc(sizeof(image*) *4);
  sprites[0] = make_img(curve);
  sprites[1] = make_img(none);
  sprites[2] = make_img(none);
  sprites[3] = make_img(none);
  write_bitmap("circle.bmp", backgrounds[0], 1);
  write_bitmap("curve.bmp", sprites[0], 1);
  planar_bitblt(backgrounds, sprites, at, 4);
  display_redraw();

  write_bitmap("curve_before_circle.bmp", pack(backgrounds,4), 4);
  image ** mixed = malloc(sizeof(image*) *4);
  mixed[0] = make_img(circle);
  mixed[1] = make_img(curve);
  mixed[2] = make_img(circle);
  mixed[3] = make_img(curve);
  write_bitmap("mixed.bmp", pack(mixed,4), 4);

  return 0;
}

int main (int argc, char ** argv) {

  int width = 320;
  int height = 200;

  coords at = { 0, 0};

  image ** display = malloc(sizeof(image*) *4);
  display[0] = make_canvas(width, height);
  display[1] = make_canvas(width, height);
  display[2] = make_canvas(width, height);
  display[3] = make_canvas(width, height);

  image * c = make_img(circle);
  planar_bitblt(&display[1], &c, at, 1);

  image ** ch = malloc(sizeof(image*) * 4);
  bool fixed = false;
  char * txt = "The quick brown fox jumps over the lazy dog.";
  int stretch = 1;
  ch[0] = render(txt, stretch, fixed);
  ch[1] = render(txt, stretch, fixed);
  ch[2] = render(txt, stretch, fixed);
  ch[3] = render(txt, stretch, fixed);
  coords at2 = { 0, 10};
  planar_bitblt(display, ch, at2, 4);
  fixed=true;
  ch[0] = render(txt, stretch, fixed);
  ch[1] = render(txt, stretch, fixed);
  ch[2] = render(txt, stretch, fixed);
  ch[3] = render(txt, stretch, fixed);
  coords at3 = { 0, 30};
  planar_bitblt(display, ch, at3, 4);
  txt = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG!?";
  fixed = false;
  ch[0] = render(txt, stretch, fixed);
  ch[1] = render(txt, stretch, fixed);
  ch[2] = render(txt, stretch, fixed);
  ch[3] = render(txt, stretch, fixed);
  txt = "!@#$%^&*()-=_+[]{}()|\\/\":;";
  coords at4 = { 0, 50};
  planar_bitblt(display, ch, at4, 4);
  fixed=true;
  ch[0] = render(txt, stretch, fixed);
  ch[1] = render(txt, stretch, fixed);
  ch[2] = render(txt, stretch, fixed);
  ch[3] = render(txt, stretch, fixed);
  coords at5 = { 0, 70};
  planar_bitblt(display, ch, at5, 4);

  pthread_t worker;
  pthread_create(&worker, NULL, worker_func, display);

  display_runloop(argc, argv, display, palette, 4);
}
