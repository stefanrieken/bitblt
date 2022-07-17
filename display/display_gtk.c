#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>

#include "display.h"

#define SCALE 4

static planar_image * _screen_data;
static uint8_t * _palette;
static uint32_t _depth;

GtkWidget * drawing_area;
static cairo_surface_t * surface;
static cairo_pattern_t * pattern;

void draw_on_surface(cairo_surface_t * surface) {
  uint8_t * pixels = cairo_image_surface_get_data(surface);
  int rowstride = cairo_image_surface_get_stride(surface);
  int width = cairo_image_surface_get_width(surface);

  for (int i=0;i<_screen_data->height;i++) {
    for(int j=0; j<_screen_data->width;j++) {
      // gather indexed pixel data; find palette color
      uint32_t idx = (i * _screen_data->width)+j;
      uint8_t * pal = &_palette[gather_pixel(_screen_data, idx)];
      // and copy to pixbuf
      uint8_t * pixel = &pixels[(i*rowstride) + (j*4)];
      pixel[0] = pal[0];
      pixel[1] = pal[1];
      pixel[2] = pal[2];
    }
  }
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

void draw_cb(GtkWidget *widget, cairo_t * cr, gpointer userdata) {
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

void display_runloop(int argc, char ** argv, pthread_t worker_thread, planar_image * screen_data, uint8_t * palette, uint32_t depth) {
  _screen_data = screen_data;
  _palette = palette;
  _depth = depth;

  gtk_init (&argc, &argv);

  GtkWindow * window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  gtk_window_set_title(window, "pixbuf");
  gtk_window_set_default_size(window, screen_data->width*SCALE, screen_data->height*SCALE);
  gtk_window_set_resizable(window, FALSE);

  g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(delete_cb), NULL);

  drawing_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(drawing_area, 320,200);
  gtk_container_add(GTK_CONTAINER(window), drawing_area);
  g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(draw_cb), NULL);
  g_signal_connect(drawing_area,"configure-event", G_CALLBACK (configure_cb), NULL);

  gtk_widget_show_all(GTK_WIDGET(window));
  gtk_main();

  pthread_join(worker_thread, NULL);
}

void display_redraw() {
  g_idle_add((GSourceFunc)gtk_widget_queue_draw,(void*)drawing_area);
}
