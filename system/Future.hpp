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

//////////////////////////////////////////
#if DEBUG
static int64_t count_ = 0;
#endif

template < typename ArgsStruct >
class Future {
    private:
        int64_t started;
        Thread * waiter;
        bool done;
        ArgsStruct * userArgs_lp;
        
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
        
        future_args task_args;

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

        /////////////////
        //Custom delegate operation
        //TODO: use a generalized mechanism that abstracts all but function
        /////////////////
        struct started_memory_descriptor {
            Thread * t;
            GlobalAddress< Future<ArgsStruct> > address;
            int64_t data;
            bool done;
        };

        struct started_reply_args {
            GlobalAddress<started_memory_descriptor> descriptor;
        };

        static void started_reply_am( started_reply_args * args, size_t size, void * payload, size_t payload_size ) {
            assert( payload_size == sizeof(int64_t ) );
            args->descriptor.pointer()->data = *(static_cast<int64_t*>(payload));
            args->descriptor.pointer()->done = true;
            SoftXMT_wake( args->descriptor.pointer()->t );
        }
        
        struct started_request_args {
            GlobalAddress<started_memory_descriptor> descriptor;
            GlobalAddress< Future<ArgsStruct> > address;
        };

        static void started_request_am( started_request_args * args, size_t size, void * payload, size_t payload_size ) {
            Future< ArgsStruct > * fptr = args->address.pointer();

            CHECK((int64_t)fptr>0x1000) << "dequeued request (node:" << args->descriptor.node()
                    << "): future ptr:" << (void*)fptr
                    << "(id:"<<fptr->getId()<<")";
            int64_t data = fptr->started;
            fptr->started = data + 1;
            
            // If future was already started in this case, it must have been started by touching thread.
            // Incrementing started again will tell the touching thread it can deallocate the Future
            if ( data > 0 ) {
                VLOG(2) << "already started:(id:"<<fptr->getId();
                if ( fptr->done ) { //if it is done then toucher is waiting for the dequeue
                    VLOG(2) << "need to wake:(id:"<<fptr->getId();
                    CHECK ( fptr->waiter!=NULL ) << "future ptr:" << (void*)fptr <<"\n done="<<fptr->done<<" (id:"<<fptr->getId()<<")";
                    SoftXMT_wake( fptr->waiter );
                    fptr->waiter = NULL;
                } else {
                    VLOG(2) << "not need to wake:(id:"<<fptr->getId();
                }
            } else {
                VLOG(2) << "not already started:(id:"<<fptr->getId();
            }
            
            started_reply_args reply_args;
            reply_args.descriptor = args->descriptor;
            SoftXMT_call_on( args->descriptor.node(), &started_reply_am, 
                    &reply_args, sizeof(reply_args),
                    &data, sizeof(data) );
        }

        static int64_t future_delegate_started( GlobalAddress< Future<ArgsStruct> > address ) {
            started_memory_descriptor md;
            md.address = address;
            md.data = 0;
            md.done = false;
            md.t = CURRENT_THREAD;
            started_request_args args;
            args.descriptor = make_global(&md);
            args.address = address;
            SoftXMT_call_on( address.node(), &started_request_am, &args );
            while( !md.done ) {
                SoftXMT_suspend();
            }
            return md.data;
        }
        //////////////////////////////////////

        static void future_function( future_args * args ) {
            // TODO #1: the make_global is just calculating location of Future->started
            //if ( SoftXMT_delegate_fetch_and_add_word( make_global( args->startedAddr, args->futureAddr.node() ), 1 ) == 0 ) { 
            VLOG(2) << CURRENT_THREAD->id << "args("<<(void*)args<<") will call started am " << args->futureAddr.pointer();
            if ( future_delegate_started( args->futureAddr ) == 0 ) {  
                VLOG(2) << CURRENT_THREAD->id << "user_args="<<args->userArgs<<" for ftraddr="<< args->futureAddr.pointer();
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

        int64_t getId() {
#if DEBUG
            return id;
#else       
            return -1;
#endif  
        }
    
    public:
#if DEBUG
        int64_t id;
#endif


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
#if DEBUG
              , id( count_++ )
#endif
              , task_args( this, userArgs, fn_p ) { 
                  
           VLOG(2) << CURRENT_THREAD->id << " creates Future:"<< (void*)this << " id:"<< getId() << " args:"<< &task_args;
        }

        void touch( ) {
            // start if not started
            if ( SoftXMT_delegate_fetch_and_add_word( make_global(&started), 1 )==0 ) {
                VLOG(2) << CURRENT_THREAD->id << " gets to touch-go " << getId();
                task_args.user_fn_p( userArgs_lp );
                done = true;
                while ( started < 2 ) { // wait until dequeued
                    VLOG(2) << CURRENT_THREAD->id << " has to wait on dequeue " << getId();
                    waiter = CURRENT_THREAD;
                    SoftXMT_suspend( );
                    VLOG(2) << CURRENT_THREAD->id << " has woke on dequeue " << getId();
                }
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
