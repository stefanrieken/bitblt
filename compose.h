#ifndef BITBLT_COMPOSE_H
#define BITBLT_COMPOSE_H

struct Composable;
typedef void ComposeCallback (struct Composable * c, Image * on, area a);

typedef struct Composable {
  area area;

  ComposeCallback * compose;
  union {
    struct Composable ** nodes; // composite node, effectively always renders 'on' a root display canvas
    Image * image; // one form of leaf data; render on 'on' by bitblt
    TileMap * map;
    void * data;   // another (abstract) form; may be e.g. a tilemap
    int color;
  };
  int num_nodes;
  int transparent; // selects transparency color; -1 for none (if transparent, we can't assume other objects are hidden behind)
  coords speed; // may be negative. TODO this lacks subtlety, e.g. moving once every n frames
} Composable;

/** Functions for marking areas 'dirty' */
void clear_dirty();
void add_dirty(area d);
void redraw_dirty(Composable * root, Image * background);
#endif /*BITBLT_COMPOSE_H*/
