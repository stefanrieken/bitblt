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
planar_image * make_img(uint8_t * rawdata) {
  planar_image * image = new_planar_image(32, 8, 1);
  for(int i = 0; i < 8; i++) {
    image->planes[0][i] = rawdata[i];
  }
  return image;
}

void * worker_func(void * args) {
  planar_image * background = (planar_image *) args;

  planar_image * img_curve = make_img(curve);
  planar_image * img_circle = make_img(circle);

  sleep(1);

  planar_image * sprite = new_planar_image(32, 8, 4);
  planar_bitblt(sprite, img_curve, 0, 0, 0);

  write_bitmap("circle.bmp", background->planes[0], background->width, background->height, 1);

  //write_bitmap("curve.bmp", sprite->planes[0], sprite->width, sprite->height, sprite->depth);
  write_bitmap("curve.bmp", img_curve->planes[0], img_curve->width, img_curve->height, 1);
  planar_bitblt(background, sprite, 0, 0, 0);
  display_redraw();

  write_bitmap("curve_before_circle.bmp", pack(background), background->width, background->height, background->depth);

  planar_image * mixed = new_planar_image(32, 8, 4);
  planar_bitblt(mixed, img_circle, 0, 0, 0);
  planar_bitblt(mixed, img_curve, 0, 0, 1);
  planar_bitblt(mixed, img_circle, 0, 0, 2);
  planar_bitblt(mixed, img_circle, 0, 0, 3);

  write_bitmap("mixed.bmp", pack(mixed), mixed->width, mixed->height, mixed->depth);

  return 0;
}

int main (int argc, char ** argv) {
  planar_image * display = new_planar_image(320, 200, 4);

  planar_image * c = make_img(circle);
  planar_bitblt(display, c, 0, 0, 1);

  bool fixed = false;
  char * txt = "The quick brown fox jumps over the lazy dog.";
  int stretch = 2;
  planar_image * ch = render_text(txt, stretch, fixed);
  
  // Copy text into all four planes
  planar_bitblt(display, ch, 0, 10, 0);
  planar_bitblt(display, ch, 0, 10, 1);
  planar_bitblt(display, ch, 0, 10, 2);
  planar_bitblt(display, ch, 0, 10, 3);

  fixed=true;
  free(ch);
  ch = render_text(txt, stretch, fixed);

  planar_bitblt(display, ch, 0, 30, 0);
  planar_bitblt(display, ch, 0, 30, 1);
  planar_bitblt(display, ch, 0, 30, 2);
  planar_bitblt(display, ch, 0, 30, 3);

  txt = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG!?";
  fixed = false;
  
  free(ch);
  ch = render_text(txt, stretch, fixed);

  planar_bitblt(display, ch, 0, 50, 0);
  planar_bitblt(display, ch, 0, 50, 1);
  planar_bitblt(display, ch, 0, 50, 2);
  planar_bitblt(display, ch, 0, 50, 3);

  txt = "!@#$%^&*()-=_+[]{}()|\\/\":;";

  free(ch);
  ch = render_text(txt, stretch, fixed);

  planar_bitblt(display, ch, 0, 70, 0);
  planar_bitblt(display, ch, 0, 70, 1);
  planar_bitblt(display, ch, 0, 70, 2);
  planar_bitblt(display, ch, 0, 70, 3);

  fixed=true;
  free(ch);
  ch = render_text(txt, stretch, fixed);

  planar_bitblt(display, ch, 0, 90, 0);
  planar_bitblt(display, ch, 0, 90, 1);
  planar_bitblt(display, ch, 0, 90, 2);
  planar_bitblt(display, ch, 0, 90, 3);

  // let's get interactive!
  pthread_t worker;
  pthread_create(&worker, NULL, worker_func, display);

  display_runloop(argc, argv, worker, display, palette, 4);
}
