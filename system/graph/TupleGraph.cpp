////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#define _FILE_OFFSET_BITS 64
  
#include "TupleGraph.hpp"
#include "ParallelLoop.hpp"
#include "FileIO.hpp"
#include "Delegate.hpp"

#include <fstream>
#include <vector>

DEFINE_bool( use_mpi_io, false, "Use MPI IO optimizations" );

/// for now, limit path lengths to this
const size_t max_path_length = 1024;

/// this is the representation of bintsv4 edges
struct Int32Edge { int32_t v0, v1; };

/// this helps compute offsets when loading in parallel
/// TODO: replace with scan operation once we're using MPI non-blocking collectives
static int64_t local_offset = 0;

namespace Grappa {

/// helper method for parallel load of a single file
TupleGraph TupleGraph::load_generic( std::string path, void (*f)( const char *, Edge*, Edge*) ) {
  // make sure file exists
  CHECK( fs::exists( path ) ) << "File not found.";
  CHECK( fs::is_regular_file( path ) ) << "File is not a regular file.";

  size_t file_size = fs::file_size( path );
  size_t nedge = file_size / (2 * sizeof(int32_t)); 
  
  TupleGraph tg( nedge );

  size_t path_length = path.size() + 1; // include space for terminator

  CHECK_LT( path_length, max_path_length )
    << "Sorry, filename exceeds preset limit. Please change max_path_length constant in this file and rerun.";

  char filename[ max_path_length ];
  strncpy( &filename[0], path.c_str(), max_path_length );

  Core mycore = Grappa::mycore();
  auto edges = tg.edges;

  on_all_cores( [=] {
      f( filename, edges.localize(), (edges+nedge).localize() );
    } );

  // done!
  return tg;
}


///
/// reading
///

/// helper function run on each core to load edges stored as int32_t
/// tuples in bintsv4 format
void local_load_bintsv4( const char * filename,
                         Grappa::TupleGraph::Edge * local_ptr,
                         Grappa::TupleGraph::Edge * local_end ) {
  Int32Edge * local_load_ptr = reinterpret_cast<Int32Edge*>(local_ptr);
  size_t local_count = local_end - local_ptr;
  
  if( !FLAGS_use_mpi_io ) {
    // use standard C++/POSIX IO
    std::ifstream infile( filename, std::ios_base::in | std::ios_base::binary );

    // TODO: fix this with scan
    local_offset = 0; // do on all cores
    Grappa::barrier();
    int64_t offset = Grappa::delegate::fetch_and_add( make_global( &local_offset, 0 ),
                                              local_count );
    Grappa::barrier();

    infile.seekg( offset * sizeof(Int32Edge) );

    infile.read( (char*) local_ptr, local_count * sizeof(Int32Edge) );
  } else {
    // load int32's into local chunk
    impl::read_unordered_shared( filename, local_ptr, local_count * sizeof(Int32Edge) );
  }

  // expand int32's into int64's
  for( int64_t i = local_count - 1; i >= 0; --i ) {
    auto v0 = local_load_ptr[i].v0;
    auto v1 = local_load_ptr[i].v1;
    local_ptr[i].v0 = v0;
    local_ptr[i].v1 = v1;
  }
}


/// helper method for parallel load of a single file
static std::vector< Grappa::TupleGraph::Edge > read_edges;
TupleGraph TupleGraph::load_tsv( std::string path ) {
  // make sure file exists
  CHECK( fs::exists( path ) ) << "File not found.";
  CHECK( fs::is_regular_file( path ) ) << "File is not a regular file.";

  size_t file_size = fs::file_size( path );

  size_t path_length = path.size() + 1; // include space for terminator

  CHECK_LT( path_length, max_path_length )
    << "Sorry, filename exceeds preset limit. Please change max_path_length constant in this file and rerun.";

  char filename[ max_path_length ];
  strncpy( &filename[0], path.c_str(), max_path_length );

  Core mycore = Grappa::mycore();
  auto bytes_each_core = file_size / Grappa::cores();

  // read into temporary buffer
  on_all_cores( [=] {
      // use standard C++/POSIX IO

      // make one core take any data remaining after truncation
      auto my_bytes_each_core = bytes_each_core;
      if( Grappa::mycore() == 0 ) {
        my_bytes_each_core += file_size - (bytes_each_core * Grappa::cores());
      }
      
      // compute initial offset into ASCII file
      // TODO: fix this with scan
      local_offset = 0; // do on all cores
      Grappa::barrier();
      int64_t start_offset = Grappa::delegate::fetch_and_add( make_global( &local_offset, 0 ),
                                                              my_bytes_each_core );
      Grappa::barrier();
      int64_t end_offset = start_offset + my_bytes_each_core;

      DVLOG(7) << "Reading about " << my_bytes_each_core
               << " bytes starting at " << start_offset
               << " of " << file_size;
      
      // start reading at start offset
      std::ifstream infile( filename, std::ios_base::in );
      infile.seekg( start_offset );

      if( start_offset > 0 ) {
        // move past next newline so we start parsing from a record boundary
        std::string s;
        std::getline( infile, s );
        DVLOG(6) << "Skipped '" << s << "'";
      }
      
      start_offset = infile.tellg();
      DVLOG(6) << "Start reading at " << start_offset;

      // read up to one entry past the end_offset
      while( infile.good() && start_offset < end_offset ) {
        int64_t v0 = -1;
        int64_t v1 = -1;
        if( infile.peek() == '#' ) { // if a comment
          std::string str;
          std::getline( infile, str );
        } else {
          infile >> v0;
          if( !infile.good() ) break;
          infile >> v1;
          Edge e = { v0, v1 };
          DVLOG(6) << "Read " << v0 << " -> " << v1;
          read_edges.push_back( e );
          start_offset = infile.tellg();
        }
      }

      DVLOG(6) << "Done reading at " << start_offset << " end_offset " << end_offset;
      
      // collect sizes
      local_offset = read_edges.size();
      DVLOG(7) << "Read " << local_offset << " edges";
    } );

  auto nedge = Grappa::reduce<int64_t,collective_add>(&local_offset);
  
  TupleGraph tg( nedge );
  auto edges = tg.edges;
  //auto leftovers = GlobalVector<int64_t>::create(N);

  on_all_cores( [=] {
      Edge * local_ptr = edges.localize();
      Edge * local_end = (edges+nedge).localize();
      size_t local_count = local_end - local_ptr;
      size_t read_count = read_edges.size();

      DVLOG(7) << "local_count " << local_count
               << " read_count " << read_count;
      
      // copy everything in our read buffer that fits locally
      auto local_max = std::min( local_count, read_count );
      std::memcpy( local_ptr, &read_edges[0], local_max * sizeof(Edge) );
      local_offset = local_max;
      Grappa::barrier();

      // get rid of remaining edges
      auto mycore = Grappa::mycore();
      auto likely_consumer = (mycore + 1) % Grappa::cores();
      while( local_max < read_count ) {
        Edge e = read_edges[local_max];
        DVLOG(7) << "Looking for somewhere to place edge " << local_max;
        
        int retval = delegate::call( likely_consumer, [=] ()->int {
            Edge * local_ptr = edges.localize();
            Edge * local_end = (edges+nedge).localize();
            auto local_count = local_end - local_ptr;

            DVLOG(7) << "Trying to place edge " << local_max
                      << " on core " << Grappa::mycore()
                      << " with local_offset " << local_offset
                      << " and local_count " << local_count;
            
            // do we have space to insert here?
            if( local_offset < local_count ) {
              // yes, so do so
              local_ptr[local_offset] = e;
              local_offset++;

              if( local_offset < local_count ) {
                DVLOG(7) << "Succeeded with space available";
                return 0; // succeeded with more space available
              } else {
                DVLOG(7) << "Succeeded with no more space available";
                return 1; // succeeded with no more space available
              }
            } else {
              // no, so return nack.
              DVLOG(7) << "Failed with no more space available";
              return -1; // did not succeed
            }
          } );

        // insert succeeded, so move to next edge
        if( retval >= 0 ) {
          local_max++;
        }

        // no more space available on target, so move to next core
        if( local_max < read_count && retval != 0 ) {
          likely_consumer = (likely_consumer + 1) % Grappa::cores();
          CHECK_NE( likely_consumer, Grappa::mycore() ) << "No more space to place edge on cluster?";
        }
      }
      
      // wait for everybody else to fill in our remaining spaces
      Grappa::barrier();
      
      // discard temporary read buffer
      read_edges.clear();
    } );

  // done!
  return tg;
}

/// Matrix Market format loader
TupleGraph TupleGraph::load_mm( std::string path ) {
  // make sure file exists
  CHECK( fs::exists( path ) ) << "File not found.";
  CHECK( fs::is_regular_file( path ) ) << "File is not a regular file.";

  size_t file_size = fs::file_size( path );

  size_t path_length = path.size() + 1; // include space for terminator

  CHECK_LT( path_length, max_path_length )
    << "Sorry, filename exceeds preset limit. Please change max_path_length constant in this file and rerun.";

  char filename[ max_path_length ];
  strncpy( &filename[0], path.c_str(), max_path_length );


  // read header
  struct {
    int header_end_offset = 0;
    bool field_double  = false;
    bool field_default_value = false;
    bool field_bidir = false;
  } header_info;
  
  size_t size_m = 0, size_n = 0, size_nonzero = 0;
  {
    std::ifstream infile( filename, std::ios_base::in );
    bool done = false;
    int line = 0;
    std::string s;

    do { // read all comment lines
      std::getline( infile, s );

      if( s[0] == '%' ) { // comment line
        if( line == 0 ) { // first line
          std::istringstream ss( s );
          std::string tmp;

          // read first token of header
          ss >> tmp;
          CHECK_EQ( tmp, "%%MatrixMarket" );

          // read data type (array or matrix)
          ss >> tmp;
          CHECK_EQ( tmp, "matrix" ) << "Currently, only MM object supported is 'matrix'";

          // read format
          ss >> tmp;
          CHECK_EQ( tmp, "coordinate" ) << "Currently, only MM format supported is 'coordinate'.";

          // read field type
          ss >> tmp;
          if( tmp == "real" || tmp == "double" ) {
            header_info.field_double = true;
          } else if( tmp == "integer" ) {
            header_info.field_double = false;
          } else if( tmp == "pattern" ) {
            header_info.field_double = false;
            header_info.field_default_value = true;
          } else {
            LOG(FATAL) << "Currently, only MM fields supported are real, double, integer, pattern (not " << tmp << ").";
          }

          // read symmetry
          ss >> tmp;
          if( tmp == "symmetric" || tmp == "hermitian" || tmp == "skew-symmetric" ) {
            header_info.field_bidir = true;
          } else if ( tmp == "general" ) {
            header_info.field_bidir = false;
          } else {
            LOG(FATAL) << "Unsupported symmetry " << tmp << ".";
          }
        } else {
          // just skip other comments
        }
      } else {
        done = true;
      }
      ++line;
    } while( !done );

    { // read dimensions from last string
      std::istringstream ss( s );
      std::string tmp;

      ss >> size_m;
      ss >> size_n;
      ss >> size_nonzero;
    }
    
    header_info.header_end_offset = infile.tellg();
    file_size -= header_info.header_end_offset;
    DVLOG(7) << "Header ends at " << local_offset;
  }

  DVLOG(7) << "Reading matrix of size " << size_m << "x" << size_n << " with " << size_nonzero << " nonzeros";


  
  Core mycore = Grappa::mycore();
  auto bytes_each_core = file_size / Grappa::cores();

  // read into temporary buffer
  on_all_cores( [=] {
      // use standard C++/POSIX IO

      // make one core take any data remaining after truncation
      auto my_bytes_each_core = bytes_each_core;
      if( Grappa::mycore() == 0 ) {
        my_bytes_each_core += file_size - (bytes_each_core * Grappa::cores());
      }
      
      // compute initial offset into ASCII file
      // TODO: fix this with scan
      local_offset = header_info.header_end_offset-1; // do on all cores
      Grappa::barrier();
      int64_t start_offset = Grappa::delegate::fetch_and_add( make_global( &local_offset, 0 ),
                                                              my_bytes_each_core );
      Grappa::barrier();
      int64_t end_offset = start_offset + my_bytes_each_core;

      DVLOG(7) << "Reading about " << my_bytes_each_core
               << " bytes starting at " << start_offset
               << " of " << file_size;
      
      // start reading at start offset
      std::ifstream infile( filename, std::ios_base::in );
      infile.seekg( start_offset );

      if( start_offset > 0 ) {
        // move past next newline so we start parsing from a record boundary
        std::string s;
        std::getline( infile, s );
        DVLOG(6) << "Skipped '" << s << "'";
      }
      
      start_offset = infile.tellg();
      DVLOG(6) << "Start reading at " << start_offset;

      // read up to one entry past the end_offset
      while( infile.good() && start_offset < end_offset ) {
        int64_t v0 = -1;
        int64_t v1 = -1;
        uint64_t data = 0;
        if( infile.peek() == '#' ) { // if a comment
          std::string str;
          std::getline( infile, str );
        } else {
          infile >> v0;
          if( !infile.good() ) break;
          infile >> v1;
          if( header_info.field_double ) {
            double d;
            infile >> d;
            data = *((uint64_t*)&d); // hack. reuse bits for data.
          } else {
            int64_t d;
            infile >> d;
            data = *((uint64_t*)&d);
          }
          Edge e = { v0, v1 };
          DVLOG(6) << "Read " << v0 << " -> " << v1 << " with data " << (void*) data;
          read_edges.push_back( e );
          start_offset = infile.tellg();
        }
      }

      DVLOG(6) << "Done reading at " << start_offset << " end_offset " << end_offset;
      
      // collect sizes
      local_offset = read_edges.size();
      DVLOG(7) << "Read " << local_offset << " edges";
    } );

  auto nedge = Grappa::reduce<int64_t,collective_add>(&local_offset);
  LOG(INFO) << "Loading " << nedge << " edges";
  TupleGraph tg( nedge );
  auto edges = tg.edges;
  //auto leftovers = GlobalVector<int64_t>::create(N);

  on_all_cores( [=] {
      Edge * local_ptr = edges.localize();
      Edge * local_end = (edges+nedge).localize();
      size_t local_count = local_end - local_ptr;
      size_t read_count = read_edges.size();

      DVLOG(7) << "local_count " << local_count
               << " read_count " << read_count;
      
      // copy everything in our read buffer that fits locally
      auto local_max = std::min( local_count, read_count );
      std::memcpy( local_ptr, &read_edges[0], local_max * sizeof(Edge) );
      local_offset = local_max;
      Grappa::barrier();

      // get rid of remaining edges
      auto mycore = Grappa::mycore();
      auto likely_consumer = (mycore + 1) % Grappa::cores();
      while( local_max < read_count ) {
        Edge e = read_edges[local_max];
        DVLOG(7) << "Looking for somewhere to place edge " << local_max;
        
        int retval = delegate::call( likely_consumer, [=] ()->int {
            Edge * local_ptr = edges.localize();
            Edge * local_end = (edges+nedge).localize();
            auto local_count = local_end - local_ptr;

            DVLOG(7) << "Trying to place edge " << local_max
                      << " on core " << Grappa::mycore()
                      << " with local_offset " << local_offset
                      << " and local_count " << local_count;
            
            // do we have space to insert here?
            if( local_offset < local_count ) {
              // yes, so do so
              local_ptr[local_offset] = e;
              local_offset++;

              if( local_offset < local_count ) {
                DVLOG(7) << "Succeeded with space available";
                return 0; // succeeded with more space available
              } else {
                DVLOG(7) << "Succeeded with no more space available";
                return 1; // succeeded with no more space available
              }
            } else {
              // no, so return nack.
              DVLOG(7) << "Failed with no more space available";
              return -1; // did not succeed
            }
          } );

        // insert succeeded, so move to next edge
        if( retval >= 0 ) {
          local_max++;
        }

        // no more space available on target, so move to next core
        if( local_max < read_count && retval != 0 ) {
          likely_consumer = (likely_consumer + 1) % Grappa::cores();
          CHECK_NE( likely_consumer, Grappa::mycore() ) << "No more space to place edge on cluster?";
        }
      }
      
      // wait for everybody else to fill in our remaining spaces
      Grappa::barrier();
      
      // discard temporary read buffer
      read_edges.clear();
    } );

  // done!
  return tg;
}



///
/// writing
/// 

/// helper method function for parallel save of a single file
void TupleGraph::save_generic( std::string path, void (*f)( const char *, Edge*, Edge*) ) {
  if( fs::exists( path ) ) {
    // warn if file exists
    CHECK( fs::is_regular_file( path ) ) << "File exists, but is not a regular file.";
    LOG(WARNING) << "Overwriting " << path;
  }

  // create empty file
  std::ofstream outfile( path, std::ofstream::binary | std::ofstream::trunc );
  
  size_t path_length = path.size() + 1; // include space for terminator

  CHECK_LT( path_length, max_path_length )
    << "Sorry, filename exceeds preset limit. Please change max_path_length constant in this file and rerun.";

  char filename[ max_path_length ];
  strncpy( &filename[0], path.c_str(), max_path_length );

  Core mycore = Grappa::mycore();
  auto edges = this->edges;
  auto nedge = this->nedge;
  
  on_all_cores( [=] {
      f( filename, edges.localize(), (edges+nedge).localize() );
    } );
  
  // done!
}

/// helper function to write to a range of a file with POSIX range locking
static void write_locked_range( const char * filename, size_t offset, const char * buf, size_t size ) {
  int fd;

  PCHECK( (fd = open( filename, O_WRONLY )) >= 0 );

  // lock range in file
  struct flock lock;
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = offset;
  lock.l_len = size;
  PCHECK( fcntl( fd, F_SETLK, &lock ) >= 0 ) << "Could not obtain record lock on file";

  // seek to range
  int64_t new_offset = 0;
  PCHECK( (new_offset = lseek( fd, offset , SEEK_SET )) >= 0 );
  CHECK_EQ( new_offset, offset );
      
  // write
  PCHECK( write( fd, buf, size ) >= 0 );

  // release lock on file range
  lock.l_type = F_UNLCK;
  PCHECK( fcntl( fd, F_SETLK, &lock ) >= 0 ) << "Could not release record lock on file";

  // close file
  PCHECK( close( fd ) >= 0 );
}

/// helper function run on each core to save edges stored as int32_t tuples
void local_save_bintsv4( const char * filename,
                         Grappa::TupleGraph::Edge * local_ptr,
                         Grappa::TupleGraph::Edge * local_end ) {
  Int32Edge * local_save_ptr = reinterpret_cast<Int32Edge*>(local_ptr);
  size_t local_count = local_end - local_ptr;
  
  // compress int64's into int32's in place
  for( int64_t i = 0; i < local_count; ++i ) {
    auto v0 = local_ptr[i].v0;
    auto v1 = local_ptr[i].v1;
    local_save_ptr[i].v0 = v0;
    local_save_ptr[i].v1 = v1;
  }
    
  if( !FLAGS_use_mpi_io ) {
    // use standard C++/POSIX IO

    // TODO: fix this with scan
    local_offset = 0; // do on all cores
    Grappa::barrier();
    int64_t offset = Grappa::delegate::fetch_and_add( make_global( &local_offset, 0 ),
                                              local_count );
    Grappa::barrier();

    write_locked_range( filename, offset * sizeof(Int32Edge),
                        (char*) local_ptr, local_count * sizeof(Int32Edge ) );
                        
  } else {
    // save int32's into local chunk
    impl::write_unordered_shared( filename, local_ptr, local_count * sizeof(Int32Edge) );
  }

  // expand int32's into int64's
  for( int64_t i = local_count - 1; i >= 0; --i ) {
    auto v0 = local_save_ptr[i].v0;
    auto v1 = local_save_ptr[i].v1;
    local_ptr[i].v0 = v0;
    local_ptr[i].v1 = v1;
  }
}

/// helper function run on each core to save edges stored as ASCII tab-delimited pairs
void local_save_tsv( const char * filename,
                     Grappa::TupleGraph::Edge * local_ptr,
                     Grappa::TupleGraph::Edge * local_end ) {
  Int32Edge * local_save_ptr = reinterpret_cast<Int32Edge*>(local_ptr);
  size_t local_count = local_end - local_ptr;

  std::stringstream ss;
  for( ; local_ptr < local_end; ++local_ptr ) {
    ss << local_ptr->v0 << "\t" << local_ptr->v1 << std::endl;
  }

  if( !FLAGS_use_mpi_io ) {
    // use standard C++/POSIX IO

    // TODO: fix this with scan
    local_offset = 0; // do on all cores
    Grappa::barrier();
    int64_t offset = Grappa::delegate::fetch_and_add( make_global( &local_offset, 0 ),
                                                      ss.str().size() );
    Grappa::barrier();

    write_locked_range( filename, offset,
                        ss.str().c_str(), ss.str().size() );
  } else {
    impl::write_unordered_shared( filename, const_cast<char*>( ss.str().c_str() ), ss.str().size() );
  }
}



/// TupleGraph constructor that loads from a file, dispatching on file format
TupleGraph TupleGraph::Load( std::string path, std::string format ) {
  if( format == "bintsv4" ) {
    return load_generic( path, local_load_bintsv4 );
  } else if( format == "tsv" ) {
    return load_tsv( path );
  } else if( format == "mm" || format == "mtx" ) {
    return load_mm( path );
  } else {
    LOG(FATAL) << "Format " << format << " not supported.";
  }
}

/// save a TupleGraph to a file
void TupleGraph::save( std::string path, std::string format ) {
  if( format == "bintsv4" ) {
    return save_generic( path, local_save_bintsv4 );
  } else if( format == "tsv" ) {
    return save_generic( path, local_save_tsv );
  } else {
    LOG(FATAL) << "Format " << format << " not supported.";
  }
}

} // namespace Grappa
