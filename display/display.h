#include <pthread.h>
#include <stdbool.h>
#include "../planar.h"

extern bool display_finished;
void display_init(int argc, char ** argv, planar_image * screen_data, uint8_t * palette, uint32_t depth);
void display_runloop(pthread_t worker_thread);
void display_redraw();
