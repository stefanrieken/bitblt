#include <pthread.h>
#include <stdbool.h>
#include "../shared.h"
#include "../planar.h"
#include "../packed.h"

typedef struct display_data {
  planar_image * planar_display;
  packed_image * packed_display;
  uint8_t * palette;
  unsigned int scale; // goes on top of built-in scale
  bool packed;
  bool display_finished;
} display_data;

typedef void drawing_cb(coords from, coords to);

void display_init(int argc, char ** argv, display_data * screen_data, drawing_cb * draw_cb);
void display_runloop(pthread_t worker_thread);
void display_redraw();
