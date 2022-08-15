#include "display.h"

// Don't wait up for us, we're not even a real display.
bool display_finished;

void display_init(int argc, char ** argv, DisplayData * screen_data, UserEventCallback * event_cb) {
  screen_data->display_finished = true;
}

void display_runloop(pthread_t worker_thread) {
  pthread_join(worker_thread, NULL);
}

void display_redraw(area a) {}
