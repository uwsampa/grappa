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
 * @ORIGINAL_AUTHOR: Gail Lyons
 */
/*! @file
 *
 * This file defines the Pin error types.  They are used
 * in conjunction with the Pin -error_file option.
 *
 */

#ifndef PIN_ERRTYPE_H
#define PIN_ERRTYPE_H

/* ADD NEW ERROR TYPES AT THE END (right before PIN_ERR_LAST)!!!
 * Descriptions on the right hand side appear the User's Guide.
 * Please document all PIN_ERRTYPEs.
 */

/*! @ingroup  ERROR_FILE_BASIC
 * Severity used by PIN_WriteErrorMessage()
 */
typedef enum 
{
    PIN_ERR_FATAL,                 /*!< fatal error, Pin exits after printing message. */
    PIN_ERR_NONFATAL               /*!< non-fatal error */
} PIN_ERR_SEVERITY_TYPE;

                                     

/*! @ingroup  ERROR_FILE_BASIC
 * Client level errors reported by Pin.
 */
typedef enum 
{
    PIN_ERR_NONE = 0,               /*!< 0 args. no error. */
    PIN_ERR_INTERNAL,               /*!< 0 args. internal pin error has occurred. */
    PIN_ERR_ASSERT,                 /*!< 0 args. internal pin assertion has occurred. */
    PIN_ERR_TOOL,                   /*!< 0 args. tool error has been detected. */
    PIN_ERR_CMD_LINE,               /*!< 0 args. tool arguments are incorrect. */
    PIN_ERR_PARSING,                /*!< 0 args. command line arguments are incorrect. */
    PIN_ERR_NO_APP,                 /*!< 0 args. missing application name. */
    PIN_ERR_INVALID_PLOAD,          /*!< 0 args. invalid pload argument. */
    PIN_ERR_FILE_NOT_FOUND,         /*!< 1 arg:  file name. error in application file name or path. */
    PIN_ERR_ELF_HEADER,             /*!< 1 arg:  file name. unable to read elf header of "filename".  */
    PIN_ERR_NO_PIE,                 /*!< 1 arg:  file name. pie binaries not supported on this system.  */
    PIN_ERR_NO_ENTRY_OFFSET,        /*!< 1 arg:  file name. no entry in aux vector. */
    PIN_ERR_INVALID_ADDRESS,        /*!< 1 arg:  address. "address" is invalid. */
    PIN_ERR_INVALID_ADDRESS_RANGE,  /*!< 2 args: low address, high address. address range is invalid. */
    PIN_ERR_CANNOT_LOAD_TOOL,       /*!< 1 arg:  tool name. pin cannot load "tool". */
    PIN_ERR_PIN_INJECTION,          /*!< 0 arg:  can't inject pin in running process. */
    PIN_ERR_WAIT_FOR_INJECTOR,      /*!< 2 args: pid, errno. injector "pid" failed to exit. */
    PIN_ERR_BAD_OS_FILE,            /*!< 0 args. cannot determine os release. */
    PIN_ERR_UNSUPPORTED_OS,         /*!< 0 args. unsupported os. */
    PIN_ERR_BIN32,                  /*!< 0 args. 32-bit binary detected. */
    PIN_ERR_BIN64,                  /*!< 0 args. 64-bit binary detected. */
    PIN_ERR_CANNOT_RUN,             /*!< 1 arg:  errno. error in running pin. */
    PIN_ERR_ATTACH_PID,             /*!< 2 arg:  pid, errno. attach to "pid" failed. */
    PIN_ERR_ATTACH_NYI,             /*!< 0 arg:  attach to running process is NYI for Pin/Windows in JIT mode. */
    PIN_ERR_DETACH_PID,             /*!< 2 arg:  pid, errno. detach failed. */
    PIN_ERR_WAIT_PID,               /*!< 2 arg:  pid, errno. wait for child failed. */
    PIN_ERR_STEP_PID,               /*!< 2 arg:  pid, errno. single step failed. */
    PIN_ERR_WAIT_EXECV,             /*!< 1 arg:  errno. wait after execv failed. */
    PIN_ERR_APP_RESUME,             /*!< 1 arg:  errno. resuming application failed. */
    PIN_ERR_NO_INSTRUMENT,          /*!< 2 args: file name, errno. could not instrument process. */
    PIN_ERR_CHILD_PTRACEME,         /*!< 1 arg:  errno. parent cannot trace the child. */
    PIN_ERR_CHILD_RESUME,           /*!< 1 arg:  errno. resuming child failed. */
    PIN_ERR_RELINK_APP,             /*!< 0 args: application address overlaps with pin, relink. */
    PIN_ERR_KNOB_BAD,               /*!< 1 arg:  knob. unknown "knob" on command line. */
    PIN_ERR_NO_RELOCS,              /*!< 0 args. regular relocations not found, relink with -q.  */
    PIN_ERR_VERSION_SKEW,           /*!< 0 args. pintool version does not match pin version. */
    PIN_ERR_MS_DOS,                 /*!< 0 args. ms-dos & win16 application are not supported. */
    PIN_ERR_LONG_PATH,              /*!< 0 args. fully qualified pathname of pin executable is too long. */
    PIN_ERR_CANNOT_EXECUTE_FILE,    /*!< 1 arg:  filename. cannot execute application. */
    PIN_ERR_BLOCKSIG_TOOL,          /*!< 1 arg: signal number.  Tool intercepted signal and forwarded
                                         while application had it blocked.*/
    PIN_ERR_BLOCKSIG_APP,           /*!< 1 arg: signal number.  Application received asynchronous
                                         blocked signal, which can't be emulated.*/
    PIN_ERR_USER_SPECIFIED_ERROR,   /*!< reserved. internal use only */
    PIN_ERR_OUT_OF_MEMORY,          /*!< 0 args. pin is out of memory. */
    PIN_ERR_NON_NUMERIC_STRING,     /*!< 1 arg: string. Non-numeric string passed to conversion utility. */
    PIN_ERR_EMPTY_STRING,           /*!< 0 args. Empty string passed to conversion utility. */
    PIN_ERR_NUMERIC_FROM_STRING,    /*!< 1 arg: string. Error in numeric to string conversion utility. */
    PIN_ERR_ALREADY_ATTACHED,       /*!< 0 args. pin is already attached to the process. */
    PIN_ERR_ILLEGAL_EXCEPTION,      /*!< 1 arg: string. Tool raised invalid exception. */
    PIN_ERR_EXCEPTION_NEEDS_ADDR,   /*!< 1 arg: string. Must specify access address for exception. */
    PIN_ERR_PREDEFINED_INSTRUMENTATION_FAILED, /*!< 1 arg: string. Specified function can not be instrumented */
    PIN_ERR_UNSUPPORTED_SYSCALL     /*!< 1 arg: string. System call is not supported by Pin */
} PIN_ERRTYPE;

#endif

