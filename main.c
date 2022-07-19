#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#include "bitmap.h"
#include "planar.h"
#include "packed.h"
#include "display/display.h"
#include "font.h"

uint16_t cat[] = {
  0b0110000000000000,
  0b0111000000001111,
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
  0b0000111111110000,

  0b0000001111000000,
  0b0000111111110000,
  0b0001111111111000,
  0b0011111111111100,
  0b0011110000111100,
  0b0011100000011100,
  0b0011100000011100,
  0b0011100000011100
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
  0b0000000000000000,

  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000001111000000,
  0b0000011111100000,
  0b0000011111100000,
  0b0000011111100000,
  0b0000011111100000
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
  0b0000000000000000,

  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000
};


// Catty paletty! (and some garbage)
uint8_t palette[] = {
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
planar_image * make_img(uint16_t * rawdata, int height) {
  planar_image * image = new_planar_image(16, height, 1);
  for(int i = 0; i < height; i++) {
    image->planes[0][i] = rawdata[i] << 16; // shove into MSB
  }
  return image;
}

void draw_text(planar_image * on, char * text, int line, bool fixed) {
  int stretch = 1;

  planar_image * txt = render_text(text, stretch, fixed);
  // copy into these 2 planes to get a green color
  planar_bitblt_plane(on, txt, (coords) {0, line*20+10}, 0);
  planar_bitblt_plane(on, txt, (coords) {0, line*20+10}, 2);
  free(txt);
}

// write directly to display
void write_intro_text(planar_image * on) {
  draw_text(on, "This screen and the cat demoes packed bitblt.", 1, false);
  draw_text(on, "Next up, we do the same trick with planar bitblt,", 2, false);
  draw_text(on, "where we can also clip the image somewhat.", 3, false);
}

void write_demo_text(planar_image * on) {
  bool fixed = false;
  char * txt = "The quick brown fox jumps over the lazy dog.";
  
  draw_text(on, txt, 1, false);
  draw_text(on, txt, 2, true);

  txt = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG!?";
  draw_text(on, txt, 3, true);

  txt = "!@#$%^&*()-=_+[]{}()|\\/\":;";
  draw_text(on, txt, 4, false);
  draw_text(on, txt, 5, true);
}

void * demo(void * args) {
  planar_image * display = (planar_image *) args; // why bother

  planar_image * background = new_planar_image(310, 200, 4);

  planar_image * img_cat = make_img(cat, 24);
  planar_image * img_cat2 = make_img(cat_color2, 24);
  planar_image * img_cat3 = make_img(cat_eyes, 24);

  planar_image * color_cat = new_planar_image(16, 24, 4);
  // copy in individual color layers
  planar_bitblt_plane(color_cat, img_cat , (coords) {0,0}, 0);
  planar_bitblt_plane(color_cat, img_cat , (coords) {0,0}, 1);
  planar_bitblt_plane(color_cat, img_cat2, (coords) {0,0}, 2);
  planar_bitblt_plane(color_cat, img_cat3, (coords) {0,0}, 3);

  // show off some 'packed' tricks
  write_intro_text(background);

  packed_image * packed_disp = to_packed_image(pack(display), display->size.x, display->size.y, display->depth);
  packed_image * packed_bg = to_packed_image(pack(background), display->size.x, display->size.y, display->depth);
  packed_image * packed_cat = to_packed_image(pack(color_cat), color_cat->size.x, color_cat->size.y, color_cat->depth);

  // Mainloop 1
  // Notice this demo is a lot slower than the next one,
  // but this is mainly due to all the conversion from planar to packed, and to planar again for the display.
  // 
  int i = 0; int j=0;
  int increment_i = 1; int increment_j = 1;

  for (int counter = 0; counter < 780; counter++) {
      // Start with clean background; opaque
      packed_bitblt(packed_disp, packed_bg, (coords) {0,0}, false);
      // Add kitty's head at coords; translucent
      packed_bitblt(packed_disp, packed_cat, (coords) {i, j}, true);

      i += increment_i;
      j += increment_j;

      // We can draw safely outside the screen!
      // But for the visual effect we will only dip outside briefly. 
      // Take into account that our sprite is actually 32 wide.
      if (i <= -16 || i >= display->size.x) increment_i = -increment_i;
      if (j <= -16 || j >= display->size.y) increment_j = -increment_j;

      // the display driver presently uses the planar 'display' for its data, so translate back
      // NOTE this part makes the packed demo seem especially slow, where it actually isn't.
      planar_image * unpacked = unpack(packed_disp);
      planar_bitblt_full(display, unpacked, (coords) {0, 0}, false);

      if (!display_finished) {
        display_redraw();
        usleep(20000); // don't time the display if there is none
      }
  }

  // now some planar tricks
  write_demo_text(background);
  // log evidence to file
  write_bitmap("demo_text.bmp", palette, pack(background), background->size.x, background->size.y, background->depth);

  // Mainloop 2
  i = 0; j=0;
  increment_i = 1; increment_j = 1;

  for (int counter = 0; counter < 780; counter++) {
      // Start with clean background; opaque bitblt
      planar_bitblt_full(display, background, (coords){0,0}, false);
      // Add kitty's head at coords; translucent bitblt
      // This only demoes height clipping; width clipping may work at word sizes only.
      // A practical work-around is to copy the clip area into a clip-sized image first.
      // So e.g. first copy out a 16- or 8-bit wide sprite from a sprite map before using it.
      planar_bitblt(display, color_cat, (coords) {0,0}, (coords) {16,16}, (coords) {i,j}, 0, true);

      i += increment_i;
      j += increment_j;

      // We can draw safely outside the screen!
      // But for the visual effect we will only dip outside briefly. 
      // Take into account that our sprite is actually 32 wide.
      if (i <= -16 || i >= display->size.x) increment_i = -increment_i;
      if (j <= -16 || j >= display->size.y) increment_j = -increment_j;

      if (!display_finished) {
        display_redraw();
        usleep(20000); // don't time the display if there is none
      }
  }

  write_bitmap("demo_text_with_kitty.bmp", palette, pack(display), background->size.x, background->size.y, background->depth);

  return 0;
}


int main (int argc, char ** argv) {
  planar_image * display = display = new_planar_image(310, 200, 4);
 
  display_init(argc, argv, display, palette, 4);

  pthread_t worker;
  pthread_create(&worker, NULL, demo, display);

  display_runloop(worker);
}
