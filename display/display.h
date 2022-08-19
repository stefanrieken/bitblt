#ifndef BITBLT_DISPLAY_H
#define BITBLT_DISPLAY_H

#include <pthread.h>
#include <stdbool.h>
#include "../image.h"
#include "../planar.h"
#include "../packed.h"

/**
 * Simple struct to allow both library user and display implementation
 * to keep track of all differnt parts of the display state.
 */
typedef struct DisplayData {
  PlanarImage * display;
  uint8_t (* palette)[];
  unsigned int scale; // goes on top of built-in scale
  bool packed;
  bool display_finished;
} DisplayData;

typedef enum UserEvent {
  POINTER_DOWN,
  POINTER_UP,
  POINTER_MOVE
} UserEvent;

typedef void UserEventCallback(UserEvent event, coords where);

void display_init(int argc, char ** argv, DisplayData * screen_data, UserEventCallback * event_cb);
void display_runloop(pthread_t worker_thread);
void display_redraw(area a);

#endif /* BITBLT_DISPLAY_H */
