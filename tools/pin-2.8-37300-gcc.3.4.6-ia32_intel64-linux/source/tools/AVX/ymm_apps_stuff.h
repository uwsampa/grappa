/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2010 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
#ifdef YMM_APPS_STUFF_H
#error duplicate inclusion of YMM_APPS_STUFF_H
#else
#define YMM_APPS_STUFF_H
#if defined( __GNUC__)

#include <stdint.h>
typedef uint8_t  UINT8;   //LINUX HOSTS
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int8_t  INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;

#define ALIGN16 __attribute__ ((aligned(16)))
#define ALIGN8  __attribute__ ((aligned(8)))

#elif defined(_MSC_VER)

typedef unsigned __int8 UINT8 ;
typedef unsigned __int16 UINT16;
typedef unsigned __int32 UINT32;
typedef unsigned __int64 UINT64;
/*
typedef __int8 INT8;
typedef __int16 INT16;
typedef __int32 INT32;
typedef __int64 INT64;
*/

#define ALIGN16 __declspec(align(16))
#define ALIGN8  __declspec(align(8))

#else
#error Expect usage of either GNU or MS compiler.
#endif


#define MAX_BYTES_PER_YMM_REG 32
#define MAX_WORDS_PER_YMM_REG (MAX_BYTES_PER_YMM_REG/2)
#define MAX_DWORDS_PER_YMM_REG (MAX_WORDS_PER_YMM_REG/2)
#define MAX_QWORDS_PER_YMM_REG (MAX_DWORDS_PER_YMM_REG/2)
#define MAX_FLOATS_PER_YMM_REG (MAX_BYTES_PER_YMM_REG/sizeof(float))
#define MAX_DOUBLES_PER_YMM_REG (MAX_BYTES_PER_YMM_REG/sizeof(double))




union ALIGN16 ymm_reg_t
{
    UINT8  byte[MAX_BYTES_PER_YMM_REG];
    UINT16 word[MAX_WORDS_PER_YMM_REG];
    UINT32 dword[MAX_DWORDS_PER_YMM_REG];
    UINT64 qword[MAX_QWORDS_PER_YMM_REG];


    float  flt[MAX_FLOATS_PER_YMM_REG];
    double dbl[MAX_DOUBLES_PER_YMM_REG];

};

extern "C" void set_ymm_reg0(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg0(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg1(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg1(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg2(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg2(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg3(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg3(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg4(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg4(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg5(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg5(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg6(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg6(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg7(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg7(ymm_reg_t& ymm_reg);
#ifdef TARGET_IA32E
extern "C" void set_ymm_reg8(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg8(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg9(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg9(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg10(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg10(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg11(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg11(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg12(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg12(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg13(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg13(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg14(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg14(ymm_reg_t& ymm_reg);
extern "C" void set_ymm_reg15(ymm_reg_t& ymm_reg);
extern "C" void get_ymm_reg15(ymm_reg_t& ymm_reg);
#endif
extern "C" void mmx_save(char* buf);
extern "C" void mmx_restore(char* buf);


static void
set_ymm_reg(ymm_reg_t& ymm_reg, UINT32 reg_no)
{

    switch (reg_no)
    {
    case 0:
        set_ymm_reg0(ymm_reg);
        break;
    case 1:
        set_ymm_reg1(ymm_reg);
        break;
    case 2:
        set_ymm_reg2(ymm_reg);
        break;
    case 3:
        set_ymm_reg3(ymm_reg);
        break;
    case 4:
        set_ymm_reg4(ymm_reg);
        break;
    case 5:
        set_ymm_reg5(ymm_reg);
        break;
    case 6:
        set_ymm_reg6(ymm_reg);
        break;
    case 7:
        set_ymm_reg7(ymm_reg);
        break;
#ifdef TARGET_IA32E
	case 8:
        set_ymm_reg8(ymm_reg);
        break;
	case 9:
        set_ymm_reg9(ymm_reg);
        break;
	case 10:
        set_ymm_reg10(ymm_reg);
        break;
	case 11:
        set_ymm_reg11(ymm_reg);
        break;
	case 12:
        set_ymm_reg12(ymm_reg);
        break;
	case 13:
        set_ymm_reg13(ymm_reg);
        break;
	case 14:
        set_ymm_reg14(ymm_reg);
        break;
	case 15:
        set_ymm_reg15(ymm_reg);
        break;
#endif
    }

} 
static void
get_ymm_reg(ymm_reg_t& ymm_reg, UINT32 reg_no)
{
    switch (reg_no)
    {
    case 0:
       get_ymm_reg0(ymm_reg);
       break;
    case 1:
        get_ymm_reg1(ymm_reg);
        break;
    case 2:
        get_ymm_reg2(ymm_reg);
        break;
    case 3:
        get_ymm_reg3(ymm_reg);
        break;
    case 4:
        get_ymm_reg4(ymm_reg);
        break;
    case 5:
        get_ymm_reg5(ymm_reg);
        break;
    case 6:
        get_ymm_reg6(ymm_reg);
        break;
    case 7:
        get_ymm_reg7(ymm_reg);
        break;
#ifdef TARGET_IA32E
	case 8:
        get_ymm_reg8(ymm_reg);
        break;
	case 9:
        get_ymm_reg9(ymm_reg);
        break;
	case 10:
        get_ymm_reg10(ymm_reg);
        break;
	case 11:
        get_ymm_reg11(ymm_reg);
        break;
	case 12:
        get_ymm_reg12(ymm_reg);
        break;
	case 13:
        get_ymm_reg13(ymm_reg);
        break;
	case 14:
        get_ymm_reg14(ymm_reg);
        break;
	case 15:
        get_ymm_reg15(ymm_reg);
        break;
#endif
    }

}






void write_ymm_reg(UINT32 reg_no, UINT32 val)
{

    ymm_reg_t ymm;
    ymm.dword[0] = val;
    ymm.dword[1] = val;
    ymm.dword[2] = val;
    ymm.dword[3] = val;
	ymm.dword[4] = val;
	ymm.dword[5] = val;
	ymm.dword[6] = val;
	ymm.dword[7] = val;
    set_ymm_reg(ymm, reg_no); 
}

void read_ymm_reg(UINT32 reg_no, ymm_reg_t& ymm)
{

    ymm.dword[0] = 0;
    ymm.dword[1] = 0;
    ymm.dword[2] = 0;
    ymm.dword[3] = 0;
	ymm.dword[4] = 0;
	ymm.dword[5] = 0;
	ymm.dword[6] = 0;
	ymm.dword[7] = 0;
    get_ymm_reg(ymm, reg_no); 
}

#ifdef TARGET_IA32E
#define NUM_YMM_REGS 16
#else
#define NUM_YMM_REGS 8
#endif
#endif