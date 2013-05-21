//  Created by Simon Kahan May 3, 2013
//  Copyright 2013 Simon Kahan. All rights reserved.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/mtaops.h>
#include <machine/runtime.h>
#else
#include "compat/xmt-ops.h"
#endif
#include "defs.h"

/// Input: an undirected graph g with unintialized array C$ = g->marks.
/// Output: return number of unique C$ values when C$[i] == C$[j] iff
/// node i and j are connected by an undirected path in g.
/// Workspace: an array V of size NV is used to mark whether nodes
/// have been visited;  A stack holding at most 2*NV integers;
/// A set of pairs of integers, expected size at most quadratic in concurrency.
///
/// The method is in three phases.
/// PHASE I:
/// Colors C$[] full-bits are set empty, visited V[] set false.
/// All vertices v are pushed onto a concurrent (approximate) stack as -v-1.
/// Vertices are popped from the stack until it empties.
/// (1) For a popped value x<0, v=-x-1. If v has not been visited, its color is set to c[v]=v
/// and every one of its unvisited neighbors is visited, colored
/// v and pushed onto the stack.
/// (2) A popped value x>=0 denotes v=x. v has already been visited and had its color set to c[v].
/// Each of v's unvisited neighbors is visited, color set to c[v], and pushed.
/// In both cases (1) & (2), when we see a visited neighbor whose color
/// is different from that of the vertex popped, we record the ordered pair of colors --
/// that of v and of its neighbor, ordered by increasing value, in a
/// hash table.
/// (3) Any popped value x<0, corresponding to v=-x-1 that has been visited is discarded.
/// 
/// PHASE II: 
/// Execute the PRAM hook and compress algorithm on the edges in the hash table.
/// The result is a subset of g's vertices no two of which are in the same component
/// unless their colors are identical.
/// 
/// PHASE III:
/// The visited array is zeroed.
/// All roots are pushed onto a concurrent approximate stack.
/// Vertices are popped from the stack until it empties.
/// For each vertex x having color c, visit each of x's neighbors
/// setting the color of unvisited neighbors to c and pushing them onto
/// the stack.
///
/// Now two vertices share the same color iff they are connected in g.
/// The number of components is the number of x st x == C$[x].
///
static unsigned int hash(int x) {return x*1299227;}
template <class T, int LOG_CONCURRENCY=8> class Set {
public:
 class T_e {
  public:
   T val;
   T_e * next;
   T_e(T v, T_e* n):val(v),next(n){}
   T_e():val(0),next(0){}
 };
protected:
  const static int width = (1<<LOG_CONCURRENCY);
  T_e * table$[width];
  int64_t count[width];
 public:
  Set() {
    memset(table$,0,sizeof(table$[0])*width);
    memset(count,0,sizeof(*count)*width);
  }
 ~Set() {
   MTA("mta assert parallel")
   for (int i = 0; i < width; i++) {
     T_e * t, * h = table$[i];
     while (h) t = h, h = h->next, delete t;
   }
 }
 void insert(T v) {
   int x = hash(v.key()) % width;
   T_e * p, * h;
   h = readfe(&table$[x]); // lock table list
   p = h;
   while (p && p->val != v) p = p->next;
   if (!p) {
     h = new T_e(v,table$[x]);
     int_fetch_add(&count[x],1);
   }
   table$[x] = h; // unlock table list
 }
 int dump(T* &dumpee) {
   //
   //   for (int i = 0; i < width; i++) {
   //     T_e * p = table$[i];
   //     fprintf(stderr, "%d: ", i);
   //     while (p) {
   //       fprintf(stderr, "(%d %d) ", p->val.x, p->val.y);
   //       p = p->next;
   //     }
   //     fprintf(stderr, "\n");
   //   }
   //
   for (int i = 1; i < width; i++) count[i]+=count[i-1];
   int size = count[width-1];
   dumpee = new T[size];
   MTA("mta assert parallel")
   for (int i = 0; i < width; i++) {
     T_e * t, * h = table$[i];
     while (h != NULL) t = h, h = h->next, dumpee[--count[i]] =t->val, delete t;
     table$[i] = NULL;
   }
   //
   fprintf(stderr, "dump size: %d\n", size);
   //
   return size;
  }
};

template <class T, int64_t EMPTY=0x7fffffffffffffff> class ApproxStack : public Set <T> {
  using Set<T>::width;
  using Set<T>::table$;
  typedef typename Set<T>::T_e T_e;
  // T_e* T_e::new() max waterline will be 2*NV, so should do own mgmt someday
 public:
  static const int64_t empty=EMPTY;
 void push(T v) {
   unsigned int x = hash(MTA_CLOCK(0)) % width;
   T_e * t;
   t = new T_e(v,0);
   t->next = readfe(&table$[x]); // lock table list
   table$[x] = t; // unlock table list
 }
 T pop() {
   T t;
   T_e * tp;
   unsigned int x = hash(MTA_CLOCK(0)) % width;
   for (int i = 0; i < width; i++) {
     tp = readfe(&table$[x]);
     if (tp) {
       table$[x] = tp->next;
       t = tp->val;
       delete tp;
       return t;
     }
     else table$[x] = tp;
     x = (x+1) % width;
   }
   return empty;
}
 void insert(T v) {push(v);}
};

template <typename T> class Pair {
public:
  T x,y;
  Pair(T x, T y):x(x),y(y){};
  Pair():x(0),y(0){};
  int key() {return ((int)x)^((int)y);}
  bool operator!=(Pair<T> b) {
    return ((x!=b.x)||(y!=b.y));
  }
};
		       
class ConnectedComponents {
  graph * g;
  Set < Pair < graphint > > InducedGraph;
  ApproxStack <graphint> Q;
  int64_t * V; //visited array
  graphint * C$; //colors of vertices
  int64_t count; //ends up with number of connected components

// visit vertex i's neighbors, propagating its color, pushing newly visited neighbors onto queue
  void explore(graphint v) {
    graphint color;
    if (v < 0) {
      v = -v-1;
      if (!int_fetch_add(&V[v],1)) {//v not visited
	C$[v] = color = v;
      } else return;
    } else {
      color = C$[v]; //no race possible because v's color is set before pushed
    }
    //visit neighbors of v
    graphint first = g->edgeStart[v];
    graphint lastP1 = g->edgeStart[v+1];
    for (graphint j = first; j < lastP1; j++) {
      int64_t nv = g->endVertex[j];
      graphint nv_color;
      if (!int_fetch_add(&V[nv], 1)) {//nv not visited
	C$[nv] = color; //mta sets full, avoids race of visited but invalid color
	Q.push(nv);
      }
      else if ((nv_color = readff(&C$[nv])) != color) { //wait for color to be set, probably is
	if (nv_color > color) InducedGraph.insert(Pair<graphint>(color,nv_color));
	else InducedGraph.insert(Pair<graphint>(nv_color,color));
      }
    }
  }
public:
  ConnectedComponents(graph *G) {
    g = G;
    count = 0;
  
    const graphint NV = g->numVertices;
    C$ = g->marks;
    V = new int64_t[NV];
    int num_procs = mta_get_num_teams();

    // PHASE I    
    for (color_t i = 0; i < NV; ++i) {
      purge(&C$[i]);
      V[i] = 0;
      Q.push(-i-1);
    }
    
    MTA("mta assert parallel")
    MTA("mta loop future")
    for (int i = 0; i < 100*num_procs; i++) {
      graphint t;
      while((t = Q.pop())!=Q.empty) explore(t);
    }

    // PHASE II
    Pair<graphint> * ig;
    int HNE = InducedGraph.dump(ig);//directed edge pairs

    //
    //    for (int i = 0; i < HNE; i++) {
    //      fprintf(stderr, "(%d %d) ", ig[i].x, ig[i].y);
    //    }
    //    fprintf(stderr, "\n");
    //

    // If it's worth it, we could unload verts into a Set<graphint>
    // then dump into an HV array and save some work in the
    // Compress loop iterating over HV instead of HNE.
    
    bool nchanged;
    do {
      nchanged = false;
      // Hook -- remember if there's an edge (i,j), (j,i) is implicit
      MTA("mta assert parallel")
      for (graphint k = 0; k < HNE; ++k) {
	Pair<graphint> e = ig[k];
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
        Pair<graphint> e = ig[k];
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

    MTA("mta assert parallel")
    for (int i = 0; i < HNE; i++) {
      Pair<graphint> e = ig[i];
      Q.push(e.x); Q.push(e.y);
    }
    for (int i = 0; i < NV; i++) V[i] = 0;

    MTA("mta assert parallel")
    MTA("mta loop future")
    for (int i = 0; i < 100*num_procs; i++) {
      graphint t;
      while((t = Q.pop())!=Q.empty) explore(t);
    }

    //    delete ig;   
  }

  graphint num_components() {
    graphint num = 0;
    graphint NV = g->numVertices;
    //for (int i =0; i < NV; i++) fprintf(stderr, "%d ", C$[i]); fprintf(stderr, "\n");
    for (int i = 0; i < NV; i++) if (C$[i] == i) num++;
    return num;
  }
};

extern "C" graphint connectedComponents(graph * g) {
  ConnectedComponents * cc = new ConnectedComponents(g);
  return cc->num_components();
}
