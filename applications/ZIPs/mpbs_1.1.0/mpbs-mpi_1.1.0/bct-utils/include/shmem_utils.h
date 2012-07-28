#ifndef __SHMEM_UTILS_H
#define __SHMEM_UTILS_H

#ifndef SHMEM
#error  "shmem_utils.h requires SHMEM to be defined."
#endif

/**
 * Compares SHMEM symmetric memory pointers to determine consistency.  If not 
 * consistent then exits here. 
 *
 * @param rank Rank of the calling process or thread.
 * @param npes Total number of processes.
 * @param s    Address to be checked.
 * @param str  String holding the name of the shmalloc'd array to be checked.
 */
void check_shptrs( int rank, int npes, void *s, char *str );

#endif 
