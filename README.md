# Bitblt

This is a growing repository of indexed-color functions and demos.

By typing 'make', it should compile on Linux, and Mac if GTK+3 libraries can be found.

## Executables
- demo-none: a headless version of demo-gtk. Outputs some BMP's.
- demo-gtk: a more interactive demo with a screen
- spredit: a sprite / tile / bitmap editor

## Word size

Low-resolution, indexed-color graphics are often referred to as '8-bit images'.
Nevertheless, this library mainly processes 32-bit words. This should be efficient
on 32-bit computers (including typical ARM microcontrollers) and not too inefficient
on 64-bit computers (although some might simply allocate 32-bit integers as 64-bit
ones, allocating double the required memory). There is some provisional provision
for compiling this library for different word sizes, but this will not presently work.

## Display modes

### Packed
- Bits of a pixel are grouped together
- E.g. 4 bpp (bits per pixel) = 8 pixels per 32-bit word

#### Advantages
- Pixel (or even pixels) can be read / written in a single operation
#### Disadvantages
- Bit shifts and masks are calculated differently for different image depths
- Will not likely choose e.g. 3bpp due to alignment constraints
- Cannot quickly juggle color data

### Planar
- Bits of a pixel are spread over multiple 'planes'
- E.g. 4 bpp = 4 planes of 1 pixel per bit

#### Advantages
- Bit shifts and masks are the same for each plane (but we may have to clear one bit and set another to get the right color)
- Can use e.g. 3 bpp and / or mix different depths (economical)
- Can easily add or remove a plane: e.g. add grayscale depth, edit an image in an editor that adds its own colors, or recolor a sprite
#### Disadvantages
- (E.g. to retrieve color from palette) combined pixel data must be gathered from multiple, far-spread words,
  which tends to be slow (funny bit: this parallel retrieval was used as a speed advantage in early computing,
  storing each plane on a separate RAM module)

### Supporting both
...is insane from a historical perspective. But there's actually very little need not to:
- The small amount of code specific to each mode can be left out by the compiler if not used
- Although the packed data form is a natural intermediate e.g. for writing to file;
- And the planar form can be fun to play with
- Even on a microcontroller we aren't likely limited by a graphics chipset capabilities

## Drawing primitives

There are primitives for drawing lines (points), rectangles and circles.

Presently, the drawing primitives operate on planar data, and lack erasure capabilities.
This could be fixed as follows:
- Keep draw functions independent of packed / planar structs (done)
- Externalize calculation of alignment / bpp
- Add pixel mask parameter (clears these n bits; 1 for planar, or 'depth' bits for packed)
- Add pixel data parameter (may be 1 bit for planar, or 'depth' bits for packed)


## Performance

Although in one way, this library is a study of '8-bit' graphical algorithms, in present times
we can easily get away with some quick and dirty implementations left and right. Nevertheless,
it is both interesting and useful to adhere to a number of decent 'old timey' principles, such as:

### Avoid floating point calculations [v]
Even though floats in general need not always be magic and evil (e.g. C64 basic used mainly
floating point numbers, which were reportedly faster than its integer routines), generally
speaking it is best avoided, even more so on microcontrollers that lack an FPU.

### Avoid repeated re-calculations and conditions [x]
This is of course especially true for loops repeated e.g. for each pixel.
We might expect e.g. a calculated value in a loop condition to be optimized to a constant by the
compiler; but then again it may be useful sometimes to make this explicit.

The following point is an even more urgent version of this one:

### Avoid multiplications and divisions [x]
On one hand, this library (now) uses Bresenham's algorithm for line drawing; but on the other
hand, even within this function, the actual pixel adressing is done by repeated multiplication.
This is even true in more sequential functions such as bitblt itself.

It would be better to in-/decrement an index counter by the right amount each time y or x changes,
and then have it ready for use when needed.

Mind, I do fully expect the compiler to optimize multiplications into bit-shifts where possible,
up to the point where I will retain their 'un-optimized' form where this is easier / more readable.

### Don't over-generalize [v]
Especially the bitblt functions themselves are highly parameterized: they can copy part of
an image, transparent or not, and in the future perhaps even scaled. Yet they can do most of
these things without adding too many on-the-fly calculations or repeated if-statements.
If this was not the case, it might have been better to provide a few flavours of the same method.
Of course, this is ultimately a matter of code size vs. performance.

### Smart screen updates [x]
Tracking, or even reparing dirty screen areas actually requires an appropiate model of te screen's
contents - an understanding of what the screen is trying to show us. While it may be interesting to
find out if we can provide a generic model that works for all applications (games, window systems,
etc.), the present library offers no such high-level functionality; but the end user should
certainly provide their own.

Assuming they did, this library should support a parameterized 'display_redraw' instead of always
translating the entire indexed display buffer as one. This behaviour will certainly find support
in microcontroller display drivers.
