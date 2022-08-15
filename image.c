#include <stdio.h>
#include "shared.h"

uint32_t image_aligned_width(uint32_t width, int bpp) {
  uint32_t px_per_word = WORD_SIZE / bpp;
  return width + ((width % px_per_word == 0) ? 0 : (px_per_word-(width % px_per_word)));
}
