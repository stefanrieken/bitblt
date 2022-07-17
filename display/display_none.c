#include "display.h"

void display_init(int argc, char ** argv, planar_image * screen_data, uint8_t * palette, uint32_t depth) {
}

void display_runloop(pthread_t worker_thread) {
  pthread_join(worker_thread, NULL);
}
void display_redraw() {}
