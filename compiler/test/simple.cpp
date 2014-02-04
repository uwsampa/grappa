#include <stdio.h>

#define global    __attribute__((address_space(100)))
#define symmetric __attribute__((address_space(200)))
#define async     __attribute__((annotate("async")))
#define unbound   __attribute__((annotate("unbound"),noinline))

struct Vertex {
  Vertex global* adj;
  long nadj;
};

struct Graph {
  Vertex global* vs;
  long nv;
  
  Graph(long nv): vs(reinterpret_cast<Vertex global*>(new Vertex[nv])), nv(nv) {}
  
  static Graph symmetric* create(long nv) {
    return reinterpret_cast<Graph symmetric*>(new Graph(nv));
  }
};

template< typename T >
T global* make_global(T* a) { return reinterpret_cast<T global*>(a); }

template< typename T >
T symmetric* make_symmetric(T* a) { return reinterpret_cast<T symmetric*>(a); }

unbound void unbound_printf() {
  printf("hello\n");
}

async int main(int argc, char* argv[]) {
  
  auto g = Graph::create(10);
  
  printf("%ld\n", g->nv);
  
  printf("%ld\n", g->vs[6].nadj);
  
  printf("%ld\n", g->vs[6].adj[3].nadj);
  
  unbound_printf();
  
  long x = argc;
  long global* a = make_global(new long);
  long global* b = make_global(new long);
  
  *a += x + *b;
  
}
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
//<pid 19269 SIGSEGV (signal 11)>
/////////////////////////////////////////////////////////////