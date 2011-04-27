#include "CaeSim.h"
#include "CaeIsa.h"
#include <stdio.h>

// number of AEG registers
#define MAX_AEG_INDEX 128

// AEG signature
#define PERS_SIGN_CAE          0x4001000101000LL

#define AEG_BASES 0
#define AEG_NUM_LISTS 1
#define AEG_COUNT 2
#define AEG_RESULT 3

#define MEM_REQ_SIZE 8

#define AEUIE 0
void
CCaeIsa::InitPers()
{
    SetAegCnt(MAX_AEG_INDEX);
    SetPersSign(PERS_SIGN_CAE);
}

void
CCaeIsa::CaepInst(int aeId, int opcode, int immed, uint32 inst, uint64 scalar) // F7,0,20-3F
{
    switch (opcode) {
	// CAEP00 - walk lists starting at 'bases', total responses 'count', 'numlists'
	case 0x20: {
        node** list_nodes;
        uint64 num_lists, count;
        uint64* result;

// TODO figure out these register namings!
	    list_nodes = ReadAeg(aeId, AEG_BASES);
	    num_lists = ReadAeg(aeId, AEG_NUM_LISTS);
	    count = ReadAeg(aeId, AEG_COUNT);
        result = ReadAeg(aeId, AEG_RESULT);
        while(count > 0) {
            for (uint64 listnum = 0; listnum < num_lists; listnum++) {
                int mc = (list_nodes[listnum] >> 6) & 7;
                node* next;
                AeMemLoad(aeI, mc, list_nodes[listnum], MEM_REQ_SIZE, false, next);
                list_nodes[listnum] = next;
                --count;
                if (count == 0) break;
            }
        }

	}

	default:{
	    printf("Default case hit - opcode = %x\n", opcode);
	    for (int aeId = 0; aeId < CAE_AE_CNT; aeId += 1)
		SetException(aeId, AEUIE);
	}
    }
}

