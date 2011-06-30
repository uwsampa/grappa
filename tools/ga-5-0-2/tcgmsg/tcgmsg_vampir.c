#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include "tcgmsg_vampir.h"

void tcgmsg_vampir_init() {
    vampir_symdef(TCGMSG_PBEGINF,"pbeginf","tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_PEND,   "pend",   "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_NNODES, "nnodes", "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_NODEID, "nodeid", "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_LLOG,   "llog",   "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_STATS,  "stats",  "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_MTIME,  "mtime",  "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_TCGTIME,"tcgtime","tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_SND,    "snd",    "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_RCV,    "rcv",    "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_PROBE,  "probe",  "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_BRDCST, "brdcst", "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_SYNCH,  "synch",  "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_SETDBG, "setdbg", "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_NXTVAL, "nxtval", "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_PARERR, "parerr", "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_WAITCOM,"waitcom","tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_DGOP,   "dgop",   "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_IGOP,   "igop",   "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_MITOB,  "mitob",  "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_MDTOB,  "mdtob",  "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_MITOD,  "mitod",  "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_MITOD,  "mitod",  "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_MDTOI,  "mdtoi",  "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_PFCOPY, "pfcopy", "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_NICEFTN,"niceftn","tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_EVON,   "evon",   "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_EVOFF,  "evoff",  "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_EVBGIN, "evbgin", "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_EVEND,  "evend",  "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_EVENT,  "event",  "tcgmsg",__FILE__,__LINE__);
    vampir_symdef(TCGMSG_EVLOG,  "evlog",  "tcgmsg",__FILE__,__LINE__);
}
