set(COMMON graphlab.hpp graphlab.cpp)

foreach(app pagerank sssp test pagerank_new cc bfs)
  add_grappa_exe(graphlab-${app} ${app}.exe ${app}.cpp ${COMMON})
  set_property(TARGET ${name} PROPERTY FOLDER "Graphlab")
endforeach()
