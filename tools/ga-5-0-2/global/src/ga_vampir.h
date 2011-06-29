/* $Id: ga_vampir.h,v 1.8.2.2 2007-09-25 22:39:44 d3g293 Exp $ */
#ifndef _GA_VAMPIR_H_
#define _GA_VAMPIR_H_

/* This file defines the codes that are used to label
   GA routines in VAMPIR
*/
#include <VT.h>
#include <ga_vt.h>

#define GA_ACC                32400
#define GA_ACCESS             32399
#define GA_ADD                32398
#define GA_ADD_PATCH          32397
#define GA_BRDCST             32396
#define GA_CHECK_HANDLE       32395
#define GA_COMPARE_DISTR      32393
#define GA_COPY               32392
#define GA_COPY_PATCH         32391
#define GA_CREATE             32390
#define GA_CREATE_IRREG       32389
#define GA_CREATE_MUTEXES     32388
#define GA_DDOT               32387
#define GA_DDOT_PATCH         32386
#define GA_DESTROY            32385
#define GA_DESTROY_MUTEXES    32384
#define GA_DGOP               32382
#define GA_GOP                32381
#define GA_DISTRIBUTION       32378
#define GA_DOT                32377
#define GA_DOT_PATCH          32376
#define GA_DUPLICATE          32375
#define GA_ERROR              32374
#define GA_FENCE              32373
#define GA_FILL               32372
#define GA_FILL_PATCH         32371
#define GA_GATHER             32369
#define GA_GET                32368
#define GA_IGOP               32367
#define GA_INITIALIZE         32366
#define GA_INITIALIZE_LTD     32365
#define GA_INIT_FENCE         32364
#define GA_INQUIRE            32363
#define GA_INQUIRE_MEMORY     32362
#define GA_INQUIRE_NAME       32361
#define GA_LIST_NODEID        32360
#define GA_LOCATE             32358
#define GA_LOCATE_REGION      32357
#define GA_LOCK               32356
#define GA_MATMUL_PATCH       32354
#define GA_MEMORY_AVAIL       32353
#define GA_MEMORY_LIMITED     32352
#define GA_MPI_COMMUNICATOR   32351
#define GA_NBLOCK             32350
#define GA_NDIM               32349
#define GA_NNODES             32348
#define GA_NODEID             32347
#define GA_PERIODIC_ACC       32346
#define GA_PERIODIC_GET       32345
#define GA_PERIODIC_PUT       32344
#define GA_PRINT              32343
#define GA_PRINT_DISTRIBUTION 32342
#define GA_PRINT_PATCH        32341
#define GA_PRINT_STATS        32340
#define GA_PROC_TOPOLOGY      32339
#define GA_PUT                32338
#define GA_READ_INC           32337
#define GA_RELEASE            32336
#define GA_RELEASE_UPDATE     32335
#define GA_SCALE              32334
#define GA_SCALE_PATCH        32333
#define GA_SCATTER            32332
#define GA_SELECT_ELEM        32331
#define GA_SET_MEMORY_LIMIT   32330
#define GA_SUMMARIZE          32327
#define GA_SYNC               32325
#define GA_TERMINATE          32324
#define GA_TRANSPOSE          32323
#define GA_UNLOCK             32322
#define GA_USES_MA            32321
#define GA_ZDOT               32320
#define GA_ZDOT_PATCH         32319
#define GA_ZERO               32318
#define GA_ZERO_PATCH         32317
#define NGA_ACC               32316
#define NGA_ACCESS            32315
#define NGA_ADD_PATCH         32314
#define NGA_COPY_PATCH        32313
#define NGA_CREATE            32312
#define NGA_CREATE_IRREG      32311
#define NGA_DDOT_PATCH        32310
#define NGA_DISTRIBUTION      32309
#define NGA_FILL_PATCH        32308
#define NGA_GATHER            32307
#define NGA_GET               32306
#define NGA_INQUIRE           32305
#define NGA_LOCATE            32304
#define NGA_LOCATE_REGION     32303
#define NGA_PERIODIC_ACC      32302
#define NGA_PERIODIC_GET      32301
#define NGA_PERIODIC_PUT      32299
#define NGA_PUT               32298
#define NGA_READ_INC          32297
#define NGA_RELEASE           32296
#define NGA_RELEASE_UPDATE    32295
#define NGA_SCALE_PATCH       32294
#define NGA_SCATTER           32293
#define NGA_SELECT_ELEM       32292
#define NGA_ZDOT_PATCH        32291
#define NGA_ZERO_PATCH        32290
#define VT_GA_DGEMM           32289
#define GA_DGEMM_SEQ          32288
#define GA_SYMMETRIZE         32287
#define NGA_CREATE_GHOSTS_IRREG_CONFIG     32286
#define NGA_CREATE_IRREG_CONFIG            32285
#define NGA_NBPUT             32284
#define NGA_STRIDED_PUT       32283
#define NGA_STRIDED_GET       32282
#define NGA_STRIDED_ACC       32281
#define GA_ALLOCATE           32280
#define GA_MATMUL             32279
#define NGA_MATMUL_PATCH      32278
#define VT_GA_SGEMM           32277
#define VT_GA_ZGEMM           32276
#define GA_PGROUP_SYNC        32275
#define GA_CDOT               32274
#define GA_CDOT_PATCH         32273
#define NGA_CDOT_PATCH        32272
#define VT_GA_CGEMM           32271
#include "ga_vampir.fh"

extern void ga_vampir_init(); 

#endif
