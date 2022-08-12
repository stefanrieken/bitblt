
#include <stdbool.h>

/**
 * Drawing primitives, presently targeting 1bpp single layer images.
 */

/**
 * Setup a drawing state.
 */
void configure_draw(int bpp);

// As long as the draw functions are not generalized to also work with packed,
// 'value' is either one or zero. Other values may be trippy.
void draw_line (WORD_T * data, coords size, coords from, coords to, uint32_t value);
void draw_rect (WORD_T * data, coords size, coords from, coords to, bool fill, uint32_t value);
void draw_circle (WORD_T * data, coords size, coords from, coords to, bool arc, bool fill, uint32_t value);
