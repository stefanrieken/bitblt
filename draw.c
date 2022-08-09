
#include <stdio.h>
#include <stdlib.h>
#include "planar.h"



#define MASK 1

int signum(int input) {
  if (input > 0) return 1;
  return ((input < 0) ? -1 : 0);
}

// This is now implemented according to Bresenham's line algorithm,
// which has the quirk that it works slightly different for each octant,
// because it wants to iterate over the 'faster moving' side of x or y,
// only incrementing the other side when it exceeds a threshold value.
// This, and the need to work with negative directions, is why we have
// the big if-else block and the quirky in- or decrement in the for loops.
void draw_line (WORD_T * data, coords size, coords from, coords to, uint32_t value) {
  // since we may assume 1bpp, this one applies
  int aligned_width = planar_aligned_width(size.x);

  int distx = abs(to.x-from.x);
  int disty = abs(to.y-from.y);

  int signx = signum(to.x-from.x);
  int signy = signum(to.y-from.y);

  int error = 0;

  // we draw the starting pixel even if the loop below won't run
  // (that happens if start an end are the same, so that the signums are zero)
  // TODO no (repeated) multiplications and divisions per pixel!
  data[(from.y*aligned_width+from.x)/WORD_SIZE] &= ~(MASK << ((WORD_SIZE-1)-(from.x%WORD_SIZE)));
  data[(from.y*aligned_width+from.x)/WORD_SIZE] |= value << ((WORD_SIZE-1)-(from.x%WORD_SIZE));

  int x = from.x;
  int y = from.y;

  if (distx > disty) {
    int y = from.y;
    for (int x = from.x; x != to.x+signx; x += signx) {
      data[(y*aligned_width+x)/WORD_SIZE] &= ~(MASK << ((WORD_SIZE-1)-(x%WORD_SIZE)));
      data[(y*aligned_width+x)/WORD_SIZE] |= value << ((WORD_SIZE-1)-(x%WORD_SIZE));
      error += disty;
      if ((error * 2) >= distx) {
        y += signy;
        error -= distx;
      }
    }
  } else {
    int x = from.x;
    for (int y = from.y; y != to.y+signy; y += signy) {
      data[(y*aligned_width+x)/WORD_SIZE] &= ~(MASK << ((WORD_SIZE-1)-(x%WORD_SIZE)));
      data[(y*aligned_width+x)/WORD_SIZE] |= value << ((WORD_SIZE-1)-(x%WORD_SIZE));
      error += distx;
      if ((error *2) >= disty) {
        x += signx;
        error -= disty;
      }
    }
  }
}

void draw_rect (WORD_T * data, coords size, coords from, coords to, bool fill, uint32_t value) {
  // since we may assume 1bpp, this one applies
  int aligned_width = planar_aligned_width(size.x);

  // do a silly thing to skip filling the middle
  int stepx = fill ? 1 : (to.x - from.x)-1;

  // draw top line
  for (int j=from.x; j<to.x;j++) {
    data[(from.y*aligned_width+j)/WORD_SIZE] &= ~(MASK << ((WORD_SIZE-1)-(j%WORD_SIZE)));
    data[(from.y*aligned_width+j)/WORD_SIZE] |= value << ((WORD_SIZE-1)-(j%WORD_SIZE));
  }
  // draw middle; either fill or only sides
  for (int i=from.y+1; i<to.y-1; i++) {
    for (int j=from.x; j<to.x; j+=stepx) {
      data[(i*aligned_width+j)/WORD_SIZE] &= ~(MASK << ((WORD_SIZE-1)-(j%WORD_SIZE)));
      data[(i*aligned_width+j)/WORD_SIZE] |= value << ((WORD_SIZE-1)-(j%WORD_SIZE));
    }
  }
  // draw bottom line
  for (int j=from.x; j<to.x;j++) {
    data[((to.y-1)*aligned_width+j)/WORD_SIZE] &= ~(MASK << ((WORD_SIZE-1)-(j%WORD_SIZE)));
    data[((to.y-1)*aligned_width+j)/WORD_SIZE] |= value << ((WORD_SIZE-1)-(j%WORD_SIZE));
  }
}

// The 'midpoint circle algorithm' is like Bresenham's line algorithm but for circles.
// Our current implementation is probably way less optimized.
void draw_circle (WORD_T * data, coords size, coords from, coords to, bool arc, bool fill, uint32_t value) {

  // since we may assume 1bpp, this one applies
  int aligned_width = planar_aligned_width(size.x);

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
        data[(i*aligned_width+j)/WORD_SIZE] &= ~(MASK << ((WORD_SIZE-1)-(j%WORD_SIZE)));
        data[(i*aligned_width+j)/WORD_SIZE] |= value << ((WORD_SIZE-1)-(j%WORD_SIZE));
      }
    }
  }
}
