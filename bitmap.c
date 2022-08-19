
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "shared.h"

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

void write_bitmap(const char * filename, uint8_t (* palette)[], uint32_t * packed_img, int width, int height, int bpp) {

  uint32_t packed_width_aligned = image_aligned_width(width, bpp);

  uint32_t data_length_words = (packed_width_aligned * height * bpp) / 32;
  uint32_t data_length_bytes = (packed_width_aligned * height * bpp) / 8;

  uint32_t num_colors = 2;
  for (int i=1; i<bpp;i++) num_colors *= 2;

  FILE * file = fopen(filename, "wb");
  // main header
  fputc('B', file); fputc('M', file);
  write_32bit(file, 14 + 40 + (num_colors * 4) + (data_length_words*4)); // file size: h1 + h2 + size
  write_16bit(file, 0); write_16bit(file, 0);
  write_32bit(file, 14 + 40 + (num_colors * 4)); // start of pixel array
  // bitmap info header. Still understood by most readers
  write_32bit(file, 40); // sizeof header
  write_32bit(file, packed_width_aligned);
  write_32bit(file, height * -1); // To invert the image the right way up!
  write_16bit(file, 1); // # color planes, always 1
  write_16bit(file, bpp);

  // The below junk is extra wrt the old 'core' header
  write_32bit(file, 0); // no compression
  write_32bit(file, data_length_bytes);
  write_32bit(file, 2834); // pixels per meter?
  write_32bit(file, 2834); // pixels per meter?
  write_32bit(file, num_colors);
  write_32bit(file, 0);

  // Color table. The old header implies RGB instead of RGBA
  // N.b. we actually have to write BGR!
  // 1bpp, 4bpp, 16bpp are all common; but 2bpp may be rare / unsupported
  for (int i=0; i<num_colors;i++) {
    fputc((*palette)[i*3], file);
    fputc((*palette)[i*3+1], file);
    fputc((*palette)[i*3+2], file);
    fputc(0, file);
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
    printf("Expected value %d for %s; got %d\n", expected, what, got);
  }
}

// Returns the packed image; also allocates and fills the pallette
PackedImage * read_bitmap(const char * filename, uint8_t (** palette)[]) {

  FILE * file = fopen(filename, "rb");
  if (file == NULL) {
    printf("file not found\n");
    return NULL;
  }
  // main header
  expect("identifier", 'B', fgetc(file)); expect("identifier", 'M', fgetc(file));
  uint32_t file_size = read_32bit(file); // file size: h1 + h2 + size
  expect("reserved", 0, read_16bit(file)); expect("reserved", 0, read_16bit(file));
  uint32_t start_of_pixel_array = read_32bit(file); // start of pixel array

  // bitmap info header.
  expect("header size", 40, read_32bit(file)); // sizeof header
  uint32_t width = read_32bit(file);
  int32_t height = (int16_t) read_32bit(file); // To invert the image the right way up!
  expect("color planes", 1, read_16bit(file)); // # color planes, always 1
  uint32_t bpp = read_16bit(file);
  uint32_t num_colors = 2;
  for (int i=1; i<bpp;i++) num_colors *= 2;

  // further nonsense not required by the old bitmap core header, or us
  expect("compression", 0, read_32bit(file)); // no compression
  uint32_t data_length_bytes = read_32bit(file);
  uint32_t pixels_per_meter_x = read_32bit(file); // pixels per meter?
  uint32_t pixels_per_meter_y = read_32bit(file); // pixels per meter?
  expect("num colors", num_colors, read_32bit(file)); // should follow from bpp
  uint32_t num_important_colors = read_32bit(file); // uhh

  // Color table. The old header implies RGB instead of RGBA
  // N.b. we actually have to read BGR!
  // 1bpp, 4bpp, 8bpp are all common; but 2bpp may be rare / unsupported
  *palette = malloc(sizeof(uint8_t) * 3 * num_colors);

  for (int i=0; i<num_colors;i++) {
    (**palette)[i*3] = fgetc(file);
    (**palette)[i*3+1] = fgetc(file);
    (**palette)[i*3+2] = fgetc(file);
    fgetc(file); // alpha / filler
  }

  // alignment
  for (int i=0; i<start_of_pixel_array - 14-40 - 4*num_colors; i++) {
    fgetc(file);
  }
  uint32_t packed_width_aligned = image_aligned_width(width, bpp);
  int ppw = 32/bpp;
  uint32_t data_length_words = (packed_width_aligned * height) / ppw;

  // finally, data
  uint32_t * data = malloc(sizeof(uint32_t) * data_length_words);

  bool invert = true;
  if (height < 0) {
    invert = false;
    height = -height;
  }

  for (int i=0; i<height; i++) {
    // image will appear 'upside down' unless we do this:
    int y = invert ?  height-i-1 : i;
    for (int j=0; j<packed_width_aligned; j += ppw) {
      data[(y*packed_width_aligned+j)/ppw] = read_32bit_data(file);
    }
  }

  PackedImage * result = malloc(sizeof(PackedImage));
  result->depth = bpp;
  result->size.x = width;
  result->size.y = height;
  result->data = data;


  fclose(file);

  return result;
}
