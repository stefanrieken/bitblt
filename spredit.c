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
#include "shared.h"

// - 2x3 8x8 tool selector
// (so image size is 16x24)
uint16_t toolbox[] = {
  // load / save; pencil
  0b0000000000000000,
  0b0111100000001100,
  0b0100010000011110,
  0b0100001000111110,
  0b0100001001111100,
  0b0100001001011000,
  0b0111111001110000,
  0b0000000000000000,
  // fill; line
  0b0000000000000000,
  0b0000000000000010,
  0b0000000000000100,
  0b0000000000001000,
  0b0111111000010000,
  0b0111111000100000,
  0b0111111001000000,
  0b0000000000000000,
  // rectangle; circle
  0b0000000000000000,
  0b0111111000111100,
  0b0100001001000010,
  0b0100001001000010,
  0b0100001001000010,
  0b0100001001000010,
  0b0111111000111100,
  0b0000000000000000,
};

;

DisplayData * dd;
PlanarImage * background;

void * mainloop(void * args) {

  configure_draw(1); // 1 bpp planes

  // DisplayData * dd = (DisplayData *) args;
  background = new_planar_image(80, 64, 4);

  // populate the palette, as such:
  for (int i=0; i<16; i++) {
    int x = (i % 2) * 8;
    int y = (i / 2) * 5;
    for (int plane=0; plane<4; plane++) {
      draw_rect(background->planes[plane], background->size, (coords) {x,y}, (coords) {x+7,y+4}, true, (i >> plane) & 0b1);
    }
  }
  // toolbox comes below palette, which is 5*8=40 high
  planar_bitblt_full(background, make_img(toolbox, 24), (coords){0,40}, -1);
  // checkerboard the toolbox
  draw_rect(background->planes[3], background->size, (coords) {8,40}, (coords) {15,47}, true, 1);
  draw_rect(background->planes[3], background->size, (coords) {0,48}, (coords) { 7,55}, true, 1);
  draw_rect(background->planes[3], background->size, (coords) {8,56}, (coords) {15,63}, true, 1);
  // checkerboard the edit window
  for (int i=0;i<64;i+=8) {
    for (int j=0;j<64;j+=8) {
      if (i%16 != j%16) {
        draw_rect(background->planes[3], background->size, (coords) {16+j,i}, (coords) {16+j+7,i+7}, true, 1);
      }
    }
  }
  // or no wait, load in data
  uint8_t * palette_read;
  PlanarImage * spritesheet = unpack(read_bitmap_nt("spritesheet.bmp", &palette_read));
  if (spritesheet != NULL)
    planar_bitblt_all(background, spritesheet, (coords){0,0}, spritesheet->size, (coords) {16,0}, -1);

  // Copy background to display.
  // Even though drawing is done directly on the background,
  // we do want to add & remove overlays on it at will.
  planar_bitblt_full(dd->planar_display, background, (coords){0,0}, -1); // TODO optimize rectangle
  display_redraw((area) {(coords) {0,0}, (coords){background->size.x, background->size.y}}); // TODO optimize rectangle
  return 0;
}

bool has_overlay;
void show_overlay(char * txt) {
  PlanarImage * text = render_text(txt, 1, false, false);
  PlanarImage * no_text = new_planar_image(text->size.x , text->size.y, 1);

  coords from;
  from.x = dd->planar_display->size.x - text->size.x;
  from.y = dd->planar_display->size.y - text->size.y;

  planar_bitblt_plane(dd->planar_display,    text, (coords) {from.x, from.y}, 0);
  planar_bitblt_plane(dd->planar_display, no_text, (coords) {from.x, from.y}, 1);
  planar_bitblt_plane(dd->planar_display, no_text, (coords) {from.x, from.y}, 2);
  planar_bitblt_plane(dd->planar_display, no_text, (coords) {from.x, from.y}, 3);

  display_redraw((area) { from, (coords) {from.x+text->size.x, from.y+text->size.y} });
  has_overlay = true;
}

uint32_t color;
void click_cb(coords from, coords to) {
  if (to.x < 0 || to.y < 0 || from.x < 0 || from.y < 0) return;
  if (from.x == to.x && from.y==to.y && to.x <16 && to.y < 40) {
    // pick a color!
    color = gather_pixel(background, to.y*image_aligned_width(background->size.x, 1)+to.x);
  }
}

void draw_cb(coords from, coords to) {
  if (to.x < 0 || to.y < 0 || from.x < 0 || from.y < 0) return;
  if(from.x > 15 && to.x > 15) {
    // draw a color!
    for(int i=0; i<background->depth; i++) {
      draw_line(background->planes[i], background->size, from, to, ((color >> i) & 1));
    }

    // calculate the dirty area as a positive area
    int fromx = (from.x < to.x ? from.x : to.x);
    int fromy = from.y < to.y ? from.y : to.y;
    int tox = from.x > to.x ? from.x : to.x;
    int toy = from.y > to.y ? from.y : to.y;

    // The '+1' is needed because 'draw_line' counts inclusive but redraw doesn't
    planar_bitblt_all(dd->planar_display, background, (coords){fromx,fromy}, (coords) {tox+1,toy+1}, (coords){fromx,fromy}, -1);
    display_redraw((area) { (coords){fromx,fromy}, (coords) {tox+1,toy+1} });
  }
}

char * tooltips[] = {
  "file", "draw", "fill", "line", "square", "circle"
};

void hover_cb(coords where) {
  if (where.x < 0 || where.y < 0) return;
  if (where.x <16 && where.y < 40) {
    // pick a color!
    uint32_t color = gather_pixel(background, where.y*image_aligned_width(background->size.x, 1)+where.x);

    char buffer[8];
    sprintf(buffer, "#%02X%02X%02X", palette[color*3], palette[color*3+1], palette[color*3+2]);
    show_overlay(buffer);
  } else {
    if (has_overlay) {
      // hide overlay by restoring background
      coords from = (coords) {16,background->size.y-8};
      coords to = (coords) {background->size.x, background->size.y};
      planar_bitblt_all(dd->planar_display, background, from, to, from, -1);
      display_redraw((area) { from, to });
      has_overlay = false;
    }
    if (where.x <16 && where.y > 40) {
      show_overlay(tooltips[(where.x/8) + (((where.y-40)/8)*2)]);
    }
  }
}

int main (int argc, char ** argv) {
  dd = malloc(sizeof(DisplayData));

  // using 80x64 for planned device compatibility (with 160x128; pygamer / meowbit)
  // - 64x64 editor window
  // - 16x64 pixels on the side:
  //  - 2x8 8x5 pixel palette selector
  //  - 2x3 8x8 tool selector
  dd->planar_display = new_planar_image(80, 64, 4);
  dd->palette = palette;
  dd->scale = 2; // comes on top of any default scaling

  display_init(argc, argv, dd, delegate_callbacks(draw_cb, click_cb, hover_cb));

  pthread_t worker;
  pthread_create(&worker, NULL, mainloop, dd);

  display_runloop(worker);
}
