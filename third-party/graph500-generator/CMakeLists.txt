
set(GENERATOR_SOURCES
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

add_library(graph500-generator STATIC ${GENERATOR_SOURCES})
set_target_properties(graph500-generator PROPERTIES
  COMPILE_FLAGS "-Drestrict=__restrict__ -DGRAPH_GENERATOR_SEQ -ffast-math ${STATIC_FLAGS}"
  FOLDER "Third Party"
)

install(TARGETS graph500-generator DESTINATION "lib")
