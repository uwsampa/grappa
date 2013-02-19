set(SOURCES
  Reduction.hpp
  spmv_mult.cpp
  spmv_mult.hpp
  ../graph500/grappa/oned_csr.cpp
)

add_grappa_application(pagerank
  ${SOURCES} pagerank.cpp
)
target_link_libraries(pagerank generator)

add_grappa_application(mult
  ${SOURCES} mult_main.cpp
)
target_link_libraries(mult generator)
