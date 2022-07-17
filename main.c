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

uint16_t cat[] = {
  0b0110000000000110,
  0b0111000000001110,
  0b0101100000011010,
  0b0100111111110010,
  0b0111111111111110,
  0b0111111111111110,
  0b0111100110011110,
  0b0111100110011110,
  0b0111111111111110,
  0b1111111111111111,
  0b0111111111111110,
  0b1111111111111111,
  0b0111111111111110,
  0b0011111111111100,
  0b0001111111111000,
  0b0000111111110000  
};

uint16_t cat_color2[] = {
  0b0000000000000000,
  0b0000000000000000,
  0b0010000000000100,
  0b0011000000001100,
  0b0000000000000000,
  0b0000000000000000,
  0b0000011001100000,
  0b0000011001100000,
  0b0000000000000000,
  0b0000000110000000,
  0b0000000000000000,
  0b0000110110110000,
  0b0000011111100000,
  0b0000001111000000,
  0b0000000000000000,
  0b0000000000000000
};

uint16_t cat_eyes[] = {
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000111001110000,
  0b0000111001110000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000110110110000,
  0b0000011111100000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000
};


// Same as in write bitmap, not very sanitized.
// Reverse from what's expected due to endianness.
uint8_t palette[] = {
	0x00, 0x00, 0x00, // black (and / or transparent!)
	0xFF, 0xFF, 0xFF, // white

	0x00, 0xFF, 0x00,
	0x00, 0x00, 0xFF,
	0xFF, 0x00, 0x00,

	0x00, 0x88, 0x00,
	0x00, 0x00, 0x88,
	0x88, 0x00, 0x00,

	0x88, 0x88, 0x88, // grey

	0x88, 0x88, 0x00,
	0x00, 0x88, 0x88,
	0x88, 0x00, 0x88, 

	0x88, 0x88, 0x88, // repeated
	0x88, 0x88, 0x00,
	0x00, 0x88, 0x88,
	0x88, 0x00, 0x88
};

// Catty paletty!
uint8_t palette2[] = {
	0x00, 0x00, 0x00, // black (and / or transparent!)
	0xFF, 0xFF, 0xFF, // white

	0x00, 0xFF, 0x00,
	0x20, 0xAA, 0xCA, // hair
	0x89, 0x75, 0xFF, // ears

	0x00, 0x88, 0x00, // text
	0x00, 0x00, 0x88,
	0x61, 0x24, 0xFF, // mouth
	0x88, 0x88, 0x88, // grey

	0x88, 0x88, 0x00,
	0x00, 0x88, 0x88,
	0xCC, 0xCC, 0xCC, // eye white 

	0x22, 0x22, 0x22, // eyes
	0x88, 0x88, 0x00,
	0x00, 0x88, 0x88,
	0x20, 0x10, 0x59 // mouth
};

/**
 * Make a simple 1-bit image.
 * Align into the first byte of the word.
 * Since we're little-endian, that's actually the bottom byte.
 */
planar_image * make_img(uint16_t * rawdata) {
  planar_image * image = new_planar_image(32, 16, 1);
  for(int i = 0; i < 16; i++) {
    image->planes[0][i] = rawdata[i];
  }
  return image;
}


planar_image * display;
planar_image * background;

void write_demo_text() {
  bool fixed = false;
  char * txt = "The quick brown fox jumps over the lazy dog.";
  int stretch = 1;
  planar_image * ch = render_text(txt, stretch, fixed);
  
  // Copy text into those planes required to get readable green color
  planar_bitblt(background, ch, 0, 10, 0, true);
  planar_bitblt(background, ch, 0, 10, 2, true);

  fixed=true;
  free(ch);
  ch = render_text(txt, stretch, fixed);
  planar_bitblt(background, ch, 0, 30, 0, true);
  planar_bitblt(background, ch, 0, 30, 2, true);

  txt = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG!?";
  fixed = false;
  
  free(ch);
  ch = render_text(txt, stretch, fixed);
  planar_bitblt(background, ch, 0, 50, 0, true);
  planar_bitblt(background, ch, 0, 50, 2, true);

  txt = "!@#$%^&*()-=_+[]{}()|\\/\":;";

  free(ch);
  ch = render_text(txt, stretch, fixed);
  planar_bitblt(background, ch, 0, 70, 0, true);
  planar_bitblt(background, ch, 0, 70, 2, true);

  fixed=true;
  free(ch);
  ch = render_text(txt, stretch, fixed);
  planar_bitblt(background, ch, 0, 90, 0, true);
  planar_bitblt(background, ch, 0, 90, 2, true);
  free(ch);

  // log evidence to file
  write_bitmap("demo_text.bmp", pack(background), background->width, background->height, background->depth);
}

void * demo(void * args) {
  // planar_image * display = (planar_image *) args; // why bother

  planar_image * img_cat = make_img(cat);
  planar_image * img_cat2 = make_img(cat_color2);
  planar_image * img_cat3 = make_img(cat_eyes);

  planar_image * color_cat = new_planar_image(32, 16, 4);

  planar_bitblt(color_cat, img_cat, 0, 0, 0, false);
  planar_bitblt(color_cat, img_cat, 0, 0, 1, false);
  planar_bitblt(color_cat, img_cat2, 0, 0, 2, false);
  planar_bitblt(color_cat, img_cat3, 0, 0, 3, false);

  int i = 0; int j = 0;
  int increment_i = 1; int increment_j = 1;

  // Mainloop
  for (int counter = 0; counter < 780; counter++) {
      // Start with clean background; opaque bitblt
      planar_bitblt(display, background, 0, 0, 0, false);
      // Add kitty at coords; translucent bitblt
      planar_bitblt(display, color_cat, i, j, 0, true);
      
      i += increment_i;
      j += increment_j;
      
      // We can draw safely outside the screen!
      // But for the visual effect we will only dip outside briefly. 
      // Take into account that our sprite is actually 32 wide.
      if (i <= -32 || i >= display->width-16) increment_i = -increment_i;
      if (j <= -16 || j >= display->height) increment_j = -increment_j;

      display_redraw();
      usleep(20000);
  }

  write_bitmap("demo_text_with_kitty.bmp", pack(display), background->width, background->height, background->depth);

  return 0;
}


int main (int argc, char ** argv) {
  display = new_planar_image(320, 200, 4);
  background = new_planar_image(320, 200, 4);
  write_demo_text();

  display_init(argc, argv, display, palette2, 4);

  pthread_t worker;
  pthread_create(&worker, NULL, demo, display);

  display_runloop(worker);
}
