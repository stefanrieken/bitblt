#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "planar.h"
#include "font.h"

uint32_t chars_encoded[] = {
	// special characters
	0x40000000, 0x48421004, 0x14aa0000, 0x00afabea, 0x08ea382e, 0x01111111, 0x1908364d, 0x44220000, 0x04442082, 0x10410888, 0x01157d51, 0x00427c84, 0xc0001088, 0x40003800, 0x4000018c, 0x00111110,
	// numeric
	0x1d3ae62e, 0x48c2108e, 0x1d11111f, 0x3c11062e, 0x04654be2, 0x3f0f043e, 0x1d0f462e, 0x3e111110, 0x1d17462e, 0x1d18bc2e, 0x40401000, 0x40401100, 0x00444104, 0x000f83e0, 0x00820888, 0x1d111004,
	// uppercase
	0x00eada0f, 0x08a8fe31, 0x3d1f463e, 0x1d18422e, 0x3928c65c, 0x3f08721f, 0x3f087210, 0x1d184e2e, 0x231fc631, 0x5c42108e, 0x0210862e, 0x232e4a31, 0x2108421f, 0x23bac631, 0x239ad671, 0x1d18c62e, 0x3d18fa10, 0x1d18d66f, 0x3d18fa51, 0x1d160a2e, 0x3e421084, 0x2318c62e, 0x2318c544, 0x2318d771, 0x22a22a31, 0x23151084, 0x3e22221f, 0x1c84210e, 0x01041041, 0x1c21084e, 0x08a88000, 0x0000001f,
	// lowercase
	0x50820000, 0x00f8c66d, 0x21e8c63e, 0x00e8c22e, 0x02f8c62f, 0x00e8fe0e, 0x1d187210, 0x9f18bc3e, 0x216cc631, 0x48021084, 0xc401084c, 0x21097251, 0x50842106, 0x01aac631, 0x016cc631, 0x00e8c62e, 0xbd18fa10, 0x9f18bc21, 0x01e8c210, 0x00e8383e, 0x11e42126, 0x0118c62e, 0x0118c544, 0x0118d7ea, 0x01151151, 0xa318bc2e, 0x01f1111f, 0x8e4c1087, 0x48421084, 0xb841909c, 0x1b600000, 0x2aaaaaaa

};

image * render (char * data, int ny, bool fixedWidth) {
	int width = strlen(data);
	while ((width %4) != 0) width++;

	image * txt = malloc(sizeof(image));
	txt->size.x = width * 8; // more than enough
	txt->size.y = 12*ny;  // 6-7 + drop
	txt->data = calloc(1, (txt->size.x/4) * txt->size.y);

	int offset = 1;
	for (int i=0; i < strlen(data); i++) {

		image * ch = to_image(data[i], ny, fixedWidth);
		coords at = {offset,0};
		planar_bitblt(&txt, &ch, at, 1);
		free(ch);

		uint32_t encoded = chars_encoded[(uint32_t) (data[i] - 32)];
		if (((encoded >> 30) & 1) && !fixedWidth) offset += 4;
		else offset += 6;
	}
	return txt;
}

image * to_image(char ch, int ny, bool fixedWidth) {
	image * image = malloc(sizeof(image));
	image->size.x = 32; // because alignment
	image->size.y = 12*ny;  // 6-7 + drop
	image->data = calloc(1, (image->size.x / 4) * image->size.y);

	if (ch < 32) return image; // control characters; return empty image

	ch -= 32;
	unsigned int encoded = chars_encoded[(unsigned int) ch];

	bool narrow = false;
	unsigned int height = 6;
	bool descender = false;
	int drop = 0;

	if ((encoded >> 31) & 1) { descender = true; drop = ny; }
	if (((encoded >> 30) & 1) && !fixedWidth) narrow = true;

	encoded = encoded << 2;

	for (int i=0; i<height*ny; i++) {
		// for characters with a descender, also repeat certain lines
		if (descender && i >= 2*ny && i < 2*ny+1) {
			unsigned int line = encoded & (0b11111 << (32-5));
			encoded = (encoded >> 5) | line;
			height += 1;
		}
/*		if (descender && i >= 5*ny && i < 6*ny) {
			unsigned int line = encoded & (0b11111 << (32-5));
			encoded = (encoded >> 5) | line;
			height += 1;
		}
*/
		if (narrow) {
			if ((i+drop) % ny == 0)
				encoded = encoded << 1;
			image->data[i+drop+1] = encoded & (0b111 << (32-3));
			if ((i+drop+1) % ny == 0)
				encoded = encoded << 4;
		} else {
			image->data[i+drop+1] = encoded & (0b11111 << (32-5));
			if ((i+drop+1) % ny == 0)
				encoded = encoded << 5;
		}
	}

	return image;
}
