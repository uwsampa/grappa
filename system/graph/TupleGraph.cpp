
#define _FILE_OFFSET_BITS 64
  
#include "TupleGraph.hpp"
#include "ParallelLoop.hpp"
#include "FileIO.hpp"
#include "Delegate.hpp"

#include <fstream>

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

/// helper method function for parallel save of a single file
void TupleGraph::save_generic( std::string path, void (*f)( const char *, Edge*, Edge*) ) {
  if( fs::exists( path ) ) {
    // warn if file exists
    CHECK( fs::is_regular_file( path ) ) << "File exists, but is not a regular file.";
    LOG(WARNING) << "Overwriting " << path;
  } else {
    // create empty file
    std::ofstream outfile( path, std::ofstream::binary );
  }
  
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


///
/// writing
/// 

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
  // } else if( format == "tsv" ) {
  //   return load_tsv( std::string path );
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
