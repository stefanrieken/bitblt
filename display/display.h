#include <pthread.h>
#include <stdbool.h>
#include "../planar.h"
#include "../packed.h"

typedef struct display_data {
  planar_image * planar_display;
  packed_image * packed_display;
  uint8_t * palette;
  bool packed;
  bool display_finished;
} display_data;

void display_init(int argc, char ** argv, display_data * screen_data);
void display_runloop(pthread_t worker_thread);
void display_redraw();
