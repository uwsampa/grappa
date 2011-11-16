
#ifndef __LISTWALK_OPTIONS_H__
#define __LISTWALK_OPTIONS_H__

#include "option_macros.h"

void set_list_size_from_log( struct options * opt );
void set_payload_size_from_log( struct options * opt );

#define OPTIONS( OPTION_INT, OPTION_BOOL, OPTION_STR )			\
  OPTION_INT( cores			, 'c', 1, NULL )		\
  OPTION_INT( sockets                   , 'k', 1, NULL )		\
  OPTION_INT( threads                   , 't', 1, NULL )		\
  OPTION_INT( payload_size_log          , 'l', 0, &set_payload_size_from_log ) \
  OPTION_INT( payload_size              , 's', 1, NULL )		\
  OPTION_INT( count                     , 'n', 1, NULL )		\
  OPTION_INT( outstanding               , 'o', 50, NULL )		\
  OPTION_INT( rdma_outstanding          , 'p', 16, NULL )		\
  OPTION_INT( batch_size                , 'b', 1, NULL )		\
  OPTION_BOOL( messages                 , 'm', 0, NULL )		\
  OPTION_BOOL( rdma_read                , 'r', 0, NULL )		\
  OPTION_BOOL( rdma_write               , 'w', 0, NULL )		\
  OPTION_BOOL( fetch_and_add            , 'f', 0, NULL )		\
  OPTION_INT( contexts                  , 'x', 1, NULL )		\
  OPTION_INT( protection_domains        , 'd', 1, NULL )		\
  OPTION_INT( queue_pairs               , 'q', 1, NULL )		\
  OPTION_BOOL( same_destination         , '0', 0, NULL )		\
  OPTION_INT( list_size_log             , 'L', 0, &set_list_size_from_log ) \
  OPTION_INT( list_size                 , 'S', 1, NULL )		\
  OPTION_BOOL( jumpthreads              , 'J', 1, NULL )		\
  OPTION_BOOL( force_local              , 'F', 0, NULL )		\
  OPTION_INT( id                        , 'I', 1, NULL )		\
  OPTION_BOOL( pause_for_debugger       , 'P', 0, NULL )		\
  OPTION_STR( type                      , 'T', "default", NULL )


#endif
