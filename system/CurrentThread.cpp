#include "CurrentThread.hpp"
#include "SoftXMT.hpp"

Thread * Grappa_current_thread() {
    return CURRENT_THREAD;
}

int Grappa_tau_id( Thread * thr ) {
#if GRAPPA_TRACE
    return thr->tau_taskid;
#endif
}
