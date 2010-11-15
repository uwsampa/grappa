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
/* ================================================================== */
/* Miscellaneous Functions (possibily used by multiple pin tools)     */
/* ================================================================== */

#ifndef PINTOOL_UTILITIES_H
#define PINTOOL_UTILITIES_H

#include "pin.H"
#include "portability.H"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------ */
/* global constants                                                   */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* PrintHexWord:                                                      */
/* prints a formatted version of a little endian memory word          */
/* ------------------------------------------------------------------ */

VOID PrintHexWord(ADDRINT addr, fstream& outfile) 
{
    for (INT32 offset = sizeof(ADDRINT) - 1; offset >= 0; offset--) 
    {
        UINT32 val = (UINT32)*(UINT8*)(addr + offset);
        if (val < 0xf)        // formatting: pad single digits
        {
            outfile << "0" << flush;
        }
        outfile << hex << val << flush;
    }
}

/* ------------------------------------------------------------------ */
/* GetArg (overloaded):                                               */
/* extracts the integer argument value or flag from the command line  */
/* ------------------------------------------------------------------ */

VOID GetArg(UINT32 argc, char** argv, const char* argname, UINT32& arg, UINT32 default_val) 
{
    BOOL found = false;
    UINT32 i = 0;
    do 
    {
        string* tmp = new string(argv[i]);
        if (tmp->find(argname) != string::npos) 
        {
            ASSERTX((i + 1) < argc);
            arg = atoi(argv[i + 1]);
            found = true;
        }
        else 
        {
            i++;
        }
        delete tmp;
    } while (!found && (i < argc));
    if (!found) 
    {
        arg = default_val;
    }
}

VOID GetArg(UINT32 argc, char** argv, const char* argname, BOOL& arg) 
{
    BOOL found = false;
    UINT32 i = 0;
    do 
    {
        string* tmp = new string(argv[i]);
        if (tmp->find(argname) != string::npos) 
        {
            ASSERTX((i + 1) < argc);
            arg = true;
            found = true;
        }
        else 
        {
            i++;
        }
        delete tmp;
    } while (!found && (i < argc));
    if (!found) 
    {
        arg = false;
    }
}

/* ------------------------------------------------------------------ */
/* RemoveToolArgs                                                     */
/* Creates from the argv,argc pair new pair that does not contain the */
/* the tool arguments                                                 */
/* ------------------------------------------------------------------ */
#define MAX_ARGV 256
void RemoveToolArgs (int argc, char *argv[], int *myArgc, char *myArgv[])
{
    int i;
    for (i=0; i<argc; i++)
    {
        // copy this argv into myArgv
        ASSERTX(i<MAX_ARGV);
        myArgv[i] = argv[i];
        if(strcmp("-t", argv[i])==0)
        {
            break;
        }
    }
    // next one is tool name
    i++;
    myArgv[i] = argv[i];
    *myArgc = i+1;
    // now skip all until --
    while (strcmp(argv[i],"--")!=0)
    {
        i++;
        ASSERTX(i<MAX_ARGV);
    }
    // now copy the -- and the rest
    while (i<argc)
    {
        ASSERTX(i<MAX_ARGV);
        myArgv[*myArgc] = argv[i];
        i++;
        (*myArgc)++;
    }
}
    
/* ------------------------------------------------------------------ */

#endif  // #ifndef PINTOOL_UTILITIES_H
