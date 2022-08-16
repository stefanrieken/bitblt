#include "packed.h"

void write_bitmap(const char * filename, uint8_t * palette, uint32_t * packed_img, int width, int height, int bpp);

// Returns the packed image; also allocates and fills the pallette
PackedImage * read_bitmap(const char * filename, uint8_t ** palette);

// Read slightly less arcane variant (NT+)
PackedImage * read_bitmap_nt(const char * filename, uint8_t ** palette);
