
set(SOURCES
  graph_generator.h
  graph_generator.c
  make_graph.h
  make_graph.c
  splittable_mrg.h
  splittable_mrg.c
  utils.h
  utils.c
  user_settings.h
  mod_arith_32bit.h mod_arith_64bit.h mod_arith.h mod_arith_xmt.h
)
set_source_files_properties( make_graph.c graph_generator.c utils.c PROPERTIES LANGUAGE CXX )

add_definitions(
  -Drestrict=__restrict__
  -DGRAPH_GENERATOR_GRAPPA
  -DGRAPH_GENERATOR_SEQ
  -DGRAPHGEN_DISTRIBUTED_MEMORY
  -D_GRAPPA
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")

add_library(generator ${SOURCES})
