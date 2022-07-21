#ifndef BITBLT_SHARED_H
#define BITBLT_SHARED_H

#define WORD_SIZE 32
#define WORD_T uint32_t

typedef struct coords {
  int x;
  int y;
} coords;

/**
 * 'Base class' for image, wherever we can be implementation agnostic.
 * (in terms of planar / packed). Not presently used yet though!
 *
 * Note that actually extending structs in C is a pain,
 * but it can of course be cast.
 */
typedef struct image {
  coords size;
  int depth;
} image;

#endif /* BITBLT_SHARED_H */
