add_grappa_application(gups.exe gups.cpp)
add_grappa_application(gups-bidir.exe gups-bidir.cpp)
add_grappa_application(gups-blocking.exe gups-blocking.cpp)
add_grappa_application(gups-getput.exe gups-getput.cpp)

add_grappa_application(scurve.exe scurve.cpp)
# If MPI doesn't link in ibverbs, we may have to do it ourselves .
#list(APPEND GRAPPA_LIBS ibverbs)

add_grappa_application(gups-verbs.exe gups-verbs.cpp Verbs.cpp)
