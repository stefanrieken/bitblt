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

PlanarImage * background;

void * mainloop(void * args) {

  configure_draw(1); // 1 bpp planes

  //display_data * dd = (display_data *) args;
  // populate the palette, as such:
  for (int i=0; i<16; i++) {
    int x = (i % 2) * 8;
    int y = (i / 2) * 5;
    for (int plane=0; plane<4; plane++) {
      draw_rect(background->planes[plane], background->size, (coords) {x,y}, (coords) {x+7,y+4}, true, (i >> plane) & 0b1);
    }
  }
  // toolbox comes below palette, which is 5*8=40 high
  planar_bitblt_full(background, make_img(toolbox, 24), (coords){0,40}, false);
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

  display_redraw((area) {(coords) {0,0}, (coords){background->size.x, background->size.y}});
  return 0;
}

uint32_t color;
void draw_cb(coords from, coords to) {
  if (to.x < 0 || to.y < 0 || from.x < 0 || from.y < 0) return;
  if (from.x == to.x && from.y==to.y && to.x <16 && to.y < 40) {
    // pick a color!
    color = gather_pixel(background, to.y*image_aligned_width(background->size.x, 1)+to.x);
  } else if(from.x > 15 && to.x > 15) {
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
    display_redraw((area) { (coords){fromx,fromy}, (coords) {tox+1,toy+1} });
  }
}

int main (int argc, char ** argv) {
  DisplayData * dd = malloc(sizeof(DisplayData));

  // using 80x64 for planned device compatibility (with 160x128; pygamer / meowbit)
  // - 64x64 editor window
  // - 16x64 pixels on the side:
  //  - 2x8 8x5 pixel palette selector
  //  - 2x3 8x8 tool selector
  background = new_planar_image(80, 64, 4);
  dd->planar_display = background;
  dd->palette = palette;
  dd->scale = 2; // comes on top of any default scaling

  display_init(argc, argv, dd, delegate_draw_callback(draw_cb));

  pthread_t worker;
  pthread_create(&worker, NULL, mainloop, dd);

  display_runloop(worker);
}
