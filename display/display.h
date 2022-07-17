#include <pthread.h>
#include "../planar.h"

void display_runloop(int argc, char ** argv, pthread_t worker_thread, planar_image * screen_data, uint8_t * palette, uint32_t depth);
void display_redraw();
