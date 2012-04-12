#ifndef __FUTURE_HPP__
#define __FUTURE_HPP__

#include "SoftXMT.hpp"
#include "Delegate.hpp"
#include "Cache.hpp"

//TODO: special ArgsCache acquire, which
//uses args pointer field as a global address
//to the future and either
//1. flip the started bit
//2. or simply acquire RW instead of RO (and Cached not Incoherent)

// idea: if we had a need for futures that
// only mutate their arguments (essentially functional but could change the inputs as well)
// then we could allow multiple copies of
// the future to execute RW-shared mode, where only the first release writeback wins


// TODO: do something like this to force users to have
// the future fields but make them invisible
/*
class FutureArgs {
    private:
    GlobalAddress<Future> futurePtr;
    void (* user_fn_p)(A *);
};
class MyArgs : public FutureArgs {
    int i;
    int j;
}
// Additional possibility is method #2 above
    */


template < typename ArgsStruct >
class Future {
    private:
        struct future_args {
            GlobalAddress<Future> futureAddr;
            void (* user_fn_p)(ArgsStruct *);
            GlobalAddress< ArgsStruct > userArgs;

            future_args( Future< ArgsStruct > * future, ArgsStruct * userArgs, void (* fn_p)(ArgsStruct *) )
                : futureAddr( make_global( future ) )
                , userArgs( make_global( userArgs ) )
                , user_fn_p( fn_p ) 
            { }
        };

        struct future_done_args { 
            Future< ArgsStruct > * futurePtr;
        };
        
        static void future_done_am( future_done_args * args, size_t args_size, void * payload, size_t payload_size ) {
            args->futurePtr->done = true;
            if ( args->futurePtr->waiter != NULL ) {
                SoftXMT_wake( args->futurePtr->waiter );
                args->futurePtr->waiter = NULL;
            }
        }

        static void future_function( future_args * args ) {
            // TODO test Global address calculations (so I can say address of 'started' instead)
            if ( SoftXMT_delegate_fetch_and_add_word( args->futureAddr, 1 ) == 0 ) { 
                
                // grab the user arguments
                size_t args_size = sizeof(ArgsStruct);
                ArgsStruct argsbuf;
                typename Incoherent<ArgsStruct>::RO cached_args( args->userArgs, args_size, &argsbuf );
                cached_args.block_until_acquired();
                
                // call the user task
                args->user_fn_p( &argsbuf );
                cached_args.block_until_released();

                // call wake up AM on Node that has the Future
                future_done_args done_args = { args->futureAddr.pointer() };
                SoftXMT_call_on( args->futureAddr.node(), &future_done_am, &done_args );
            }
        }
    
    public:
        int64_t started;//MUST be first (because address)
        Thread * waiter;
        bool done;
        ArgsStruct * userArgs_lp;

        future_args task_args;


        // TODO: NOTE that this does not copy user arguments because we want to 
        // preserve the same semantics as normal tasks.
        // --Unfortunately this means there are three communications to start
        // 1) Task.execute fetches args
        // 2) Future atomic started
        // 3) Bring in user arguments and start
        Future ( void (* fn_p)(ArgsStruct *), ArgsStruct * userArgs )
            : started( 0 )
              , waiter( NULL )
              , done( false )
              , userArgs_lp( userArgs )
              , task_args( this, userArgs, fn_p ) { }

        void touch( ) {
            // start if not started
            if ( SoftXMT_delegate_fetch_and_add_word( make_global(&started), 1 )==0 ) {
                task_args.user_fn_p( userArgs_lp );
                done = true;
            } else  {
                // otherwise block on done event
                while ( !done ) {
                    waiter = CURRENT_THREAD;
                    SoftXMT_suspend( );
                }
            }
        }

        void asPublicTask( ) {
            SoftXMT_publicTask( &future_function, &task_args );
        }
};

#endif
