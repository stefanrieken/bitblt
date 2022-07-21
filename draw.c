
#include <stdio.h>
#include <stdlib.h>
#include "planar.h"

/**
 * Drawing primitives, presently targeting 1bpp single layer images.
 */


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

      // To get a solid line, establish wether we're crossing a boundary at this pixel,
      // with 'outward' being more outwards from the center.
      int value = (x*x)*(ry*ry) + (y*y)*(rx*rx);

      int xsign = (x == 0) ? 0 : (x/abs(x)); // missing a standard C 'signum' function here
      int ysign = (y == 0) ? 0 : (y/abs(y));

      int outward;
      if (x*xsign > y*ysign) {
        outward = ((x+xsign)*(x+xsign)) * (ry*ry) + (y*y) * (rx*rx);
      } else {
        outward = (x*x) * (ry*ry) + ((y+ysign)*(y+ysign)) * (rx * rx);
      }

      int bias = y == 0 ? x : x / y;
 
      // Granted, these numbers get big, but the regular version where the
      // outcome is near or below 1 would have required us to use floats.
      bool inside = value < (rx*rx*ry*ry);
      bool on_line = inside && (outward >= (rx*rx*ry*ry));

      if (on_line || (fill && inside)) {
        // paint the pixel alright!
        data[(i*aligned_width+j)/WORD_SIZE] |= 1 << ((WORD_SIZE-1)-(j%WORD_SIZE)); 
        
      }
    }
  }
}
