#include <stdbool.h>

/**
 * TODO might want to make this agnostic from planar vs packed, as it is 1-bit anyway.
 * In that case either re-use any struct we use for packed data, or add some kind of &size_pointer
 * to return rendered dimensions.
 */

/**
 * data: the text to render
 * ny: scale n times in the y direction
 * fixedWidth: character width is fixed (to 6); otherwise it is either 6 or 4
 * fixedHeight: do not stretch descenders, fixing height to 6 (otherwise it is 6 or 8)
 */
planar_image * render_text (char * data, int ny, bool fixedWidth, bool fixedHeight);
void render_char(planar_image * image, char ch, int ny, bool fixedWidth, bool fixedHeight);
