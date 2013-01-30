
file(GLOB TEST_SOURCES
  "*_tests.cpp"
)

# file(GLOB SYSTEM_SOURCES
#   "*(!_tests).cpp"
#   "tasks/*.cpp"
# )
# file(GLOB SYSTEM_HEADERS
#   "*(_tests)!.hpp"
#   "tasks/*.hpp"
# )

set(SYSTEM_SOURCES
  Addressing.hpp
  Aggregator.cpp
  Aggregator.hpp
  Allocator.cpp
  Allocator.hpp
  Array.hpp
  AsyncParallelFor.cpp
  AsyncParallelFor.hpp
  Cache.cpp
  Cache.hpp
  Collective.cpp
  Collective.hpp
  Communicator.cpp
  Communicator.hpp
  ConditionVariable.hpp
  ConditionVariableLocal.hpp
  CurrentThread.cpp
  CurrentThread.hpp
  Delegate.cpp
  Delegate.hpp
  Doxyfile
  FileIO.hpp
  ForkJoin.cpp
  ForkJoin.hpp
  FullEmpty.hpp
  Future.cpp
  Future.hpp
  GlobalAllocator.cpp
  GlobalAllocator.hpp
  GlobalMemory.cpp
  GlobalMemory.hpp
  GlobalMemoryChunk.cpp
  GlobalMemoryChunk.hpp
  GlobalTaskJoiner.cpp
  GlobalTaskJoiner.hpp
  Grappa.cpp
  Grappa.hpp
  IncoherentAcquirer.cpp
  IncoherentAcquirer.hpp
  IncoherentReleaser.cpp
  IncoherentReleaser.hpp
  Message.hpp
  MessageBase.hpp
  MutableHeap.hpp
  Mutex.hpp
  ParallelLoop.hpp
  ParallelLoop_future.hpp
  PerformanceTools.cpp
  PerformanceTools.hpp
  ProfilerConfig.hpp
  ProfilerGroups.cpp
  ProfilerGroups.hpp
  PushBuffer.hpp
  StateTimer.cpp
  StateTimer.hpp
  Statistics.hpp
  Statistics.cpp
  StatisticsTools.hpp
  Synchronization.hpp
  Tasking.hpp
  Timestamp.cpp
  Timestamp.hpp
  Uid.cpp
  Uid.hpp
  boost_helpers.hpp
  common.hpp
  coro.cpp
  coro.h
  stack.S
  stack.h
  tasks/BasicScheduler.cpp
  tasks/BasicScheduler.hpp
  tasks/DictOut.hpp
  tasks/Scheduler.hpp
  tasks/StealQueue.hpp
  tasks/Task.cpp
  tasks/Task.hpp
  tasks/TaskingScheduler.cpp
  tasks/TaskingScheduler.hpp
  tasks/Thread.cpp
  tasks/Thread.hpp
)


if( GOOGLE_PROFILER )
  list(APPEND GRAPPA_LIBS "profiler")
endif()

# (a bit of a hack) without this, stack.o isn't generated
# ... could also add custom command, but this seems easier
set_source_files_properties( stack.S PROPERTIES LANGUAGE C )

add_library(Grappa
  ${SYSTEM_SOURCES}
  ${SYSTEM_HEADERS}
)
target_link_libraries(Grappa ${GRAPPA_LIBS})

set(GRAPPA_TESTS "")

# create targets for each test
foreach(test_cpp ${TEST_SOURCES})
  get_filename_component(test_name ${test_cpp} NAME_WE) # name without extension
  set(test "${test_name}.test")

  # include Boost test automagic main() generation
  # add an additional rule if you need to include multiple components

  add_executable(${test} EXCLUDE_FROM_ALL ${test_cpp})
  target_link_libraries(${test} Grappa)
  set_property(TARGET ${test} PROPERTY FOLDER "Tests") # For organization in Xcode project
  set_target_properties(${test} PROPERTIES
    COMPILE_FLAGS "-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=${test_name}"
  )

  # TODO: use "ctest" to make whole regression test suite
  add_custom_target("check-${test_name}"
    COMMAND ${CMAKE_SOURCE_DIR}/bin/grappa_srun.sh -- "${test_name}.test" --log_level=nothing --report_level=no --run_test="${test_name}"
    DEPENDS Grappa "${test_name}.test"
  )
  set_property(TARGET "check-${test_name}" PROPERTY FOLDER "Tests")
  list(APPEND GRAPPA_TESTS "check-${test_name}")
endforeach()

add_custom_target("check-all" DEPENDS ${GRAPPA_TESTS})
