AR = ar
RANLIB = ranlib
# CC is the C++ compiler, while cc is the C compiler
COMPILER = CC
CFLAGS = -Drestrict=__restrict__ -O2 -DNDEBUG -DGRAPH_GENERATOR_SEQ -fPIC # -g # -pg
# CFLAGS = -Drestrict=__restrict__
LDFLAGS = #-g # -pg

all: libgraph_generator_seq.a 

generator_test_seq: generator_test_seq.c libgraph_generator_seq.a
	CC $(CFLAGS) $(LDFLAGS) -o generator_test_seq generator_test_seq.c -L. -lgraph_generator_seq -lm


# mrg_transitions.c only contains one global array. mpicxx will ignore it, needs a C compiler for it to even exist in the object file.
# If mpicxx is used to compile this then you'll get an "undefined mrg_skip_matrices" linker error.
mrg_transitions.o:
	cc $(CFLAGS) -c -o mrg_transitions.o mrg_transitions.c

libgraph_generator_seq.a: btrd_binomial_distribution.o splittable_mrg.o mrg_transitions.o graph_generator.o permutation_gen.o make_graph.o utils.o scramble_edges.o
	$(AR) cruv libgraph_generator_seq.a btrd_binomial_distribution.o splittable_mrg.o mrg_transitions.o graph_generator.o permutation_gen.o make_graph.o utils.o scramble_edges.o
	$(RANLIB) libgraph_generator_seq.a

clean:
	-rm -f generator_test_seq libgraph_generator_seq.a *.o
