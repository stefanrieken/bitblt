#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "shared.h"
#include "packed.h"
#include "planar.h"
#include "draw.h"
#include "font.h"
#include "tilemap.h"
#include "bitmap.h"
#include "display/display.h"

#include "compose.h"


static inline
bool overlaps (area a, area b) {
  // not using <= as to is not inclusive (?)
  return (a.from.x < b.to.x && b.from.x < a.to.x) &&
         (a.from.y < b.to.y && b.from.y < a.to.y);
}

static inline
void move(Composable * c) {
  if (c->speed.x == 0 && c->speed.y == 0) return;

  add_dirty(c->area);
  c->area.from.x += c->speed.x;
  c->area.from.y += c->speed.y;
  c->area.to.x += c->speed.x;
  c->area.to.y += c->speed.y;
  add_dirty(c->area);
}

void compose_move(Composable * tree) {
  // note that this externally applied behaviour
  // isn't activated in any sub-tree nodes, if we ever have them
  for (int i=0;i<tree->num_nodes;i++) {
    Composable * node = tree->nodes[i];
    move(node);
  }
}

void compose_tree (Composable * tree, Image * on, area dirty) {
  for (int i=0;i<tree->num_nodes;i++) {
    Composable * node = tree->nodes[i];
    if (overlaps(node->area, dirty)) {
      // notice that we keep drawing on the original background
      node->compose(node, on, dirty);
    }
  }
}

void compose_packed(Composable * packed, PackedImage * on, area dirty) {
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
  packed_bitblt(on, packed->image, (coords){0,0}, packed->image->size, packed->area.from, packed->transparent);
}

void compose_planar(Composable * planar, PlanarImage * on, area dirty) {
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
  planar_bitblt(on, planar->image, (coords){0,0}, planar->image->size, planar->area.from, 0, planar->transparent);
}

// A simple background color painter.
// Optimized by looking at the dirty area (for a change).
void compose_fill_packed(Composable * fill, PackedImage * on, area dirty) {
  draw_rect(on->data, on->size, dirty.from, dirty.to, true, fill->color);
}

void compose_tilemap(Composable * tilemap, Image * on, area a) {
  // the un-repeated size
  coords tilemap_size;
  tilemap_size.x = tilemap->map->map_size.x * tilemap->map->tile_size.x;
  tilemap_size.y = tilemap->map->map_size.y * tilemap->map->tile_size.y;

  // quick-hack fix for negative fall-through
  // in actuality a moving tilemap should keep a fixed window
  if (tilemap->area.from.x <= -tilemap_size.x) { tilemap->area.from.x += tilemap_size.x; tilemap->area.to.x += tilemap_size.x; }
  if (tilemap->area.from.y <= -tilemap_size.y) { tilemap->area.from.y += tilemap_size.y; tilemap->area.to.y += tilemap_size.y; }

  for (int y = tilemap->area.from.y; y < tilemap->area.to.y; y += tilemap_size.y) {
    for (int x = tilemap->area.from.x; x < tilemap->area.to.x; x += tilemap_size.x) {
      apply_plain_tile_map(tilemap->map, on, (coords) {0,0}, tilemap->map->map_size, packed_bitblt, (coords) {x,y}, -1);
    }
  }
}


static inline
Composable * new_composable(ComposeCallback * cb, coords from, coords to) {
  Composable * result = calloc(sizeof(Composable), 1);
  result->compose = cb;
  result->area.from = from;
  result->area.to = to;
  result->speed = (coords) {0,0};
  return result;
}

static inline
Composable * add_composable(Composable * tree, Composable * node) {
  tree->nodes = realloc(tree->nodes, sizeof(Composable *) * (tree->num_nodes+1));
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

/**
 * merges a and b into a; returns a.
 */
static inline
area * merge_dirty(area * a, area * b) {
  a->from.x = a->from.x < b->from.x ? a->from.x : b->from.x;
  a->from.y = a->from.y < b->from.y ? a->from.y : b->from.y;
  a->to.x = a->to.x > b->to.x ? a->to.x : b->to.x;
  a->to.y = a->to.y < b->to.y ? a->to.y : b->to.y;
  return a;
}

/**
 * Add dirty area to list of dirty areas, OR, if full,
 * merge two dirty areas in list.
 */
void add_dirty(area d) {
  for (int i=0; i<num_dirty;i++) {
    if (overlaps(dirty[i], d)) {
      merge_dirty(&dirty[i], &d);

      // In case d overlaps (connects) multiple existing areas,
      // joining them is more complex, as it reduces the number of
      // items in the list. For now we accept a sub-optimal result.
      return;
    }
  }

  // if no overlap:

  if (num_dirty == MAX_DIRTY) {
    // either we're full or we are configured to only have one dirty area
    // simply merge our rectangle with another entry
    merge_dirty(&dirty[0], &d);
  } else {
    // add new entry
    dirty[num_dirty].from.x = d.from.x;
    dirty[num_dirty].from.y = d.from.y;
    dirty[num_dirty].to.x = d.to.x+1;
    dirty[num_dirty].to.y = d.to.y+1;
    num_dirty++;
  }
}
void redraw_dirty(Composable * root, Image * background) {
  for (int i=0; i<num_dirty; i++) {
    display_redraw(dirty[i]);
  }
}

PackedImage * draw_text2(char * text, int line, bool fixedWidth, bool fixedHeight) {
  int stretch = 1;

  PlanarImage * txt = render_text(text, stretch, fixedWidth, fixedHeight);
  PlanarImage * result = new_planar_image(txt->size.x, txt->size.y, 4);
    // copy into these 2 planes to get a green color
  planar_bitblt_plane(result, txt, (coords) {0, 0}, 0);
  planar_bitblt_plane(result, txt, (coords) {0, 0}, 2);
  free(txt);
  return to_packed_image(pack(result), result->size.x, result->size.y, result->depth);
}

PackedImage * background;
void * demo(void * args) {
  DisplayData * dd = (DisplayData *) args;

  configure_draw(4); // 4 bpp

  Composable * root = new_composable(compose_tree, (coords) {0,0}, background->size);
  Composable * fill = add_composable(root, new_composable(compose_fill_packed, root->area.from, root->area.to));
  fill->color = 9;

  uint8_t (*palette_read)[];
  PackedImage * tileset = read_bitmap("spritesheet.bmp", &palette_read);
  for (int i=0;i<(1<<tileset->depth)*3;i++) (*palette)[i] = (*palette_read)[i];
  free(palette_read);

  TileMap strato = {
    tileset,
    (coords) {8,8}, // tile size
    (coords) {4,1}, // num tiles
    (uint8_t[]){
      2,2,3,2,
    },
    0xFF // mask
  };
  Composable * strato_c = add_composable(root, new_composable(compose_tilemap, (coords) {0,0}, (coords) {200,8*1}));
  strato_c->map = &strato;

  TileMap sky = {
    tileset,
    (coords) {8,8}, // tile size
    (coords) {4,1}, // num tiles
    (uint8_t[]){
      10,11,2,2
    },
    0xFF // mask
  };
  
  // We just assign a larger-than-screen strip here
  Composable * sky_c = add_composable(root, new_composable(compose_tilemap, (coords) {0,8}, (coords) {232,8+8*1}));
  sky_c->map = &sky;

  sky_c->speed = (coords) {-1, 0};

  TileMap bricks = {
    tileset,
    (coords) {8,8}, // tile size
    (coords) {4,2}, // num tiles
    (uint8_t[]){
      48,49,48,49,
      48,49,48,49
    },
    0xFF // mask
  };

  Composable * bricks_c = add_composable(root, new_composable(compose_tilemap, (coords) {0,16}, (coords) {232,16+8*2}));
  bricks_c->map = &bricks;
  bricks_c->speed = (coords) {-2, 0};

  PackedImage * txt1 = draw_text2("Compose & redraw", 0, false, false);
  Composable * text1 = add_composable(root, new_composable(compose_packed, (coords){8,8}, (coords) {8+txt1->size.x, 8+txt1->size.y}));
  text1->image = txt1;

  PackedImage * txt2 = draw_text2("objects!", 0, false, false);
  Composable * text2 = add_composable(root, new_composable(compose_packed, (coords){8,8}, (coords) {8+txt2->size.x, 8+txt2->size.y}));
  text2->image = txt2;
  text2->speed = (coords) {1,0};

  // first a 'manual' composition
  root->compose(root, background, root->area);
  display_redraw(root->area);

  // then through 'dirty'
  for (int i=0; i<96;i++) {
    clear_dirty();
    compose_move(root);
    for (int j=0;j<num_dirty;j++)
      root->compose(root, background, dirty[j]);
    redraw_dirty(root, background);
    usleep(30000);
    if (dd->display_finished) return 0;
  }
  text2->speed.x = 0;
  bricks_c->area.to.x=400;
  for (int i=0; i<96;i++) {
    clear_dirty();
    compose_move(root);
    for (int j=0;j<num_dirty;j++)
      root->compose(root, background, dirty[j]);
    redraw_dirty(root, background);
    usleep(30000);
    if (dd->display_finished) return 0;
  }

  return 0;
}

int main (int argc, char ** argv) {
  DisplayData * dd = malloc(sizeof(DisplayData));
  background = new_packed_image(200, 128, 4);
  dd->packed = true;
  dd->display = background;
  dd->palette = palette;
  dd->scale = 1; // comes on top of any default scaling
  dd->display_finished = false;

  display_init(argc, argv, dd, NULL);

  pthread_t worker;
  pthread_create(&worker, NULL, demo, dd);

  display_runloop(worker);
}

// So, when moving anything from old to new place,
// add both old and new place to 'dirty' areas to have them redrawn.
// A smart 'add_dirty' method may e.g. combine overlapping dirty areas as one.
