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
 * This test application is used to verify correctness of handling Windows callbacks in Pin.
 */

#include <windows.h>
#include <string>
#include <iostream>

using namespace std;

#define USR_MESSAGE	(WM_USER + 1)


// Print the specified error message and abort the process
static void Abort(const string & errorMessage)
{
    cout << "[win_callback_app] Failure: " << errorMessage << endl;
    exit(1);
}

// Execute a system call from a Window procedure. This procedure can be replaced by a tool.
extern "C" __declspec(dllexport) void SyscallInCallback()
{
    Sleep(1);
}

// Window (callback) procedure
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case USR_MESSAGE:
        cout << "[win_callback_app] Received USR_MESSAGE, lParam = " << hex << lParam << endl;

        SyscallInCallback(); // execute a system call (in a replacement routine)
        return lParam;       // return from callback

    case WM_DESTROY: 
        PostQuitMessage(0); 
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Thread root procedure that sends a message to the window whose handle is specified by <pArg>
DWORD WINAPI ThreadProc(LPVOID pArg)
{
    HWND hWnd = (HWND)pArg;

    cout << "[win_callback_app] Sending USR_MESSAGE ..." << endl;
    LRESULT lRes = SendMessage( hWnd, USR_MESSAGE, 0, 0x1234);
    cout << "[win_callback_app] USR_MESSAGE sent. Result = " << hex << lRes << endl;

    if (lRes != 0x1234)
    {
        Abort("SendMessage");
    }

    SendMessage( hWnd, WM_CLOSE, 0, 0);
    return 0;
}

// Main procedure: create a window and a thread that sends a message (callback) to the window. 
int main()
{
    cout << "[win_callback_app] Registering window class ..." << endl;

    WNDCLASS  wc;
    wc.hInstance = NULL;
    wc.lpszClassName = "TestWndClass";
    wc.lpfnWndProc = (WNDPROC)WindowProc;
    wc.style = 0;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.lpszMenuName =  NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)NULL;

    if (RegisterClass(&wc) == 0)
    {
        Abort("RegisterClass");
    }

    //--------------------------------------------------------------------------------
    cout << "[win_callback_app] Creating a window ..." << endl;

    HWND hWnd = CreateWindow(
            "TestWndClass",      // window class
            "",                  // title
            0,                   // style
            CW_USEDEFAULT,       // default horizontal position
            CW_USEDEFAULT,       // default vertical position
            CW_USEDEFAULT,       // default width
            CW_USEDEFAULT,       // default height
            HWND_MESSAGE,        // parent
            (HMENU)NULL,         // menu
            NULL,                // application instance
            NULL);               // window-creation data

    if (hWnd == 0)
    {
        Abort("CreateWindow");
    }

    //--------------------------------------------------------------------------------
    cout << "[win_callback_app] Creating a thread ..." << endl;

    HANDLE hThread = CreateThread( NULL, 0, ThreadProc, (LPVOID)hWnd, 0, 0);

    if ( hThread == NULL )
    {
        Abort("CreateThread");
    }

    //--------------------------------------------------------------------------------
    cout << "[win_callback_app] Receiving messages ..." << endl;

    while (TRUE)
    {
        // Use PeekMessage() instead of GetMessage() to expose bug from Mantis #2221
        MSG  msg;
        BOOL bRet = PeekMessage( &msg, hWnd, 0, 0, PM_REMOVE);
        if (!bRet)
        {
            Sleep(10);
        }
        else if (msg.message == WM_QUIT)
        {
            break;
        }
    } 

    cout << "[win_callback_app] Success" << endl;
    return 0;
}


