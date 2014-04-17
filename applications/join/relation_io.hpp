#pragma once

#include <string>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
//#include <regex>

#include <Grappa.hpp>
#include <Cache.hpp>
#include <ParallelLoop.hpp>
#include "Tuple.hpp"

#include "grappa/graph.hpp"

DECLARE_string(relations);


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

/// Read tuples of format
/// src dst
/// 
/// create as array of Tuple
void readEdges( std::string fn, GlobalAddress<Tuple> tuples, uint64_t numTuples ) {
  std::ifstream testfile(fn);
  CHECK( testfile.is_open() );

  // shared by the local tasks reading the file
  int fin = 0;

  // synchronous IO with asynchronous write
  Grappa::forall_here<1>(0, numTuples, [tuples,numTuples,&fin,&testfile](int64_t s, int64_t n) {
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


/// Read tuples of format
/// src dst
/// 
/// create as tuple_graph
tuple_graph readEdges( std::string fn, int64_t numTuples ) {
  std::ifstream testfile(fn, std::ifstream::in);
  CHECK( testfile.is_open() );

  // shared by the local tasks reading the file
  int64_t fin = 0;

  auto edges = Grappa::global_alloc<packed_edge>(numTuples);
 
  // token delimiter  
//  std::regex rgx("\\s+");

  // synchronous IO with asynchronous write
  Grappa::forall_here<&Grappa::impl::local_gce, 1>(0, numTuples, [edges,numTuples,&fin,&testfile](int64_t s, int64_t n) {
    std::string line;
    for (int ignore=s; ignore<s+n; ignore++) {
      CHECK( testfile.good() );
      std::getline( testfile, line );

      // parse and create the edge
      // c++11 but NOT SUPPORTED/broken in gcc4.7.2
//      std::regex_token_iterator<std::string::iterator> iter(line.begin(), line.end(),
//                                     rgx, -1); // -1 = matches as splitters
//      auto src = std::stoi(*iter); iter++;
//      auto dst = std::stoi(*iter);
      std::stringstream ss(line);
      std::string buf;
      ss >> buf; 
      auto src = std::stoi(buf);
      ss >> buf;
      auto dst = std::stoi(buf);
      
      VLOG(5) << src << "->" << dst;
      packed_edge pe;
      write_edge(&pe, src, dst);

      // write edge to location
      int myindex = fin++;
      Grappa::delegate::write<async>(edges+myindex, pe);
    }
  });


  CHECK( fin == numTuples );
  testfile.close();

  tuple_graph tg { edges, numTuples };
  return tg;
}



template <typename T>
Relation<T> readTuplesUnordered( std::string fn ) {
  /*
  std::string metadata_path = FLAGS_relations+"/"+fn+"."+metadata; //TODO replace such metadatafiles with a real catalog
  std::ifstream metadata_file(metadata_path, std::ifstream::in);
  CHECK( metadata_file.is_open() ) << metadata_path << " failed to open";
  int64_t numcols;
  metadata_file >> numcols;  
  */

  // binary; TODO: factor out to allow other formats like fixed-line length ascii
  std::string data_path = FLAGS_relations+"/"+fn;
  size_t file_size = fs::file_size( path );
  size_t ntuples = file_size / row_size_bytes; 
  CHECK( ntuples * row_size_bytes == file_size ) << "File is ill-formatted; perhaps not all rows have " << numcols << " columns?";
  
  auto tuples = Grappa::global_alloc<T>(numTuples);
  
  size_t offset_counter;
  auto offset_counter_addr = make_global( &offset_counter, Grappa::mycore() );
  on_all_cores( [=] {
    // find my array split
    auto local_start = tuples.localize();
    auto local_end = (tuples+numTuples).localize();
    size_t local_count = local_end - local_start;

    // reserve a file split
    int64_t offset = Grappa::delegate::fetch_and_add( offset_counter_addr, local_count )

    std::ifstream data_file(data_path, std::ifstream::in | std::ios_base::binary);
    CHECK( data_file.is_open() ) << data_path << " failed to open";
    infile.seekg( offset * sizeof(T) );
    infile.read( (char*) local_start, local_count * sizeof(T) ) 
    data_file.close();
  });

  Relation<T> r = { tuples, ntuples };
  return r;
}

template <typename T>
GlobalAddress<T> readTuples( std::string fn, int64_t numTuples ) {
  std::string path = FLAGS_relations+"/"+fn;
  std::ifstream testfile(path, std::ifstream::in);
  CHECK( testfile.is_open() ) << path << " failed to open";

  // shared by the local tasks reading the file
  int64_t fin = 0;

  auto tuples = Grappa::global_alloc<T>(numTuples);
 
  // token delimiter  
//  std::regex rgx("\\s+");

  // synchronous IO with asynchronous write
  Grappa::forall_here<&Grappa::impl::local_gce, 1>(0, numTuples, [tuples,numTuples,&fin,&testfile](int64_t s, int64_t n) {
    std::string line;
    for (int ignore=s; ignore<s+n; ignore++) {
      CHECK( testfile.good() );
      std::getline( testfile, line );

      std::vector<int64_t> readFields;
      
      // TODO: compiler should use catalog to statically insert num fields
      std::stringstream ss(line);
      while (true) {
        std::string buf;
        ss >> buf; 
        if (buf.compare("") == 0) break;

        auto f = std::stoi(buf);
        readFields.push_back(f);
      }

      T val( readFields );
      
      VLOG(5) << val;

      // write edge to location
      int myindex = fin++;
      Grappa::delegate::write<async>(tuples+myindex, val);
    }
  });


  CHECK( fin == numTuples );
  testfile.close();

  return tuples;
}


