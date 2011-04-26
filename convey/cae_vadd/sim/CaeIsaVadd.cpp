#include "CaeSim.h"
#include "CaeIsa.h"
#include <stdio.h>

#define MAX_AEG_INDEX 128
#define PERS_SIGN_CAE          0x4001000101000LL

#define AEG_MA1 0
#define AEG_MA2 1
#define AEG_MA3 2
#define AEG_CNT 3
#define AEG_SAE_BASE 30 
#define AEG_STL 40

#define NUM_MCS 8
#define NUM_PIPES 16
#define MEM_REQ_SIZE 8

#define AEUIE 0
void
CCaeIsa::InitPers()
{
    SetAegCnt(MAX_AEG_INDEX);
    WriteAeg(0, 0, 0);
    SetPersSign(PERS_SIGN_CAE);
    // clear the sum registers
    for (int aeId = 0; aeId < 4; aeId += 1) {
	WriteAeg(aeId, AEG_SAE_BASE+aeId, 0);
    }
}

void
CCaeIsa::CaepInst(int aeId, int opcode, int immed, uint32 inst, uint64 scalar) // F7,0,20-3F
{
    switch (opcode) {
	// CAEP00 - M[a1] + M[a2] -> M[a3]
	case 0x20: {
	    uint64 length, a1, a2, a3;
	    uint64 val1, val2, val3, sum = 0;

	    length = ReadAeg(aeId, AEG_CNT);
	    a1 = ReadAeg(aeId, AEG_MA1);
	    a2 = ReadAeg(aeId, AEG_MA2);
	    a3 = ReadAeg(aeId, AEG_MA3);
	    for (int mc = 0; mc < NUM_MCS; mc += 1) {
		for (uint64 i = 0; i < length; i += 1) {
		    // Check that address is right for this MC (virtual address bits 8:6 for binary interleave)
		    if ((int)((a1+i*8 >> 6) & 7) == mc) {
			AeMemLoad(aeId, mc, a1+i*8, MEM_REQ_SIZE, false, val1);
			AeMemLoad(aeId, mc, a2+i*8, MEM_REQ_SIZE, false, val2);
			val3 = val1 + val2;
			sum += val3;
			AeMemStore(aeId, mc, a3+i*8, MEM_REQ_SIZE, false, val3);
		    }
		}
	    }
	    WriteAeg(aeId, AEG_SAE_BASE+aeId, sum);
	    break;
	}

	default:{
	    printf("Default case hit - opcode = %x\n", opcode);
	    for (int aeId = 0; aeId < CAE_AE_CNT; aeId += 1)
		SetException(aeId, AEUIE);
	}
    }
}

