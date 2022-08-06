
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "bitmap.h"

// Little endian
static void write_32bit(FILE * file, uint32_t data) {
  fputc(data & 0xFF, file); fputc((data >> 8) & 0xFF, file);
  fputc((data >> 16) & 0xFF, file); fputc((data >> 24) & 0xFF, file);
}

// Big-endian / character stream data
static void write_32bit_data(FILE * file, uint32_t data) {
  fputc((data >> 24) & 0xFF, file); fputc((data >> 16) & 0xFF, file);
  fputc((data >> 8) & 0xFF, file); fputc(data & 0xFF, file);
}

// Little endian
static void write_16bit(FILE * file, uint32_t data) {
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
  for (int i=0; i<3*num_colors;i++) {
    fputc(palette[i], file);
  }
  // finally, data
  // NOTE BPM thinks upside down. Can't just invert this loop.
  // So instead we have set image height negative above!
  for (int i=0; i<data_length_words; i++) {
    write_32bit_data(file, packed_img[i]);
  }
  fclose(file);
}

///
//
// BMP Read
//
///


// Little endian
uint16_t read_16bit(FILE * file) {
  uint16_t result = 0;
  result |= fgetc(file);
  result |= fgetc(file) << 8;
  return result;
}

// Little endian
static uint32_t read_32bit(FILE * file) {
  uint32_t result = 0;
  result |= fgetc(file);
  result |= fgetc(file) << 8;
  result |= fgetc(file) << 16;
  result |= fgetc(file) << 24;
  return result;
}

// Big-endian / character stream data;
// but we read in chunks of 4 bytes
uint32_t read_32bit_data(FILE * file) {
  uint32_t result = 0;
  result |= fgetc(file) << 24;
  result |= fgetc(file) << 16;
  result |= fgetc(file) << 8;
  result |= fgetc(file);
  return result;
}

static void expect(char * what, int expected, int got) {
  if (expected != got) {
    printf("Expected value %c for %s; got %c\n", expected, what, got);
  }
}

// Returns the packed image; also allocates and fills the pallette
packed_image * read_bitmap(const char * filename, uint8_t ** palette) {

  FILE * file = fopen(filename, "rb");
  // main header
  expect("identifier", 'B', fgetc(file)); expect("identifier", 'M', fgetc(file));
  uint32_t file_size = read_32bit(file); // file size: h1 + h2 + size
  expect("reserved", 0, read_16bit(file)); expect("reserved", 0, read_16bit(file));
  uint32_t start_of_pixel_array = read_32bit(file); // start of pixel array

  // bitmap core header. Trying to get away with the oldest, simplest variant
  expect("header size", 12, read_32bit(file)); // sizeof header
  uint32_t width = read_16bit(file);
  int16_t height = (int16_t) read_16bit(file); // To invert the image the right way up!
  expect("color planes", 1, read_16bit(file)); // # color planes, always 1
  uint32_t bpp = read_16bit(file);
  uint32_t num_colors = 2;
  for (int i=1; i<bpp;i++) num_colors *= 2;

  // Alternatively, extend to bitmap info header (and set x and y to 32 bit)
  // read_32bit(file, 0); // no compression
  // read_32bit(file, data_length_bytes);
  // read_32bit(file, 2834); // pixels per meter?
  // read_32bit(file, 2834); // pixels per meter?
  // read_32bit(file, 0);
  // read_32bit(file, 0);

  // Color table. The old header implies RGB instead of RGBA
  // N.b. we actually have to read BGR!
  // 1bpp, 4bpp, 8bpp are all common; but 2bpp may be rare / unsupported
  *palette = malloc(sizeof(uint8_t) * 3 * num_colors);

  for (int i=0; i<3*num_colors;i++) {

    (*palette)[i] = fgetc(file);
  }
  // finally, data
  // NOTE BPM thinks upside down. Can't just invert this loop.
  // So instead we have set image height negative above!
  if (height > 0) {
    printf("Image height was not expressed negative (like we save it); image may be upside down!\n");
  } else {
    height = -height;
  }

  uint32_t packed_width_aligned = packed_aligned_width(width, bpp);
  uint32_t data_length_words = (packed_width_aligned * height * bpp) / 32;

  packed_image * result = malloc(sizeof(packed_image));
  result->depth = bpp;
  result->size.x = width;
  result->size.y = height;
  result->data = malloc(sizeof(WORD_T) * data_length_words);

  for (int i=0; i<data_length_words; i++) {
    result->data[i] = read_32bit_data(file);
  }

  fclose(file);
  
  return result;
}

