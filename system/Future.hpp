// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef FUTURE_HPP
#define FUTURE_HPP

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

extern uint64_t global_futures_num_touched;
extern uint64_t global_futures_num_local_started;
extern uint64_t global_futures_num_remote_started;
extern uint64_t global_futures_num_local_dq;
extern uint64_t global_futures_num_remote_dq;

/// A Future is a task that can be tested for completion or
/// executed directly if still local.
///
/// @tparam UserArg type of task argument
/// @tparam F type of task function
template < typename UserArg, void (*F)(UserArg) >
class Future {
    
    typedef GlobalAddress<Future<UserArg, F> > FutureAddress_t;

    private:
        int64_t started;
        Thread * waiter;
        UserArg userArg;
        bool done;

        
        struct future_done_args { 
            Future< UserArg, F > * futurePtr;
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
            FutureAddress_t address;
            UserArg result;
            bool futureStarted;
            bool done;
        };

        struct started_reply_args {
            GlobalAddress<started_memory_descriptor> descriptor;
            bool started;
        };

        static void started_reply_am( started_reply_args * args, size_t size, void * payload, size_t payload_size ) {
            args->descriptor.pointer()->futureStarted = args->started;
            
            if ( !args->started ) {
                args->descriptor.pointer()->result = *(static_cast<UserArg*>(payload));
            }
            
            args->descriptor.pointer()->done = true;
            SoftXMT_wake( args->descriptor.pointer()->t );
        }
        
        struct started_request_args {
            GlobalAddress<started_memory_descriptor> descriptor;
            FutureAddress_t address;
        };

        static bool started_request_helper( Future<UserArg, F> * f ) {
            int64_t st = f->started;
            bool alreadyStarted = (st > 0);
            f->started = st + 1;
            
            if ( alreadyStarted ) {
                DVLOG(5) << "already started:(id:"<<f->getId();
                if ( f->done ) { //if it is done then toucher is waiting for the dequeue
                    DVLOG(5) << "need to wake:(id:"<<f->getId();
                    CHECK ( f->waiter!=NULL ) << "future ptr:" << (void*)f <<"\n done="<<f->done<<" (id:"<<f->getId()<<") st="<<st;//<< " (AM from "<<args->descriptor.node();
                    
                    SoftXMT_wake( f->waiter );
                    f->waiter = NULL;
                } else {
                    DVLOG(5) << "not need to wake:(id:"<<f->getId();
                }
            }

            return alreadyStarted;
        }

        static void started_request_am( started_request_args * args, size_t size, void * payload, size_t payload_size ) {
            Future< UserArg, F > * fptr = args->address.pointer();
            
            DVLOG(5) << "Future(IN_AM) FID "<< fptr->getId();

            // hack to check segfaults
//            CHECK((int64_t)fptr>0x1000) << "dequeued request (descriptor gaddress:" << args->descriptor
//                    << "): future ptr:" << (void*)fptr
//                    << "(future gaddress:"<<args->address;

            void * reply_payload;
            size_t reply_payload_size;
           
            bool alreadyStarted = started_request_helper( fptr );
            
            // If future was already started in this case, it must have been started by touching thread.
            // Incrementing started again will tell the touching thread it can deallocate the Future
            if ( alreadyStarted ) {
                reply_payload = NULL;
                reply_payload_size = 0;
            } else {
                // need to start so include arguments in the reply
                reply_payload = &(fptr->userArg);
                reply_payload_size = sizeof(UserArg);
                DVLOG(5) << "not already started:(id:"<<fptr->getId();
            }
            
            started_reply_args reply_args;
            reply_args.descriptor = args->descriptor;
            reply_args.started = alreadyStarted;
           
            SoftXMT_call_on( args->descriptor.node(), &started_reply_am, 
                    &reply_args, sizeof(reply_args),
                    reply_payload, reply_payload_size );
        }

        static bool future_delegate_started( FutureAddress_t address, UserArg * arg ) {
            if ( address.node() == SoftXMT_mynode() ) {
                // local shortcut
                bool started = started_request_helper(address.pointer());
                if ( !started ) {
                    *arg = address.pointer()->userArg;
                }
                return started;
            } else {
                started_memory_descriptor md;
                md.address = address;
                md.futureStarted = false;
                md.done = false;
                md.t = CURRENT_THREAD;

                started_request_args args;
                args.descriptor = make_global(&md);
                args.address = address;
                SoftXMT_call_on( address.node(), &started_request_am, &args );

                while( !md.done ) {
                    SoftXMT_suspend();
                }
                *arg = md.result;
                return md.futureStarted;
            }
        }
        //////////////////////////////////////
        
        /// Just passes the UserArg argument to the user function
        static void call_as_future(FutureAddress_t futureAddr) {
            DVLOG(4) << "Future(other) "<< futureAddr;
            DVLOG(5) << CURRENT_THREAD->id << "will call started am " << futureAddr.pointer();

            if (futureAddr.node() == SoftXMT_mynode()) {
                global_futures_num_local_dq++;
            } else {
                global_futures_num_remote_dq++;
            }
            
            UserArg arg;
            // check whether it is already started (and notify of dequeueing the task)
            if ( !future_delegate_started( futureAddr, &arg ) ) {
                DVLOG(5) << CURRENT_THREAD->id << "user_args="<<arg<<" for ftraddr="<< futureAddr.pointer();
                // call user task 
                F(arg);
               
                if (futureAddr.node() == SoftXMT_mynode()) {
                    global_futures_num_local_started++;
                } else {
                    global_futures_num_remote_started++;
                }
                
                // call wake up AM on Node that has the Future
                future_done_args done_args = { futureAddr.pointer() };
                SoftXMT_call_on( futureAddr.node(), &future_done_am, &done_args );
            } 
        }

        /// The id of this Future.
        /// The value returned is only valid during DEBUG
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

        /// Construct new Future with the argument
        Future ( UserArg userArg )
            : started( 0 )
              , waiter( NULL )
              , done( false )
              , userArg( userArg )
#if DEBUG
              , id( SoftXMT_mynode() + ((count_++)*SoftXMT_nodes() ) )
#endif
            { 
           DVLOG(5) << CURRENT_THREAD->id << " creates Future:"<< (void*)this << " id:"<< getId();
        }


        /// If the Future is still local then execute it directly,
        /// otherwise block until completion.
        ///
        /// TODO: ways of providing an actual touch that executes
        /// the Future as long as no one has started it
        void touch( ) {
            DVLOG(4) << "Future(touch) FID "<< this->getId() << " address:"<< (void*)this;
            
            // start if not started
            if ( SoftXMT_delegate_fetch_and_add_word( make_global(&started), 1 )==0 ) {
                DVLOG(5) << CURRENT_THREAD->id << " gets to touch-go " << getId();
           
                // call the user task
                F(userArg);
                done = true;
                
                global_futures_num_touched++;
            
                CHECK( started <= 2 ) << "started=" << started << " (at most one additional increment should occur)";
            
                while ( started < 2 ) { // wait until dequeued
                    DVLOG(5) << CURRENT_THREAD->id << " has to wait on dequeue " << getId();
                    waiter = CURRENT_THREAD;
                    SoftXMT_suspend( );
                    DVLOG(5) << CURRENT_THREAD->id << " has woke on dequeue " << getId();
                }
            } else  {
                // otherwise block on done event
                while ( !done ) {
                    waiter = CURRENT_THREAD;
                    SoftXMT_suspend( );
                }
            }
        }

        /// Spawn the Future as a task in the global task pool
        void addAsPublicTask( ) {
            DVLOG(4) << "Future(spawn) " << this->getId() << " address:"<< (void*)this;
            SoftXMT_publicTask( &call_as_future, make_global(this) );
        }

};

void futures_dump_stats();
void futures_reset_stats();

#endif // FUTURE_HPP
