/*** UNCLASSIFIED//FOR OFFICIAL USE ONLY ***/

// Copyright © 2007, Institute for Defense Analyses, 4850 Mark Center
// Drive, Alexandria, VA 22311-1882; 703-845-2500.
//
// This material may be reproduced by or for the U.S. Government 
// pursuant to the copyright license under the clauses at DFARS 
// 252.227-7013 and 252.227-7014.

/* $Id: usort.h,v 0.3 2008/03/17 18:40:33 fmmaley Exp $ */

// F. Miller Maley, IDA/CCR (fmmaley@ccr.ida.nsa)
//
// SUMMARY:
//     The functions `usort' and `nusort' sort of an array of unsigned
// 64-bit quantities.  If they are able to allocate sufficient scratch
// space, they perform a stable, out-of-place sort; if not, they perform
// an unstable, partly in-place sort that may be slower.  (A sort is
// stable if items with equal keys stay in their original order.)  Both
// functions were originally optimized for the Alpha EV6.  They have the
// same interface, but `usort' is much faster than `nusort' if the sort
// keys are uniformly distributed.  It will run correctly but more
// slowly than `nusort' if this assumption is violated.
//     The function `usortx' is similar, but allows extra options: (0)
// an input flag specifies whether the keys are uniform; (1) multiword
// items can be sorted, although the sort key must be contained in the
// first word of an item; (2) halfword (32-bit) quantities can be
// sorted; (3) sorting can be done under an arbitrary mask; (4) the user
// can supply preallocated scratch space or control the amount of space
// to be allocated; (5) items whose keys are unique can be discarded,
// useful when sorting is being done to find duplicates; and (6) the
// user has some control over the sort radix.

// INTERFACES:

#include <micro64.h>
int64 usort(uint64* x, int64 n, int64 lb, int64 ub);
int64 nusort(uint64* x, int64 n, int64 lb, int64 ub);

// where
//     x[] = array of items (input and output)
//     n = number of items to be sorted (input)
//     lb,ub = lower and upper limits of the bitfield on which to sort
//         (input); the sort key comprises bits lb through ub inclusive,
//         where 0 means the lowest-order bit

#include <stddef.h>
#define USORT_UNIFORM  UI64(1)
#define USORT_BIGPAGES UI64(2)
#define USORT_UNSTABLE UI64(4)
#define USORT_PAREMASK UI64(8)
#define USORT_KILLUNIQUES UI64(16)
int64 usortx(void* x, int64 n, size_t s, uint64 m,
             void* y, size_t t, uint64 flags);

// where
//     x, n = as above
//     s = size of items to sort; must be 4 or a multiple of 8 (input)
//     m = mask under which to sort (input); must be nonzero
//     y = pointer to scratch space (input); can be NULL
//     t = size of scratch space in bytes (input); can be 0
//     flags = logical OR of any subset of the following values (input):
//         USORT_UNIFORM    if the keys are uniform;
//         USORT_BIGPAGES   if TLB misses can be ignored, which affects
//                              the choice of radix;
//         USORT_UNSTABLE   if an unstable sort is acceptable, which
//                              may be faster;
//         USORT_PAREMASK   to avoid sorting on constant bits;
//         USORT_KILLUNIQUES
//                          if items whose key matches no other item's
//                              key may be discarded; ignored unless
//                              USORT_UNIFORM; implies USORT_UNSTABLE
//         other options may be added in the future
//
// The pointer x, and the pointer y if non-NULL, must be 4-byte aligned
// if s==4 and 8-byte aligned otherwise.
//
// In the absence of the USORT_KILLUNIQUES option, a return value of 0
// means that a stable sort was performed, while a return value of 1
// means that an unstable sort was performed.  A negative return value
// indicates that an error occurred: -1 means that the parameters made
// no sense; -2 means that the item size s is not supported; -3 means
// that either x or y is not properly aligned; -4 means that pointers
// require more than 64 bits on this system (unlikely!) or that a
// structure containing a single 32-bit item is not 4 bytes long (also
// unlikely), in which case halfwords cannot be sorted.
//
// If the USORT_KILLUNIQUES flag is given, a nonnegative return value
// r is the number of items retained; only the first r items in x[] are
// then valid.  In this mode, usortx will try to discard most items
// whose keys are unique, but it does not guarantee to discard them all.
// It will discard no items unless USORT_UNIFORM is claimed, the mask
// bits are contiguous, and usortx is allowed to use scratch space.
// The retained items will be sorted, but not stably.
//
// If x is NULL, the return value (assuming the other parameters are
// valid) is either the number of bytes of scratch space needed to
// guarantee stable sorting, or if usortx was called with the
// USORT_UNSTABLE option, the ideal amount of scratch space for
// unstable sorting.
//
// If usortx is called with y non-NULL, it will use only the t bytes
// starting at address y for scratch space, and if t is zero, it will
// sort entirely in place.  If y is NULL, then usortx allocates its own
// space and deallocates it before returning.  In this case it will
// allocate at most t bytes if t is nonzero, and otherwise try to
// allocate enough scratch space to sort stably (unless USORT_UNSTABLE
// is selected) and as fast as possible.
//
// The amount of scratch space requested or allocated by usortx will
// depend, in general, on whether USORT_UNIFORM is selected.
//
// METHOD:
//     Both `usort' and `nusort' rely on radix sorts that typically
// sort on 5 or 6 bits per pass.
// 
// (1) `usort' uses fixed-size pockets that are large enough so that
// overflow should be very rare on random input.  The pockets are
// cache-aligned and padded so that "prestashing" (write hints) can be
// used without bounds-checking.  Normally, `usort' sorts on some number
// of high-order bits and uses insertion sort to finish up.  Pocket
// overflow is detected before it can cause any harm.  If a sorting pass
// fails due to overflow, `nusort' is called to finish up.
//
// (2) `nusort' performs a straight radix sort using variable pocket
// sizes that are computed by counting.  During each sorting pass
// except the last, `nusort' accumulates the counts for the next
// sorting pass; this reduces memory traffic and saves time.
// "Prestashing" is done when possible.
//
// (3) If insufficient scratch space is available, both functions
// perform one or more recursive passes of in-place radix sorting,
// finishing up with insertion sort if necessary.
//
// (4) If USORT_PAREMASK is specified, `usortx' makes an initial pass
// over the data to remove bits from the mask under which the data is
// constant.  This may improve performance on structured data.
//
// CAVEATS:
//     Both `usort' and `nusort' try to allocate temporary storage at
// least as large than the array of data to be sorted (larger for
// 'usort').  The first invocation of either function for a given data
// size will be slower than normal if the process heap needs to be
// expanded.  The same is true of `usortx' when no workspace is given.
// Performance of `nusort', or 'usortx' without the USORT_UNIFORM
// option, may suffer on some platforms due to cache thrashing if the
// keys are actually uniform and the data size is close to an exact
// multiple of a large power of two.
//     
// COMPILATION:
//     Requires the header files <micro64.h> and <bitops.h>, typically
// found in /usr/local/include/CANDLE/.  When compiled for the CCR
// library, it also needs <ccropt.h> in /usr/local/include/CCR/.  The C
// preprocessor option -DCACHE_MB=n should be given with the integer n
// being the size, in megabytes, of the target CPU's largest cache.  For
// a shared cache, divide the cache size by the number of CPUs sharing
// the cache.  If this option is omitted, it defaults to 1 (one MB).
//
// RESTRICTIONS:
//     The code *does not run* on Alphas prior to EV6 if compiled with
// the HP compiler (or any compiler that defines __DECC), because in
// those circumstances it uses the WH64 instruction in an attempt to
// improve performance.  Also, the code insists that pointers occupy at
// most 64 bits.  This should not be a problem in the foreseeable future.

/* $Log: usort.h,v $
 * Revision 0.3  2008/03/17 18:40:33  fmmaley
 * Added USORT_PAREMASK and USORT_KILLUNIQUES options to usortx
 *
 * Revision 0.2  2006/08/25 21:14:36  fmmaley
 * Removed radix parameter from usortx(); added flags for unstable sorting
 * big pages; revised the documentation, notably to explain additional
 * memory management options and the compile-time CACHE_MB parameter.
 *
 * Revision 0.1  2005/12/30 01:09:50  fmmaley
 * *** empty log message ***
 *
 */
