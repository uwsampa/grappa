#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <Delegate.hpp>

#include "common.h"

//Generates the tuple graph represetation for a few simple graph structures

using namespace Grappa;

/* Generates a standard N row by M Col meshgrid graph
 * Each node except for edge nodes are connected to their N,S,W,E neighbors by one edge
 */
void meshgrid_graph(int64_t * num_edges, GlobalAddress<packed_edge> * tuple_edges, int n, int m)
{
  int nedges = 2 * ((n-1) * (m-1)) + m + n - 2;

  //generates a mesh grid graph
  GlobalAddress<packed_edge> edges = global_alloc<packed_edge>(nedges);
  *num_edges = nedges;

  int edge_index = 0;
  packed_edge * pe = new packed_edge;

  //generate the edges and write to them
  for (int64_t row = 0; row < n; row++) {
    for (int64_t col = 0; col < m; col++) {
      if (row < n-1) {
	//	write_edge(pe, n * row + col, (n + 1) * row + col); //write edge to South neighbor
       	delegate::call(edges+edge_index, [n, row, col](packed_edge *e) 
		       {write_edge(e, n*row+col, n*(row+1)+col);});
	edge_index++;
      }
      if (col < m - 1) {
       	delegate::call(edges+edge_index, [n, row, col](packed_edge *e) 
		       {write_edge(e, n*row+col, n*row+col+1);});
	edge_index++;
      }
    }
  }

  *tuple_edges = edges;
}

/* Generates a balanced tree with some number of levels and some number of branches per node
 * Writes the edges to a tuple list and the number of edges to * num_edges
 */
void balanced_tree_graph(int64_t * num_edges, GlobalAddress<packed_edge> * tuple_edges, int lvs, int branches) {
  int64_t nedges = 0;
  int64_t lv_edges = branches;
  for (int i = 0; i < lvs; i++) {
    nedges += lv_edges;
    lv_edges *= branches;
  }
  
  LOG(INFO) << "Allocating " << nedges << " edges...";

  int64_t edge_index = 0;
  int64_t next_vtx = 0;

  GlobalAddress<packed_edge> edges = global_alloc<packed_edge>(nedges);
  *num_edges = nedges;
  
  std::stack<int64_t> prev;
  std::stack<int64_t> next;

  prev.push(next_vtx);
  next_vtx++;

  int64_t vtx;
  for (int i = 0; i < lvs; i++) {
    while(!prev.empty()) {
      vtx = prev.top();
      for (int j = 0; j < branches; j++) {
	delegate::call(edges+edge_index, [vtx, next_vtx] (packed_edge *e) { 
	LOG(INFO) << "Wrote edge (" << vtx << "," << next_vtx << ")";

		       write_edge(e, vtx, next_vtx);});
	//LOG(INFO) << "Wrote edge (" << vtx << "," << next_vtx << ")";
	next.push(next_vtx);
	next_vtx++;
	edge_index++;
      }
      prev.pop();
    }
    //copy the next level over to the stack
    while (!next.empty()) {
      prev.push(next.top()) ;
      next.pop();
    }
  }
  for (int i = 0; i < nedges; i++) {
    delegate::call(edges+i, [](packed_edge * e) {
	LOG(INFO) << "G: e.v0: " << e->v0 << "e.v1: " << e->v1;
      });
  }
  *tuple_edges = edges;
}

/* Generates a complete graph with the specified number of vertices
 * Writes the edges to the list of packed edges *tuple_edges
 */
void complete_graph(int64_t * num_edges, GlobalAddress<packed_edge> * tuple_edges, int vertices) {
  int64_t nedges = ((1 + vertices) * vertices) >> 1;
  *num_edges = nedges;

  GlobalAddress<packed_edge> edges = global_alloc<packed_edge>(nedges);
  int64_t edge_index = 0;
  
  for (int i = 0; i < vertices; i++) {
    for (int j = i; j < vertices; j++) {
      if (i != j) {
	delegate::call(edges+edge_index, [i, j] (packed_edge *e)
		       {write_edge(e, i, j);});
	edge_index++;
      }
    }
  }
  *tuple_edges = edges;
}
