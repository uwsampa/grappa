///////////////////////////////
// tutorial/hello_world_1.cpp
///////////////////////////////
#include <Grappa.hpp>
#include <iostream>
int main(int argc, char *argv[]) {
  // this code is running on all cores
  
  // initialize Grappa
  Grappa::init(&argc, &argv);

  // spawn the root task
  Grappa::run([]{
    // this code is running as a task on a single core
    std::cout << "Hello world from the root task!\n";
  });

  // shutdown Grappa
  Grappa::finalize();
}
