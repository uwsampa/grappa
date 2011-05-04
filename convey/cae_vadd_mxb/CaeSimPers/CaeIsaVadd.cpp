#include "CaeSim.h"
#include "CaeIsa.h"
#include <stdio.h>
#include <stdlib.h>

#include "../app/linked_list-node.h"
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
	list_nodes = (node**) ReadAeg(aeId, AEG_BASES);
	num_lists = ReadAeg(aeId, AEG_NUM_LISTS);
	    count = ReadAeg(aeId, AEG_COUNT);
	    result = (uint64*) ReadAeg(aeId, AEG_RESULT);
	    uint64 result_val = 0;

	    node** mybases = (node**)malloc(sizeof(node*)*num_lists);
            for (unsigned int i=0; i< num_lists; ++i) {
              int mc = (((uint64)(&list_nodes[i])) >> 6) & 7;
	      uint64 base_int = 0;
	      AeMemLoad(aeId, mc, (uint64) &list_nodes[i], MEM_REQ_SIZE, false, base_int);
	      mybases[i] = (node*) base_int;
	    }

        while(count > 0) {
            for (uint64 listnum = 0; listnum < num_lists; listnum++) {
	      

	      int mc = (((uint64)mybases[listnum]) >> 6) & 7;
                node* next;
                //AeMemLoad(aeId, mc, (uint64)mybases[listnum], MEM_REQ_SIZE, false, (uint64)next);
		uint64 addr_int = (uint64) mybases[listnum];
		uint64 next_int = 0;
		printf("loading from %lp\n", addr_int);
		AeMemLoad(aeId, mc, addr_int, MEM_REQ_SIZE, false, next_int);
		next = (node*) next_int;
                mybases[listnum] = next;
		result_val += addr_int;
                --count;
                if (count == 0) break;
            }
        }
	printf("result_val is %lp\n", result_val);
	WriteAeg(aeId, 30+aeId, result_val);
	
	  break;
	}

	default:{
	    printf("Default case hit - opcode = %x\n", opcode);
	    for (int aeId = 0; aeId < CAE_AE_CNT; aeId += 1)
		SetException(aeId, AEUIE);
	}
    }
}

