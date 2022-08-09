# Bitblt

This is a growing repository of indexed-color functions and demos.

By typing 'make', it should compile on Linux, and Mac if GTK+3 libraries can be found.

## Executables
- demo-none: a headless version of demo-gtk. Outputs some BMP's.
- demo-gtk: a more interactive demo with a screen
- spredit: a sprite / tile / bitmap editor
- compose: implementation and demo of screen objects

## Word size

Low-resolution, indexed-color graphics are often referred to as '8-bit images'.
Nevertheless, this library mainly processes 32-bit words. This should be efficient
on 32-bit computers (including typical ARM microcontrollers) and not too inefficient
on 64-bit computers (although some might simply allocate 32-bit integers as 64-bit
ones, bloating up image memory with air pockets). There is some provisional provision
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
- Can only juggle color data through palette manipulations

### Planar
- Bits of a pixel are spread over multiple 'planes'
- E.g. 4 bpp = 4 planes of 1 pixel per bit

#### Advantages
- Bit shifts and masks are the same for each plane
- Can use e.g. 3 bpp and / or mix different depths (economical)
- Can easily add or remove a plane: e.g. add grayscale depth, edit an image in an editor that adds its own colors, or recolor a sprite
#### Disadvantages
- Combined pixel data must be gathered from multiple, far-spread words, which tends to be slow.
  (Funny bit: this parallel retrieval was used as a speed advantage in early computing,
  where each plane was stored on a separate RAM module)

### Supporting both
...is insane from a historical perspective. But there's actually very little need not to:
- The small amount of code specific to each mode can be left out by the compiler if not used
- The packed data form is a natural intermediate e.g. for writing to file
- The planar form is also interesting to explore
- Even on a microcontroller we aren't likely limited by a graphics chipset's capabilities


## Drawing primitives

There are primitives for drawing lines (points), rectangles and circles.

Presently, the drawing primitives operate on planar (or 1bpp 'packed') data, requiring
repeated calls for each field (both setting and clearing bits). Re-writing them to support
both might just be possible, but it is a challenge. It would involve:
- Externalizing calculation of alignment / bpp
- Let pixel value parameter be 1 bit for planar, or 'depth' bits for packed
- Add a flexible pixel mask parameter that indicates the value depth


## Performance

Although in one way, this library is a study of '8-bit' graphical algorithms, in present times
we can easily get away with some quick-and-dirty implementations left and right. Nevertheless,
it is both interesting and useful to adhere to a number of decent 'old timey' principles, such as:

### Avoid floating point calculations [v]
Even though floats in general need not always be magic and evil (e.g. C64 basic used mainly
floating point numbers, which were reportedly faster than its integer routines), generally
speaking it is best avoided, even more so on microcontrollers that lack an FPU.

### Avoid repeated re-calculations and conditions [x]
This is of course especially true for loops repeated e.g. for each pixel. In certain cases
we might expect such values to be optimized out by the compiler; but then again it may be useful
to make this more explicit where possible.

The following point is an even more urgent version of this one:

### Avoid multiplications and divisions [x]
Although this library (now) uses Bresenham's algorithm for line drawing, even within this function
the actual pixel adressing is still done by repeated multiplication.
This is also the case in more sequential functions such as bitblt itself, where it could easily
be replaced by a single addition per loop on a dedicated counter.

Mind, I do fully expect the compiler to optimize multiplications into bit-shifts where possible,
up to the point where I will retain their 'un-optimized' form where this is easier / more readable.

### Don't over-generalize [v]
Especially the bitblt functions themselves are highly parameterized: they can copy part of
an image, transparent or not, and in the future perhaps even scaled. Yet they can do most of
these things without adding too many on-the-fly calculations or repeated if-statements.
If this was not the case, it might have been better to provide a few flavours of the same method.
Of course, this is ultimately a matter of code size vs. performance.

### Smart screen updates [v]
We can now tell the screen to only update a certain area.
Keeping track of 'dirty' areas on the screen is more application-specific. For instance, a graphical
user interface will typically track a single square only, enlarging it when necessary; but a game
with multiple sprites would update much quicker if it tracked all the sprites individually.
Experiments on this matter are found in 'compose.c'.

