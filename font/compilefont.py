#!/usr/bin/env python3

# Compiles the .56font files to 32-bits integers
# with two leading zeroes.
# Intended for use on the bare bones RasPi framebuffer.

import sys

i=0
n=0
v=0
s=""

file = open (sys.argv[1], "r")

for line in file:
#  sys.stdout.write(line)
  i = i + 1
  #if i%7 != 0:
  for ch in line:
    if ch == "#":
      v = (v << 1) | 1
      n = n + 1
    elif ch==".":
      v = (v << 1)
      n = n + 1

  if n == 32:
    sys.stdout.write(s + format(v, "#010x"))
    n = 0
    v = 0
    s = ", "

file.close()

sys.stdout.write("\n")

