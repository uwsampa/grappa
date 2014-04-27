#include "relation_io.hpp"

int main(int argc, char** argv) {

  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " FILE TYPE{i,d} BURNS" << std::endl;
    exit(1);
  }
  
  if (strncmp(argv[2], "i", 1) == 0) {
    convert2bin<int64_t,decltype(&toInt)>( argv[1], &toInt, atoi(argv[3]) );
  } else if (strncmp(argv[2], "d", 1) == 0) {
    convert2bin<double,decltype(&toDouble)>( argv[1], &toDouble, atoi(argv[3]) );
  } else {
    std::cerr << "unrecognized type " << argv[2] << std::endl;
  }
}
