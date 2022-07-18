
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "bitmap.h"
#include "packed.h"

// Little endian
void write_32bit(FILE * file, uint32_t data) {
  fputc(data & 0xFF, file); fputc((data >> 8) & 0xFF, file);
  fputc((data >> 16) & 0xFF, file); fputc((data >> 24) & 0xFF, file);
}

// Big-endian / character stream data
void write_32bit_data(FILE * file, uint32_t data) {
  fputc((data >> 24) & 0xFF, file); fputc((data >> 16) & 0xFF, file);
  fputc((data >> 8) & 0xFF, file); fputc(data & 0xFF, file);
}

// Little endian
void write_16bit(FILE * file, uint32_t data) {
  fputc(data & 0xFF, file); fputc((data >> 8) & 0xFF, file);
}

void write_bitmap(const char * filename, uint8_t * palette, uint32_t * packed_img, int width, int height, int bpp) {

  uint32_t packed_width_aligned = packed_aligned_width(width, bpp);

  uint32_t data_length_words = (packed_width_aligned * height * bpp) / 32;

  uint32_t num_colors = 2;
  for (int i=1; i<bpp;i++) num_colors *= 2;

  FILE * file = fopen(filename, "wb");
  // main header
  fputc('B', file); fputc('M', file);
  write_32bit(file, 14 + 12 + (num_colors * 3) + (data_length_words*4)); // file size: h1 + h2 + size
  write_16bit(file, 0); write_16bit(file, 0);
  write_32bit(file, 14 + 12 + (num_colors * 3)); // start of pixel array
  // bitmap core header. Trying to get away with the oldest, simplest variant
  write_32bit(file, 12); // sizeof header
  write_16bit(file, packed_width_aligned);
  write_16bit(file, height * -1); // To invert the image the right way up!
  write_16bit(file, 1); // # color planes, always 1
  write_16bit(file, bpp);
  // Alternatively, extend to bitmap info header (and set x and y to 32 bit)
  // write_32bit(file, 0); // no compression
  // write_32bit(file, data_length_bytes);
  // write_32bit(file, 2834); // pixels per meter?
  // write_32bit(file, 2834); // pixels per meter?
  // write_32bit(file, 0);
  // write_32bit(file, 0);
  // Color table. The old header implies RGB instead of RGBA
  // N.b. we actually have to write BGR!
  // 1bpp, 4bpp, 16bpp are all common; but 2bpp may be rare / unsupported
  for (int i=0; i<3*2;i++) {
    fputc(palette[i], file);
  }
  if (bpp >= 2) {
    for (int i=3*2; i<3*4;i++) {
      fputc(palette[i], file);
    }
  }
  if (bpp >= 4) {
    for (int i=3*4; i<3*16;i++) {
      fputc(palette[i], file);
    }
  }
  // finally, data
  // NOTE BPM thinks upside down. Can't just invert this loop.
  // So instead we have set image height negative above!
  for (int i=0; i<data_length_words; i++) {
    write_32bit_data(file, packed_img[i]);
  }
  fclose(file);
}
