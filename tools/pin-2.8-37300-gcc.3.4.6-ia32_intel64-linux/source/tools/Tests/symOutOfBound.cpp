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
/*
  @ORIGINAL_AUTHOR: Daniel Lemel
 */

/*! @file
 *  Make sure the second SYM has greater index than the first one,
 *  and the SYM before the first one is invalid.
 */

#include "pin_tests_util.h"

BOOL test = true;

VOID ImgFirst(IMG img, VOID * v)
{
    if (!test)
        return;
    test = false;

    SYM sym = IMG_RegsymHead(img);
    TEST(SYM_Valid(sym), "IMG_RegsymHead failed");
    UINT32 headIndex = SYM_Index(sym);

    sym = SYM_Next(sym);
    TEST(SYM_Valid(sym) && SYM_Index(sym) > headIndex, "SYM_Index failed");

    sym = SYM_Prev(IMG_RegsymHead(img));
    TEST(sym == SYM_Invalid(), "SYM_Prev failed");
}

int main(INT32 argc, CHAR **argv)
{
    PIN_InitSymbols();
    PIN_Init(argc, argv);
    
    IMG_AddInstrumentFunction(ImgFirst, 0);
    
    // Never returns
    PIN_StartProgram();
    
    return 0;
}
