#pragma once

#include <string>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
//#include <regex>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <Grappa.hpp>
#include <Cache.hpp>
#include <ParallelLoop.hpp>
#include "Tuple.hpp"
#include "relation.hpp"

#include "grappa/graph.hpp"

DECLARE_string(relations);
DECLARE_bool(bin);


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
      Grappa::delegate::write<Grappa::async>(edges+myindex, pe);
    }
  });


  CHECK( fin == numTuples );
  testfile.close();

  tuple_graph tg { edges, numTuples };
  return tg;
}


// assumes that for object T, the address of T is the address of its fields
template <typename T>
size_t readTuplesUnordered( std::string fn, GlobalAddress<T> * buf_addr, int64_t numfields ) {
  /*
  std::string metadata_path = FLAGS_relations+"/"+fn+"."+metadata; //TODO replace such metadatafiles with a real catalog
  std::ifstream metadata_file(metadata_path, std::ifstream::in);
  CHECK( metadata_file.is_open() ) << metadata_path << " failed to open";
  int64_t numcols;
  metadata_file >> numcols;  
  */

  // binary; TODO: factor out to allow other formats like fixed-line length ascii

// we get just the size of the fields (since T is a padded data type)
  size_t row_size_bytes = sizeof(int64_t) * numfields;
  VLOG(2) << "row_size_bytes=" << row_size_bytes;
  std::string data_path = FLAGS_relations+"/"+fn;
  size_t file_size = fs::file_size( data_path );
  size_t ntuples = file_size / row_size_bytes;
  CHECK( ntuples * row_size_bytes == file_size ) << "File is ill-formatted; perhaps not all rows have same columns?";
  VLOG(1) << fn << " has " << ntuples << " rows";
  
  auto tuples = Grappa::global_alloc<T>(ntuples);
  
  size_t offset_counter;
  auto offset_counter_addr = make_global( &offset_counter, Grappa::mycore() );

  // we will broadcast the file name as bytes
  CHECK( data_path.size() <= 2040 );
  char data_path_char[2048];
  sprintf(data_path_char, "%s", data_path.c_str());

  on_all_cores( [=] {
    VLOG(5) << "opening addr next";
    VLOG(5) << "opening addr " << &data_path_char; 
    VLOG(5) << "opening " << data_path_char; 

    // find my array split
    auto local_start = tuples.localize();
    auto local_end = (tuples+ntuples).localize();
    size_t local_count = local_end - local_start;

    // reserve a file split
    int64_t offset = Grappa::delegate::fetch_and_add( offset_counter_addr, local_count );
    VLOG(2) << "file offset " << offset;
  
    std::ifstream data_file(data_path_char, std::ios_base::in | std::ios_base::binary);
    CHECK( data_file.is_open() ) << data_path_char << " failed to open";
    VLOG(5) << "seeking";
    data_file.seekg( offset * row_size_bytes );
    VLOG(5) << "reading";
    data_file.read( (char*) local_start, local_count * row_size_bytes );
    data_file.close();

    // expand packed data into T's if necessary
    if (row_size_bytes < sizeof(T)) {
      VLOG(5) << "inflating";
      char * byte_ptr = reinterpret_cast<char*>(local_start);
      // go backwards so we never overwrite
      for( int64_t i = local_count - 1; i >= 0; --i ) {
        char * data = byte_ptr + i * row_size_bytes;
        memcpy(&local_start[i], data, row_size_bytes);
      }
    }

    VLOG(4) << "local first row: " << *local_start;
  });

  *buf_addr = tuples;
  return ntuples;
}

// convenient version for Relation<T> type
template <typename T>
Relation<T> readTuplesUnordered( std::string fn ) {
  GlobalAddress<T> tuples;
  
  T sample;
  CHECK( reinterpret_cast<char*>(&sample._fields) == reinterpret_cast<char*>(&sample) ) << "IO assumes _fields is the first field, but it is not for T";

  auto ntuples = readTuplesUnordered<T>( fn, &tuples, sizeof(sample._fields)/sizeof(int64_t) );  
  Relation<T> r = { tuples, ntuples };
  return r;
}

// assumes that for object T, the address of T is the address of its fields
template <typename T>
void writeTuplesUnordered(std::vector<T> * vec, std::string fn ) {
  std::string data_path = FLAGS_relations+"/"+fn;
  
  // we will broadcast the file name as bytes
  CHECK( data_path.size() <= 2040 );
  char data_path_char[2048];
  sprintf(data_path_char, "%s", data_path.c_str());

  on_all_cores( [=] {
    VLOG(5) << "opening addr next";
    VLOG(5) << "opening addr " << &data_path_char; 
    VLOG(5) << "opening " << data_path_char; 

    std::ofstream data_file(data_path_char, std::ios_base::out | std::ios_base::app | std::ios_base::binary);
    CHECK( data_file.is_open() ) << data_path_char << " failed to open";
    VLOG(5) << "writing";

    for (auto it = vec->begin(); it < vec->end(); it++) {
      for (int j = 0; j < it->numFields(); j++) {
	int64_t val = it->get(j);
	data_file.write((char*)&val, sizeof(val));
      }
    }

    data_file.close();
    });
}

void writeSchema(std::string names, std::string types, std::string fn ) {
  std::string data_path = FLAGS_relations+"/"+fn;
  
  CHECK( data_path.size() <= 2040 );
  char data_path_char[2048];
  sprintf(data_path_char, "%s", data_path.c_str());

  std::ofstream data_file(data_path_char, std::ios_base::out);
  CHECK( data_file.is_open() ) << data_path_char << " failed to open";
  VLOG(5) << "writing";

  data_file << names << "\n" << types << std::endl;
  data_file.close();
}
  
int64_t toInt(std::string& s) {
  return std::stoi(s);
}
double toDouble(std::string& s) {
  return std::stod(s);
}
#include <boost/tokenizer.hpp>
template< typename N=int64_t, typename Parser=decltype(toInt) >
void convert2bin( std::string fn, Parser parser=&toInt, char * separators=" ", uint64_t burn=0 ) {
  std::ifstream infile(fn, std::ifstream::in);
  CHECK( infile.is_open() ) << fn << " failed to open";
  
  std::string outpath = fn+".bin";
  std::ofstream outfile(outpath, std::ios_base::out | std::ios_base::binary );
  CHECK( outfile.is_open() ) << outpath << " failed to open";
    
  std::string line;
  int64_t expected_numcols = -1;
  uint64_t linenum = 0;
  while( infile.good() ) {
    std::getline( infile, line );

    std::vector<N> readFields;

    uint64_t j = 0;
    boost::char_separator<char> sep(separators);
    boost::tokenizer<boost::char_separator<char>> tk (line, sep);
    for (boost::tokenizer<boost::char_separator<char>>::iterator i(tk.begin());
        i!=tk.end();++i) {
      if (j++ >= burn) { 
        std::string s(*i);
        N f = parser(s);
        readFields.push_back(f);
      }
    }
    
    CHECK(expected_numcols > 0 || readFields.size() > 0) << "first line had 0 columns";
    if (readFields.size() == 0) break; // takes care of EOF

    if (expected_numcols < 0) { 
      expected_numcols = readFields.size();
      std::cout << expected_numcols << " cols from example " << readFields << std::endl;
    }
    CHECK (expected_numcols == readFields.size()) << "line " << linenum 
                                                  << " does not have " << expected_numcols << " columns";


    outfile.write((char*) &readFields[0], sizeof(int64_t)*readFields.size()); 
    ++linenum;
  }

  infile.close();
  outfile.close();
  std::cout << "binary: " << outpath << std::endl;
  std::cout << "rows: " << linenum << std::endl;
  std::cout << "cols: " << expected_numcols << std::endl;

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


