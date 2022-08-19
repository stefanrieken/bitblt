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
  // color; line
  0b0000000000000000,
  0b0010010000000010,
  0b0111111000000100,
  0b0010010000001000,
  0b0010010000010000,
  0b0111111000100000,
  0b0010010001000000,
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

char * overlay_txt;
void show_overlay(char * txt) {
  PlanarImage * text = render_text(txt, 1, false, false);
  PlanarImage * no_text = new_planar_image(text->size.x , text->size.y, 1);

  coords from;
  from.x = dd->display->size.x - text->size.x;
  from.y = dd->display->size.y - text->size.y;

  planar_bitblt_plane(dd->display,    text, (coords) {from.x, from.y}, 0);
  planar_bitblt_plane(dd->display, no_text, (coords) {from.x, from.y}, 1);
  planar_bitblt_plane(dd->display, no_text, (coords) {from.x, from.y}, 2);
  planar_bitblt_plane(dd->display, no_text, (coords) {from.x, from.y}, 3);

  display_redraw((area) { from, (coords) {from.x+text->size.x, from.y+text->size.y} });
  overlay_txt = txt;
}

PlanarImage * keypad;
int32_t num_typed = -1;
uint32_t typedcolor = 0;
void make_keypad() {
  PlanarImage * text = render_text("0123456789abcdef", 1, true, false);

  keypad = new_planar_image(64,64,4);

  for (int i=0; i< 4; i++) {
    draw_line(keypad->planes[0], keypad->size, (coords) {i*16,0}, (coords) {i*16,63}, 1);
    draw_line(keypad->planes[0], keypad->size, (coords) {0,i*16}, (coords) {63,i*16}, 1);

    for (int j=0; j< 4; j++) {
      planar_bitblt(keypad, text, (coords) {(i*4+j)*6, 0}, (coords) {(i*4+j)*6+6, 8}, (coords) {j*16+5, i*16+5}, 0, 0);
    }
  }
  free(text->data);
  free(text);
}
void show_keypad() {
  num_typed = 0;
  typedcolor = 0;
  planar_bitblt_full(dd->display, keypad, (coords) {16, 0}, -1);
  display_redraw((area) { (coords) {16, 0}, (coords){80,64} });
}


uint32_t color;
void click_cb(coords from, coords to) {
  if (to.x < 0 || to.y < 0 || from.x < 0 || from.y < 0) return;

  if (from.x == to.x && from.y==to.y && to.x <16 && to.y < 40) {
    if (num_typed == 6) {
      // save color!
      color = (to.y/5)*2+(to.x/8);
      (*dd->palette)[color*3] = typedcolor & 0xFF;
      (*dd->palette)[color*3+1] = (typedcolor>>8) & 0xFF;
      (*dd->palette)[color*3+2] = (typedcolor>>16) & 0xFF;
      num_typed = -1;
      typedcolor = 0;
      display_redraw((area) { (coords){0,0}, (coords) {80,64} });
      show_overlay("color set");
    } else if (num_typed < 0) {
      // pick a color!
      color = gather_pixel(background, to.y*image_aligned_width(background->size.x, 1)+to.x);
    }
  } else if (num_typed >= 0) {
    if (to.x < 16) {
      // restore screen
      planar_bitblt_all(dd->display, background, (coords){16,0}, (coords) {80,64}, (coords) {16,0}, -1);
      display_redraw((area) { (coords){16,0}, (coords) {80,64} });
      num_typed = -1;
      typedcolor = 0;
    } else {
      typedcolor = (typedcolor <<4) | ((from.y/16)*4+((from.x-16)/16));
      if (++num_typed == 6) {
        // restore screen
        planar_bitblt_all(dd->display, background, (coords){16,0}, (coords) {80,64}, (coords) {16,0}, -1);
        display_redraw((area) { (coords){16,0}, (coords) {80,64} });
        char buffer[16];
        sprintf(buffer, "#%06x?", typedcolor);
        show_overlay(buffer);
      }
    }
  } else if (overlay_txt != NULL) {
    if (strcmp("save", overlay_txt) == 0) {
      PackedImage * out = new_packed_image(64,64,background->depth);
      PackedImage * bg = to_packed_image(pack(background), background->size.x, background->size.y, background->depth);
      packed_bitblt(out, bg, (coords){16,0}, bg->size, (coords) {0,0}, -1); // N.b. will cut off size to spreadsheet size!
      write_bitmap("out.bmp",dd->palette, out->data, out->size.x, out->size.y, out->depth);
      free(out->data);
      free(out);
      show_overlay("saved");
    } else if (strcmp("color", overlay_txt) == 0) {
      show_keypad();
      overlay_txt = NULL;
      }
  }
}

void draw_cb(coords from, coords to) {
  if (num_typed >= 0) return; // keypad is showing
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
    planar_bitblt_all(dd->display, background, (coords){fromx,fromy}, (coords) {tox+1,toy+1}, (coords){fromx,fromy}, -1);
    display_redraw((area) { (coords){fromx,fromy}, (coords) {tox+1,toy+1} });
  }
}

char * tooltips[] = {
  "save", "draw", "color", "line", "square", "circle"
};

void hover_cb(coords where) {
  if (num_typed >= 0) return; // keypad is on screen

  char buffer[8];
  if (where.x < 0 || where.y < 0) return;
  if (where.x <16 && where.y < 40) {
    // pick a color!
    uint32_t color = gather_pixel(background, where.y*image_aligned_width(background->size.x, 1)+where.x);

    sprintf(buffer, "#%02X%02X%02X", (*palette)[color*3+2], (*palette)[color*3+1], (*palette)[color*3]);
    show_overlay(buffer);
  } else {
    if (overlay_txt != NULL) {
      // hide overlay by restoring background
      coords from = (coords) {16,background->size.y-8};
      coords to = (coords) {background->size.x, background->size.y};
      planar_bitblt_all(dd->display, background, from, to, from, -1);
      display_redraw((area) { from, to });
      overlay_txt = NULL;
    }
    if (where.x <16 && where.y > 40) {
      show_overlay(tooltips[(where.x/8) + (((where.y-40)/8)*2)]);
    } else if (where.x >=16 && where.y < 64-8) {
      // show coords
      sprintf(buffer, "%d,%d", where.x-16, where.y);
      show_overlay(buffer);
    }
  }
}

void * mainloop(void * args) {
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

  // dot-mark edit window
  for (int i=0;i<64;i+=8) {
    for (int j=0;j<64;j+=8) {
      draw_line(background->planes[0], background->size, (coords) {16+j+7,i+7}, (coords) {16+j+7,i+7}, 1);
    }
  }
  // or no wait, load in data
  uint8_t (*palette_read)[];
  PackedImage * spritesheet = read_bitmap("spritesheet.bmp", &palette_read);
  if (spritesheet != NULL) {
    PlanarImage * unpacked = unpack(spritesheet);
    planar_bitblt_all(background, unpacked, (coords){0,0}, spritesheet->size, (coords) {16,0}, -1);
    free(spritesheet->data);
    free(spritesheet);

    for (int i=0;i<(1<<spritesheet->depth)*3;i++) (*dd->palette)[i] = (*palette_read)[i];
    free(palette_read);
  }

  // Copy background to display.
  // Even though drawing is done directly on the background,
  // we do want to add & remove overlays on it at will.
  planar_bitblt_full(dd->display, background, (coords){0,0}, -1);
  display_redraw((area) {(coords) {0,0}, (coords){background->size.x, background->size.y}});
  return 0;
}

int main (int argc, char ** argv) {
  dd = malloc(sizeof(DisplayData));

  // using 80x64 for planned device compatibility (with 160x128; pygamer / meowbit)
  // - 64x64 editor window
  // - 16x64 pixels on the side:
  //  - 2x8 8x5 pixel palette selector
  //  - 2x3 8x8 tool selector
  dd->packed = false;
  dd->display = new_planar_image(80, 64, 4);
  dd->palette = palette;
  dd->scale = 2; // comes on top of any default scaling

  overlay_txt = NULL;
  num_typed = -1;
  color = 0;

  configure_draw(1); // 1 bpp planes
  make_keypad();

  display_init(argc, argv, dd, delegate_callbacks(draw_cb, click_cb, hover_cb));

  pthread_t worker;
  pthread_create(&worker, NULL, mainloop, dd);

  display_runloop(worker);
}
