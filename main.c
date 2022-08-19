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
#include "tilemap.h"
#include "shared.h"

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

// write directly to display
void write_intro_text(PlanarImage * on) {
  draw_text(on, "This screen and the cat show off packed bitblt,", 1, false, true);
  draw_text(on, "including some vertical clipping of the cat's body.", 2, false, true);
  draw_text(on, "The next screen shows the same in planar bitblt.", 3, false, true);
  draw_text(on, "During animation you can also draw on both screens.", 4, false, true);
}

void write_demo_text(PlanarImage * on) {
  bool fixed = false;
  char * txt = "The quick brown fox jumps over the lazy dog. ";

  draw_text(on, txt, 1, false, false);
  draw_text(on, txt, 2, true, false);

  txt = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG!?";
  draw_text(on, txt, 3, true, false);

  txt = "!@#$%^&*()-=_+[]{}()|\\/\":;                     ";
  draw_text(on, txt, 4, false, false);
  draw_text(on, txt, 5, true, false);
}

DisplayData * dd;
PlanarImage * background;
PackedImage * packed_bg;
area all;

void * demo(void * args) {
  //dd = (DisplayData *) args;
  PackedImage * packed_disp = dd->display;

  // PlanarImage *
  background = new_planar_image(310, 200, 4);
  all = (area) {(coords) {0,0}, (coords) {background->size.x, background->size.y}};

  PlanarImage * img_cat = make_img(cat, 24);
  PlanarImage * img_cat2 = make_img(cat_color2, 24);
  PlanarImage * img_cat3 = make_img(cat_eyes, 24);

  PlanarImage * color_cat = new_planar_image(16, 24, 4);
  // copy in individual color layers
  planar_bitblt_plane(color_cat, img_cat , (coords) {0,0}, 0);
  planar_bitblt_plane(color_cat, img_cat , (coords) {0,0}, 1);
  planar_bitblt_plane(color_cat, img_cat2, (coords) {0,0}, 2);
  planar_bitblt_plane(color_cat, img_cat3, (coords) {0,0}, 3);

  // show off some 'packed' tricks

  write_intro_text(background);

  packed_bg = to_packed_image(pack(background), background->size.x, background->size.y, background->depth);
  PackedImage * packed_cat = to_packed_image(pack(color_cat), color_cat->size.x, color_cat->size.y, color_cat->depth);

  configure_draw(packed_bg->depth); // 4 bpp packed
  draw_rect(packed_bg->data, packed_bg->size, (coords) {120,95}, (coords) {190,160}, true, 4);
  draw_rect(packed_bg->data, packed_bg->size, (coords) {120,95}, (coords) {190,160}, false, 6);

  draw_line (packed_bg->data, packed_bg->size, (coords) {10,15}, (coords) {170,170}, 4);
  draw_line (packed_bg->data, packed_bg->size, (coords) {170,15}, (coords) {10,170}, 4);
  // rotation in the algorithm draws the same line going back slightly different
  draw_line (packed_bg->data, packed_bg->size, (coords) {170,170}, (coords) {10,15}, 6);
  draw_line (packed_bg->data, packed_bg->size, (coords) {10,170}, (coords) {170,15}, 6);


  // Tile map short demo
  TileMap * map = malloc (sizeof(TileMap));
  map->tileset = packed_cat;
  map->tile_size.x = 8;
  map->tile_size.y = 4; // just for demo purposes, rectangular tiles
  map->map = malloc (sizeof(uint8_t) * 12);
  map->map_size = (coords) {2,6};
  map->mask=0xFF;

  // because we want 'all' the tiles in the same order,
  // the maps is easily filled like so:
  for (int i=0;i<12;i++)
    map->map[i]=i;

  apply_plain_tile_map(map, packed_bg, (coords) {0,0}, map->map_size, packed_bitblt, (coords) {100,100}, 0);
  display_redraw(all);

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
      if (i <= -16 || i >= packed_disp->size.x) increment_i = -increment_i;
      if (j <= -16 || j >= packed_disp->size.y) increment_j = -increment_j;

      if (!dd->display_finished) {
        display_redraw(all);
        usleep(20000); // don't time the display if there is none
      }
  }

  // now some planar tricks
  PlanarImage * display = new_planar_image(dd->display->size.x, dd->display->size.y, dd->display->depth);
  dd->display = display;
  dd->packed = false;
  write_demo_text(background);

  // drawing primitives!
  configure_draw(1); // 1 bpp planes
  draw_circle(background->planes[1], background->size, (coords) {80,80}, (coords) {160,140}, false, false, 1);
  draw_circle(background->planes[3], background->size, (coords) {80,80}, (coords) {160,140}, false, true, 1);

  draw_rect(background->planes[1], background->size, (coords) {120,95}, (coords) {190,160}, false, 1);
  draw_rect(background->planes[2], background->size, (coords) {120,95}, (coords) {190,160}, true, 1);

  draw_line (background->planes[1], background->size, (coords) {10,15}, (coords) {170,170}, 1);
  draw_line (background->planes[1], background->size, (coords) {170,15}, (coords) {10,170}, 1);
  // rotation in the algorithm draws the same line going back slightly different
  // because we add a different color plane going back, we produce more complex color artifacts
  // than in the packed version
  draw_line (background->planes[2], background->size, (coords) {170,170}, (coords) {10,15}, 1);
  draw_line (background->planes[2], background->size, (coords) {10,170}, (coords) {170,15}, 1);

  map->tileset = color_cat; // switch to planar image
  apply_plain_tile_map(map, background, (coords) {0,0}, map->map_size, planar_bitblt_all, (coords) {100,100}, 0);
  display_redraw(all);

  // log evidence to file
  write_bitmap("demo_text_out.bmp", palette, pack(background), background->size.x, background->size.y, background->depth);

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
        display_redraw(all);
        usleep(20000); // don't time the display if there is none
      }
  }

  write_bitmap("demo_text_with_kitty_out.bmp", palette, pack(display), background->size.x, background->size.y, background->depth);

  uint8_t (*palette_read)[];
  PackedImage * image_read = read_bitmap("demo_text_with_kitty_out.bmp", &palette_read);
  write_bitmap("read_write_out.bmp", palette, image_read->data, image_read->size.x, image_read->size.y, image_read->depth);

  for (int i=0;i<(1<<image_read->depth)*3;i++) (*dd->palette)[i] = (*palette_read)[i];
  free(palette_read);

  return 0;
}

void draw_cb(coords from, coords to) {
  if (dd->packed) {
    draw_line(packed_bg->data, background->size, from, to, 1);
  } else {
    draw_line(background->planes[0], background->size, from, to, 1);
  }
  display_redraw(all);
}

int main (int argc, char ** argv) {
  dd = malloc(sizeof(DisplayData));

  dd->packed = true;
  dd->display = new_packed_image(310, 200, 4);
  dd->palette = palette;
  dd->scale = 1;

  display_init(argc, argv, dd, delegate_callbacks(draw_cb, NULL, NULL));

  pthread_t worker;
  pthread_create(&worker, NULL, demo, dd);

  display_runloop(worker);
}
