
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "packed.h"
#include "bitmap.h"

/**
 * Catty paletty! (as well as some badly defined colors)
 */
uint8_t (*palette)[] = &(uint8_t[]) {
 0x00, 0x00, 0x00, // black (and / or transparent!)
 0xFF, 0xFF, 0xFF, // white

 0x00, 0xFF, 0x00,
 0x20, 0xAA, 0xCA, // hair
 0x89, 0x75, 0xFF, // ears

 0x00, 0x88, 0x00, // text
 0x00, 0x00, 0x88,
 0x61, 0x24, 0xFF, // mouth
 0x88, 0x88, 0x88, // grey

 0x88, 0x88, 0x00,
 0x00, 0x88, 0x88,
 0xCC, 0xCC, 0xCC, // eye white

 0x22, 0x22, 0x22, // eyes
 0x88, 0x88, 0x00,
 0x00, 0x88, 0x88,
 0x20, 0x10, 0x59 // mouth
};


char * print_usage(char * name) {
  printf("Usage: %s (image.bmp | new <x> <y> <depth>) image.bmp <from_x> <from_y> <to_y> <to_y> <at_x> <at_y> <transparent> out.bmp\n", name);
  exit(-1);
}

int main (int argc, char ** argv) {
  PackedImage * background;
  int current_arg = 1;
  if (argc-current_arg >= 4 && strcmp("new", argv[1]) == 0) {
    background = new_packed_image(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
    current_arg+=4;
  } else if (argc-current_arg >= 1) {
    background = read_bitmap(argv[current_arg++], &palette);
  } else {
    print_usage(argv[0]);
  }

  PackedImage * sprite;
  if (argc-current_arg >= 6) {
    sprite=read_bitmap(argv[current_arg++], &palette);
    coords from = (coords) {atoi(argv[current_arg++]), atoi(argv[current_arg++])};
    coords to = (coords) {atoi(argv[current_arg++]), atoi(argv[current_arg++])};
    coords at = (coords) {atoi(argv[current_arg++]), atoi(argv[current_arg++])};
    int transparent = atoi(argv[current_arg++]);
    printf("blitting\n");
    packed_bitblt(background, sprite, from, to, at, transparent);
  }

  if(argc-current_arg == 1) {
    printf("writing\n");
    write_bitmap(argv[current_arg++], palette, background->data, background->size.x, background->size.y, background->depth);
  } else {
    print_usage(argv[0]);
  }
}
