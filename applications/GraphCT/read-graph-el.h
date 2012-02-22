#if !defined(READ_GRAPH_EL_HEADER_)
#define READ_GRAPH_EL_HEADER_

struct el {
  int64_t nv, ne;
  int64_t * restrict d;
  int64_t * restrict el;

  int64_t * mem;
  size_t memsz;
  void (*free) (struct el*);
};

void read_graph_el (const char *, struct el *);
void free_graph_el (struct el *);

#if !defined(NO_HELPER_MACROS)
#define CCAT_EL_(A,B) A##B
#define CONCAT_EL_(A,B) CCAT_EL_(A,B)

#define CDECL_EL(G_) const int64_t * restrict CONCAT_EL_(edgedata__,G_) = (G_).el; const int64_t * restrict CONCAT_EL_(vtxdata__,G_) = (G_).d;
#define DECL_EL(G_) int64_t * restrict CONCAT_EL_(edgedata__,G_) = (G_).el; int64_t * restrict CONCAT_EL_(vtxdata__,G_) = (G_).d;

#define D(G_,K_) (CONCAT_EL_(vtxdata__,G_)[(K_)])
#define I(G_,K_) (CONCAT_EL_(edgedata__,G_)[0+3*(K_)])
#define J(G_,K_) (CONCAT_EL_(edgedata__,G_)[1+3*(K_)])
#define W(G_,K_) (CONCAT_EL_(edgedata__,G_)[2+3*(K_)])
#endif /* NO_HELPER_MACROS */

#endif /* READ_GRAPH_EL_HEADER_ */
