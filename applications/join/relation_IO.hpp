#ifndef RELATIONIO_HPP
#define RELATIONIO_HPP

#include <string>
#include <fstream>
#include <vector>

#include <Grappa.hpp>
#include <Cache.hpp>
#include "Tuple.hpp"



std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while(std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  return split(s, delim, elems);
}


void readTuples( std::string fn, GlobalAddress<Tuple> tuples, uint64_t numTuples ) {
std::ifstream testfile(fn);
    std::string line;
    int fin = 0;
    CHECK( testfile.is_open() );

    while (testfile.good() && fin<numTuples) {
      std::getline( testfile, line );
      Incoherent<Tuple>::WO lr(tuples+fin, 1);
      std::vector<std::string> tokens = split( line, '\t' );
      VLOG(5) << tokens[0] << "->" << tokens[1];
      (*lr).columns[0] = std::stoi(tokens[0]);
      (*lr).columns[1] = std::stoi(tokens[1]);
      fin++;
    }
    CHECK( fin == numTuples );
    testfile.close();
}

#endif // RELATIONIO_HPP
