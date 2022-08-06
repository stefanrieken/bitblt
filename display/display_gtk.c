#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>

#include "display.h"

#define SCALE 4

static display_data * _screen_data;
static uint8_t * _palette;
static int _depth;

static drawing_cb * _draw_cb;

GtkWidget * drawing_area;
static cairo_surface_t * surface;
static cairo_pattern_t * pattern;

bool display_finished = false;

void draw_planar_on_surface(cairo_surface_t * surface) {
  uint8_t * pixels = cairo_image_surface_get_data(surface);
  int rowstride = cairo_image_surface_get_stride(surface);
  int width = cairo_image_surface_get_width(surface);

 uint32_t screen_data_width_aligned = planar_aligned_width(_screen_data->planar_display->size.x);

  for (int i=0;i<_screen_data->planar_display->size.y;i++) {
    for(int j=0; j<_screen_data->planar_display->size.x;j++) {
      // gather indexed pixel data; find palette color
      uint32_t idx = (i * screen_data_width_aligned)+j;
      uint8_t * pal = &_palette[3*gather_pixel(_screen_data->planar_display, idx)];
      // and copy to pixbuf
      uint8_t * pixel = &pixels[(i*rowstride) + (j*4)];
      pixel[0] = pal[0];
      pixel[1] = pal[1];
      pixel[2] = pal[2];
    }
  }
}

void draw_packed_on_surface(cairo_surface_t * surface) {
  uint8_t * pixels = cairo_image_surface_get_data(surface);
  int rowstride = cairo_image_surface_get_stride(surface);
  int width = cairo_image_surface_get_width(surface);

  uint32_t pixels_per_word = WORD_SIZE / _screen_data->packed_display->depth;

  uint32_t screen_data_width_aligned = packed_aligned_width(_screen_data->packed_display->size.x, _screen_data->packed_display->depth);

  for (int i=0;i<_screen_data->packed_display->size.y;i++) {
    for(int j=0; j<_screen_data->packed_display->size.x;j++) {
      // gather indexed pixel data; find palette color
      uint32_t pixel_idx = (i * screen_data_width_aligned)+j;
      uint32_t pixel_word_idx = pixel_idx / pixels_per_word;
      uint32_t shift = (pixel_idx % pixels_per_word) * _depth;

      uint8_t px = (_screen_data->packed_display->data[pixel_word_idx] >> ((WORD_SIZE-_depth) - shift)) & 0b1111; // TODO that's a fixed 4-bit mask
      uint8_t * pal = &_palette[3*px];
      // and copy to pixbuf
      uint8_t * pixel = &pixels[(i*rowstride) + (j*4)];
      pixel[0] = pal[0];
      pixel[1] = pal[1];
      pixel[2] = pal[2];
    }
  }
}

void draw_on_surface(cairo_surface_t * surface) {
  if (_screen_data->packed) draw_packed_on_surface(surface);
  else draw_planar_on_surface(surface);

  cairo_surface_mark_dirty(surface);
}

void configure_cb(GtkWidget * widget, GdkEventConfigure * event, gpointer data) {
  // The surface is 'patterned off' the Gdk Window,
  // but apart from thatis just an off-display buffer.
  // So we can draw it just once here (pending data changes)
  GdkWindow * window = gtk_widget_get_window(widget);
  surface = gdk_window_create_similar_surface(window,
        CAIRO_CONTENT_COLOR,
        gtk_widget_get_allocated_width (widget),
        gtk_widget_get_allocated_height (widget));

  // The derived 'pattern' object may also be set up once
  pattern = cairo_pattern_create_for_surface(surface);
  cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
}

void draw_screen_cb(GtkWidget *widget, cairo_t * cr, gpointer userdata) {
  if (!(cairo_in_clip(cr, 0,0) || cairo_in_clip(cr, 31, 7))) return; // not (presently) drawing anything outside this area

  draw_on_surface(surface);

  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_scale(cr, SCALE, SCALE);
  cairo_set_source(cr, pattern);
  cairo_paint(cr);
}

void delete_cb(GtkWidget *widget, GdkEventType *event, gpointer userdata) {
  // destroy any global drawing state here,
  // like the cairo surface (if we make it global)
  cairo_surface_destroy(surface);
  gtk_main_quit();
}

static coords previous;
void clicked_cb(GtkWidget * widget, GdkEventButton * event, gpointer userdata) {
  previous.x = event->x / SCALE;
  previous.y = event->y / SCALE;
  _draw_cb(previous, previous);
}

void released_cb(GtkWidget * widget, GdkEventButton * event, gpointer userdata) {
  // we might want to dynamically (de)activate GDK_POINTER_MOTION_MASK as well
  previous.x=-1;
  previous.y=-1;
}

void move_cb(GtkWidget * widget, GdkEventButton * event, gpointer userdata) {
  if (previous.x != -1) {
    _draw_cb(previous, (coords) {event->x / SCALE, event->y / SCALE});

    previous.x = event->x / SCALE;
    previous.y = event->y / SCALE;
  }
}

void display_init(int argc, char ** argv, display_data * screen_data, drawing_cb * draw_cb) {
  _screen_data = screen_data;
  _palette = _screen_data->palette;
  _depth = _screen_data->planar_display->depth;

  gtk_init (&argc, &argv);

  GtkWindow * window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  gtk_window_set_title(window, "pixbuf");
  gtk_window_set_default_size(window, screen_data->planar_display->size.x*SCALE, screen_data->planar_display->size.y*SCALE);
  gtk_window_set_resizable(window, FALSE);

  g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(delete_cb), NULL);

  drawing_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(drawing_area, 320,200);
  gtk_container_add(GTK_CONTAINER(window), drawing_area);
  g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(draw_screen_cb), NULL);
  g_signal_connect(drawing_area,"configure-event", G_CALLBACK (configure_cb), NULL);

  _draw_cb = draw_cb;

  gtk_widget_set_events(GTK_WIDGET(drawing_area),
    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);

  previous.x=-1;
  previous.y=-1;

  g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(clicked_cb), NULL);
  g_signal_connect(drawing_area, "button-release-event", G_CALLBACK(released_cb), NULL);
  g_signal_connect (G_OBJECT (drawing_area), "motion-notify-event",G_CALLBACK (move_cb), NULL);

  gtk_widget_show_all(GTK_WIDGET(window));
}

void display_runloop(pthread_t worker_thread) {
  gtk_main();
  
  _screen_data->display_finished = true;

  pthread_join(worker_thread, NULL);
}

void display_redraw() {
  if (drawing_area != NULL)
    g_idle_add((GSourceFunc)gtk_widget_queue_draw,(void*)drawing_area);
}
