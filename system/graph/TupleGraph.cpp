
#include "TupleGraph.hpp"
#include "ParallelLoop.hpp"
#include "FileIO.hpp"

#include <fstream>

DEFINE_bool( use_mpi_io, false, "Use MPI IO optimizations" );

namespace Grappa {

/// load edges stored as int32_t tuples
TupleGraph TupleGraph::load_bintsv4( std::string path ) {
  // make sure file exists
  CHECK( fs::exists( path ) ) << "File not found.";
  CHECK( fs::is_regular_file( path ) ) << "File is not a regular file.";

  size_t file_size = fs::file_size( path );
  size_t nedge = file_size / (2 * sizeof(int32_t)); 
  LOG(INFO) << "Found " << nedge << " edges";
  
  TupleGraph tg( nedge );

  // helper struct for filename propagation
  struct ArrayReadHelper {
    std::unique_ptr< char[] > filename;
  } GRAPPA_BLOCK_ALIGNED;
  auto helper = symmetric_global_alloc<ArrayReadHelper>();

  size_t filename_size = path.size();
  Core mycore = Grappa::mycore();
  auto edges = tg.edges;
  on_all_cores( [=,&path] {

      // distribute filename across all cores
      helper->filename.reset( new char[filename_size+1] );
      if( Grappa::mycore() == mycore ) {
        strncpy( &helper->filename[0], path.c_str(), path.size()+1 );
      }
      MPI_CHECK( MPI_Bcast( &helper->filename[0], filename_size+1, MPI_CHAR, mycore, MPI_COMM_WORLD ) );
      
      // get local chunk of array
      Edge * local_ptr = edges.localize();
      Edge * local_end = (edges+nedge).localize();
      int64_t local_count = local_end - local_ptr;

      struct Int32Edge { int32_t v0, v1; };
      Int32Edge * local_load_ptr = reinterpret_cast<Int32Edge*>(local_ptr);

      if( !FLAGS_use_mpi_io ) {
        std::ifstream infile( &helper->filename[0],
                              std::ios_base::in | std::ios_base::binary );
        int64_t offset = local_count * Grappa::mycore();
        infile.seekg( offset * sizeof(Int32Edge) );
        
        int32_t src, dst;
        for( int64_t count = 0; count < local_count && infile.good(); ++count ) {
          infile.read(reinterpret_cast<char*>(&src), 4);
          infile.read(reinterpret_cast<char*>(&dst), 4);
          if( infile.fail() ) break;
          local_ptr[count].v0 = src;
          local_ptr[count].v1 = dst;
        }
      } else {
        
        // load int32's into local chunk
        impl::read_unordered_shared( &helper->filename[0], local_ptr, local_count * sizeof(Int32Edge) );
        
        // expand int32's into int64's
        for( int64_t i = local_count - 1; i >= 0; --i ) {
          auto v0 = local_load_ptr[i].v0;
          auto v1 = local_load_ptr[i].v1;
          local_ptr[i].v0 = v0;
          local_ptr[i].v1 = v1;
        }
      }

      Grappa::barrier();
    } );

  // Do this once it's supported
  // symmetric_global_free(helper);

  // done!
  return tg;
}

/// save edges stored as int32_t tuples
void TupleGraph::save_bintsv4( std::string path ) {
  
  // helper struct for filename propagation
  struct ArrayReadHelper {
    std::unique_ptr< char[] > filename;
  } GRAPPA_BLOCK_ALIGNED;
  auto helper = symmetric_global_alloc<ArrayReadHelper>();

  size_t filename_size = path.size();
  Core mycore = Grappa::mycore();
  on_all_cores( [=,&path] {

      // distribute filename across all cores
      helper->filename.reset( new char[filename_size+1] );
      if( Grappa::mycore() == mycore ) {
        strncpy( &helper->filename[0], path.c_str(), path.size()+1 );
      }
      MPI_CHECK( MPI_Bcast( &helper->filename[0], filename_size+1, MPI_CHAR, mycore, MPI_COMM_WORLD ) );

      // get local chunk of array
      Edge * local_ptr = edges.localize();
      Edge * local_end = (edges+nedge).localize();
      int64_t local_count = local_end - local_ptr;

      struct Int32Edge { int32_t v0, v1; };
      CHECK_EQ( sizeof(Int32Edge) * 2, sizeof(Edge) );
      Int32Edge * local_load_ptr = reinterpret_cast<Int32Edge*>(local_ptr);

      
      // compress int64's into int32's
      for( int64_t i = 0; i < local_count; ++i ) {
        auto v0 = local_ptr[i].v0;
        auto v1 = local_ptr[i].v1;
        local_load_ptr[i].v0 = v0;
        local_load_ptr[i].v1 = v1;
      }
      
      // // load int32's into local chunk
      impl::write_unordered_shared( &helper->filename[0], local_ptr, local_count * sizeof(Int32Edge) );
      
      // expand int32's into int64's
      for( int64_t i = local_count - 1; i >= 0; --i ) {
        auto v0 = local_load_ptr[i].v0;
        auto v1 = local_load_ptr[i].v1;
        local_ptr[i].v0 = v0;
        local_ptr[i].v1 = v1;
      }
      
    } );

  // Do this once it's supported
  // symmetric_global_free(helper);

  // done!
}

/// save edges stored as tab-delimited ints
void TupleGraph::save_tsv( std::string filename ) {

  // helper struct for filename propagation
  struct ArrayWriteHelper {
    std::unique_ptr< char[] > filename;
  } GRAPPA_BLOCK_ALIGNED;
  auto helper = symmetric_global_alloc<ArrayWriteHelper>();

  size_t filename_size = filename.size();
  Core mycore = Grappa::mycore();
  on_all_cores( [=,&filename] {

      // distribute filename across all cores
      helper->filename.reset( new char[filename_size+1] );
      if( Grappa::mycore() == mycore ) {
        strncpy( &helper->filename[0], filename.c_str(), filename.size()+1 );
      }
      MPI_CHECK( MPI_Bcast( &helper->filename[0], filename_size+1, MPI_CHAR, mycore, MPI_COMM_WORLD ) );

      // get local chunk of array
      Edge * local_ptr = edges.localize();
      Edge * local_end = (edges+nedge).localize();
      int64_t local_count = local_end - local_ptr;

      std::stringstream ss;
      for( ; local_ptr < local_end; ++local_ptr ) {
        ss << local_ptr->v0 << "\t" << local_ptr->v1 << std::endl;
      }
      
      // write from local chunk
      impl::write_unordered_shared( &helper->filename[0], const_cast<char*>( ss.str().c_str() ), ss.str().size() );
      
    } );

  // Do this once it's supported
  // symmetric_global_free(helper);

  // done!
}


TupleGraph TupleGraph::Load( std::string path, std::string format ) {
  if( format == "bintsv4" ) {
    return load_bintsv4( path );
  // } else if( format == "tsv" ) {
  //   return load_tsv( std::string path );
  } else {
    LOG(FATAL) << "Format " << format << " not supported.";
  }
}

void TupleGraph::save( std::string path, std::string format ) {
  if( format == "bintsv4" ) {
    save_bintsv4( path );
  } else if( format == "tsv" ) {
    return save_tsv( path );
  } else {
    LOG(FATAL) << "Format " << format << " not supported.";
  }
}

} // namespace Grappa
