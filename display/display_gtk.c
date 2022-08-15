#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>

#include "display.h"

#define FIXED_SCALE 4

static DisplayData * _screen_data;
static uint8_t * _palette;
static int _depth;
static unsigned int _scale;

static UserEventCallback * _event_cb;

GtkWidget * drawing_area;
static cairo_surface_t * surface;
static cairo_pattern_t * pattern;

bool display_finished = false;

void draw_planar_on_surface(cairo_surface_t * surface, int x, int y, int width, int height) {
  uint8_t * pixels = cairo_image_surface_get_data(surface);
  int rowstride = cairo_image_surface_get_stride(surface);

  uint32_t screen_data_width_aligned = image_aligned_width(_screen_data->planar_display->size.x, 1);

  for (int i=y;i<y+height;i++) {
    for(int j=x; j<x+width;j++) {
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

void draw_packed_on_surface(cairo_surface_t * surface, int x, int y, int width, int height) {
  uint8_t * pixels = cairo_image_surface_get_data(surface);
  int rowstride = cairo_image_surface_get_stride(surface);

  uint32_t pixels_per_word = WORD_SIZE / _screen_data->packed_display->depth;

  uint32_t screen_data_width_aligned = image_aligned_width(_screen_data->packed_display->size.x, _screen_data->packed_display->depth);

  for (int i=y;i<y+height;i++) {
    for(int j=x; j<x+width;j++) {
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

void draw_on_surface(cairo_surface_t * surface, int x, int y, int width, int height) {
  if (_screen_data->packed) draw_packed_on_surface(surface, x, y, width, height);
  else draw_planar_on_surface(surface, x, y, width, height);
  cairo_surface_mark_dirty_rectangle(surface, x*_scale, y*_scale, width*_scale, height*_scale);
}

void configure_cb(GtkWidget * widget, GdkEventConfigure * event, gpointer data) {
  GdkWindow * window = gtk_widget_get_window(widget);
  surface = cairo_image_surface_create(
    CAIRO_FORMAT_RGB24,
    gtk_widget_get_allocated_width (widget),
    gtk_widget_get_allocated_height (widget)
  );

  pattern = cairo_pattern_create_for_surface(surface);
  cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
}

void draw_screen_cb(GtkWidget *widget, cairo_t * cr, gpointer userdata) {

  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_scale(cr, _scale, _scale);
  cairo_set_source(cr, pattern);

  cairo_paint(cr);
}

void delete_cb(GtkWidget *widget, GdkEventType *event, gpointer userdata) {
  // destroy any global drawing state here,
  // like the cairo surface (if we make it global)
  cairo_surface_destroy(surface);
  gtk_main_quit();
}

void pressed_cb(GtkWidget * widget, GdkEventButton * event, gpointer userdata) {
  coords where;
  where.x = event->x / (_scale);
  where.y = event->y / (_scale);
  _event_cb(POINTER_DOWN, where);
}

void released_cb(GtkWidget * widget, GdkEventButton * event, gpointer userdata) {
  coords where;
  where.x = event->x / (_scale);
  where.y = event->y / (_scale);
  _event_cb(POINTER_UP, where);
}

void motion_cb(GtkWidget * widget, GdkEventButton * event, gpointer userdata) {
  coords where;
  where.x = event->x / (_scale);
  where.y = event->y / (_scale);
  _event_cb(POINTER_MOVE, where);
}

void display_init(int argc, char ** argv, DisplayData * screen_data, UserEventCallback * event_cb) {
  _screen_data = screen_data;
  _palette = _screen_data->palette;
  _depth = _screen_data->planar_display->depth;
  _scale = FIXED_SCALE * _screen_data->scale;

  gtk_init (&argc, &argv);

  GtkWindow * window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  gtk_window_set_title(window, "bitblt on Cairo");
  gtk_window_set_default_size(window, screen_data->planar_display->size.x*_scale, screen_data->planar_display->size.y*_scale);
  gtk_window_set_resizable(window, FALSE);

  g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(delete_cb), NULL);

  drawing_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(drawing_area, screen_data->planar_display->size.x*_scale, screen_data->planar_display->size.y*_scale);
  gtk_container_add(GTK_CONTAINER(window), drawing_area);
  g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(draw_screen_cb), NULL);
  g_signal_connect(G_OBJECT(drawing_area),"configure-event", G_CALLBACK (configure_cb), NULL);

  _event_cb = event_cb;

  gtk_widget_set_events(GTK_WIDGET(drawing_area),
    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);

  g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(pressed_cb), NULL);
  g_signal_connect(drawing_area, "button-release-event", G_CALLBACK(released_cb), NULL);
  g_signal_connect (G_OBJECT (drawing_area), "motion-notify-event",G_CALLBACK (motion_cb), NULL);

  gtk_widget_show_all(GTK_WIDGET(window));
}

void display_runloop(pthread_t worker_thread) {
  gtk_main();

  _screen_data->display_finished = true;

  pthread_join(worker_thread, NULL);
}

void display_redraw(area a) {
  coords size = (coords) {(a.to.x-a.from.x), (a.to.y-a.from.y)};

  // update cairo's buffer with ours
  draw_on_surface(surface, a.from.x, a.from.y, size.x, size.y);

  // then queue a gtk update from cairo's buffer
    gtk_widget_queue_draw_area(
      drawing_area,
      a.from.x*_scale,
      a.from.y*_scale,
      size.x*_scale,
      size.y*_scale
    );
}
