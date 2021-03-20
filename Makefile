all:
	gcc -pthread *.c display/display_none.c -o bitblt

gtk:
	gcc *.c display/display_gtk.c -o bitblt `pkg-config --libs --cflags cairo gdk-3.0 gtk+-3.0`

clean:
	rm -f *.bmp bitblt
