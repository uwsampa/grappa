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
/*! @file
 * This application verifies that Pin correctly emulates exceptions when the 
 * application attempts to execute an invalid or inaccessible instruction.
 */

#include <string>
#include <iostream>


#if defined(TARGET_WINDOWS)
#include "windows.h"
#define EXPORT_CSYM extern "C" __declspec( dllexport )
#else
#error Unsupported OS
#endif

using namespace std;



//==========================================================================
// Printing utilities
//==========================================================================
string UnitTestName("raise_exception");
string FunctionTestName;

static void StartFunctionTest(const string & functionTestName)
{
    if (FunctionTestName != "")
    {
        cerr << UnitTestName << "[" << FunctionTestName  << "] Start" << endl;
    }
    FunctionTestName = functionTestName;
}

static void ExitUnitTest()
{
    if (FunctionTestName != "")
    {
        cerr << UnitTestName << "[" << FunctionTestName  << "] Success" << endl;
    }
    cerr << UnitTestName << " : Completed successfully" << endl;
    exit(0);
}

static void Abort(const string & msg)
{
    cerr << UnitTestName << "[" << FunctionTestName  << "] Failure: " << msg << endl;
    exit(1);
}

typedef unsigned __int8 UINT8 ;
typedef unsigned __int16 UINT16;
typedef unsigned __int32 UINT32;
typedef unsigned __int64 UINT64;

#include "xsave_type.h"
#include "fp_context_manip.h"




//==========================================================================
// Exception handling utilities
//==========================================================================

/*!
 * Exception filter for the ExecuteSafe function: copy the exception record 
 * to the specified structure.
 * @param[in] exceptPtr        pointers to the exception context and the exception 
 *                             record prepared by the system
 * @param[in] pExceptRecord    pointer to the structure that receives the 
 *                             exception record
 * @return the exception disposition
 */
static int enteredFilter = 0;
static int SafeExceptionFilter(LPEXCEPTION_POINTERS exceptPtr, 
                               EXCEPTION_RECORD * pExceptRecord)
{
    enteredFilter = 1;
    // check the fp state in the exceptPtr->ContextRecord is what was set just prior to the exception 
    // by call to SetMyFpContext in ExecuteSafe below.
    // Note that the raise_exception tool will have changed the fp register state to something else just before it
    // calls PIN_RaiseException
    if (!CheckMyFpContextInContext(exceptPtr->ContextRecord, 0))
    {
        Abort("Mismatch in the FP context");
    }
    *pExceptRecord = *exceptPtr->ExceptionRecord;
    return EXCEPTION_EXECUTE_HANDLER;
}

/*!
 * Execute the specified function and return to the caller even if the function raises 
 * an exception.
 * @param[in] pExceptRecord    pointer to the structure that receives the 
 *                             exception record if the function raises an exception
 * @return TRUE, if the function raised an exception
 */
template <typename FUNC>
    bool ExecuteSafe(FUNC fp, EXCEPTION_RECORD * pExceptRecord)
{
    __try
    {
        SetMyFpContext(0); // set fp context to a state that will be checked at the exception reception point:
                           // SafeExceptionFilter
        fp();  // raise_exception tool will have replaced this function (WriteToNull) with a function
               // that calls PIN_RaiseException to raise the EXCEPTION_ACCESS_VIOLATION
        return false;
    }
    __except (SafeExceptionFilter(GetExceptionInformation(), pExceptRecord))
    {
        return true;
    }
}


template <typename FUNC, typename ARG> 
    bool ExecuteSafeWithArg(FUNC fp, ARG arg, EXCEPTION_RECORD * pExceptRecord)
{
    __try
    {
        SetMyFpContext(0); // set fp context to a state that will be checked at the exception reception point:
                           // SafeExceptionFilter
        fp(arg);
        return false;
    }
    __except (SafeExceptionFilter(GetExceptionInformation(), pExceptRecord))
    {
        return true;
    }
}



/*!
 * The tool intercepts this function and raises an EXCEPTION_ACCESS_VIOLATION exception. 
 */
EXPORT_CSYM void WriteToNull()
{
    ;
}


/*!
 * The tool intercepts this function and raises an EXCEPTION_X87_OVERFLOW exception. 
 */
EXPORT_CSYM void RaiseX87OverflowException()
{
    ;
}

/*!
* The tool intercepts this function and raises the specified system exception. 
*/
EXPORT_CSYM void RaiseSystemException(unsigned sysExceptCode)
{
    ;
}




/*!
 * Check to see if the specified exception record represents an exception with the 
 * specified exception code.
 */
static bool CheckExceptionCode(EXCEPTION_RECORD * pExceptRecord, unsigned exceptCode)
{
    if (pExceptRecord->ExceptionCode != exceptCode) 
    {
        cerr << "Unexpected exception code " << 
            hex << pExceptRecord->ExceptionCode << ". Should be " << 
            hex << exceptCode << endl; 
        return false;
    }
    return true;
}





typedef void FUNC_NOARGS(); 

/*!
 * The main procedure of the application.
 */
int main(int argc, char *argv[])
{
    FUNC_NOARGS * fpNull;
    EXCEPTION_RECORD exceptRecord;
    bool exceptionCaught;

    

    // Raise FP exception in the tool
    StartFunctionTest("Raise FP exception in the tool");
    exceptionCaught = ExecuteSafe(RaiseX87OverflowException, &exceptRecord);
    if (!exceptionCaught) {Abort("Unhandled exception");}
    if (!CheckExceptionCode(&exceptRecord, EXCEPTION_FLT_OVERFLOW)) 
    {
        Abort("Incorrect exception information");
    }
    if (!enteredFilter)
    {
        Abort("SafeExceptionFilter never called");
    }

    enteredFilter = 0;
    // Raise a system-specific exception in the tool
    StartFunctionTest("Raise system-specific exception in the tool");
    exceptionCaught = ExecuteSafeWithArg(RaiseSystemException, EXCEPTION_INVALID_HANDLE, &exceptRecord);
    if (!exceptionCaught) {Abort("Unhandled exception");}
    if (!CheckExceptionCode(&exceptRecord, EXCEPTION_INVALID_HANDLE)) 
    {
        Abort("Incorrect exception information");
    }



    ExitUnitTest();
}