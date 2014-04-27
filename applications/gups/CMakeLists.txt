add_grappa_application(gups.exe gups.cpp)

# If MPI doesn't link in ibverbs, we may have to do it ourselves .
#list(APPEND GRAPPA_LIBS ibverbs)

add_grappa_application(gups-verbs.exe gups-verbs.cpp Verbs.cpp)
