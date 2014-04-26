#include "relation_io.hpp"

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " FILE" << std::endl;
    exit(1);
  }

  convert2bin( argv[1] );
}
