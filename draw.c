
#include <stdio.h>
#include <stdlib.h>
#include "planar.h"

/**
 * Drawing primitives, presently targeting 1bpp single layer images.
 */


// There exists a function called Bresenham's line algorithm,
// which may account for the quirks converting pixels to coordinates.
// Here we just derive the formula and caluclate Y for X.
void draw_line (WORD_T * data, coords size, coords from, coords to) {
  // since we may assume 1bpp, this one applies
  int aligned_width = planar_aligned_width(size.x);

  // missing a standard C 'signum' function here  int dirx = (to.x-from.x) / abs(to.x-from.x);
  int dirx = (to.x-from.x) / abs(to.x-from.x);
  int diry = (to.y-from.y) / abs(to.y-from.y);

  int distx = ((dirx > 0 ? (to.x-from.x) : (from.x - to.x))+1) * dirx;
  int disty = ((diry > 0 ? (to.y-from.y) : (from.y - to.y))+1) * diry;

  for (int j=from.x; j!=to.x+dirx;j+=dirx) {
    int y = from.y + (((j-from.x) * disty) / distx);
    data[(y*aligned_width+j)/WORD_SIZE] |= 1 << ((WORD_SIZE-1)-(j%WORD_SIZE));
  }
}

void draw_rect (WORD_T * data, coords size, coords from, coords to, bool fill) {
  // since we may assume 1bpp, this one applies
  int aligned_width = planar_aligned_width(size.x);

  // do a silly thing to skip filling the middle
  int stepx = fill ? 1 : (to.x - from.x)-1; 

  // draw top line
  for (int j=from.x; j<to.x;j++) {
    data[(from.y*aligned_width+j)/WORD_SIZE] |= 1 << ((WORD_SIZE-1)-(j%WORD_SIZE));
  }
  // draw middle; either fill or only sides
  for (int i=from.y+1; i<to.y-1; i++) {
    for (int j=from.x; j<to.x; j+=stepx) {
      data[(i*aligned_width+j)/WORD_SIZE] |= 1 << ((WORD_SIZE-1)-(j%WORD_SIZE)); 
    }
  }
  // draw bottom line
  for (int j=from.x; j<to.x;j++) {
    data[((to.y-1)*aligned_width+j)/WORD_SIZE] |= 1 << ((WORD_SIZE-1)-(j%WORD_SIZE));
  }
}

// The 'midpoint circle algorithm' is likes Bresenham's line algorithm but for circles.
// Our current implementation is probably way less optimized.
void draw_circle (WORD_T * data, coords size, coords from, coords to, bool arc, bool fill) {

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

      int value = (x*x)*(ry*ry) + (y*y)*(rx*rx);

      // missing a standard C 'signum' function here
      int xsign = (x == 0) ? 0 : (x/abs(x));
      int ysign = (y == 0) ? 0 : (y/abs(y));

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
      bool inside = value < (rx*rx*ry*ry);
      bool on_line = inside && (outward >= (rx*rx*ry*ry));

      if (on_line || (fill && inside)) {
        // paint the pixel alright!
        data[(i*aligned_width+j)/WORD_SIZE] |= 1 << ((WORD_SIZE-1)-(j%WORD_SIZE)); 
        
      }
    }
  }
}
