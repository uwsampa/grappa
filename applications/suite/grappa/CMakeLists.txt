set(SOURCES
  main.cpp
  defs.hpp
  timer.cpp
  ../compat/mersenne.h
  ../compat/mersenne.c
  centrality.cpp
  computeGraph.cpp
  connectedComponents.cpp
  genScalData.cpp
  graphManip.cpp
  pathIsomorphism.cpp
  triangles.cpp
)

add_grappa_application(suite.exe ${SOURCES})
