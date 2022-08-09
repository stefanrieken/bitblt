#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "shared.h"
#include "packed.h"
#include "planar.h"
#include "draw.h"
#include "font.h"
#include "display/display.h"

struct composable;
typedef void compose_cb (struct composable * c, image * on, area a);

typedef struct composable {
  coords from;
  coords to;
  compose_cb * compose;
  union {
    struct composable ** nodes; // composite node, effectively always renders 'on' a root display canvas
    image * image; // one form of leaf data; render on 'on' by bitblt
    void * data;   // another (abstract) form; may be e.g. a tilemap
    int color;
  };
  int num_nodes;
  int transparent; // selects transparency color; -1 for none (if transparent, we can't assume other objects are hidden behind)
} composable;

static inline
bool overlaps (composable * c, area a) {
  // not using <= as to is not inclusive (?)
  return (c->from.x < a.to.x && a.from.x < c->to.x) &&
         (c->from.y < a.to.y && a.from.y < c->to.y);
}

void compose_tree (composable * tree, image * on, area dirty) {
  printf("composing tree\n");
  for (int i=0;i<tree->num_nodes;i++) {
    composable * node = tree->nodes[i];
    if (overlaps(node, dirty)) {
      printf("leaf #%d overlaps 'dirty' area\n", i);
      // notice that we keep drawing on the original background
      node->compose(node, on, dirty);
    }
  }
}

void compose_packed(composable * packed, packed_image * on, area dirty) {
  // N.b.:
  // - We assume we have at least some overlap with the dirty area,
  //   and do not presently use it to refine our call to bitblt
  // - packed.from / to means: where does the image go? ('at' in bitblt)
  // - bitblt.from / to means: which part of the image to copy over?
  //
  // So bitblt.from = 0,0;
  // and bitblt.to is either:
  // - packed.from - packed.to, to clip as instructed; or
  // - simply image.size assuming that the image isn't clipped anyway
  packed_bitblt(on, packed->image, (coords){0,0}, packed->image->size, packed->from, packed->transparent);
}

void compose_planar(composable * planar, planar_image * on, area dirty) {
  printf("composing planar image\n");
  // N.b.:
  // - We assume we have at least some overlap with the dirty area,
  //   and do not presently use it to refine our call to bitblt
  // - planar.from / to means: where does the image go? ('at' in bitblt)
  // - bitblt.from / to means: which part of the image to copy over?
  //
  // So bitblt.from = 0,0;
  // and bitblt.to is either:
  // - planar.from - packed.to, to clip as instructed; or
  // - simply image.size assuming that the image isn't clipped anyway
  planar_bitblt(on, planar->image, (coords){0,0}, planar->image->size, planar->from, 0, planar->transparent);
}

// A simple background color painter.
// Optimized by looking at the dirty area (for a change).
void compose_fill_planar(composable * fill, planar_image * on, area dirty) {
  printf("composing fill\n");

  for (int i=0;i<on->depth;i++) {
    draw_rect(on->planes[i], on->size, dirty.from, dirty.to, true, (fill->color >> i) & 0b01);
  }
}

// This one is presently just a teaser for the imagination
void compose_tilemap(composable * tilemap, image * on, area a) {
}



static inline
composable * new_composable(compose_cb * cb, coords from, coords to) {
  composable * result = calloc(sizeof(composable), 1);
  result->compose = cb;
  result->from = from;
  result->to = to;
  return result;
}

static inline
composable * add_composable(composable * tree, composable * node) {
  tree->nodes = realloc(tree->nodes, sizeof(composable *) * (tree->num_nodes+1));
  tree->nodes[tree->num_nodes] = node;
  tree->num_nodes++;
  return node;
}

#define MAX_DIRTY 8
area dirty[MAX_DIRTY];
int num_dirty;

void clear_dirty() {
  num_dirty=0;
}

void add_dirty(area d) {
  // TODO also check for overlaps
  if (num_dirty == MAX_DIRTY) {
    // simply merge our rectangle with another entry
    dirty[0].from.x = d.from.x < dirty[0].from.x ? d.from.x : dirty[0].from.x;
    dirty[0].from.y = d.from.y < dirty[0].from.y ? d.from.y : dirty[0].from.y;
    dirty[0].to.x = d.to.x > dirty[0].to.x ? d.to.x : dirty[0].to.x;
    dirty[0].to.y = d.to.y < dirty[0].to.y ? d.to.y : dirty[0].to.y;
  } else {
    printf("add dirty: from %d %d to %d %d\n", d.from.x, d.from.y, d.to.x,d.to.y);
    dirty[num_dirty].from.x = d.from.x;
    dirty[num_dirty].from.y = d.from.y;
    dirty[num_dirty].to.x = d.to.x+1;
    dirty[num_dirty].to.y = d.to.y+1;
    num_dirty++;
  }
}
void recompose_dirty(composable * root, image * background) {
  for (int i=0; i<num_dirty; i++) {
    root->compose(root, background, dirty[i]);
    display_redraw(dirty[i]);
  }
}

void move(composable * c, coords dest) {
  add_dirty((area) {c->from, c->to});
  c->to.x = (c->to.x - c->from.x) + dest.x;
  c->to.y = (c->to.y - c->from.y) + dest.y;
  c->from = dest;
  add_dirty((area) {c->from, c->to});
}

// Catty paletty! (and some garbage)
uint8_t palette[] = {
	0x00, 0x00, 0x00, // black (and / or transparent!)
	0xFF, 0xFF, 0xFF, // white

	0x00, 0xFF, 0x00,
	0x20, 0xAA, 0xCA, // hair
	0x89, 0x75, 0xFF, // ears

	0x00, 0x88, 0x00, // text
	0x00, 0x00, 0x88,
	0x61, 0x24, 0xFF, // mouth
	0x88, 0x88, 0x88, // grey

	0x88, 0x88, 0x00,
	0x00, 0x88, 0x88,
	0xCC, 0xCC, 0xCC, // eye white

	0x22, 0x22, 0x22, // eyes
	0x88, 0x88, 0x00,
	0x00, 0x88, 0x88,
	0x20, 0x10, 0x59 // mouth
};

planar_image * draw_text(char * text, int line, bool fixedWidth, bool fixedHeight) {
  int stretch = 1;

  planar_image * txt = render_text(text, stretch, fixedWidth, fixedHeight);
  planar_image * result = new_planar_image(txt->size.x, txt->size.y, 4);
    // copy into these 2 planes to get a green color
  planar_bitblt_plane(result, txt, (coords) {0, 0}, 0);
  planar_bitblt_plane(result, txt, (coords) {0, 0}, 2);
  free(txt);
  return result;
}

planar_image * background;
void * demo(void * args) {
  composable * root = new_composable(compose_tree, (coords) {0,0}, background->size);
  composable * fill = add_composable(root, new_composable(compose_fill_planar, root->from, root->to));
  fill->color = 0b01;

  planar_image * txt1 = draw_text("Compose & redraw", 0, false, false);
  composable * text1 = add_composable(root, new_composable(compose_planar, (coords){0,0}, (coords) {txt1->size.x, txt1->size.y}));
  text1->image = txt1;

  planar_image * txt2 = draw_text("objects!", 0, false, false);
  composable * text2 = add_composable(root, new_composable(compose_planar, (coords){0,0}, (coords) {txt2->size.x, txt2->size.y}));
  text2->image = txt2;

  // first a 'manual' composition
  root->compose(root, background, (area) {root->from, root->to});
  display_redraw((area) {root->from, root->to});

  // then through 'dirty'

  for (int i=0; i<96;i++) {
    clear_dirty();
    move(text2, (coords) {i,0});
    recompose_dirty(root, background);
    usleep(30000);
  }

  return 0;
}

int main (int argc, char ** argv) {
  display_data * dd = malloc(sizeof(display_data));
  background = new_planar_image(200, 128, 4);
  dd->planar_display = background;
  dd->palette = palette;
  dd->scale = 1; // comes on top of any default scaling

  display_init(argc, argv, dd, NULL);

  pthread_t worker;
  pthread_create(&worker, NULL, demo, dd);

  display_runloop(worker);
}

// So, when moving anything from old to new place,
// add both old and new place to 'dirty' areas to have them redrawn.
// A smart 'add_dirty' method may e.g. combine overlapping dirty areas as one.
