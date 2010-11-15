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
#include <pin.H>
#include <map>
#include <fstream>
#include <iostream>

class REF
{
  public:
    REF(UINT64 global, UINT64 nonglobal) 
    {
        _global = global;
        _nonglobal = nonglobal;
    };
    BOOL PredictNonGlobal() const;
    UINT64 Count() const 
    {
        return _global + _nonglobal;
    };
    
    
    UINT64 _global;
    UINT64 _nonglobal;
};

class PROFILE
{
  private:
    typedef map<string, REF> REFS;
    
  public:
    VOID ReadFile(char * filename);
    UINT64 SumRefs();
    VOID NonGlobalPredicts(PROFILE * fullProfile, UINT64 * correct, UINT64 * incorrect, UINT64 * predicted);
    
    REFS _refs;
    UINT32 _tracesCodeCacheSize;
    UINT32 _expiredTracesCodeCacheSize;
    float _cpuTime;
};

BOOL REF::PredictNonGlobal() const
{
    return (float)_global/(_global + _nonglobal) < .01;
}

VOID PROFILE::ReadFile(CHAR * filename)
{
    ifstream infile(filename);

    if (!infile)
    {
        cerr << "Could not open " << filename << endl;
        exit(1);
    }

    while(true)
    {
        string command;
        
        infile >> command;

        if (command == "BeginProfile")
        {
            break;
        }
        else if (command == "TracesCodeCacheSize")
        {
            infile >> _tracesCodeCacheSize;
        }
        else if (command == "ExpiredTracesCodeCacheSize")
        {
            infile >> _expiredTracesCodeCacheSize;
        }
        else if (command == "CpuTime")
        {
            infile >> _cpuTime;
        }
        else
        {
            cerr << "Unknown command " << command << endl;
            exit(1);
        }
    }

    while(true)
    {
        string address;
        UINT64 global;
        UINT64 nonglobal;
        
        infile >> address;

        if (infile.eof())
            break;
        
        infile >> global;
        infile >> nonglobal;

        _refs.insert(pair<string,REF>(address,REF(global,nonglobal)));
    }
}

UINT64 PROFILE::SumRefs()
{
    UINT64 numRefs = 0;
    
    for (REFS::const_iterator ri = _refs.begin(); ri != _refs.end(); ri++)
    {
        numRefs += ri->second.Count();
    }

    return numRefs;
}

    
// The number of dynamic times we predicted non-global and were wrong
// Use the fullprofile for the correct/incorrect counts
// also compute number of global predicts using our count
VOID PROFILE::NonGlobalPredicts(PROFILE * fullProfile, UINT64 * correct, UINT64 * incorrect, UINT64 * predicted)
{
    *correct = 0;
    *incorrect = 0;
    *predicted = 0;
    
    for (REFS::const_iterator ri = _refs.begin(); ri != _refs.end(); ri++)
    {
        string address = ri->first;
        const REF & partialRef = ri->second;
        
        if (partialRef.PredictNonGlobal())
        {
            *predicted += partialRef.Count();
            
            REFS::iterator ri = fullProfile->_refs.find(address);

            // Nothing in the reference? It is not a problem.
            if (ri == fullProfile->_refs.end())
                continue;

            REF & fullRef = ri->second;
            
            if (fullRef.PredictNonGlobal())
                *correct += fullRef.Count();
            else
                *incorrect += fullRef.Count();
        }
    }
}

    
int main(int argc, char * argv[])
{
    if (argc < 3)
    {
        fprintf(stderr,"Usage: %s <fullprofile> <partialprofile>\n",argv[0]);
        exit(1);
    }

    PROFILE fullProfile, partialProfile;
    fullProfile.ReadFile(argv[1]);
    partialProfile.ReadFile(argv[2]);

    //UINT64 fullNumRefs = fullProfile.SumRefs();

    UINT64 partialCorrect, partialIncorrect, partialPredicts;
    partialProfile.NonGlobalPredicts(&fullProfile, &partialCorrect, &partialIncorrect, &partialPredicts);

    UINT64 fullCorrect, fullIncorrect, fullPredicts;
    fullProfile.NonGlobalPredicts(&fullProfile, &fullCorrect, &fullIncorrect, &fullPredicts);

    cout << "CpuTime " << partialProfile._cpuTime << endl;
    cout << "ExpiredTraces " << partialProfile._expiredTracesCodeCacheSize << " " << partialProfile._tracesCodeCacheSize
         << " " << (float)partialProfile._expiredTracesCodeCacheSize/(float)partialProfile._tracesCodeCacheSize << endl;
    cout << "NonGlobalCoverage " << partialCorrect << " " << fullPredicts << " " << (float)partialCorrect/(float)fullPredicts << endl;
    cout << "NonGlobalMispredict " << partialIncorrect << " " << fullPredicts << " " << (float)partialIncorrect/(float)fullPredicts << endl;
}

    
