// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "CurrentThread.hpp"
#include "Grappa.hpp"

Thread * Grappa_current_thread() {
    return CURRENT_THREAD;
}

int Grappa_tau_id( Thread * thr ) {
#if GRAPPA_TRACE
    return thr->tau_taskid;
#else
    return -1;
#endif
}
