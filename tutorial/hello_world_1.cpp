///////////////////////////////
// tutorial/hello_world_1.cpp
///////////////////////////////
#include <Grappa.hpp>
#include <iostream>
int main(int argc, char *argv[]) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{
    std::cout << "Hello world from the root task!\n";
  });
  Grappa::finalize();
}
