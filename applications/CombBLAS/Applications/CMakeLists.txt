# Top level directory has the include files

ADD_EXECUTABLE( mcl MCL.cpp )
ADD_EXECUTABLE( graph500 Graph500.cpp )
ADD_EXECUTABLE( betwcent BetwCent.cpp )
ADD_EXECUTABLE( fbfs FilteredBFS.cpp )
ADD_EXECUTABLE( fmis FilteredMIS.cpp )


TARGET_LINK_LIBRARIES( mcl CommGridlib MPITypelib MemoryPoollib )
TARGET_LINK_LIBRARIES( graph500 CommGridlib MPITypelib MemoryPoollib GraphGenlib)
TARGET_LINK_LIBRARIES( betwcent CommGridlib MPITypelib MemoryPoollib GraphGenlib)
TARGET_LINK_LIBRARIES( fbfs CommGridlib MPITypelib MemoryPoollib GraphGenlib)
TARGET_LINK_LIBRARIES( fmis CommGridlib MPITypelib MemoryPoollib GraphGenlib)


