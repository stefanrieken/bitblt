#include <stdlib.h>
#include <stdio.h>
#include "shared.h"
#include "font.h"

/**
 * Catty paletty! (as well as some badly defined colors)
 */
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

static coords original;
static coords previous;
static area draw_area;
static DrawEventCallback * _draw_cb;
static ClickEventCallback * _click_cb;
static HoverEventCallback * _hover_cb;

/**
 * Interprets pointer up, down, and move into a single draw event.
 * (For now) delegates (only) this event to a user function.
 */
void delegate_draw_cb(UserEvent event, coords where) {

  if (event  == POINTER_DOWN) {
    original.x = where.x;
    original.y = where.y;
    previous.x = where.x;
    previous.y = where.y;
    _draw_cb(previous, previous);
  }
  if (event == POINTER_UP) {
    // we might want to dynamically (de)activate GDK_POINTER_MOTION_MASK as well
    previous.x=-1;
    previous.y=-1;
    _click_cb(original, where);
  }
  if (event == POINTER_MOVE) {
    if (previous.x == -1) {
      if (_hover_cb != NULL) _hover_cb(where);
    } else {
      _draw_cb(previous, where);
      previous.x = where.x;
      previous.y = where.y;
    }
  }
}

void callback_none(coords from, coords to) {}

/** Call this to set up the delegate draw event with your user function. */
UserEventCallback * delegate_callbacks(
  DrawEventCallback * draw_cb,
  ClickEventCallback * click_cb,
  HoverEventCallback * hover_cb
) {
  previous.x=-1;
  previous.y=-1;
  _draw_cb = draw_cb == NULL ? callback_none : draw_cb;
  _click_cb = click_cb == NULL ? callback_none : click_cb;
  _hover_cb = hover_cb;
  return delegate_draw_cb;
}

/**
 * Make a simple 1-bit image.
 * Align into the first byte of the word.
 * Since we're little-endian, that's actually the bottom byte.
 */
PlanarImage * make_img(uint16_t * rawdata, int height) {
  PlanarImage * image = new_planar_image(16, height, 1);
  for(int i = 0; i < height; i++) {
    image->planes[0][i] = rawdata[i] << 16; // shove into MSB
  }
  return image;
}

void draw_text(PlanarImage * on, char * text, int line, bool fixedWidth, bool fixedHeight) {
  int stretch = 1;

  PlanarImage * txt = render_text(text, stretch, fixedWidth, fixedHeight);
  // copy into these 2 planes to get a green color
  planar_bitblt_plane(on, txt, (coords) {1, line*20+10}, 0);
  planar_bitblt_plane(on, txt, (coords) {1, line*20+10}, 2);
  free(txt);
}
