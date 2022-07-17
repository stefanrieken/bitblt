#include "display.h"

void display_runloop(int argc, char ** argv, pthread_t worker_thread, planar_image * screen_data, uint8_t * palette, uint32_t depth) {
  pthread_join(worker_thread, NULL);
}
void display_redraw() {}
