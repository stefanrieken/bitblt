LIBOBJECTS=bitmap.o draw.o font.o packed.o planar.o

all: demo-none demo-gtk spredit compose

%.o: %.c
	gcc -c $< -o $@

%.c: %.h

display/display_gtk.o: display/display_gtk.c
	gcc -c $^ -o $@ `pkg-config --cflags cairo gdk-3.0 gtk+-3.0`

libbitblt.a: $(LIBOBJECTS)
	ar -rcs $@ $^

demo-gtk: display/display_gtk.o main.o libbitblt.a
	gcc -pthread $^ -o $@ `pkg-config --libs --cflags cairo gdk-3.0 gtk+-3.0`

demo-none: display/display_none.o main.o libbitblt.a
	gcc -pthread $^ -o $@

spredit: display/display_gtk.o spredit.o libbitblt.a
	gcc -pthread $^ -o $@ `pkg-config --libs --cflags cairo gdk-3.0 gtk+-3.0`

compose: display/display_gtk.o compose.o libbitblt.a
	gcc -pthread $^ -o $@ `pkg-config --libs --cflags cairo gdk-3.0 gtk+-3.0`

clean:
	rm -f *.bmp *.a *.o display/*.o demo-gtk demo-none
