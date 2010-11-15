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
// my-types.H
// Mark Charney   <mark.charney@intel.com>
// $Id: my-types.H,v 1.2 2003/05/06 20:05:40 rscohn1 Exp $


#ifndef _MY_TYPES_H_
# define _MY_TYPES_H_

//#include "types_base.H"

typedef unsigned int UINT;



typedef enum {
    LOAD_CODE    = 0,
    STORE_CODE   = 1,
    PREFETCH_CODE= 2,
    ATOMIC_CODE  = 3,
    IFETCH_CODE   = 4,
    LAST_REF_CODE    = 5
} REF_CODE_ENUM;

class IPEA_T
{
  public:
    ADDRINT ip;
    ADDRINT ea;
};

class MISS_DATA_T
{
  public:
    ADDRINT ip;
    ADDRINT ea;
    REF_CODE_ENUM t;
};

class CACHE_CONFIG_T // simple cache configuration class
{
  public:
    UINT capacity; // in bytes!! typically input in KB and multiplied
    UINT assoc;
    UINT linesz; // bytes
};

////////////////////////////////////////////////////////////////////////////
#endif
