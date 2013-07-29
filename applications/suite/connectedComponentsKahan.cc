//  Created by Simon Kahan May 3-June 30, 2013
//  Copyright 2013 Simon Kahan. All rights reserved.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/mtaops.h>
#include <machine/runtime.h>
#else
#include "compat/xmt-ops.h"
#endif
#include "defs.h"
#include <math.h>

/// Input: an undirected graph g with unintialized array C$ = g->marks.
/// Output: return number of unique C$ values when C$[i] == C$[j] iff
/// vert i and j are connected by an undirected path in g.
///
/// The method is in three phases.
/// PHASE I: (Colour Graph in Parallel, Recording Conflicts)
/// Colours C$[] full-bits are set empty, visited V[] set false.
/// Traverse from higher degree verts first to prevent excessive conflicts.
/// More sophisticated traversals with greater robustness are possible and implemented
/// in other versions of this code. 
///
/// PHASE II: (Resolve Conflicts)
/// Execute the PRAM hook and compress algorithm on the edges in the hash table.
/// The result is a subset of g's vertices no two of which are in the same component
/// unless their colours are identical.
/// 
/// PHASE III: (Propagate Resolution)
/// Create and make num_procs copies of look up tables containing the conflict resolutions,
/// copies are to prevent hot spots.
/// Run through C$[] mapping any colour found in the table to its resolved colour. 
/// Now two vertices share the same colour iff they are connected in g.
/// Compute and return the number of components: the number of x st x == C$[x].
///
/// Note: This particular implementation is limited to graphs w/ 8B vertices
/// due to the way it encodes decomposition of high deg vertices as a single integer,
/// but could easily be modified to handle much larger graphs were they encoded as a pair
/// of integers in the work queue(see encoding in explore).
///
#define MIN(a,b) ((a < b)?a:b)
static unsigned int hash(int x) {return x*5473939012511;}
static double bad_random() {return ((MTA_CLOCK(0)*1299227)&0xffffffff)*1.0/(1<<32);}
class Pair {
public:
  graphint x,y;
  Pair(graphint x, graphint y):x(x),y(y){};
  Pair():x(0),y(0){};
  int key() {return ((int)x)^((int)y);}
  bool operator!=(Pair b) {
    return ((x!=b.x)||(y!=b.y));
  }
};
class Pair1:public Pair {
public:
  Pair1(Pair p) {x = p.x; y = p.y;}
  Pair1(graphint a, graphint b){x=a;y=b;};
  int key() {return x;}
};
template <typename T, int LOG_CONCURRENCY=7> class Set {
public:
 class T_e {
  public:
   T val;
   T_e * next;
   T_e(T v, T_e* n):val(v),next(n){}
   T_e():val(),next(0){}
 };
protected:
  const static int width = (1<<LOG_CONCURRENCY);
  T_e * table$[width];
  int64_t count[width];
  struct {
    T_e *elt;
    int num;
  } heap[width];
  int num_per_heap;
 public:

  Set(){
    memset(table$,0,sizeof(table$[0])*width);
    memset(count,0,sizeof(*count)*width);
  }
  ~Set() {
  }
  bool check_cache(T v) {
    static const unsigned cache_size = 1<<7;
    static typeof(v) cache[cache_size];
    int x;
    for (int i = 0; i < 16; i++) {
      x = hash(MTA_CLOCK(0))&(cache_size-1);
      if (v != cache[x]) continue; else return true;
    }
    cache[x] = v;
    return false;
  }
  // nonzero copy looks up corresponding duplicate.
  // this allows greater concurrency.
  // (dump still dumps only one copy per unique entry.)
  void insert(T v, int copy=0) {
    //    if (check_cache(v)) return;
    int x = (hash(v.key())+copy*8) & (width-1);
    T_e * p, * h;
    for (int i = 0; i < 2; i++) {
      if (i == 0)
	h = table$[x];
      else 
	h = readfe(&table$[x]); // lock table list
      p = h;
      while (p && p->val != v) p = p->next;
      if (!p) {
	if(i==0) continue;
	h = new T_e(v,h);
	if (!copy) int_fetch_add(&count[x],1);
      } else {
	if (i==0) break;
      }
      table$[x] = h; // unlock table list
    }
  }
  // parallel safe only when all ops are finds
  // works for types where key returns first element of pair.
  // assumes each value has been inserted for all values of copy used in calls.
  T * match_on_first(graphint first, int copy) {
    int x = (hash(first)+copy*8) & (width-1);
    T_e * p;
    p = table$[x];
    while (p && p->val.key() != first) p = p->next;
    return &(p->val);
  }
  int size() {
    int total_count = 0;
    for (int i = 0; i < width; i++) int_fetch_add(&total_count, count[i]);
    return total_count;
  }
  int dump(T* &dumpee) {
    MTA("mta assert parallel")
    for (int i = 0; i < width; i++) {
      T_e *t, *h = table$[i];
      while (h != NULL) {
	t = h; h = h->next;
	if ((hash(t->val.key())&(width-1))!=i) insert(t->val,0);
      }
    }
    for (int i = 1; i < width; i++) count[i]+=count[i-1];
    int size = count[width-1];
    dumpee = new T[size];
    MTA("mta assert parallel")
    for (int i = 0; i < width; i++) {
      T_e * t, * h = table$[i];
      while (h != NULL) {
	t = h; h = h->next;
	if ((hash(t->val.key())&(width-1))==i) dumpee[--count[i]] =t->val;
      }
      table$[i] = NULL;
    }
    delete &heap[0].elt[0];

   //
    fprintf(stderr, "dump size: %d\n", size);
   //
    return size;
 }
};
// stack works with push/pop, but also remove: remove pops in fifo order
template <class T> class ApproxStack {
  int num_stacks, log_num_per_stack, num_per_stack;//num_per_stack must be power of 2
  T* base;
  int * sp;//base[i*num_per_stack + (sp[8*i]&(num_per_stack-1))]= next location to push onto stack i
  int * ep;//base[i*num_per_stack + (sp[8*i]&(num_per_stack-1))]= next location to remove from stack i
 public:
  static const int64_t empty=0x7fffffffffffffff;
  ApproxStack(unsigned ns, graphint expected):num_stacks(ns) {
    int log_num_stacks = 0;
    while ((1<<log_num_stacks)<num_stacks) log_num_stacks++;
    num_stacks = 1<<log_num_stacks;
    num_per_stack = (expected+num_stacks)/num_stacks;
    log_num_per_stack = 0;
    while ((1<<log_num_per_stack)<num_per_stack) log_num_per_stack++;
    num_per_stack = 1<<log_num_per_stack;

    fprintf(stderr, "creating %d queues, %d entries each\n", num_stacks, num_per_stack);
    base = (T*) malloc(num_stacks*num_per_stack*sizeof(T));
    sp = (int *) malloc(8*num_stacks*sizeof(int)); //potential hot-spot: use only every 8th entry
    ep = (int *) malloc(8*num_stacks*sizeof(int)); //potential hot-spot: use only every 8th entry
    MTA("mta assert parallel");
    for (int i = 0; i < num_stacks*num_per_stack; i++) purge(&base[i]);
    MTA("mta assert parallel");
    for (int i = 0; i < num_stacks; i++) sp[8*i] = ep[8*i] = 0;
  }
  ~ApproxStack() {
    free(base);
    free(sp);
    free(ep);
  }

  // terminate should be called only once all pushes have completed.
  // therefore, sp no longer advances.
  // removes may be in flight or blocked, so ep may continue to advance after
  // terminate begins -- therefore, every remove must propagate empty to the
  // next location; to provide for some additional parallelism, terminate fills
  // between sp and ep.
  void terminate() {
    MTA("mta assert parallel")
    MTA("mta loop future")
    for (int i = 0; i < num_stacks; i++) {
      int start = ep[8*i]; int end = sp[8*i];
      MTA("mta assert parallel")
      MTA("mta loop future")
      for (int j = end; j < start; j++) {
	base[num_per_stack*i+(j&(num_per_stack-1))]=empty;
      }
    }
  }
  void append(T v, int x=-1) {
    if (x<0) x = hash(MTA_CLOCK(0));
    x &= num_stacks-1;
    int p = x*num_per_stack+(int_fetch_add(&sp[8*x],1) & (num_per_stack-1));
    writeef(&base[p],v);
  }

  void push(T v, int x=-1) {
    if (x<0) x = hash(MTA_CLOCK(0));
    x &= num_stacks-1;
    int p = x*num_per_stack+((int_fetch_add(&ep[8*x],-1)-1) & (num_per_stack-1));
    writeef(&base[p],v);
  }

  T pop(int x=-1) {
    if (x<0) x = hash(MTA_CLOCK(0));
    x &= num_stacks-1;
    int p = x*num_per_stack+((int_fetch_add(&sp[8*x],-1)-1)&(num_per_stack-1));
    T r = readfe(&base[p]);
    if (r == empty) writeef(&base[p],empty);
    return r;
  }

  T remove(int x=-1) {
    if (x<0) x = hash(MTA_CLOCK(0));
    x &= num_stacks-1;
    int p = x*num_per_stack+(int_fetch_add(&ep[8*x],1)&(num_per_stack-1));
    T r = readfe(&base[p]);
    if (r == empty) base[p+1] = empty;
    return r;
  }

  void status() {
    for (int i = 0; i < num_stacks; i++) fprintf(stdout, "[%d,%d] ", ep[8*i],sp[8*i]);
    fprintf(stdout, "\n");
  }
};

int *time;
class ConnectedComponents {
  int starttime, num_procs, num_procs_rounded_up;
  graph * g;
  Set < Pair, 17 > *InducedGraph;
  ApproxStack < graphint > *Q;
  int64_t * V; //visited array
  int * Done; //implicit tree of size NV used to determine termination of Phase I
  graphint * C$; //colours of vertices
  int64_t count; //ends up with number of connected components

  // Done[v] is 0 iff all neighbors of v have been colored and pushed 
  //    AND !Done[2*v+1]&&!Done[2*v+2]
  // Once Done[0] is 0, all nodes have been colored and pushed, every edge explored.
  // IS THAT ENOUGH TO SAY THE ENTIRE PHASE I IS DONE??? 
  // Initialize Done[v] to 3
  //count_vert is a non-hotspotting counter that runs in amortized constant
  // time per call.
  bool count_vert(graphint v) {
    graphint x = int_fetch_add(&Done[v],-1) - 1;
    while(v && !x) {
      v = (v-1)/2;
      x = int_fetch_add(&Done[v],-1) - 1;
    }
    return (!v && !x);
  }
  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx80
  // Explore not only explores verts, but also invokes the bookkeeping that
  // triggers termination of the overall exploration, and does so with no hot-spot.
  // Termination involves signaling poppers there are no more pushes upcoming.
  // This condition occurs once & only once every edge has been traversed and
  // every negative vert has been visited.
  // We determine this condition by initializing a Done[v] counter for each vertex v
  // and decrementing with each event: (1) all edges have been traversed adjacent to
  // the vert, its negative or a phantom (see below); (2) the vert's negative has been
  // popped; and (3) either of a vert's "child" Done variable becomes zero.
  // We increment the Done counter whenever a phantom is pushed.
  // The initial value of each Done variable should be set such that
  // Done[0] is zero iff all Done[v] are zero.  Namely, 
  // Done[v] = 1+1+(2*v+1<NV?1:0)+(2*v+2<NV?1:0)
  // Then, termination occurs when Done[0] = 0.  count_vert is used to decrement
  // and update ancestor nodes returning 0 the first time Done[0] is 0.
  // 
  // To ensure parallelism on high degree vertices, we introduce the concept of
  // a phantom vertex:  when a high degree vertex is explored, it is colored
  // but its neighbors are not enqueued directly.  Instead
  // two phantom vertices, each with the same vertex id, are pushed back
  // into the work queue with high order bits set that define a subrange of
  // the original adjacency list.  When a phantom is explored, it either pushes
  // back two phantoms, or, if its subrange is small enough, is processed as if
  // it were a normal non-seed vertex.  In this way, we prevent premature processing
  // of seed vertices that would result were the concurrency in processing a
  // high degree vertex not exposed promptly.
  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx80
#define BIT_MASK(a, b) (((unsigned) -1 >> (63 - (b))) & ~((1U << (a)) - 1))
  bool explore(graphint x) {
    static const int log_block_size = 5; //could be set dynamically
    static const int block_size = 1<<log_block_size;
    class Vertex {
    public:
      int x;
      Vertex(int x): x(x){};
      unsigned int seed() {return BIT_MASK(63,63)&x;} //x<0;}//[63]
      unsigned int phantom() {return (!seed()) && (BIT_MASK(62,62))&x;}//[62]
      void set_phantom(unsigned int y) {y<<=62; x &= ~BIT_MASK(62,62); x |= (y&BIT_MASK(62,62));}
      unsigned int offset() {return (BIT_MASK(38,61)&x)>>38;}//[38..61]
      void set_offset(unsigned int y) {y<<=38; x &= ~BIT_MASK(38,61); x |= (y&BIT_MASK(38,61));}
      unsigned int log_blocks() {return (x&BIT_MASK(33,37))>>33;}//[33..37]
      void set_log_blocks(unsigned int y) {y<<=33; x &= ~BIT_MASK(33,37); x |= (y&BIT_MASK(33,37));}
      unsigned int v_id() {return x>=0?(x&BIT_MASK(0,32)):-x-1;}//[0..32]
    };
    Vertex w(x);
    //    fprintf(stderr, "explore %s %d(%x)\n", w.seed()?"seed":"",w.v_id(),x);
    graphint color, v;
    int edgeStartModifier = 0, offset = 0;
    int num_times_to_count_vert = 0;

    int NV = g->numVertices;
    v = w.v_id();
    if (w.seed()) {
      if (int_fetch_add(&V[v],1)) {//v already visited, schedule additional seeds
	if (2*v + 1 < NV) Q->append(-(2*v+1)-1);
	if (2*v + 2 < NV) Q->append(-(2*v+2)-1);
	return count_vert(v);//because seed has been visited
      } else { //delay scheduling additional seeds until edges of v have been traversed
	C$[v] = color = v;
      }
    } else { //vertex is already colored, explore and propagate color
      if (w.phantom()) {
	if (w.log_blocks()) {// subdivide further
	  int_fetch_add(&Done[v],1);//replace 1 phantom by 2=> #count_vert(v) calls rises by 1 
	  w.set_log_blocks(w.log_blocks()-1);
	  Q->push(w.x);
	  w.set_offset(w.offset()+(1<<w.log_blocks()));
	  Q->push(w.x);
	  return false;
	} //otherwise, process a particular subset of v's adj list below
      }
      color = C$[v]; //no race possible because v's color is set before pushed
    }

    //visit neighbors of v
    graphint first, lastP1;
    if (w.phantom()) {//know we're to process at most 1 block
      first = g->edgeStart[v] + w.offset()*block_size;
      lastP1 = MIN(first + block_size, g->edgeStart[v+1]);
    } else {
      first = g->edgeStart[v];
      lastP1 = g->edgeStart[v+1];
      if (lastP1 - first > block_size) { //parallelize by creating multiple explores
	int neighbor_blocks = (lastP1-first+block_size)/block_size;
	int log_neighbor_blocks = 0; while (neighbor_blocks/=2) log_neighbor_blocks++;
	Vertex u = Vertex(v); u.set_phantom(1);	u.set_log_blocks(log_neighbor_blocks);
	Q->push(u.x);
	if (w.seed()) return count_vert(v);//will be false
	return false;//v's edges not yet traversed
      }
    }

    for (graphint j = first; j < lastP1; j++) {
      int64_t nv = g->endVertex[j];
      graphint nv_color;
      if (!int_fetch_add(&V[nv], 1)) {//nv not visited
	C$[nv] = color; //mta sets full, avoids race of visited but invalid color
	Q->push(nv);
      }
      //wait for color to be set, probably is
      else if ((nv_color = readff(&C$[nv])) != color) {
	if (nv_color > color) {
	  InducedGraph->insert(Pair(color,nv_color),hash(MTA_CLOCK(0))&(32*num_procs_rounded_up-1));
	}
	else {
	  InducedGraph->insert(Pair(nv_color,color),hash(MTA_CLOCK(0))&(32*num_procs_rounded_up-1));
	}
      }
    }    

    if (w.seed()) {// now that edges have been traversed, schedule next seeds
      int NV = g->numVertices;
      if (2*v + 1 < NV) Q->append(-(2*v+1)-1);
      if (2*v + 2 < NV) Q->append(-(2*v+2)-1);
      count_vert(v);//because seed has been visited
    } 
    return count_vert(v);//because v's edges have been traversed
  }
  void stats() {
    // min & max stack sizes, #lookups
    /*
    int num_stacks = Q->num_stacks;
    int min = 1<<62, max = -(1<<62); 
    for (int i =0; i < num_stacks; i++) {
      int ss = (Q->sp[8*i]-Q->ep[8*i]);
      min = min < ss ? min : ss;
      max = max > ss ? max : ss;
    }
    fprintf(stderr, "%d stacks: [%d,%d], lookups: ",MTA_CLOCK(0),min,max);
    */
    fprintf(stderr, "%d: ", mta_get_clock(0)); Q->status();
  }

public:
  ConnectedComponents(graph *G) {
    starttime = mta_get_clock(0);
    g = G;
    count = 0;
    num_procs = mta_get_num_teams();
    num_procs_rounded_up = 1;
    while (num_procs_rounded_up < num_procs) num_procs_rounded_up*=2;
  
    const graphint NV = g->numVertices;
#define max(a,b) (a>b?a:b)
    fprintf(stderr, "%d ApproxStack: ",mta_get_clock(starttime));
    //    Q = new ApproxStack<graphint>((num_procs*3+8)/8,200*num_procs);
    Q = new ApproxStack<graphint>((num_procs*3+8)/8,2*NV+num_procs*128);
    InducedGraph = new Set < Pair, 17 >();
    C$ = g->marks;
    V = new int64_t[NV];
    Done = new int64_t[NV];

    // PHASE I    
    fprintf(stderr, "%d Initializing Phase I\n", mta_get_clock(starttime));
    struct timeval tp1; gettimeofday(&tp1, 0);
    MTA("mta assert parallel")
    for (color_t i = 0; i < NV; ++i) {
      purge(&C$[i]);
      V[i] = 0;
      Done[i] = 2*i+2<NV?4:(2*i+1<NV?3:2);
    }
    Q->push(-1);

    volatile int end_stats = 0;
    //    future(&end_stats) { while(!end_stats) {mta_sleep(250000000,0);stats();}}
    fprintf(stderr, "%d Executing Phase I\n", mta_get_clock(starttime));
    MTA("mta assert parallel")
    MTA("mta loop future")
    for (int i = 0; i < 80*num_procs; i++) {
      graphint t = 0;
      while ((t = Q->remove(i))!=Q->empty) {
	if (explore(t)) {
	  fprintf(stderr, "%d terminating!\n", mta_get_clock(starttime));
	  Q->terminate();
	  fprintf(stderr, "%d terminate returns.\n", mta_get_clock(starttime));
	  end_stats=1;
	  break;
	}
      }
    }

    //  struct timeval tp2; gettimeofday(&tp2, 0);
    //  int usec = (tp2.tv_sec-tp1.tv_sec)*1000000+(tp2.tv_usec-tp1.tv_usec);
    //  fprintf(stderr, "(%d TEPS)\n", g->numEdges*1000000/usec);
    fprintf(stderr, "%d Executing Phase II\n", mta_get_clock(starttime));
    // PHASE II
    Pair * ig;
    int HNE = InducedGraph->dump(ig);//directed edge pairs

    //
    //    for (int i = 0; i < HNE; i++) {
    //      fprintf(stderr, "(%d %d) ", ig[i].x, ig[i].y);
    //    }
    //    fprintf(stderr, "\n");
    //

    // If it's worth it, we could unload verts into a Set<graphint>
    // then dump into an HV array and save some work in the
    // Compress loop iterating over HV instead of HNE.

    // An invariant is that if a component contains a node of color c,
    // then that component includes node c. So once every node in a
    // component shares the same color, there is exactly one such node.
    bool nchanged;
    do {
      nchanged = false;
      // Hook -- remember if there's an edge (i,j), (j,i) is implicit
      MTA("mta assert parallel")
      for (graphint k = 0; k < HNE; ++k) {
	Pair e = ig[k];
	int i = e.x, j = e.y;
	if (C$[i] < C$[j] && C$[j] == C$[C$[j]]) {
	  C$[C$[j]] = C$[i];
	  nchanged = true;
	} else {
	  if (C$[j] < C$[i] && C$[i] == C$[C$[i]]) {
	    C$[C$[i]] = C$[j];
	    nchanged = true;
	  }
	}
      }
      // Compress -- we do redundant work here; asymptotics are unchanged
      MTA("mta assert parallel")
      for (graphint k = 0; k < HNE; ++k) {
        Pair e = ig[k];
	int i = e.x, j = e.y;
	while (C$[i] != C$[C$[i]]) {
	  nchanged = true;
	  C$[i] = C$[C$[i]];
	}
	while (C$[j] != C$[C$[j]]) {
	  nchanged = true;
	  C$[j] = C$[C$[j]];
	}
      }
    } while (nchanged);
    
    // PHASE III
    fprintf(stderr, "%d Phase III: duplicate renaming tables\n", mta_get_clock(starttime));

    Set < Pair1, 20 > LookUp;
    Set <int, 0 > CacheV;
    fprintf(stderr, "%d Phase III: initialized tables\n", mta_get_clock(starttime));

    int vcount=0; int time[num_procs_rounded_up*HNE]; 
    MTA("mta assert parallel")
    for (int i = 0; i < HNE; i++) {
      Pair e = ig[i];
      int v = e.x, w = e.y;
      if (!CacheV.check_cache(v))
	for (int j = 0; j < num_procs_rounded_up; j++) {
	  LookUp.insert(Pair1(v,C$[v]),j),int_fetch_add(&vcount,1);
	}
      for (int j = 0; j < num_procs_rounded_up; j++)  {
	unsigned t = mta_get_clock(0);
	LookUp.insert(Pair1(w,C$[w]),j);
	time[i*num_procs_rounded_up+j] = mta_get_clock(t);
      }
    }
    int min = 0xfffffffffffffff, max = 0, sum = 0; double stdev = 0;
    int lessthan100k = 0, greatthan1M = 0;
    for (int i = 0; i < num_procs_rounded_up*HNE; i++) {
      min = min < time[i] ? min : time[i];
      max = max > time[i] ? max : time[i];
      lessthan100k += time[i]<100000 ? 1 : 0;
      greatthan1M += time[i]>1000000 ? 1 : 0;
      sum += time[i];
      stdev += time[i]*time[i];
    }
    fprintf(stderr, "[%d, %d] avg: %d stdev: %g <100k: %d >1M: %d\n", min, max, HNE?sum/(num_procs_rounded_up*HNE):-1, HNE?sqrt((double)sum*sum-stdev):-1,lessthan100k,greatthan1M);
    /*
    for (int i =0 ; i < num_procs_rounded_up*HNE; i++) {
      fprintf(stdout, "%12d %12d\n", stime[i], etime[i]);
    }
    */

    fprintf(stderr, "LookUp size is %d, vcount %d\n", LookUp.size(),vcount);

    fprintf(stderr, "%d Phase III: Propagate root colours and count components\n", mta_get_clock(starttime));
    count = 0;

    MTA("mta assert parallel")
    for (int i = 0; i < NV; i++) {
      int j = C$[i];
      Pair1 * p = LookUp.match_on_first(j,i&(num_procs_rounded_up-1));
      if (p) {
	C$[i] = p->y;
      } else {
	if(i == j) count++;
      }
    }

    //TEST: 
#if 0
    fprintf(stderr, "%d Test edge connected vertices match\n", mta_get_clock(starttime));
        int NE = g->numEdges, failures = 0;
        graphint * sv = g->startVertex;
        graphint * ev = g->endVertex;
        MTA("mta assert parallel")
        for (int i = 0; i < NE; i++) {
          if (C$[sv[i]] != C$[ev[i]]) {
    	int_fetch_add(&failures, 1);
          }
        }
        fprintf(stderr, "%d failures\n", failures);
#endif
    ////////
	 
    fprintf(stderr, "%d Deleting V \n", mta_get_clock(starttime));
    delete V;
    fprintf(stderr, "%d Deleting InducedGraph \n", mta_get_clock(starttime));
    delete InducedGraph;
    //    delete ig;   
    fprintf(stderr, "%d Exiting \n", mta_get_clock(starttime));
  }
  int num_components() {return count;}
};   
extern "C" graphint connectedComponents(graph * g) {
  ConnectedComponents * cc = new ConnectedComponents(g);
  graphint c = cc->num_components();
  fprintf(stderr, "Deleting cc \n");
  delete cc;
  return c;
}

