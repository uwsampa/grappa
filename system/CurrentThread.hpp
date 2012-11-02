// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#ifndef CURRENT_THREAD_HPP
#define CURRENT_THREAD_HPP

class Thread;
Thread * Grappa_current_thread();
int Grappa_tau_id( Thread * );

#endif // CURRENT_THREAD_HPP
