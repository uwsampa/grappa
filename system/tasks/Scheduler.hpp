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
#pragma once

#include "Worker.hpp"

namespace Grappa {

class Scheduler {
    private:
        void dummy() {};
    public:
       virtual Worker * get_current_thread() = 0;
       virtual void assignTid( Worker * thr ) = 0;

       virtual void ready( Worker * thr ) = 0;
       virtual void periodic( Worker * thr ) = 0;
       virtual void run( ) = 0;
       
       virtual bool thread_yield( ) = 0;
       virtual void thread_suspend( ) = 0;
       virtual void thread_wake( Worker * next ) = 0;
       virtual void thread_yield_wake( Worker * next ) = 0;
       virtual void thread_suspend_wake( Worker * next ) = 0;

       virtual Worker * thread_wait( void **result ) = 0;
       virtual void thread_on_exit( ) = 0;
};

}