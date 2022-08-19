
#include <stdio.h>
#include <stdlib.h>
#include "planar.h"



int signum(int input) {
  if (input > 0) return 1;
  return ((input < 0) ? -1 : 0);
}

/**
 * Set drawing state to match a certain image depth.
 * Use a depth of 1 to work with planar image planes.
 */
int _bpp; // bits per pixel
int _ppw; // pixels per word
WORD_T _mask;
void configure_draw(int bpp) {
  _bpp = bpp;
  _ppw = WORD_SIZE / bpp;

  _mask = 0;
  for (int i=0;i<bpp;i++) _mask |= 1<<i;
}

// This is now implemented according to Bresenham's line algorithm,
// which has the quirk that it works slightly different for each octant,
// because it wants to iterate over the 'faster moving' side of x or y,
// only incrementing the other side when it exceeds a threshold value.
// This, and the need to work with negative directions, is why we have
// the big if-else block and the quirky in- or decrement in the for loops.
void draw_line (WORD_T * data, coords size, coords from, coords to, uint32_t value) {
  int aligned_width = image_aligned_width(size.x, _bpp);

  int distx = abs(to.x-from.x);
  int disty = abs(to.y-from.y);

  int signx = signum(to.x-from.x);
  int signy = signum(to.y-from.y);

  int error = 0;

  // pre-calculate these multipliers before
  // using them repeatedly in for loop
  int y_offset = from.y*aligned_width*_bpp;
  int addy = aligned_width*signy*_bpp;

  int x_offset = from.x*_bpp;
  int addx = signx*_bpp;

  // we draw the starting pixel even if the loop below won't run
  // (that happens if start an end are the same, so that the signums are zero)
  data[WORD(y_offset+x_offset)] &= ~(_mask << ((WORD_SIZE-_bpp)-BIT(x_offset)));
  data[WORD(y_offset+x_offset)] |= value << ((WORD_SIZE-_bpp)-BIT(x_offset));

  if (distx > disty) {
    int x_offset_end = (to.x+signx)*_bpp; // add signx to make it inclusive
    for (; x_offset != x_offset_end; x_offset += addx) {
      // TODO surely we can replace / and % by WORD_SIZE
      // with some smart bit shifting / masking as well?
      data[WORD(y_offset+x_offset)] &= ~(_mask << ((WORD_SIZE-_bpp)-BIT(x_offset)));
      data[WORD(y_offset+x_offset)] |= value << ((WORD_SIZE-_bpp)-BIT(x_offset));
      error += disty;
      if ((error * 2) >= distx) {
        y_offset += addy;
        error -= distx;
      }
    }
  } else {
    // same as above, but for y
    int y_offset_end = (to.y+signy) * aligned_width * _bpp;
    for (; y_offset != y_offset_end; y_offset += addy) {
      data[WORD(y_offset+x_offset)] &= ~(_mask << ((WORD_SIZE-_bpp)-BIT(x_offset)));
      data[WORD(y_offset+x_offset)] |= value << ((WORD_SIZE-_bpp)-BIT(x_offset));
      error += distx;
      if ((error *2) >= disty) {
        x_offset += addx;
        error -= disty;
      }
    }
  }
}

void draw_rect (WORD_T * data, coords size, coords from, coords to, bool fill, uint32_t value) {
  int aligned_width = image_aligned_width(size.x, _bpp);

  int stepx = fill ? _bpp : ((to.x - from.x)*_bpp); // do a silly thing to skip filling the middle
  int stepy = aligned_width*_bpp;

  int y_offset = from.y*aligned_width*_bpp;
  int y_offset_end = to.y*aligned_width*_bpp;

  int x_offset_end=to.x*_bpp;

  // draw top line
  for (int x_offset=from.x*_bpp; x_offset<=x_offset_end;x_offset += _bpp) {
    data[WORD(y_offset+x_offset)] &= ~(_mask << ((WORD_SIZE-_bpp)-BIT(x_offset)));
    data[WORD(y_offset+x_offset)] |= value << ((WORD_SIZE-_bpp)-BIT(x_offset));
  }
  // draw middle; either fill or only sides
  for (int i=y_offset+stepy; i<=y_offset_end; i+=stepy) {
    for (int x_offset=from.x*_bpp; x_offset<=x_offset_end; x_offset+=stepx) {
      data[WORD(i+x_offset)] &= ~(_mask << ((WORD_SIZE-_bpp)-BIT(x_offset)));
      data[WORD(i+x_offset)] |= value << ((WORD_SIZE-_bpp)-BIT(x_offset));
    }
  }
  // draw bottom line
  for (int x_offset=from.x*_bpp; x_offset<=x_offset_end;x_offset += _bpp) {
    data[WORD(y_offset_end+x_offset)] &= ~(_mask << ((WORD_SIZE-_bpp)-BIT(x_offset)));
    data[WORD(y_offset_end+x_offset)] |= value << ((WORD_SIZE-_bpp)-BIT(x_offset));
  }
}

// The 'midpoint circle algorithm' is like Bresenham's line algorithm but for circles.
// Our current implementation is probably way less optimized.
void draw_circle (WORD_T * data, coords size, coords from, coords to, bool arc, bool fill, uint32_t value) {

  int aligned_width = image_aligned_width(size.x, _bpp);

  // We decide for each pixel in the given selection whether it's inside or outside the circle.
  // TODO Doubtlessly there will be more efficient algorithms which just calculate y from x.

  int rx = abs(to.x - from.x) / (arc ? 1 : 2);
  int ry = abs(to.y - from.y) / (arc ? 1 : 2);

  // Also n.b. that 'arc' is also very poorly implemented right now.
  // Should be nice to draw a half circle into any direction just by
  // allowing expressing from and to in the negative.

  for (int i=from.y; i<to.y; i++) {
    for (int j=from.x; j<to.x; j++) {

      // calculate x and y as values from origin (center of circle)
      int x = (j-from.x-rx);
      int y = (i-from.y-ry);

      int exp = (x*x)*(ry*ry) + (y*y)*(rx*rx);

      int xsign = signum(x);
      int ysign = signum(y);

      // To get a solid line, establish wether we're crossing a boundary at this pixel,
      // with 'outward' being more outwards from the center.
      int outward;
      if (x*xsign > y*ysign) {
        outward = ((x+xsign)*(x+xsign)) * (ry*ry) + (y*y) * (rx*rx);
      } else {
        outward = (x*x) * (ry*ry) + ((y+ysign)*(y+ysign)) * (rx * rx);
      }

      // N.b. these numbers get way too big way too fast, but the regular version
      // ( x^2/rx^2 + y^2/ry^2 = 1) would have required us to use floats. (What's worse?)
      // In the end we're just using the ratio between rx^2and ry^2 to get our ellipse;
      // we might also express this ratio with much less precision.
      bool inside = exp < (rx*rx*ry*ry);
      bool on_line = inside && (outward >= (rx*rx*ry*ry));

      if (on_line || (fill && inside)) {
        // paint the pixel alright!
        data[(i*aligned_width+j)/WORD_SIZE] &= ~(_mask << ((WORD_SIZE-1)-(j%WORD_SIZE)));
        data[(i*aligned_width+j)/WORD_SIZE] |= value << ((WORD_SIZE-1)-(j%WORD_SIZE));
      }
    }
  }
}
