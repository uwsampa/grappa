set(SOURCES
  spmv_mult.cpp
  spmv_mult.hpp
)

add_definitions(-Drestrict=__restrict__ -DGRAPH_GENERATOR_GRAPPA -D_GRAPPA)

add_grappa_application(pagerank_old.exe
  ${SOURCES}
  pagerank_old.cpp
  ../graph500/grappa/oned_csr.cpp
)
target_link_libraries(pagerank_old.exe generator)

add_grappa_application(pagerank.exe ${SOURCES} pagerank.cpp)

add_grappa_application(mult.exe
  ${SOURCES} mult_main.cpp
)
target_link_libraries(mult.exe generator)
