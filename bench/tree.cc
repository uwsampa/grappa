#include <stdlib.h>
#include <stdio.h>
#ifdef __MTA__
  #include <machine/mtaops.h>
  #include <machine/runtime.h>
  #include <sys/mta_task.h>
  #define myrandom MTA_CLOCK(0)
  #define TOUCH(x) touch(x)
#else
  #define myrandom random()
  #define future
  #define TOUCH(x)
#endif

/* Imbalanced Binary Tree class
** An imbalanced binary tree is constructed such that sibling subtrees 
** are of substantially different size.  In this implementation, a tree of any
** specified "size" nodes is constructed with each subtree having one child
** with a specified fraction 1/"part" of that subtree's nodes while its sibling
** has the remainder.  With increasing value of "part", the tree produced
** has increasing imbalance.
** As written, the tree is constructed to exhibit little spatial locality.
** By replacing Permute with the identity function, a tree that respects locality
** is constructed, in the sense that every subtree fills contiguous locations.
*/

class IBT {

  struct TreeNode {
    struct TreeNode * left;
    struct TreeNode * right;
  };

  int size, part;
  TreeNode * ibt;
  static const int LARGEPRIME = 961748941;
public:
  IBT(int s, int f) : size(s), part(f) {
    if (part <= 0) part = 2;
    if (size >= LARGEPRIME || !(ibt = new TreeNode[size]))
      fprintf(stderr, "class IBT: size too big.\n"), exit(0);
    BuildIBT(0, size);
  }
  ~IBT() {delete ibt;}
  void Print() {
    PrintTree(ibt, 0);
  }
  void Explore() {
    ExploreTree(ibt);
  }
private:
  /* Maps 0.. size-1 into itself, for the purpose of eliminating locality. */
  /* return i if a tree respecting spatial locality is desired. */
  int Permute(int i) {
    long long int j = i;
    return (j*LARGEPRIME) % size;
    /* return i; */
  }
  void BuildIBT(int root, int size) {
    future int d$;
    int t1_size = (size-1)/part;
    int t2_size = size - t1_size - 1;
    TreeNode * r = &ibt[Permute(root)];
    TreeNode * t1 = NULL;
    TreeNode * t2 = NULL;
    if (t1_size) {
#ifdef __MTA__
      future d$ (root, t1_size) {
#endif
        BuildIBT(root+1, t1_size);
#ifdef __MTA__
	return 0;
      }
#endif
      t1 = &ibt[Permute(root+1)];
    }
    if (t2_size) {
      BuildIBT(root+1+t1_size, t2_size);
      t2 = &ibt[Permute(root+1+t1_size)];
    }
    if (myrandom % 2) r -> left = t1, r -> right = t2;
    else              r -> left = t2, r -> right = t1;
    TOUCH(&d$);
  }
  void PrintTree(TreeNode * root, int depth) {
    if (root) {
      PrintTree(root -> right, depth + 1);
      for (int i = 0; i < depth; i++) printf(" "); printf("O\n");
      PrintTree(root -> left, depth + 1);
    }
  }

  void ExploreTree(TreeNode * root) {
    if (root) {
      future int d$;
#ifdef __MTA__
      future d$ (root) {
#endif
        ExploreTree(root->left);
#ifdef __MTA__
      }
#endif
      ExploreTree(root->right);
      TOUCH(&d$);
    }
  }
};

void usage() {
  fprintf(stderr, "\nusage:\n\ttree <size > 0> <imbalance > 0> \ncreates binary tree with <size> > 0 nodes each internal node having one subtree with 1/<imbalance> of the descendents.\nFor example\n\ttree 100 2\ncreates a balanced binary tree with 100 nodes.\n\ttree 100 3\ncreates a tree in which each subtree has about either half or twice the nodes of its sibling, ensuring moderate imbalance.\n");
  exit(0);
}

main(int argc, char * argv[]) {
  int size, imbalance;
  if (argc != 3) usage();
  if ((size = atoi(&argv[1][0])) < 1) usage();
  if ((imbalance = atoi(&argv[2][0])) < 1) usage();
  IBT * t = new IBT(size, imbalance);
  /* t -> Print(); */

  t -> Explore();

  /* for MTA, run a second time now that runtime pools are full: */
#ifdef __MTA__
  int ticks = mta_get_clock(0);
  t -> Explore();
  ticks = mta_get_clock(ticks);

  fprintf(stderr, "Explore: %g seconds.  %g (%g) nodes per second (per processor).\n",
          ((double) ticks)/mta_clock_freq(),
          size/(((double) ticks)/mta_clock_freq()),
          size/(((double) ticks)/mta_clock_freq())/mta_get_num_teams());
#endif
}
