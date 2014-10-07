
#include <map>
#include "FileIO.hpp"

DEFINE_bool( optimize_for_lustre, true, "Set MPI IO flags for faster Lustre performance" );


namespace Grappa {

namespace impl {

static void dump_mpi_info( MPI_Info info, const char * prefix = NULL  ) {
  int nkeys;
  std::stringstream ss;

  ss << "MPI Info";
  if( prefix ) {
    ss << " for " << prefix;
  }
  ss << ":\n";
  
  MPI_Info_get_nkeys(info, &nkeys);
  for( int i=0; i<nkeys; i++ ) {
    char key[MPI_MAX_INFO_KEY], value[MPI_MAX_INFO_VAL];
    int  valuelen, flag;
    
    MPI_Info_get_nthkey(info, i, key);
    MPI_Info_get_valuelen(info, key, &valuelen, &flag);
    CHECK( flag ) << "somehow key not defined?";
    MPI_Info_get(info, key, valuelen+1, value, &flag);
    CHECK( flag ) << "somehow key not defined?";
    
    ss << "   key " << i << ": " << key << " => " << value;
  }

  LOG(INFO) << ss.str();
}

static void dump_mpi_file_info( MPI_File f, const char * prefix = NULL ) {
  MPI_Info info;

  MPI_CHECK( MPI_File_get_info( f, &info ) );

  dump_mpi_info( info, prefix );

  MPI_CHECK( MPI_Info_free( &info ) );
}

static void set_mpi_info( MPI_Info info, std::map< const std::string, const std::string >& info_map ) {
  for( auto& kv : info_map ) {
    MPI_CHECK( MPI_Info_set( info,
                             const_cast<char*>( kv.first.c_str() ),
                             const_cast<char*>( kv.second.c_str() ) ) );
  }
}


void read_unordered_shared( const char * filename, void * local_ptr, size_t size ) {
  MPI_Status status;
  MPI_File infile;
  MPI_Datatype datatype;
  MPI_Info info;
  char * local_char_ptr = static_cast< char * >( local_ptr );

  // Stupid MPI uses a signed integer for the count of data elements
  // to write. On all the machines we use, this means the max count
  // is 2^31-1. To work around this, we create a datatype large
  // enough that we never need a count value larger than 2^30.

  // move this many gigabyte chunks
  const size_t gigabyte = 1L << 30;
  size_t round_count = size / gigabyte;

  // if there will be any bytes left over, move those too
  if( size & (gigabyte - 1) ) round_count++;

  MPI_CHECK( MPI_Info_create( &info ) );

  if( FLAGS_optimize_for_lustre ) {
    std::map< const std::string, const std::string > info_map = {{
        // disable independent file operations, since we know this
        // routine is the only one touching the file
        { "romio_no_indep_rw", "true" }

        // // disable collective io on lustre for "small" files <= 1GB
        // , { "romio_lustre_ds_in_coll", "1073741825" ) );
        
        // set collective buffering block size to something reasonable
        , { "cb_buffer_size", "33554432" }
        
        // // disable collective buffering for writing
        // , { "romio_cb_write", "disable" }
        // // disable collective buffering for writing
        // , { "romio_cb_read", "disable" }
        
        // disable data sieving for writing
        , { "romio_ds_write", "disable" }
        // disable data sieving for writing
        , { "romio_ds_read", "disable" }
        
        
        // enable direct IO
        , { "direct_read", "true" }
        , { "direct_write", "true" }
        
        // ???
        // , { "romio_lustre_co_ratio", "1" }
        
        // maybe 
        // , { "access_style", "read_once" }
      }};

    set_mpi_info( info, info_map );
  }

  // open file for reading
  int mode = MPI_MODE_RDONLY;
  MPI_CHECK( MPI_File_open( global_communicator.grappa_comm, const_cast< char * >( filename ), mode, info, &infile ) );

  // make sure we will read exactly the whole file
  int64_t total_size = 0;
  MPI_CHECK( MPI_Reduce( &size, &total_size, 1, MPI_INT64_T, MPI_SUM, 0, global_communicator.grappa_comm ) );
  if( 0 == Grappa::mycore() ) {
    MPI_Offset file_size = 0;
    MPI_CHECK( MPI_File_get_size( infile, &file_size ) );
    CHECK_EQ( file_size, total_size ) << "Sizes don't line up to read the enitre file?";
  }

  // // dump file info for debugging
  // if( Grappa::mycore == 0 ) {
  //   dump_mpi_file_info( infile );
  // }

  // compute number of rounds required to read entire file
  MPI_CHECK( MPI_Allreduce( MPI_IN_PLACE, &round_count, 1, MPI_INT64_T, MPI_MAX, global_communicator.grappa_comm ) );

  // read file in gigabyte-sized rounds
  for( int i = 0; i < round_count; ++i ) {
    if( size > gigabyte ) {
      MPI_CHECK( MPI_File_read_shared( infile, local_char_ptr, gigabyte, MPI_BYTE, &status ) );
      size -= gigabyte;
      local_char_ptr += gigabyte;
    } else {
      MPI_CHECK( MPI_File_read_shared( infile, local_char_ptr, size, MPI_BYTE, &status ) );
    }
  }

  MPI_CHECK( MPI_Info_free( &info ) );
  MPI_CHECK( MPI_File_close( &infile ) );
}

void write_unordered_shared( const char * filename, void * local_ptr, size_t size ) {
  MPI_Status status;
  MPI_File outfile;
  MPI_Datatype datatype;
  MPI_Info info;
  char * local_char_ptr = static_cast< char * >( local_ptr );

  // Stupid MPI uses a signed integer for the count of data elements
  // to write. On all the machines we use, this means the max count
  // is 2^31-1. To work around this, we create a datatype large
  // enough that we never need a count value larger than 2^30.

  // move this many gigabyte chunks
  const size_t gigabyte = 1L << 30;
  size_t round_count = size / gigabyte;

  // if there will be any bytes left over, move those too
  if( size & (gigabyte - 1) ) round_count++;

  MPI_CHECK( MPI_Info_create( &info ) );

  if( FLAGS_optimize_for_lustre ) {
    std::map< const std::string, const std::string > info_map = {{
        // disable independent file operations, since we know this
        // routine is the only one touching the file
        { "romio_no_indep_rw", "true" }

        // it's important to set lustre striping properly for performance.
        // we're supposed to be able to do this through MPI ROMIO, but mpi installations are not always configured to allow this.
        // if not, then before saving a file, run a command like this to create it:
        //    lfs setstripe -c -1 -s 32m <filename>
        // below are what we'd like to do.
        // we want 32MB lustre blocks, but this probably doesn't do anything
        , { "striping_unit", "33554432" }
        // we want to use all lustre OSTs, but this probably doesn't do anything
        , { "striping_factor", "-1" }
        
        // // disable collective io on lustre for "small" files <= 1GB
        // , { "romio_lustre_ds_in_coll", "1073741825" ) );
        
        // set collective buffering block size to something reasonable
        , { "cb_buffer_size", "33554432" }
        
        // // disable collective buffering for writing
        // , { "romio_cb_write", "disable" }
        // // disable collective buffering for writing
        // , { "romio_cb_read", "disable" }
        
        // disable data sieving for writing
        , { "romio_ds_write", "disable" }
        // disable data sieving for writing
        , { "romio_ds_read", "disable" }
        
        
        // enable direct IO
        , { "direct_read", "true" }
        , { "direct_write", "true" }
        
        // ???
        // , { "romio_lustre_co_ratio", "1" }
        
        // maybe 
        // , { "access_style", "read_once" }
      }};

    set_mpi_info( info, info_map );
  }

  // open file for writing
  int mode = MPI_MODE_CREATE | MPI_MODE_WRONLY;
  MPI_CHECK( MPI_File_open( global_communicator.grappa_comm, const_cast< char * >( filename ), mode, info, &outfile ) );
  
  // drop any data previously in file
  MPI_CHECK( MPI_File_set_size( outfile, 0 ) );
  
  // dump file info for debugging
  // if( Grappa::mycore == 0 ) {
  //   dump_mpi_file_info( outfile );
  // }

  // compute number of rounds required to read entire file
  MPI_CHECK( MPI_Allreduce( MPI_IN_PLACE, &round_count, 1, MPI_INT64_T, MPI_MAX, global_communicator.grappa_comm ) );

  // write file in gigabyte-sized rounds
  for( int i = 0; i < round_count; ++i ) {
    if( size > gigabyte ) {
      MPI_CHECK( MPI_File_write_shared( outfile, local_char_ptr, gigabyte, MPI_BYTE, &status ) );
      size -= gigabyte;
      local_char_ptr += gigabyte;
    } else {
      MPI_CHECK( MPI_File_write_shared( outfile, local_char_ptr, size, MPI_BYTE, &status ) );
    }
  }

  MPI_CHECK( MPI_Info_free( &info ) );
  MPI_CHECK( MPI_File_close( &outfile ) );
}

} // namespace impl

} // namespace Grappa
