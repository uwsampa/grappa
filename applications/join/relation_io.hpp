#pragma once

#include <string>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdio>
#include "json/json.h"
//#include <regex>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <Grappa.hpp>
#include <Cache.hpp>
#include <ParallelLoop.hpp>
#include "Tuple.hpp"
#include "relation.hpp"

#include "strings.h"

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
      Grappa::delegate::write<async>(edges+myindex, pe);
    }
  });


  CHECK( fin == numTuples );
  testfile.close();

  tuple_graph tg { edges, numTuples };
  return tg;
}


std::string get_split_name(const std::string& base, int part) {
  const int digits = 5;
  assert(part <= 99999);

  std::stringstream ss;
  ss << base << "/part-" << std::setw(digits) << std::setfill('0') << part;
  return ss.str();
}

size_t get_lines( const std::string& fname ) {
#if 0
  // POPEN and MPI don't mix well
  char cmd[256];
  sprintf(cmd, "wc -l %s | awk '{print $1}'", fname.c_str());
  FILE* pipe = popen(cmd, "r");
  if (!pipe) return -1;
  char buffer[128];
  std::string result = "";
  while(!feof(pipe)) {
    if(fgets(buffer, 128, pipe) != NULL)
      result += buffer;
  }
  pclose(pipe);
  return std::stoi(result);
#endif
    size_t count = 0;
    std::ifstream inp(fname);
    std::string line;
    while (std::getline(inp, line)) {
      ++count;
    }
    return count;
}


size_t get_total_lines( const std::string& basename ) {
#if 0
  // POPEN and MPI don't mix well
  char cmd[128];
  sprintf(cmd, "wc -l %s-* | tail -n 1 | awk '{print $1}'", basename.c_str());
  FILE* pipe = popen(cmd, "r");
  CHECK( pipe != NULL);
  if (!pipe) return -1;
  char buffer[128];
  std::string result = "";
  while(!feof(pipe)) {
    if(fgets(buffer, 128, pipe) != NULL)
      result += buffer;
  }
  CHECK( pclose(pipe) != -1 );
  return std::stoi(result);
#endif
  int i = 0;
  int64_t sum = 0;
  while (true) {
    auto fname = get_split_name(basename, i);
    std::ifstream inp(fname);
    // if file doesn't exist then stop
    if (!inp.good()) break; 
    inp.close();
    sum += get_lines(fname);
    ++i;
  }
  return sum;
}

// Wraps character array in a non-array type (non-pointer converted type) so that
// we can copy strings between cores
struct CharArray {
 char arr[180];
 CharArray() { } // no-arg constructor required for various object copying codes
 CharArray(std::string s) {
   CHECK(s.size() < 180) << "filenames limited to 180 bytes";
   sprintf(arr, "%s", s.c_str());
 }
};

template <typename T>
class RelationFileReader {

public:
  Relation<T> read( const std::string& base ) {
    GlobalAddress<T> tuples;

    T sample;
    CHECK( reinterpret_cast<char*>(&sample.f0) == reinterpret_cast<char*>(&sample) ) << "IO assumes f0 is the first field, but it is not for T";

    auto ntuples = this->_read( base, &tuples );
    Relation<T> r = { tuples, ntuples };
    return r;
  }
protected:
  virtual size_t _read( const std::string& basename, GlobalAddress<T>* buf_addr ) = 0;
};

template <typename T>
class RowParser {
public:
  virtual T parseRow(const std::string& line) = 0;
  virtual bool eof(const std::string& line) = 0;
};

template <typename T, std::vector<std::string>* Schema>
class JSONRowParser : public RowParser<T> {
private:
  bool eof(const std::string& line) {
    return line.length() < 4;
  }

  T parseRow(const std::string& line) {
    // get first attribute, which is a json string
    std::stringstream inss(line);
    Json::Value root;
    inss >> root;  // ignore other json objects

    VLOG(5) << root;

    // json to csv to use fromIStream
    std::stringstream ascii_s;

    // this way is broken because it doesn't regain order
    /*
    for ( Json::ValueIterator itr = root.begin(); itr != root.end(); itr++ ) {
      char truncated[MAX_STR_LEN-1];
      strncpy(truncated, itr->asString().c_str(), MAX_STR_LEN-2);
      ascii_s << truncated << ",";
    }
     */

    for (auto name : *Schema) {
      char truncated[MAX_STR_LEN-1];
      strncpy(truncated, root[name].asString().c_str(), MAX_STR_LEN-2);
      ascii_s << truncated << ",";
    }

    VLOG(5) << ascii_s.str();

    return T::fromIStream(ascii_s, ',');
  }
};

template <typename Parser, typename T>
class SplitsRelationFileReader : public RelationFileReader<T> {
protected:
  size_t _read( const std::string& basename, GlobalAddress<T> * buf_addr ) {
    uint64_t part = 0;
    auto part_addr = make_global(&part);
    bool done = false;
    auto done_addr = make_global(&done);
    uint64_t offset_counter = 0;
    auto offset_counter_addr = make_global( &offset_counter );

    auto ntuples = get_total_lines(basename);
    CHECK(ntuples >= 0);
    auto tuples = Grappa::global_alloc<T>(ntuples);

    // choose to new here simply to save stack space
    auto basename_ptr = make_global(new CharArray(basename));

    on_all_cores( [part_addr,done_addr,offset_counter_addr,tuples,basename_ptr] {
      auto fname_arr = delegate::read(basename_ptr);
      auto basename_copy = std::string(fname_arr.arr);

      VLOG(5) << "readSplits reporting in";
      while (!delegate::read(done_addr)) {
        auto my_part = delegate::fetch_and_add( part_addr, 1 );
        auto fname = get_split_name(basename_copy, my_part);

        VLOG(5) << "split " << fname;
        std::ifstream inp(fname);
        if (!inp.good()) {
          delegate::call(done_addr.core(), [=] {
            *(done_addr.pointer()) = true;
          });
        } else {
          // two pass; count lines in part then reserve and read
          auto nlines = get_lines(fname);

          auto offset = delegate::fetch_and_add( offset_counter_addr, nlines );
          auto suboffset = 0;

          std::string line;
          Parser parser;
          while (std::getline(inp, line)) {
            // check other EOF conditions, like empty line
            if (parser.eof(line)) break;

            auto val = parser.parseRow(line);

            //Grappa::delegate::write<async>(tuples+offset+suboffset, val);
            Grappa::delegate::write(tuples+offset+suboffset, val);
            ++suboffset;
          }
        }
      }
    });
//  Grappa::impl::local_gce.wait();
    delete basename_ptr.pointer();

    *buf_addr = tuples;
    return ntuples;
  }
};



// assumes that for object T, the address of T is the address of its fields
template <typename T>
class BinaryRelationFileReader : public RelationFileReader<T> {
protected:
  size_t _read( const std::string& fn, GlobalAddress<T> * buf_addr ) {
    /*
    std::string metadata_path = FLAGS_relations+"/"+fn+"."+metadata; //TODO replace such metadatafiles with a real catalog
    std::ifstream metadata_file(metadata_path, std::ifstream::in);
    CHECK( metadata_file.is_open() ) << metadata_path << " failed to open";
    int64_t numcols;
    metadata_file >> numcols;
    */

    // binary; TODO: factor out to allow other formats like fixed-line length ascii

// we get just the size of the fields (since T is a padded data type)
    size_t row_size_bytes = T::fieldsSize();
    VLOG(2) << "row_size_bytes=" << row_size_bytes;
    std::string data_path = FLAGS_relations+"/"+fn;
    size_t file_size = fs::file_size( data_path );
    size_t ntuples = file_size / row_size_bytes;
    CHECK( ntuples * row_size_bytes == file_size ) << "File " << data_path << " is ill-formatted; perhaps not all rows have same columns? file size = " << file_size << " row_size_bytes = " << row_size_bytes;
    VLOG(1) << fn << " has " << ntuples << " rows";

    auto tuples = Grappa::global_alloc<T>(ntuples);

    size_t offset_counter = 0;
    auto offset_counter_addr = make_global( &offset_counter, Grappa::mycore() );

    auto data_path_ptr = make_global(new CharArray(data_path));

    on_all_cores( [=] {
      auto data_path_arr = delegate::read(data_path_ptr);
      auto fname = std::string(data_path_arr.arr);

      // find my array split
      auto local_start = tuples.localize();
      auto local_end = (tuples+ntuples).localize();
      size_t local_count = local_end - local_start;

      // reserve a file split
      int64_t offset = Grappa::delegate::fetch_and_add( offset_counter_addr, local_count );
      VLOG(2) << "file offset " << offset;

      std::ifstream data_file(fname, std::ios_base::in | std::ios_base::binary);
      CHECK( data_file.is_open() ) << fname << " failed to open";
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

    delete data_path_ptr.pointer();

    *buf_addr = tuples;
    return ntuples;
  }
};

// assumes that for object T, the address of T is the address of its fields
template <typename T>
void writeTuplesUnordered(std::vector<T> * vec, std::string fn ) {
  std::string data_path = FLAGS_relations+"/"+fn;
  
  std::ofstream for_trunc(data_path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
  //no writes
  for_trunc.close();
  
  // we will broadcast the file name as bytes
  CharArray data_path_arr(data_path);
    
  // sequentiall open for append and write
  for (int i=0; i<Grappa::cores(); i++) {
    int ignore = delegate::call(i, [=] {
        auto fname = data_path_arr.arr;
        std::ofstream data_file(fname, std::ios_base::out | std::ios_base::app | std::ios_base::binary);
        CHECK( data_file.is_open() ) << fname << " failed to open";
        VLOG(4) << "writing";

        for (auto it = vec->begin(); it < vec->end(); it++) {
        it->toOStream(data_file);
        }

        data_file.close();
        return 1;
        });
  }
}

void writeSchema(std::string scheme, std::string fn ) {
  std::string data_path = FLAGS_relations+"/"+fn;
  
  CHECK( data_path.size() <= 2040 );
  char data_path_char[2048];
  sprintf(data_path_char, "%s", data_path.c_str());

  std::ofstream data_file(data_path_char, std::ios_base::out);
  CHECK( data_file.is_open() ) << data_path_char << " failed to open";
  VLOG(5) << "writing";

  data_file << scheme << std::endl;
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
      CHECK( testfile.good() ) << "index=" << ignore << " start=" << s;
      std::getline( testfile, line );
      if(line.length()==0) break; //takes care of EOF

      std::istringstream iss(line);
      auto val = T::fromIStream(iss);

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


