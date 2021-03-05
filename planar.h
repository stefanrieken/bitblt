#ifndef PLANAR_H
#define PLANAR_H
typedef struct coords {
  uint32_t x;
  uint32_t y;
} coords;
typedef struct image {
  uint32_t * data;
  coords size;
} image;

image * pack(image ** images, uint32_t num);
void planar_bitblt(image ** backgrounds, image ** sprites, coords at, int depth);
#endif
