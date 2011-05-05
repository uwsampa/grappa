#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/timeb.h>
#include <inttypes.h>
#include <limits.h>

// Treap implementations and algorithms from
// http://www.cs.cmu.edu/~scandal/treaps.html

static uint64_t get_ns()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  uint64_t ns = 0;
  ns += ts.tv_nsec;
  ns += ts.tv_sec * (1000LL * 1000LL * 1000LL);
  return ns;
}

static unsigned int goodseed() {
  FILE *urandom = fopen("/dev/urandom", "r");
  unsigned int seed;
  assert(1 == fread(&seed, sizeof(unsigned int), 1, urandom));
  fclose(urandom);
  if (seed == 0) seed = 1;
  return seed;
}

// returns uniformly chosen integer in [i,j)
static int32_t rand_range(int32_t i, int32_t j) {
  int32_t spread = j - i;
  int32_t limit = RAND_MAX - RAND_MAX % spread;
  int32_t rnd;
  do {
    rnd = random();
  } while (rnd >= limit);
  rnd %= spread;
  return (rnd + i);
}


struct treap {
  unsigned int prio;
  int key;
  struct treap *left;
  struct treap *right;
};

typedef struct treap *node;

unsigned int genprio() {
  return random();
}

// slow, avoid in fast code
node new_node(int prio, int key) {
  node res = calloc(1, sizeof(struct treap));
  assert (res != NULL);
  res->prio = prio;
  res->key = key;
  return res;
}

// kills recursively
void delete_node(node n) {
  if (n == NULL) return;
  delete_node(n->left);
  delete_node(n->right);
  free(n);
}

// delete only this
void delete_just(node n) {
  if (n == NULL) return;
  n->left = NULL;
  n->right = NULL;
  delete_node(n);
}

// inplace
node splithere(node *less, node *more, node r, int on) {
  if (r == NULL) {
    *less = NULL;
    *more = NULL;
    return NULL;
  }

  if (r->key == on) {
    *less = r->left;
    *more = r->right;
    r->left = NULL;
    r->right = NULL;
    return r;
  }

  if (r->key < on) {
    *less = r;
    return splithere(&(r->right), more, r->right, on);
  } else {
    *more = r;
    return splithere(less, &(r->left), r->left, on);
  }
}


node joinhere(node less, node more) {
  if (less == NULL) return more;
  if (more == NULL) return less;

  if (less->prio < more->prio) {
    more->left = joinhere(less, more->left);
    return more;
  } else {
    less->right = joinhere(less->right, more);
    return less;
  }
}

// inplace
node unionhere(node x, node y) {
  if (x == NULL) return y;
  if (y == NULL) return x;

  node tmp;
  if (x->prio < y->prio) {
    tmp = x;
    x = y;
    y = tmp;
  }
  node less, more;
  node dup = splithere(&less, &more, y, x->key);
  delete_just(dup);
  x->left = unionhere(x->left, less);
  x->right = unionhere(x->right, more);
  return x;
}

node intersecthere(node x, node y) {
  if (x == NULL || y == NULL) {
    delete_node(x);
    delete_node(y);
    return NULL;
  }
  
  node tmp;
  if (x->prio < y->prio) {
    tmp = x;
    x = y;
    y = tmp;
  }
  node less, more;
  node dup = splithere(&less, &more, y, x->key);
  x->left = intersecthere(x->left, less);
  x->right = intersecthere(x->right, more);
  if (dup == NULL) {
    node left = x->left;
    node right = x->right;
    delete_just(x);
    return joinhere(left, right);
  } else {
    delete_just(dup);
  }
  return x;
}

node create_random(int size, int range) {
  assert(size > 0);
  if (size == 1) {
    return new_node(genprio(), rand_range(0,range));
  }

  int half = size/2;
  node left = create_random(half, range);
  node right = create_random(size - half, range);
  node result = unionhere(left, right);
  return result;
}

int find(node treap, int key) {
  if (treap == NULL) return 0;
  if (key == treap->key) return 1;
  if (key > treap->key) return find(treap->right, key);
  return find(treap->left, key);
}

void test_subset(node small, node big) {
  if (small == NULL) return;
  assert(find(big, small->key));
  test_subset(small->left, big);
  test_subset(small->right, big);
}

// tests if res \subseteq x \cup y
void test_unionsubset(node x, node y, node res) {
  if (res == NULL) return;
  assert(find(x, res->key) || find(y, res->key));
  test_unionsubset(x, y, res->left);
  test_unionsubset(x, y, res->right);
}

void test_union(node x, node y, node res) {
  test_unionsubset(x, y, res);
  test_subset(x, res);
  test_subset(y, res);
}

// hard to name this.
// tests each element of X: if it's in Y, it better be in RES.
void test_NAME(node x, node y, node res) {
  if (x == NULL) return;
  assert (!find(y, x->key) || find(res, x->key));
  test_NAME(x->left, y, res);
  test_NAME(x->right, y, res);
}

void test_intersect(node x, node y, node res) {
  test_subset(res, x);
  test_subset(res, y);
  test_NAME(x, y, res);
  test_NAME(y, x, res);
}

void print_treapwalk(node x, int depth) {
  if (x == NULL) return;
  for (int i = 0; i < depth; ++i) {
    printf(" ");
  }
  printf("<%d,%d>\n", x->prio, x->key);
  print_treapwalk(x->left, depth+1);
  print_treapwalk(x->right, depth+1);
}

void print_treap(node x) {
  print_treapwalk(x, 0);
}

int depth_treap(node x) {
  if (x == NULL) return 0;
  int left = depth_treap(x->left);
  int right = depth_treap(x->right);
  int max  = left > right? left : right;
  return (max+1);
}

int size_treap(node x) {
  if (x == NULL) return 0;
  int left = size_treap(x->left);
  int right = size_treap(x->right);
  return (left+right+1);
}

void dump_elements(node x) {
  if (x == NULL) return;
  dump_elements(x->left);
  printf("%d\n", x->key);
  dump_elements(x->right);
}

void verify_treapwalk(node x, unsigned int p, int min, int max) {
  if (x == NULL) return;
  assert (x->prio <= p &&
          min <= x->key &&
          max >= x->key);
  verify_treapwalk(x->left,p-1,min,x->key-1);
  verify_treapwalk(x->right,p-1,x->key+1,max);
}

void verify_treap(node x) {
  verify_treapwalk(x, UINT_MAX, INT_MIN, INT_MAX);
}

node copy(node x) {
  if (x == NULL) return NULL;
  node root = new_node(x->prio, x->key);
  root->left = copy(x->left);
  root->right = copy(x->right);
  return root;
}

#define ALLOC_NODE(_name, _prio, _key) \
  (_name = (*slab)++, _name->prio = _prio, _name->key = _key, _name)

node split_fast(node *less, node *more, node r, int on, node *slab) {
  if (r == NULL) {
    *less = NULL;
    *more = NULL;
    return NULL;
  }
  node root = ALLOC_NODE(root, r->prio, r->key);
  if (r->key < on) {
    *less = root;
    root->left = r->left;
    return split_fast(&(root->right), more, r->right, on, slab);
  } else if (r->key > on) {
    *more = root;
    root->right = r->right;
    return split_fast(less, &(root->left), r->left, on, slab);
  } else {
    *less = r->left;
    *more = r->right;
    return root;
  }
}

node intersect_fast(node x, node y, node *slab) {
  if (x == NULL) return NULL;
  if (y == NULL) return NULL;

  node tmp;
  if (x->prio < y->prio) {
    tmp = x;
    x = y;
    y = tmp;
  }
  node lessr, morer;
  
  //  node dup = split_fast(&less, &more, y, x->key, slab);
  node r = y;
  node *less = &lessr, *more = &morer;
  node gen;
  int dup;
  while (r != NULL && !(x->key == r->key)) {
    gen = ALLOC_NODE(gen, r->prio, r->key);
    if (r->key < x->key) {
      *less = gen;
      gen->left = r->left;
      less = &(gen->right);
      r = r->right;
    } else {
      *more = gen;
      gen->right = r->right;
      more = &(gen->left);
      r = r->left;
    }
  }

  if (r == NULL) {
    *less = NULL;
    *more = NULL;
    dup = 0;
  } else {
    *less = r->left;
    *more = r->right;
    dup = 1;
  }

  node left = intersect_fast(x->left, lessr, slab);
  node right = intersect_fast(x->right, morer, slab);

  if (dup == 0) {
    return joinhere(left, right);
  } else {
    //    printf("intersect: %d\n", dup->key);
    node root = ALLOC_NODE(root, x->prio, x->key);
    root->left = left;
    root->right = right;
    return root;
  }
}


int main(int argc, char *argv[]) {
  if (argc != 5 && argc != 6) {
	printf("usage: %s ncores-min ncores-max size range seed\n"
               "ncores-min, ncores-max - number of hardware threads\n"
               "size - number of items in each set\n"
               "range - draw elements from [0,range)\n"
               "seed - RNG seed (optional)\n",
               argv[0]);
	exit(1);
  }

  int nruns = 4;
  int cf = strtol(argv[1], NULL, 0);
  int ct = strtol(argv[2], NULL, 0);
  int size = strtol(argv[3], NULL, 0);
  int range = strtol(argv[4], NULL, 0);
  unsigned int seed = 0;
  if (argc == 6) seed = strtol(argv[5], NULL, 0);
  if (seed == 0) seed = goodseed();
  printf("seed: %d\n", seed);
  srand(seed);

  node x = create_random(size, range);
  node y = create_random(size, range);
  /*
    print_treap(x);
    printf("---\n");
    print_treap(y);
  */
  verify_treap(x);
  verify_treap(y);/*
  dump_elements(x);
  printf("---\n");
  dump_elements(y);*/
  printf("%d %d\n", depth_treap(x), depth_treap(y));
  printf("%d %d\n", size_treap(x), size_treap(y));
  // no multicore support yet
  assert(cf == 1);
  assert(ct == 1);
#define LOTS (1 << 30)
  for (int c = cf; c <= ct; ++c) {
    node slab = calloc(LOTS, sizeof(char));
    for (int i = 0; i < nruns; ++i) {
      node bump = slab;
      uint64_t before = get_ns();
      node result = intersect_fast(x, y, &bump);
      uint64_t after = get_ns();
      uint64_t elapsed = after - before;
      printf("result size: %d\n", size_treap(result));
      verify_treap(result);
      /*      printf("---res---\n");
      dump_elements(result);
      printf("---\n");
      */
      assert ((char *)bump - (char *)slab < LOTS);
      double rate = elapsed;
      rate /= size;
      test_intersect(x, y, result);
      printf("%d: %lu ns -> %f ns/element \n", c, elapsed, rate);
    }
    free(slab);
  }

  delete_node(x);
  delete_node(y);
  return 0;
}
