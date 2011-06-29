/** @file
 * This file defines the identifiers that are used to label the 
 * states of the TCGMSG activity in Vampir.
 */
#ifndef _TCGMSG_VAMPIR_H_
#define _TCGMSG_VAMPIR_H_

#include <VT.h>
#include <ga_vt.h>

#define TCGMSG_PBEGINF 32567
#define TCGMSG_PEND    32566
#define TCGMSG_NNODES  32565
#define TCGMSG_NODEID  32564
#define TCGMSG_LLOG    32563
#define TCGMSG_STATS   32562
#define TCGMSG_MTIME   32561
#define TCGMSG_TCGTIME 32560
#define TCGMSG_SND     32559
#define TCGMSG_RCV     32558
#define TCGMSG_PROBE   32557
#define TCGMSG_BRDCST  32556
#define TCGMSG_SYNCH   32555
#define TCGMSG_SETDBG  32554
#define TCGMSG_NXTVAL  32553
#define TCGMSG_PARERR  32552
#define TCGMSG_WAITCOM 32551
#define TCGMSG_DGOP    32550
#define TCGMSG_IGOP    32549
#define TCGMSG_MITOB   32548
#define TCGMSG_MDTOB   32547
#define TCGMSG_MITOD   32546
#define TCGMSG_MDTOI   32545
#define TCGMSG_PFCOPY  32544
#define TCGMSG_NICEFTN 32543
#define TCGMSG_EVON    32542
#define TCGMSG_EVOFF   32541
#define TCGMSG_EVBGIN  32540
#define TCGMSG_EVEND   32539
#define TCGMSG_EVENT   32538
#define TCGMSG_EVLOG   32537

extern void tcgmsg_vampir_init();

#endif /* _TCGMSG_VAMPIR_H_ */
