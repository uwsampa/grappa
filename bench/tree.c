#include "tree.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

// uniform random in [0,1)
static double drandom() {
  return rand() * (1.0 / (RAND_MAX + 1.0));
}

static node *make_node() {
  node *n = malloc(sizeof(node));
  assert(n != NULL);
  n->value = drandom();
  n->left = NULL;
  n->right = NULL;
  return n;
}

// For both of these I'm going to assume we're shallow enough I can just use recursion.  Might rewrite later.
node *read_tree(FILE *f) {
  node *root = make_node();
  int children;
  assert(2 == fscanf(f, "%lf %d\n", &root->value, &children));
  if (children > 0) root->left = read_tree(f);
  if (children > 1) root->right = read_tree(f);
  return root;
}

void write_tree(FILE *f, node *tree) {
  if (tree == NULL) return;
  int children = 0;
  if (tree->left != NULL) children++;
  if (tree->right != NULL) children++;
  fprintf(f, "%.14f %d\n", tree->value, children);
  write_tree(f, tree->left);
  write_tree(f, tree->right);
}

node *make_tree(double branch_factor, int maxdepth, int *actual_size) {
  int size = 1;
  node *root = make_node();
  // store all the current tips (same depth) in a linked list
  // via <right> pointers.
  node *branches;
  int num_branches = 1;
  branches = root;
  for (int depth = 2; depth <= maxdepth; ++depth) {
    node *parent = branches;
    node *lastchild = NULL;
    while (parent != NULL) {
      node *nextparent = parent->right;
      node *child = make_node();
      parent->left = child;
      if (lastchild != NULL) {
        lastchild->right = child;
      }
      lastchild = child;
      double branch = drandom();
      if (branch < branch_factor) {
        node *branch = make_node();
        // store the new root
        parent->right = branch;
        // and insert into the list of tips
        child->right = branch;
        lastchild = branch;
        num_branches++;
      } else {
        parent->right = NULL;
      }
      parent = nextparent;
    }
    size += num_branches;
    branches = branches->left;
  }

  // clear out that linked list
  for (node *at = branches; at != NULL;) {
    node *next = at->right;
    assert(at->left == NULL);
    at->right = NULL;
    at = next;
  }

  if (actual_size != NULL) {
    *actual_size = size;
  }

  return root;
}

void free_tree(node *root) {
  if (root == NULL) return;
  free_tree(root->left);
  free_tree(root->right);
  free(root);
}
