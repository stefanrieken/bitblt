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
#include "draw.h"

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
  draw_text(on, "This screen and the cat show off packed bitblt,", 1, false);
  draw_text(on, "including some vertical clipping of the cat's body.", 2, false);
  draw_text(on, "The next screen shows the same in planar bitblt.", 3, false);
  draw_text(on, "You can also draw some pixels there during animation.", 4, false);
}

void write_demo_text(planar_image * on) {
  bool fixed = false;
  char * txt = "The quick brown fox jumps over the lazy dog.";

  draw_text(on, txt, 1, false);
  draw_text(on, txt, 2, true);

  txt = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG!?";
  draw_text(on, txt, 3, true);

  txt = "!@#$%^&*()-=_+[]{}()|\\/\":;                    ";
  draw_text(on, txt, 4, false);
  draw_text(on, txt, 5, true);
}

planar_image * background;

void * demo(void * args) {
  display_data * dd = (display_data *) args;
  planar_image * display = dd->planar_display;

  // planar_image *
  background = new_planar_image(310, 200, 4);

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
  dd->packed = true;

  write_intro_text(background);

  packed_image * packed_disp = dd->packed_display;
  packed_image * packed_bg = to_packed_image(pack(background), background->size.x, background->size.y, background->depth);
  packed_image * packed_cat = to_packed_image(pack(color_cat), color_cat->size.x, color_cat->size.y, color_cat->depth);

  // Mainloop 1
  int i = 0; int j=0;
  int increment_i = 1; int increment_j = 1;

  for (int counter = 0; counter < 780; counter++) {
      // Start with clean background; opaque
      packed_bitblt_full(packed_disp, packed_bg, (coords) {0,0}, -1);
      // Add kitty's head at coords; translucent
      int translucent_color = (counter % 68 > 34) ? 3 : 0; // switch translucent color half of the time
      packed_bitblt(packed_disp, packed_cat, (coords) {0,0}, (coords){16,16 + ((i>>3)%8)}, (coords) {i, j}, translucent_color);

      i += increment_i;
      j += increment_j;

      // We can draw safely outside the screen!
      // But for the visual effect we will only dip outside briefly.
      // Take into account that our sprite is actually 32 wide.
      if (i <= -16 || i >= display->size.x) increment_i = -increment_i;
      if (j <= -16 || j >= display->size.y) increment_j = -increment_j;

      if (!dd->display_finished) {
        display_redraw();
        usleep(20000); // don't time the display if there is none
      }
  }

  // now some planar tricks
  dd->packed = false;
  write_demo_text(background);

  // drawing primitives!
  draw_circle(background->planes[1], background->size, (coords) {40,40}, (coords) {120,100}, false, false);
  draw_circle(background->planes[2], background->size, (coords) {40,40}, (coords) {120,100}, false, true);

  draw_rect(background->planes[1], background->size, (coords) {80,70}, (coords) {120,100}, false);
  draw_rect(background->planes[2], background->size, (coords) {80,70}, (coords) {120,100}, false);

  draw_line (background->planes[1], background->size, (coords) {10,15}, (coords) {170,170});
  // integer rounding differences draws the same line going back slightly different
  draw_line (background->planes[2], background->size, (coords) {170,170}, (coords) {10,15});
  draw_line (background->planes[2], background->size, (coords) {10,170}, (coords) {170,15});

  // log evidence to file
  write_bitmap("demo_text.bmp", palette, pack(background), background->size.x, background->size.y, background->depth);

  // Mainloop 2
  i = 0; j=0;
  increment_i = 1; increment_j = 1;

  for (int counter = 0; counter < 780; counter++) {
      // Start with clean background; opaque bitblt
      planar_bitblt_full(display, background, (coords){0,0}, -1);
      // Add kitty's head at coords; translucent bitblt
      // This only demoes height clipping; width clipping may work at word sizes only.
      // A practical work-around is to copy the clip area into a clip-sized image first.
      // So e.g. first copy out a 16- or 8-bit wide sprite from a sprite map before using it.
      int translucent_color = (counter % 68 > 34) ? 3 : 0; // switch translucent color half of the time
      planar_bitblt(display, color_cat, (coords) {0,0}, (coords) {16,16 + ((i>>3)%8)}, (coords) {i,j}, 0, translucent_color);

      i += increment_i;
      j += increment_j;

      // We can draw safely outside the screen!
      // But for the visual effect we will only dip outside briefly.
      // Take into account that our sprite is actually 32 wide.
      if (i <= -16 || i >= display->size.x) increment_i = -increment_i;
      if (j <= -16 || j >= display->size.y) increment_j = -increment_j;

      if (!dd->display_finished) {
        display_redraw();
        usleep(20000); // don't time the display if there is none
      }
  }

  write_bitmap("demo_text_with_kitty.bmp", palette, pack(display), background->size.x, background->size.y, background->depth);

  uint8_t * palette_read;
  packed_image * image_read = read_bitmap("demo_text_with_kitty.bmp", &palette_read);
  write_bitmap("i_read_this_you_know.bmp", palette_read, image_read->data, image_read->size.x, image_read->size.y, image_read->depth);

  return 0;
}

void draw_cb(coords from, coords to) {
  draw_line(background->planes[0], background->size, from, to);
}

int main (int argc, char ** argv) {
  display_data * dd = malloc(sizeof(display_data));

  dd->planar_display = new_planar_image(310, 200, 4);
  dd->packed_display = new_packed_image(310, 200, 4);
  dd->palette = palette;
  dd->scale = 1;

  display_init(argc, argv, dd, draw_cb);

  pthread_t worker;
  pthread_create(&worker, NULL, demo, dd);

  display_runloop(worker);
}
