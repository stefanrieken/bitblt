#include <stdbool.h>

/**
 * TODO might want to make this agnostic from planar vs packed, as it is 1-bit anyway.
 * In that case either re-use any struct we use for packed data, or add some kind of &size_pointer
 * to return rendered dimensions.
 */

planar_image * render_text (char * data, int ny, bool fixedWidth);
void render_char(planar_image * image, char ch, int ny, bool fixedWidth);
