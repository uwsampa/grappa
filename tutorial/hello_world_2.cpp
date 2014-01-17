///////////////////////////////
// tutorial/hello_world_2.cpp
///////////////////////////////
#include <Grappa.hpp>
#include <Collective.hpp>
#include <iostream>
int main(int argc, char *argv[]) {

  Grappa::init(&argc, &argv);

  Grappa::run([]{
    std::cout << "Hello world from the root task!\n";

    // SPMD execution on all cores
    Grappa::on_all_cores([]{
      std::cout << "Hello world from Core " << Grappa::mycore() << " of " << Grappa::cores()
                << " (locale " << Grappa::mylocale() << ")"<< "\n";
    });
    std::cout << "Exiting root task.\n";
  });

  Grappa::finalize();
}
