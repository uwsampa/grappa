#ifndef TREE_H
#define TREE_H

#include <stdio.h>

typedef struct node {
  struct node *left,  *right;
  double value;
} node;


// Make a tree with fixed <maxdepth>.  Each node (other than a leaf) has a small
// chance (<branch_factor>) of having two children, both of which have
// the given depth. If non-NULL, store the total size in <size>.
node *make_tree(double branch_factor, int maxdepth, int *size);

void free_tree(node *root);

// Read a tree in format given by write_tree();
node *read_tree(FILE *f);

// write a tree to file.  Nodes are stored in DFS (left first) order, separated by newlines.  Node format is "<value> <# of children>".  Termination is implicit (we know when we've hit a leaf.)
void write_tree(FILE *f, node *tree);
#endif
