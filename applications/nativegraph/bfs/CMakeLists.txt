set(SOURCES main.cpp common.hpp)

add_grappa_application(bfs_queues.exe bfs_queues.cpp ${SOURCES})
add_grappa_application(bfs_spmd.exe bfs_spmd.cpp ${SOURCES})
add_grappa_application(bfs_beamer.exe bfs_beamer.cpp ${SOURCES})
