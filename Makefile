LIBOBJECTS=bitmap.o draw.o font.o shared.o packed.o planar.o image.o tilemap.o

# Allows gprof profiling with GCC. En- / disable at wish.
# Run profiled program first and then view stats with 'make stats'.
# Probably only makes sense in combination with 'demo-none'.
#CFLAGS=-pg

all: demo-none demo-gtk spredit compose blit

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

%.c: %.h

display/display_gtk.o: display/display_gtk.c
	gcc $(CFLAGS) -c $^ -o $@ `pkg-config --cflags cairo gdk-3.0 gtk+-3.0`

libbitblt.a: $(LIBOBJECTS)
	ar -rcs $@ $^

demo-gtk: display/display_gtk.o shared.o main.o libbitblt.a
	gcc $(CFLAGS) -pthread $^ -o $@ `pkg-config --libs --cflags cairo gdk-3.0 gtk+-3.0`

demo-none: display/display_none.o shared.o main.o libbitblt.a
	gcc $(CFLAGS) -pthread $^ -o $@
stats:
	gprof demo-none gmon.out

spredit: display/display_gtk.o shared.o spredit.o libbitblt.a
	gcc -pthread $^ -o $@ `pkg-config --libs --cflags cairo gdk-3.0 gtk+-3.0`

compose: display/display_gtk.o shared.o compose.o libbitblt.a
	gcc -pthread $^ -o $@ `pkg-config --libs --cflags cairo gdk-3.0 gtk+-3.0`

blit: blit.c libbitblt.a
	gcc -pthread $^ -o $@

clean:
	rm -f *out.bmp *.a *.o display/*.o demo-gtk demo-none spredit compose blit
