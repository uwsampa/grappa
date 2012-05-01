/*
 * $Header: /home/cristiv/Projects/DebugAllocator/Source/rcs/msvcnowarn.h,v 1.1 2001/10/21 03:19:57 root Exp root $
 * $NoKeywords: $
 */
// Microsoft Compiler ?
#if defined (_MSC_VER) && !defined(__MWERKS__)


// nonstandard extension 'single line comment' was used
#pragma warning(disable: 4001)
// typedef-name 'abc' used as synonym for class-name 'ABC'
#pragma warning(disable: 4097)
// nonstandard extension used : nameless struct/union
#pragma warning(disable: 4201)
// nonstandard extension used : bit field types other than int
#pragma warning(disable: 4214)
// Note: Creating precompiled header 
#pragma warning(disable: 4699)
// decorated name length exceeded, name was truncated
#pragma warning(disable: 4503)
// unreferenced inline function has been removed
#pragma warning(disable: 4514)
// unreferenced formal parameter
#pragma warning(disable: 4100)
// indirection to slightly different base types
#pragma warning(disable: 4057)
// named type definition in parentheses
#pragma warning(disable: 4115)
// nonstandard extension used : benign typedef redefinition
#pragma warning(disable: 4209)

// no matching operator delete found; 
// memory will not be freed if initialization throws an exception
#pragma warning(disable: 4291)

// copy constructor could not be generated
#pragma warning(disable: 4511)
// copy assignment could not be generated
#pragma warning(disable: 4512)
// function '__thiscall ...' not expanded
#pragma warning(disable: 4710)
// identifier was truncated to '255' characters in the debug information
#pragma warning(disable:4786)

// #pragma warning(disable:4127)

// function selected for automatic inline expansion
#pragma warning(disable:4711)

// #pragma warning(disable:4530)

// C++ language change: to explicitly specialize class template...
#pragma warning(disable: 4663)

#ifdef INCLUDES_H_
// signed/unsigned mismatch
 #pragma warning(disable: 4018)

// 'initializing' : conversion from 'const int' to 'const unsigned int
 #pragma warning(disable: 4245)
#endif
#endif // MICROSOFT COMPILER

