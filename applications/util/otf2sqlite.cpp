////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

// simple reader for time series from vampirtrace-generated OTF file
// 
// this is built following the docs here
//    http://wwwpub.zih.tu-dresden.de/~jurenz/otf/api/current/group__reader.html
// and the code for the otfdump/otfprint tool in the vampirtrace source directory
//    <vampirtrace dir>/extlib/otf/tools/otfdump/otfdump.cpp

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <sstream>
#include <string>
#include <unordered_map>

#include <otf.h>
#include <sqlite3.h>

DEFINE_string( otf, "", "Name of OTF file" );
DEFINE_string( db, "", "Name of Sqlite DB" );

DEFINE_string( table, "vampirtrace", "Name of Sqlite table in db" );

DEFINE_string( counter, "", "Name of counter to turn into time series" );

DEFINE_int64( min_time, 0, "Time to start converting trace" );
DEFINE_int64( max_time, -1, "Max time to convert in trace" );
DEFINE_int64( buffer_size, 4*1024, "OTF buffer size" );

DEFINE_bool( list_counters, false, "Print list of counters" );

std::unordered_map< uint32_t, std::string > counter_map;
std::unordered_map< uint32_t, bool > counter_double_map;

uint32_t relevant_counter = -1;

sqlite3 * db = NULL;
sqlite3_stmt * stmt = NULL;



int defcounter_handler( void* userData,
                        uint32_t stream,
                        uint32_t counter,
                        const char* name,
                        uint32_t properties,
                        uint32_t counterGroup,
                        const char* unit,
                        OTF_KeyValueList *list ) {
  if( FLAGS_list_counters ) {
    if( OTF_COUNTER_VARTYPE_ISINTEGER(properties) ) {
      LOG(INFO) << "Found int counter " << name << " in unit " << unit;
    } else {
      LOG(INFO) << "Found double counter " << name << " in unit " << unit;
    }
  }

  counter_map[ counter ] = name;
  counter_double_map[ counter ] = !OTF_COUNTER_VARTYPE_ISINTEGER(properties);

  if( counter_map[ counter ] == FLAGS_counter ) {
    relevant_counter = counter;
  }

  return OTF_RETURN_OK; 
};


int counter_handler( void* userData,
                     uint64_t time,
                     uint32_t process,
                     uint32_t counter,
                     uint64_t value,
                     OTF_KeyValueList *list ) {
  if( counter == relevant_counter ) {
    if( stmt ) {
      CHECK_EQ( SQLITE_OK, sqlite3_reset( stmt ) ) << sqlite3_errmsg(db);
      CHECK_EQ( SQLITE_OK, sqlite3_bind_int64( stmt, 1, time ) ) << sqlite3_errmsg(db);
      CHECK_EQ( SQLITE_OK, sqlite3_bind_text( stmt, 2, counter_map[counter].c_str(), -1, 0 ) ) << sqlite3_errmsg(db);
      CHECK_EQ( SQLITE_OK, sqlite3_bind_int( stmt, 3, process ) ) << sqlite3_errmsg(db);
      CHECK_EQ( SQLITE_OK, sqlite3_bind_int64( stmt, 4, value ) ) << sqlite3_errmsg(db);
      CHECK_EQ( SQLITE_OK, sqlite3_bind_double( stmt, 5, *((double*)&value) ) ) << sqlite3_errmsg(db);
      CHECK_EQ( SQLITE_DONE, sqlite3_step( stmt ) ) << sqlite3_errmsg(db);
    }
  }
}


/// watch out for bullshit! the type of the function pointer depends on which record you're reading.
/// refer to this for details:
///    http://wwwpub.zih.tu-dresden.de/~jurenz/otf/api/current/OTF__Handler_8h.html
template< typename F >
void register_otf_handler( OTF_HandlerArray * handlers, int record, F f, void * user_data = NULL ) {
  OTF_HandlerArray_setHandler( handlers, (OTF_FunctionPointer*) f, record );
  if( user_data ) {
    OTF_HandlerArray_setFirstHandlerArg( handlers, &user_data, record );
  }
}


/// Execute SQL statement
template< typename CALLBACK, typename ARG >
void sqlite_exec( sqlite3 * db, const char * sql, CALLBACK callback, ARG arg ) {
  char * zErrMsg = NULL;
  typedef int (*callback_t)(void*,int,char**,char**);
  int rc = sqlite3_exec(db, sql, (callback_t) callback, (void*) arg, &zErrMsg);
  if( rc != SQLITE_OK ){
    LOG(FATAL) << "SQLite error: " << zErrMsg;
    sqlite3_free(zErrMsg);
  }
}

void sqlite_prepare( sqlite3 * db, const char * sql, int len, sqlite3_stmt ** stmt ) {
  const char *pzTail;     /* OUT: Pointer to unused portion of zSql */
  int rc = sqlite3_prepare_v2( db, sql, len, stmt, &pzTail );
  if( rc != SQLITE_OK ) {
    LOG(FATAL) << "SQLite preparation error; skipped " << pzTail;
  }
}


int main( int argc, char** argv ) {
  google::ParseCommandLineFlags( &argc, &argv, true);
  google::InitGoogleLogging( argv[0] );

  FLAGS_logtostderr = 1;
  FLAGS_v = 1;
  LOG(INFO) << "Starting!";
  
  OTF_FileManager * manager = OTF_FileManager_open( 100 ); // limit to 100 filehandles
  CHECK_NOTNULL( manager ); 

  CHECK_NE( FLAGS_otf, "" ) << "Please specify input OTF file";
  OTF_Reader * reader = OTF_Reader_open( FLAGS_otf.c_str(), manager ); 
  CHECK_NOTNULL( reader );

  OTF_Reader_setBufferSizes( reader, FLAGS_buffer_size );

  
  if( FLAGS_db != "" ) {
    auto retval = sqlite3_open( FLAGS_db.c_str(), &db );
    if( retval ) {
      LOG(FATAL) << "Can't open db " << FLAGS_db << ": " << sqlite3_errmsg(db);
    }

    std::stringstream ss;
    ss << "CREATE TABLE IF NOT EXISTS " << FLAGS_table << "("
       << " TIME INT PRIMARY KEY NOT NULL,"
       << " COUNTER_NAME   TEXT,"
       << " PROCESS        INT,"
       << " INT_VALUE      INT,"
       << " DOUBLE_VALUE   REAL);";
    sqlite_exec( db, ss.str().c_str(), NULL, NULL );

    std::stringstream ss2;
    ss2 << "INSERT OR IGNORE INTO " << FLAGS_table
        << " VALUES (?, ?, ?, ?, ?);";
    sqlite_prepare( db, ss2.str().c_str(), -1, &stmt );
  }
  
  if( -1 != FLAGS_max_time ) {
    OTF_Reader_setTimeInterval( reader, FLAGS_min_time, FLAGS_max_time );
  }

  
  OTF_HandlerArray * handlers = OTF_HandlerArray_open(); 
  CHECK_NOTNULL( handlers ); 

  // register handlers to do whatever you want.
  // many handlers can be registered; full list here:
  //   http://wwwpub.zih.tu-dresden.de/~jurenz/otf/api/current/OTF__Definitions_8h.html
  register_otf_handler( handlers, OTF_DEFCOUNTER_RECORD, defcounter_handler );
  register_otf_handler( handlers, OTF_COUNTER_RECORD, counter_handler );


  uint64_t read;
  read = OTF_Reader_readDefinitions( reader, handlers );
  LOG(INFO) << "Read " << read << " definitions";
  
  read = OTF_Reader_readEvents( reader, handlers );
  LOG(INFO) << "Read " << read << " events";
  
  read = OTF_Reader_readStatistics( reader, handlers );
  LOG(INFO) << "Read " << read << " statistics";
  
  read = OTF_Reader_readSnapshots( reader, handlers );
  LOG(INFO) << "Read " << read << " snapshots";
  
  read = OTF_Reader_readMarkers( reader, handlers );
  LOG(INFO) << "Read " << read << " markers";

  LOG(INFO) << "Done.";
  if( stmt ) sqlite3_finalize( stmt );
  if( db ) sqlite3_close( db );
  OTF_Reader_close( reader ); 
  OTF_HandlerArray_close( handlers ); 
  OTF_FileManager_close( manager ); 
  return 0; 
} 

