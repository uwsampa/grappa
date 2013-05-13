#ifndef RELATIONIO_HPP
#define RELATIONIO_HPP

#include <string>
#include <fstream>
#include <vector>

#include <Grappa.hpp>
#include <Cache.hpp>
#include <ParallelLoop.hpp>
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
  CHECK( testfile.is_open() );

  // shared by the local tasks reading the file
  int fin = 0;

  // synchronous IO with asynchronous write
  Grappa::forall_here<&Grappa::impl::local_ce, 1>(0, numTuples, [tuples,numTuples,&fin,&testfile](int64_t s, int64_t n) {
    std::string line;
    for (int ignore=s; ignore<s+n; ignore++) {
      CHECK( testfile.good() );
      std::getline( testfile, line );

      int myindex = fin++;
      Incoherent<Tuple>::WO lr(tuples+myindex, 1);

      std::vector<std::string> tokens = split( line, '\t' );
      VLOG(5) << tokens[0] << "->" << tokens[1];
      (*lr).columns[0] = std::stoi(tokens[0]);
      (*lr).columns[1] = std::stoi(tokens[1]);
    }
  });

  CHECK( fin == numTuples );
  testfile.close();
}

#endif // RELATIONIO_HPP
