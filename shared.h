#ifndef BITBLT_SHARED_H
#define BITBLT_SHARED_H

#include <stdint.h>

#define WORD_SIZE 32
#define WORD_T uint32_t

typedef struct coords {
  int x;
  int y;
} coords;

typedef struct area {
  coords from;
  coords to;
} area;

/**
 * 'Abstract class' for image,
 * as well as their 'implementation' aliases,
 * combined using a union.
 */
typedef struct image {
  union {
    WORD_T ** planes; // for planar images
    WORD_T * data; // for packed images
  };
  coords size;
  int depth;
} image, planar_image, packed_image;

#endif /* BITBLT_SHARED_H */
