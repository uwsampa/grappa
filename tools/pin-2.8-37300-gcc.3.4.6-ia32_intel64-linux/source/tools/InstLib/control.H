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
#ifndef CONTROL_H
#define CONTROL_H

#include <vector>
#include <iostream>
#include <algorithm>

#if defined(TARGET_IA32) || defined(TARGET_IA32E)
extern "C" {
#include "xed-interface.h"
}
#endif

namespace INSTLIB 
{
/*! @defgroup CONTROLLER
  It is often desirable to use instrumentation to observe an interval of
  the execution of a program. Controllers are used to detect the beginning
  or end of an interval. Some of the methods are instruction counts, or the
  nth time an address or symbol is executed.

  A controller is usually an ALARM with some coordination for stop and start and
  some built in command line switches.

  The example below can be found in InstLibExamples/control.cpp

  \include control.cpp

*/

/*! @ingroup CONTROLLER
  Event that is passed to handler when controller detects the beginning or end of an interval
*/
typedef enum 
{
    CONTROL_INVALID,
    CONTROL_START,    ///< Beginning of interval
    CONTROL_STOP,     ///< End of interval
    CONTROL_THREADID,  ///< Capture executing thread id for later processing
    CONTROL_WARMUP_START,  ///< Beginning of warmup region
    CONTROL_WARMUP_STOP,  ///< End of warmup region
    CONTROL_PROLOG_START,  ///< Beginning of prolog  region
    CONTROL_PROLOG_STOP,  ///< End of prolog region
    CONTROL_EPILOG_START,  ///< Beginning of epilog region
    CONTROL_EPILOG_STOP  ///< End of epilog region
} CONTROL_EVENT;

//#define DEBUG_CONTROLLER


/*! @ingroup CONTROLLER
  Type for generic event handler
*/

typedef VOID (*CONTROL_HANDLER)(CONTROL_EVENT, VOID *, CONTEXT *, VOID *, THREADID tid);

/*! @defgroup CONTROLLER_LENGTH
  @ingroup CONTROLLER
  Controller for detecting the end of an interval using instruction count
  Use -length <n> to capture n instructions

  It is the tool's responsibility to take care of interval overlapping when
  using the control classes with multiple threads. If the knobs controlling
  which thread id to use are not specified, the default behavior is to
  monitor events on a per thread basis. For instance, if -skip N -length M
  is passed, each thread will generate a start event after executing N
  instructions and a stop event after executing M instructions. If these
  intervals overlap in time, the tool writer has to make sure to handle it
  appropriately. The same applies for the other controllers
*/

/*! @ingroup CONTROLLER_LENGTH
*/
class CONTROL_LENGTH
{
  public:
    CONTROL_LENGTH(BOOL passContext=false, const string & knobPrefix = "",
                   const string& knob_family= "pintool:control") 
        : _lengthKnob(KNOB_MODE_WRITEONCE, 
                      knob_family,
                      "length", 
                      "",
                      "Number of instructions to execute before stopping", knobPrefix),
          _alarmIcount(passContext)
    {}

    /*! @ingroup CONTROLLER_LENGTH
      Activate the controller if the -length knob is provided
      @return 1 if controller can start an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        if (_lengthKnob.Value() == "")
            return 0;
        ADDRESS_COUNT ac = ParseCount(_lengthKnob.Value().c_str() );
        // ac.print();
        _length = ac.count;
        _tid = ac.tid;

        _controlHandler = ch;
        _controlVal = val;
        _alarmIcount.Activate(ac.low_thread, ac.high_thread);

        return 0;
    }

    /*! @ingroup CONTROLLER_LENGTH
      Notify the controller about a start event. It counts instructions until the end
      of the interval
    */
    VOID Event(CONTROL_EVENT ev)
    {
        if (_length <= 0)
            return;

        switch(ev)
        {
          case CONTROL_START:
            _alarmIcount.SetAlarm(_length, Stop, this, _tid);
            break;

          default:
            break;
        }
    }

  private:
    static VOID Stop(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_LENGTH * cl = static_cast<CONTROL_LENGTH*>(val);

        cl->_controlHandler(CONTROL_STOP, cl->_controlVal, ctxt, ip, tid);
    }

    KNOB<string> _lengthKnob;
    UINT64 _length;
    UINT64 _tid;
    ALARM_ICOUNT _alarmIcount;
    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};


/*! @defgroup CONTROLLER_START_ADDRESS_GENERIC
  @ingroup CONTROLLER
  Generic controller for detecting the start of an interval using an address or symbol and a count, etc.
  This takes a generic event, knob and knob usage string.
*/

/*! @ingroup CONTROLLER_START_ADDRESS_GENERIC
*/
class CONTROL_START_ADDRESS_GENERIC
{
  public:
    CONTROL_START_ADDRESS_GENERIC(CONTROL_EVENT event, 
                                  string knob_name,
                                  string knob_usage,
                                  BOOL passContext=false, 
                                  const string & prefix = "",
                                  string knob_family = "pintool:control") 
        : _event(event),
          _passContext(passContext),
          _startAddress(KNOB_MODE_WRITEONCE, 
                        knob_family,
                        knob_name,
                        "",
                        knob_usage,
                        prefix)
    {}
    
    /*! @ingroup CONTROLLER_START_ADDRESS_GENERIC
      Activate the controller if the -start_address knob is provided
      @return 1 if controller can start an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        if (_startAddress.Value() == "")
            return 0;

        ADDRESS_COUNT  addressCount = ParseAddressCount(_startAddress);
        if (addressCount.name != "")
        {
            if(addressCount.offset == 0)
            {
#if defined(DEBUG_CONTROLLER)
                cerr << "Symbol " << addressCount.name 
                     << " Count: " << dec << addressCount.count << endl;
#endif
                _symbolAlarm = new ALARM_SYMBOL_COUNT(_passContext);
                _symbolAlarm->Activate(addressCount.name.c_str());
                _symbolAlarm->SetAlarm(addressCount.count, Start, this,
                                       addressCount.tid,
                                       addressCount.rearm, addressCount.always_enabled);
            }
            else
            {
#if defined(DEBUG_CONTROLLER)
                cerr << "Image " << addressCount.name 
                     << " Offset:0x" << hex << addressCount.offset
                     << " Count: " << dec << addressCount.count << endl;
#endif
                _image_offset_Alarm = new ALARM_IMAGE_OFFSET_COUNT(_passContext);
                _image_offset_Alarm->Activate(addressCount.name.c_str(), addressCount.offset);
                _image_offset_Alarm->SetAlarm(addressCount.count, Start, this,
                                              addressCount.tid,
                                              addressCount.rearm, addressCount.always_enabled);
            }
        }
        else
        {
#if defined(DEBUG_CONTROLLER)
            cerr << "Address: 0x" << hex <<  addressCount.address
                 << " Count: " << dec << addressCount.count << endl;
#endif
            _addressAlarm = new ALARM_ADDRESS_COUNT(_passContext);
            _addressAlarm->Activate(addressCount.address);

            _addressAlarm->SetAlarm(addressCount.count, Start, this,
                                    addressCount.tid,
                                    addressCount.rearm, addressCount.always_enabled);
        }

        _controlHandler = ch;
        _controlVal = val;

        return 1;
    }

    bool IsActive() const { return _startAddress.Value() != ""; }
  private:
    static VOID Start(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_START_ADDRESS_GENERIC * cs = static_cast<CONTROL_START_ADDRESS_GENERIC*>(val);

        // Notify the parent
        cs->_controlHandler(cs->_event, cs->_controlVal, ctxt, ip, tid);
    }

    CONTROL_EVENT _event;
    BOOL _passContext;
    KNOB<string> _startAddress;

    ALARM_ADDRESS_COUNT * _addressAlarm;
    ALARM_SYMBOL_COUNT * _symbolAlarm;
    ALARM_IMAGE_OFFSET_COUNT * _image_offset_Alarm;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};



/*! @defgroup CONTROLLER_START_ADDRESS
  @ingroup CONTROLLER
  Controller for detecting the start of an interval using an address or symbol and a count.
  Use -start_address [address|address:count|symbol|symbol:count|address:count]
*/

/*! @ingroup CONTROLLER_START_ADDRESS
*/
class CONTROL_START_ADDRESS : public CONTROL_START_ADDRESS_GENERIC
{
  public:
    CONTROL_START_ADDRESS(BOOL passContext=false, 
                          const string & prefix = "", 
                          const string& knob_family = "pintool:control") 
        : CONTROL_START_ADDRESS_GENERIC(CONTROL_START, 
                                        "start_address",
                                        "Address and count to trigger a start (e.g. 0x400000, main, memcpy:2, /lib/tls/libc.so.6+0x1563a:1)",
                                        passContext, 
                                        prefix,
                                        knob_family)
    {}
};


/*! @defgroup CONTROLLER_START_THREADID
  @ingroup CONTROLLER
  Controller for capturing the thread id when a certain address is executed.
  Use -capture_tid_address [address|address:count|symbol|symbol:count|address:count]
*/

/*! @ingroup CONTROLLER_START_THREADID
*/
class CONTROL_START_THREADID : public CONTROL_START_ADDRESS_GENERIC
{
  public:
    CONTROL_START_THREADID(BOOL passContext=false, const string & prefix = "",
                           const string& knob_family = "pintool:control") 
        : CONTROL_START_ADDRESS_GENERIC(CONTROL_THREADID,
                                        "capture_tid_address",
                                        "Address and count to trigger a thread-capture event (e.g. 0x400000, main, memcpy:2, /lib/tls/libc.so.6+0x1563a:1)",
                                        passContext, 
                                        prefix, knob_family)
    {}
};


#if defined(TARGET_IA32) || defined(TARGET_IA32E)
/*! @defgroup CONTROLLER_START_INT3
  @ingroup CONTROLLER
  Controller for detecting the start of an interval using an int3 instruction.
  Use -start_int3 
*/

/*! @ingroup CONTROLLER_START_INT3
*/
class CONTROL_START_INT3
{
  public:
    CONTROL_START_INT3(BOOL passContext=false, const string & prefix = "",
                       const string& knob_family = "pintool:control") 
        : _passContext(passContext),
          _startInt3(KNOB_MODE_WRITEONCE, 
                     knob_family,
                     "start_int3",
                     "",
                     "Trigger a start on seeing an 'int 3' instruction. Requires a count argument.",
                     prefix)
    {
    }
    
    /*! @ingroup CONTROLLER_START_INT3
      Activate the controller if the -start_int3 knob is provided
      @return 1 if controller can start an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        if (_startInt3.Value() == "")
            return 0;

        ADDRESS_COUNT  ac = ParseCount( _startInt3.Value().c_str() );
        INT64 count = ac.count;
        BOOL rearm = ac.rearm;
        BOOL always_armed = ac.always_enabled;
        _int3Alarm = new ALARM_INT3(_passContext);
        _int3Alarm->Activate();
        _int3Alarm->SetAlarm(Start, this, ac.tid, rearm, count, always_armed);
        _controlHandler = ch;
        _controlVal = val;
        return 1;
    }
    bool IsActive() const { return _startInt3.Value() != ""; };

  private:
    static VOID Start(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_START_INT3 * ci = static_cast<CONTROL_START_INT3*>(val);

        // Notify the parent
        ci->_controlHandler(CONTROL_START, ci->_controlVal, ctxt, ip, tid);
    }
    
    BOOL _passContext;
    KNOB<string> _startInt3;

    ALARM_INT3 * _int3Alarm;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};



/*! @defgroup CONTROLLER_STOP_INT3
  @ingroup CONTROLLER
  Controller for detecting the stop of an interval using an int3 instruction.
  Use -stop_int3 
*/

/*! @ingroup CONTROLLER_STOP_INT3
*/
class CONTROL_STOP_INT3
{
  public:
    CONTROL_STOP_INT3(BOOL passContext=false, const string & prefix = "", 
                      const string& knob_family = "pintool:control") 
        : _passContext(passContext),
          _stopInt3(KNOB_MODE_WRITEONCE, 
                    knob_family,
                     "stop_int3",
                     "",
                     "Trigger a stop on seeing an 'int 3' instruction. Requires a count argument.",
                     prefix)
    {
    }
    
    /*! @ingroup CONTROLLER_STOP_INT3
      Activate the controller if the -stop_int3 knob is provided
      @return 1 if controller can stop an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        if (_stopInt3.Value() == "")
            return 0;
        ADDRESS_COUNT  ac = ParseCount( _stopInt3.Value().c_str() );
        INT64 count = ac.count;
        BOOL rearm = ac.rearm;
        BOOL always_armed = ac.always_enabled;

        _int3Alarm = new ALARM_INT3(_passContext);
        _int3Alarm->Activate();
        _int3Alarm->SetAlarm(Stop, this, ac.tid, rearm, count, always_armed);
        _controlHandler = ch;
        _controlVal = val;
        return 1;
    }
    bool IsActive() const { return _stopInt3.Value() != ""; };

  private:
    static VOID Stop(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_STOP_INT3 * ci = static_cast<CONTROL_STOP_INT3*>(val);

        // Notify the parent
        ci->_controlHandler(CONTROL_STOP, ci->_controlVal, ctxt, ip, tid);
    }
    
    BOOL _passContext;
    KNOB<string> _stopInt3;
    ALARM_INT3 * _int3Alarm;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};

#endif

#if defined(TARGET_IA32) || defined(TARGET_IA32E)

LOCALFUN unsigned char convert_nibble(unsigned char n) {
    if (n >= '0' && n <= '9') return n -'0';
    if (n >= 'a' && n <= 'f') return n -'a'+10;
    if (n >= 'A' && n <= 'F') return n -'A'+10;
    cerr << "Bad nibble in hex string: " << (char)n << endl;
    ASSERTX(0);
    return 0;
}
LOCALFUN void str2hex(const char* in, unsigned char* out, unsigned int len) {
    unsigned int i=0, j=0;
    for(i=0;i<len;i+=2) 
        out[j++] = convert_nibble(in[i])*16+ convert_nibble(in[i+1]);
}



/*! @defgroup CONTROLLER_START_ITEXT
  @ingroup CONTROLLER
  Controller for detecting the start of an interval using a instruction text
  Use -start_itext <hex-pattern>:<count>
*/

/*! @ingroup CONTROLLER_START_ITEXT
*/
class CONTROL_START_ITEXT
{
  public:
    CONTROL_START_ITEXT(BOOL passContext=false, const string & prefix = "", 
                        const string& knob_family = "pintool:control") 
        : _passContext(passContext),
          _startItext(KNOB_MODE_WRITEONCE, 
                      knob_family,
                     "start_itext",
                     "",
                     "Trigger a start on seeing specified instruction bytes (max 15Bytes=30nibbles)",
                     prefix)
    {
    }
    
    /*! @ingroup CONTROLLER_START_ITEXT
      Activate the controller if the -start_itext knob is provided
      @return 1 if controller can start an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        if (_startItext.Value() == "")
            return 0;

        _itextAlarm = new ALARM_ITEXT(_passContext);
        ADDRESS_COUNT  ac = ParsePatternCount(_startItext.Value().c_str());
        const char* p = ac.name.c_str();
        unsigned int plen = strlen(p);
        if ((plen & 1) != 0) {
            cerr << "Must have an even number of nibbles in the input string for -start_itext" << endl;
            ASSERTX(0);
        }

        unsigned char* hexstr = new unsigned char[plen/2];
        str2hex(p,hexstr,plen);
        _itextAlarm->Activate(hexstr, plen/2);
        delete [] hexstr;
        _itextAlarm->SetAlarm(Start, this, ac.tid, ac.rearm, ac.count, ac.always_enabled);
        _controlHandler = ch;
        _controlVal = val;
        return 1;
    }
    bool IsActive() const { return _startItext.Value() != "0"; };

  private:
    static VOID Start(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_START_ITEXT * ci = static_cast<CONTROL_START_ITEXT*>(val);

        // Notify the parent
        ci->_controlHandler(CONTROL_START, ci->_controlVal, ctxt, ip, tid);
    }
    
    BOOL _passContext;
    KNOB<string> _startItext;

    ALARM_ITEXT * _itextAlarm;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};



/*! @defgroup CONTROLLER_STOP_ITEXT
  @ingroup CONTROLLER
  Controller for detecting the end of an interval using a instruction text
  Use -stop_itext <hex-pattern>:<count>
*/

/*! @ingroup CONTROLLER_STOP_ITEXT
*/
class CONTROL_STOP_ITEXT
{
  public:
    CONTROL_STOP_ITEXT(BOOL passContext=false, const string & prefix = "",
                       const string& knob_family = "pintool:control" ) 
        : _passContext(passContext),
          _stopItext(KNOB_MODE_WRITEONCE, 
                     knob_family,
                     "stop_itext",
                     "",
                     "Trigger a stop on seeing a specified instruction pattern (max 15Byte=30nibbles)",
                     prefix)
    {
    }
    
    /*! @ingroup CONTROLLER_STOP_ITEXT
      Activate the controller if the -stop_itext knob is provided
      @return 1 if controller can stop an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        if (_stopItext.Value() == "")
            return 0;
        _itextAlarm = new ALARM_ITEXT(_passContext);
        ADDRESS_COUNT  ac = ParsePatternCount(_stopItext.Value().c_str());
        const char* p = ac.name.c_str();
        unsigned int plen = strlen(p);
        if ((plen & 1) != 0) {
            cerr << "Must have an even number of nibbles in the input string for -stop_itext" << endl;
            ASSERTX(0);
        }
        unsigned char* hexstr = new unsigned char[plen/2];
        str2hex(p,hexstr,plen);
        _itextAlarm->Activate(hexstr, plen/2);
        delete [] hexstr;
        _itextAlarm->SetAlarm(Stop, this, ac.tid, ac.rearm, ac.count, ac.always_enabled);
        _controlHandler = ch;
        _controlVal = val;
        return 1;
    }
    bool IsActive() const { return _stopItext.Value() != "0"; };

  private:
    static VOID Stop(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_STOP_ITEXT * ci = static_cast<CONTROL_STOP_ITEXT*>(val);

        // Notify the parent
        ci->_controlHandler(CONTROL_STOP, ci->_controlVal, ctxt, ip, tid);
    }
    
    BOOL _passContext;
    KNOB<string> _stopItext;
    ALARM_ITEXT * _itextAlarm;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};


/*! @defgroup CONTROLLER_START_SSCMARK
  @ingroup CONTROLLER
  Controller for detecting the start of an interval using a sequence of bytex BB xx xx xx xx 64 67 90
  Use -start_ssc_mark <hex-pattern>:<count>
*/

/*! @ingroup CONTROLLER_START_SSCMARK
*/
class CONTROL_START_SSCMARK
{
  public:
    CONTROL_START_SSCMARK(BOOL passContext=false, const string & prefix = "",
                          const string& knob_family = "pintool:control") 
        : _passContext(passContext),
          _startItext(KNOB_MODE_WRITEONCE, 
                      knob_family,
                     "start_ssc_mark",
                     "",
                     "Trigger a start on seeing specified SSC marker -- big-endian hex without an 0x prefix",
                     prefix)
    {
    }
    
    /*! @ingroup CONTROLLER_START_SSCMARK
      Activate the controller if the -start_ssc knob is provided
      @return 1 if controller can start an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        if (_startItext.Value() == "")
            return 0;

        _itextAlarm = new ALARM_ITEXT(_passContext);
        ADDRESS_COUNT  ac = ParsePatternCount(_startItext.Value().c_str());
        string full_hex("0x");
        full_hex += ac.name;
        UINT32 h = Uint64FromString(full_hex);
        unsigned char ssc_marker[] = { 0xbb, 0x00, 0x00, 0x00, 0x00, 0x64, 0x67, 0x90};
        for(int i=0;i<4;i++) 
            ssc_marker[1+i]= (h>>(i*8))&0xff;
        _itextAlarm->Activate(ssc_marker, 8);
        _itextAlarm->SetAlarm(Start, this, ac.tid, ac.rearm, ac.count, ac.always_enabled);
        _controlHandler = ch;
        _controlVal = val;
        return 1;
    }
    bool IsActive() const { return _startItext.Value() != "0"; };

  private:
    static VOID Start(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_START_SSCMARK * ci = static_cast<CONTROL_START_SSCMARK*>(val);

        // Notify the parent
        ci->_controlHandler(CONTROL_START, ci->_controlVal, ctxt, ip, tid);
    }
    
    BOOL _passContext;
    KNOB<string> _startItext;

    ALARM_ITEXT * _itextAlarm;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};


/*! @defgroup CONTROLLER_STOP_SSCMARK
  @ingroup CONTROLLER
  Controller for detecting the end of an interval using a sequence of bytex BB xx xx xx xx 64 67 90
  Use -stop_ssc_mark number
*/

/*! @ingroup CONTROLLER_STOP_SSCMARK
*/
class CONTROL_STOP_SSCMARK
{
  public:
    CONTROL_STOP_SSCMARK(BOOL passContext=false, const string & prefix = "", 
                         const string& knob_family = "pintool:control") 
        : _passContext(passContext),
          _stopItext(KNOB_MODE_WRITEONCE, 
                     knob_family,
                     "stop_ssc_mark",
                     "",
                     "Trigger a stop on seeing specified SSC marker -- big-endian hex without an 0x prefix",
                     prefix)
    {
    }
    
    /*! @ingroup CONTROLLER_STOP_SSCMARK
      Activate the controller if the -stop_ssc knob is provided
      @return 1 if controller can stop an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        if (_stopItext.Value() == "")
            return 0;
        _itextAlarm = new ALARM_ITEXT(_passContext);
        ADDRESS_COUNT  ac = ParsePatternCount(_stopItext.Value().c_str());
        string full_hex("0x");
        full_hex += ac.name;
        UINT32 h = Uint64FromString(full_hex);
        unsigned char ssc_marker[] = { 0xbb, 0x00, 0x00, 0x00, 0x00, 0x64, 0x67, 0x90};
        for(int i=0;i<4;i++) 
            ssc_marker[1+i]= (h>>(i*8))&0xff;
        _itextAlarm->Activate(ssc_marker, 8);

        _itextAlarm->SetAlarm(Stop, this, ac.tid, ac.rearm, ac.count, ac.always_enabled);
        _controlHandler = ch;
        _controlVal = val;
        return 1;
    }
    bool IsActive() const { return _stopItext.Value() != "0"; };

  private:
    static VOID Stop(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_STOP_SSCMARK * ci = static_cast<CONTROL_STOP_SSCMARK*>(val);

        // Notify the parent
        ci->_controlHandler(CONTROL_STOP, ci->_controlVal, ctxt, ip, tid);
    }
    
    BOOL _passContext;
    KNOB<string> _stopItext;
    ALARM_ITEXT * _itextAlarm;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};

#endif








#if defined(TARGET_IA32) || defined(TARGET_IA32E)
/*! @defgroup CONTROLLER_START_ISA_EXTENSION
  @ingroup CONTROLLER
  Controller for detecting the start of an interval using a the XED ISA extension for instructions.
  Use -start_extension <extension_name>
*/

/*! @ingroup CONTROLLER_START_ISA_EXTENSION
*/
class CONTROL_START_ISA_EXTENSION
{
  public:
    CONTROL_START_ISA_EXTENSION(BOOL passContext=false, const string & prefix = "",
                                const string& knob_family = "pintool:control") 
        : _passContext(passContext),
          _startISAExtension(KNOB_MODE_WRITEONCE, 
                             knob_family,
                             "start_extension",
                             "INVALID",
                             "Trigger a start on seeing an instruction from this XED ISA extension",
                             prefix
          )
    {
    }
    
    /*! @ingroup CONTROLLER_START_ISA_EXTENSION
      Activate the controller if the -start_extension knob is provided
      @return 1 if controller can start an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        if (_startISAExtension.Value() == "INVALID")
            return 0;
        ADDRESS_COUNT  extension_and_count = ParseAddressCount(_startISAExtension.Value().c_str());

        xed_extension_enum_t extension = str2xed_extension_enum_t(extension_and_count.name.c_str());
        if (extension == XED_EXTENSION_INVALID) {
            cerr << "Bad ISA Extension name: " << _startISAExtension.Value() << endl;
            ASSERTX(0);
        }
        _ISAExtensionAlarm = new ALARM_ISA_EXTENSION(_passContext);
        _ISAExtensionAlarm->Activate();
        _ISAExtensionAlarm->SetAlarm(extension, extension_and_count.count, Start, this,
                                     extension_and_count.tid,
                                     extension_and_count.rearm, extension_and_count.always_enabled);
        _controlHandler = ch;
        _controlVal = val;
        return 1;
    }
    bool IsActive() const { return _startISAExtension.Value() != "INVALID"; };

  private:
    static VOID Start(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_START_ISA_EXTENSION * ci = static_cast<CONTROL_START_ISA_EXTENSION*>(val);

        // Notify the parent
        ci->_controlHandler(CONTROL_START, ci->_controlVal, ctxt, ip, tid);
    }
    
    BOOL _passContext;
    KNOB<string> _startISAExtension;

    ALARM_ISA_EXTENSION * _ISAExtensionAlarm;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};


/*! @defgroup CONTROLLER_START_ISA_CATEGORY
  @ingroup CONTROLLER
  Controller for detecting the start of an interval using a the XED ISA category for instructions.
  Use -start_category <category_name>
*/

/*! @ingroup CONTROLLER_START_ISA_CATEGORY
*/
class CONTROL_START_ISA_CATEGORY
{
  public:
    CONTROL_START_ISA_CATEGORY(BOOL passContext=false, const string & prefix = "",
                               const string& knob_family = "pintool:control") 
        : _passContext(passContext),
          _startISACategory(KNOB_MODE_WRITEONCE, 
                            knob_family,
                             "start_category",
                             "INVALID",
                             "Trigger a start on seeing an instruction from this XED ISA category",
                             prefix
          )
    {
    }
    
    /*! @ingroup CONTROLLER_START_ISA_CATEGORY
      Activate the controller if the -start_category knob is provided
      @return 1 if controller can start an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        //cerr << "CATSTART " << _startISACategory.Value() << endl;
        if (_startISACategory.Value() == "INVALID")
            return 0;
        ADDRESS_COUNT  category_and_count = ParseAddressCount(_startISACategory.Value().c_str());

        xed_category_enum_t category = str2xed_category_enum_t(category_and_count.name.c_str());
        //cerr << "CATSTART " << xed_category_enum_t2str(category) << endl; category_and_count.print();

        if (category == XED_CATEGORY_INVALID) {
            cerr << "Bad ISA Category name: " << _startISACategory.Value() << endl;
            ASSERTX(0);
        }
        _ISACategoryAlarm = new ALARM_ISA_CATEGORY(_passContext);
        _ISACategoryAlarm->Activate();
        _ISACategoryAlarm->SetAlarm(category, category_and_count.count, Start, this, 
                                    category_and_count.tid,
                                    category_and_count.rearm, category_and_count.always_enabled);
        _controlHandler = ch;
        _controlVal = val;
        return 1;
    }
    bool IsActive() const { return _startISACategory.Value() != "INVALID"; };

  private:
    static VOID Start(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        //cerr << "CATSTART START" << endl;
        CONTROL_START_ISA_CATEGORY * ci = static_cast<CONTROL_START_ISA_CATEGORY*>(val);

        // Notify the parent
        ci->_controlHandler(CONTROL_START, ci->_controlVal, ctxt, ip, tid);
    }
    
    BOOL _passContext;
    KNOB<string> _startISACategory;

    ALARM_ISA_CATEGORY * _ISACategoryAlarm;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};



////////////////////////////////////////////////////////////////////////////
// STOP
////////////////////////////////////////////////////////////////////////////

/*! @defgroup CONTROLLER_STOP_ISA_EXTENSION
  @ingroup CONTROLLER
  Controller for detecting the stop of an interval using a the XED ISA extension for instructions.
  Use -stop_extension <extension_name>
*/

/*! @ingroup CONTROLLER_STOP_ISA_EXTENSION
*/
class CONTROL_STOP_ISA_EXTENSION
{
  public:
    CONTROL_STOP_ISA_EXTENSION(BOOL passContext=false, const string & prefix = "",
                               const string& knob_family = "pintool:control") 
        : _passContext(passContext),
          _stopISAExtension(KNOB_MODE_WRITEONCE, 
                            knob_family,
                             "stop_extension",
                             "INVALID",
                             "Trigger a stop on seeing an instruction from this XED ISA extension",
                             prefix
          )
    {
    }
    
    /*! @ingroup CONTROLLER_STOP_ISA_EXTENSION
      Activate the controller if the -stop_extension knob is provided
      @return 1 if controller can stop an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        if (_stopISAExtension.Value() == "INVALID")
            return 0;
        ADDRESS_COUNT  extension_and_count = ParseAddressCount(_stopISAExtension.Value().c_str());

        xed_extension_enum_t extension = str2xed_extension_enum_t(extension_and_count.name.c_str());
        if (extension == XED_EXTENSION_INVALID) {
            cerr << "Bad ISA Extension name: " << _stopISAExtension.Value() << endl;
            ASSERTX(0);
        }
        _ISAExtensionAlarm = new ALARM_ISA_EXTENSION(_passContext);
        _ISAExtensionAlarm->Activate();
        _ISAExtensionAlarm->SetAlarm(extension, extension_and_count.count, Stop, this, 
                                     extension_and_count.tid,
                                     extension_and_count.rearm, extension_and_count.always_enabled);
        _controlHandler = ch;
        _controlVal = val;
        return 1;
    }
    bool IsActive() const { return _stopISAExtension.Value() != "INVALID"; }

  private:
    static VOID Stop(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_STOP_ISA_EXTENSION * ci = static_cast<CONTROL_STOP_ISA_EXTENSION*>(val);

        // Notify the parent
        ci->_controlHandler(CONTROL_STOP, ci->_controlVal, ctxt, ip, tid);
    }
    
    BOOL _passContext;
    KNOB<string> _stopISAExtension;

    ALARM_ISA_EXTENSION * _ISAExtensionAlarm;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};


/*! @defgroup CONTROLLER_STOP_ISA_CATEGORY
  @ingroup CONTROLLER
  Controller for detecting the stop of an interval using a the XED ISA category for instructions.
  Use -stop_category <category_name>
*/

/*! @ingroup CONTROLLER_STOP_ISA_CATEGORY
*/
class CONTROL_STOP_ISA_CATEGORY
{
  public:
    CONTROL_STOP_ISA_CATEGORY(BOOL passContext=false, const string & prefix = "",
                              const string& knob_family = "pintool:control") 
        : _passContext(passContext),
          _stopISACategory(KNOB_MODE_WRITEONCE, 
                           knob_family,
                             "stop_category",
                             "INVALID",
                             "Trigger a stop on seeing an instruction from this XED ISA category",
                             prefix
          )
    {
    }
    
    /*! @ingroup CONTROLLER_STOP_ISA_CATEGORY
      Activate the controller if the -stop_category knob is provided
      @return 1 if controller can stop an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        if (_stopISACategory.Value() == "INVALID")
            return 0;
        ADDRESS_COUNT  category_and_count = ParseAddressCount(_stopISACategory.Value().c_str());

        xed_category_enum_t category = str2xed_category_enum_t(category_and_count.name.c_str());
        if (category == XED_CATEGORY_INVALID) {
            cerr << "Bad ISA Category name: " << _stopISACategory.Value() << endl;
            ASSERTX(0);
        }
        _ISACategoryAlarm = new ALARM_ISA_CATEGORY(_passContext);
        _ISACategoryAlarm->Activate();
        _ISACategoryAlarm->SetAlarm(category, category_and_count.count, Stop, this, 
                                    category_and_count.tid,
                                    category_and_count.rearm, category_and_count.always_enabled);
        _controlHandler = ch;
        _controlVal = val;
        return 1;
    }
    bool IsActive() const { return _stopISACategory.Value() != "INVALID"; };

  private:
    static VOID Stop(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_STOP_ISA_CATEGORY * ci = static_cast<CONTROL_STOP_ISA_CATEGORY*>(val);

        // Notify the parent
        ci->_controlHandler(CONTROL_STOP, ci->_controlVal, ctxt, ip, tid);
    }
    
    BOOL _passContext;
    KNOB<string> _stopISACategory;

    ALARM_ISA_CATEGORY * _ISACategoryAlarm;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};



#endif

/*! @defgroup CONTROLLER_STOP_ADDRESS
  @ingroup CONTROLLER
  Controller for detecting the end of an interval using an address or symbol and a count
  Use -stop_address [address|address:count|symbol|symbol:count|address:count]
*/

/*! @ingroup CONTROLLER_STOP_ADDRESS
*/
class CONTROL_STOP_ADDRESS
{
  public:
    CONTROL_STOP_ADDRESS(BOOL passContext=false, const string & prefix = "", 
                         const string& knob_family="pintool:control")
        : _passContext(passContext),
          _stopAddress(KNOB_MODE_WRITEONCE, 
                       knob_family,
                       "stop_address", 
                       "",
                       "Address and count to trigger a stop (e.g. 0x400000, main, memcpy:2, /lib/tls/libc.so.6+0x1563a:1)",
                       prefix),
          _addressAlarm(0),
          _symbolAlarm(0)
    {}
    
    /*! @ingroup CONTROLLER_STOP_ADDRESS
      Activate the controller if the -stop_address knob is provided
      @return 1 if controller can start an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        if (_stopAddress.Value() == "")
            return 0;

        ADDRESS_COUNT  addressCount = ParseAddressCount(_stopAddress);
        _count = addressCount.count;
        _rearm = addressCount.rearm;
        _always_enabled = addressCount.always_enabled;
        _tid =  addressCount.tid;

        if (addressCount.name != "")
        {
            if(addressCount.offset == 0)
            {
                _symbolAlarm = new ALARM_SYMBOL_COUNT(_passContext);
                _symbolAlarm->Activate(addressCount.name.c_str());
            }
            else
            {
#if defined(DEBUG_CONTROLLER)
                cerr << "Image " << addressCount.name
                     << " Offset:0x" << hex << addressCount.offset
                     << " Count: " << dec << _count << endl;
#endif
                _image_offset_Alarm = new ALARM_IMAGE_OFFSET_COUNT(_passContext);
                _image_offset_Alarm->Activate(addressCount.name.c_str(), 
                                              addressCount.offset);
            }
        }
        else
        {
#if defined(DEBUG_CONTROLLER)
                cerr << "Address: 0x" << hex << addressCount.address
                     << " Count: " << dec << _count << endl;
#endif
                _addressAlarm = new ALARM_ADDRESS_COUNT(_passContext);
                _addressAlarm->Activate(addressCount.address);
        }

        _controlHandler = ch;
        _controlVal = val;

        return 0;
    }

    /*! @ingroup CONTROLLER_STOP_ADDRESS
      Notify the controller about a start event. It uses the address/count to find the end
      of the interval
    */
    VOID Event(CONTROL_EVENT ev)
    {
        switch(ev)
        {
          case CONTROL_START:
            if (_symbolAlarm)
                _symbolAlarm->SetAlarm(_count, Stop, this, 
                                       _tid,
                                       _rearm, _always_enabled);
            else if (_addressAlarm)
                _addressAlarm->SetAlarm(_count, Stop, this, _tid,
                                        _rearm, _always_enabled);
            else if (_image_offset_Alarm)
                _image_offset_Alarm->SetAlarm(_count, Stop, this, _tid,
                                              _rearm, _always_enabled);
            break;

          default:
            break;
        }
    }


  private:
    static VOID Stop(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_STOP_ADDRESS * cs = static_cast<CONTROL_STOP_ADDRESS*>(val);

        // Notify the parent
        cs->_controlHandler(CONTROL_STOP, cs->_controlVal, ctxt, ip, tid);
    }

    BOOL _passContext;
    KNOB<string> _stopAddress;

    UINT64 _count;
    BOOL _always_enabled;
    BOOL _rearm;
    UINT64 _tid;
    ALARM_ADDRESS_COUNT * _addressAlarm;
    ALARM_SYMBOL_COUNT * _symbolAlarm;
    ALARM_IMAGE_OFFSET_COUNT * _image_offset_Alarm;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};

////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////

/*! @defgroup CONTROLLER_FINI
  @ingroup CONTROLLER
  Controller ends interval when program exits, not controlled by a switch
*/

/*! @ingroup CONTROLLER_FINI
*/
class CONTROL_FINI
{
  public:
    CONTROL_FINI()
    {}
    
    /*! @ingroup CONTROLLER_FINI
      Always active, sends stop at the fini if we are in an interval
      @return 1 if controller can start an interval, otherwise 0
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        _controlHandler = ch;
        _controlVal = val;

        PIN_AddFiniFunction(Fini, this);
        return 0;
    }

    /*! @ingroup CONTROLLER_FINI
      Notify the controller about a start event.
    */
    VOID Event(CONTROL_EVENT ev)
    {
        _ev = ev;
    }
            
  private:
    static VOID Fini(INT32 code, VOID * val)
    {
        CONTROL_FINI * cs = static_cast<CONTROL_FINI*>(val);

        // If we are in an interval, end it
        if (cs->_ev == CONTROL_START)
            cs->_controlHandler(CONTROL_STOP, cs->_controlVal, NULL, NULL, 0);

    }
    
    CONTROL_EVENT _ev;

    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};

/*! @defgroup CONTROLLER_SKIP
  @ingroup CONTROLLER
  Controller for counting instructions to skip before beginning an interval.
  Use -skip <n> to skip n instructions before starting an interval.
*/

/*! @ingroup CONTROLLER_SKIP
*/
class CONTROL_SKIP
{
  public:
    CONTROL_SKIP(BOOL passContext=false, const string & prefix = "",
                 const string& knob_family = "pintool:control")
        : _skipKnob(KNOB_MODE_WRITEONCE,
                    knob_family,
                    "skip",
                    "",
                    "Number of instructions to skip from beginning", prefix),
          _alarmIcount(passContext)
    {}
    
    /*! @ingroup CONTROLLER_SKIP
      Activate the controller if the -skip knob is provided
      @return true if controller can start an interval
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        _controlHandler = ch;
        _controlVal = val;
        
        if (_skipKnob.Value() == "")
            return 0;
        ADDRESS_COUNT ac = ParseCount(_skipKnob.Value().c_str() );
        _skip = ac.count;
        _tid = ac.tid;

        _alarmIcount.Activate();
        _alarmIcount.SetAlarm(_skip, Start, this, _tid);
        return 1;
    }

  private:
    static VOID Start(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_SKIP * cs = static_cast<CONTROL_SKIP*>(val);

        // Notify the parent
        cs->_controlHandler(CONTROL_START, cs->_controlVal, ctxt, ip, tid);
    }
   
    KNOB<string> _skipKnob;
    UINT64 _skip;
    UINT64 _tid;
    ALARM_ICOUNT _alarmIcount;
    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};

/*! @defgroup CONTROLLER_UNIFORM
  @ingroup CONTROLLER
  Controller for periodically counting instructions to skip before 
  beginning an interval.
  Use -uniform_period <n> to skip n instructions before starting an interval.
  Use -uniform_period_tid <t> to skip n instructions from thread t starting an
  interval
  Use -uniform_length <m> to end  the interval after m instructions.
  Use -uniform_length_tid <t> to end the interval after m instruction from thread t
*/

/*! @ingroup CONTROLLER_SKIP
*/
class CONTROL_UNIFORM
{
  public:
    CONTROL_UNIFORM(BOOL passContext=false, const string & prefix = "",
                    const string& knob_family = "pintool:control") 
        : _periodKnob(KNOB_MODE_WRITEONCE, knob_family, "uniform_period", "",
                      "Number of instructions to skip periodically", prefix),
          _lengthKnob(KNOB_MODE_WRITEONCE, knob_family, "uniform_length", "",
                      "Number of instructions to capture periodically", prefix),
          _alarmPeriodIcount(passContext),
          _alarmLengthIcount(passContext),
          _period(0),
          _length(0),
          _period_tid(PIN_CONTROLLER_ALL_TIDS),
          _length_tid(PIN_CONTROLLER_ALL_TIDS)
    {}
    
    /*! @ingroup CONTROLLER_UNIFORM
      Activate the controller if the -uniform_period  knob is provided
      @return true if controller can start an interval
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        _controlHandler = ch;
        _controlVal = val;
        
        if (_periodKnob.Value() == "" && 
            _lengthKnob.Value() == "")
            return 0;

        ADDRESS_COUNT lac = ParseCount(_lengthKnob.Value().c_str() );
        _length = lac.count;
        _length_tid = lac.tid;
        ADDRESS_COUNT pac = ParseCount(_periodKnob.Value().c_str() );
        _period = pac.count;
        _period_tid = pac.tid;

        /* FIXME is it valid for the  length and period TIDs to be different? */

        if ( _period <= _length )
        {
            cerr << "Uniform period (" << _period << ") must be greater than uniform length (" 
                 << _length << ")" << endl;
            ASSERTX(0);
        }
        _active = true;
        _alarmPeriodIcount.Activate();
        _alarmLengthIcount.Activate();
        _alarmPeriodIcount.SetAlarm(_period, Start, this, _period_tid);
        return 1;
    }
    bool IsActive() const { return _active; };


  private:
    static VOID Start(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_UNIFORM * cu = static_cast<CONTROL_UNIFORM*>(val);

        // Set alarm for the end of this interval
        cu->_alarmLengthIcount.SetAlarm(cu->_length, Stop, val, cu->_length_tid);
        // Set alarm for the beginning of the next interval
        cu->_alarmPeriodIcount.SetAlarm(cu->_period, Start, val, cu->_period_tid);
        // Notify the parent
        cu->_controlHandler(CONTROL_START, cu->_controlVal, ctxt, ip, tid);
    }
    static VOID Stop(VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL_UNIFORM * cu = static_cast<CONTROL_UNIFORM*>(val);

        cu->_controlHandler(CONTROL_STOP, cu->_controlVal, ctxt, ip, tid);
    }
    
    KNOB<string> _periodKnob;
    KNOB<string> _lengthKnob;
    ALARM_ICOUNT _alarmPeriodIcount;
    ALARM_ICOUNT _alarmLengthIcount;
    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
    bool _active;
    UINT64 _period;
    UINT64 _length;
    UINT64 _period_tid;
    UINT64 _length_tid;

};

/*! @defgroup CONTROLLER_INIT
  @ingroup CONTROLLER
  Controller for unconditionally starting at the beginning
*/

/*! @ingroup CONTROLLER_INIT
*/
class CONTROL_INIT
{
  public:
    CONTROL_INIT(BOOL passContext=false)
        : _first(true), _passContext(passContext)
    {}

    /*! @ingroup CONTROLLER_INIT
      Activate unconditionally
      @return true if controller can start an interval
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        _controlHandler = ch;
        _controlVal = val;

        TRACE_AddInstrumentFunction(OnTrace, this);
#ifdef TARGET_LINUX
        PIN_AddForkFunction(FPOINT_AFTER_IN_CHILD, AfterForkInChildCallback,  (VOID *)this);
#endif
        return 1;
    }

  private:
    static VOID OnTrace(TRACE trace, VOID *vthis)
    {
        CONTROL_INIT *me = static_cast<CONTROL_INIT*>(vthis);

        // Add an instrumentation point to the very first instruction.
        if (me->_first)
        {
            INS firstIns = BBL_InsHead(TRACE_BblHead(trace));
            if (me->_passContext)
            {
                INS_InsertCall(firstIns, IPOINT_BEFORE, AFUNPTR(Start),
                    IARG_CONTEXT, IARG_INST_PTR, IARG_THREAD_ID, IARG_PTR, me, IARG_END);
            }
            else
            {
                INS_InsertCall(firstIns, IPOINT_BEFORE, AFUNPTR(Start),
                    IARG_PTR, static_cast<VOID *>(0), IARG_INST_PTR, IARG_THREAD_ID, IARG_PTR, me, IARG_END);
            }
            me->_first = false;
        }
    }

#ifdef TARGET_LINUX
    // After fork() the child inherits all the data structures from the parent
    // so we need to reset the "_first" flag.
    static VOID AfterForkInChildCallback(THREADID tid, const CONTEXT* ctxt, VOID * arg)
    {
        CONTROL_INIT *me = static_cast<CONTROL_INIT*>(arg);
        me->_first = TRUE;
    }
#endif

    static VOID Start(CONTEXT *ctxt, ADDRINT ip, THREADID tid, VOID *vthis)
    {
        CONTROL_INIT *me = static_cast<CONTROL_INIT*>(vthis);
        me->_controlHandler(CONTROL_START, me->_controlVal, ctxt, reinterpret_cast<VOID *>(ip), tid);

        // We want to call the handler exactly once.  Invalidate the trace to prevent it from being called
        // again even if the program's first instruction is re-executed.
        CODECACHE_InvalidateTraceAtProgramAddress(ip);
    }

    BOOL _first;
    BOOL _passContext;
    CONTROL_HANDLER _controlHandler;
    VOID * _controlVal;
};

#include "regions_control.H"
#include "pinpoint_control.H"

/*! @defgroup CONTROLLER_MULTI
  @ingroup CONTROLLER

  Controller that includes controllers for -skip -length -start_address -stop_address
  See @ref CONTROLLER_LENGTH, @ref CONTROLLER_SKIP, @ref CONTROLLER_START_ADDRESS, @ref CONTROLLER_STOP_ADDRESS, @ref CONTROLLER_UNIFORM,
  @ref CONTROLLER_PINPOINT
*/

/*! @ingroup CONTROLLER_MULTI
*/
class CONTROL
{
  public:
    /*! @ingroup CONTROLLER_MULTI
      Open outstream
    */
    CONTROL(BOOL passContext= false, const string & prefix = "", 
            string knob_family = "pintool:control",
            string knob_family_description = "Controller knobs" ) 
        : _controller_knob_family(knob_family, knob_family_description),
          _passContext(passContext),
          _outKnob(KNOB_MODE_WRITEONCE,
                   knob_family,
                   "control_log",
                   "", 
                   "log file for start/stop conditions",
                   prefix
          ),
          _default_start_thread(false),
          _init(passContext),
          _length(passContext, prefix, knob_family),
          _skip(passContext, prefix, knob_family),
          _startAddress(passContext, prefix, knob_family),
          _startTID(passContext, prefix, knob_family),
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
          _startInt3(passContext, prefix, knob_family),
          _stopInt3(passContext, prefix, knob_family),
          _startItext(passContext, prefix, knob_family),
          _stopItext(passContext, prefix, knob_family),
          _startSscmark(passContext, prefix, knob_family),
          _stopSscmark(passContext, prefix, knob_family),
          _startISAExtension(passContext, prefix, knob_family),
          _startISACategory(passContext, prefix, knob_family),
          _stopISAExtension(passContext, prefix, knob_family),
          _stopISACategory(passContext, prefix, knob_family),
#endif
          _stopAddress(passContext, prefix, knob_family),
          _pinpoints(passContext, prefix, knob_family),
          _uniform(passContext, prefix, knob_family),
          _iregions(passContext, prefix, knob_family)
    {
    }
    
    /*! @ingroup CONTROLLER_MULTI
      Activate all the component controllers
    */
    INT32 CheckKnobs(CONTROL_HANDLER ch, VOID * val)
    {
        _val = val;
        _controlHandler = ch;
        string filename =  _outKnob.Value();

        if (filename != "")
        {
            _outstream = new ofstream(filename.c_str());
            _outstream->setf(ios::showbase);
        }


        // All the controllers call back via our LocalHandler -- which calls back to the
        // pintool which created this controller.
        INT32 start = 0;
        start = start + _skip.CheckKnobs(LocalHandler, this);
        start = start + _length.CheckKnobs(LocalHandler, this);
        start = start + _startAddress.CheckKnobs(LocalHandler, this);
        start = start + _startTID.CheckKnobs(LocalHandler, this);
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
        start = start + _startInt3.CheckKnobs(LocalHandler, this);
        start = start + _stopInt3.CheckKnobs(LocalHandler, this);

        start = start + _startItext.CheckKnobs(LocalHandler, this);
        start = start + _stopItext.CheckKnobs(LocalHandler, this);
        start = start + _startSscmark.CheckKnobs(LocalHandler, this);
        start = start + _stopSscmark.CheckKnobs(LocalHandler, this);

        start = start + _startISAExtension.CheckKnobs(LocalHandler, this);
        start = start + _startISACategory.CheckKnobs(LocalHandler, this);
        start = start + _stopISAExtension.CheckKnobs(LocalHandler, this);
        start = start + _stopISACategory.CheckKnobs(LocalHandler, this);
#endif
        start = start + _stopAddress.CheckKnobs(LocalHandler, this);
        start = start + _fini.CheckKnobs(LocalHandler, this);

        INT32 ppstart = 0;
        ppstart = _pinpoints.CheckKnobs(LocalHandler, this, _outstream);
        INT32 rstart = 0;
        rstart = _iregions.CheckKnobs(LocalHandler, this);
        
        
        if (ppstart || rstart)
        {
            if(start)
            {
                cerr << "Can not combine pinpoints/regions with any other controller!" << endl;
                ASSERTX(0);
            }
            else 
                start = ppstart || rstart;
        } 


        INT32 ustart = 0;
        ustart = _uniform.CheckKnobs(LocalHandler, this);
        if ( ustart )
        {
            if(start)
            {
                cerr << "Can not combine uniform with any other controller!" << endl;
                ASSERTX(0);
            } 
            else 
                start = ustart;
        }

        // If none of the controllers has a start condition, then start immediately
        if (start == 0)
        {
            _default_start_thread = true;
            start = start + _init.CheckKnobs(LocalHandler, this);
        }

        return start;
    }

#if defined(TARGET_IA32) || defined(TARGET_IA32E)
    bool StartInt3Active() const { return _startInt3.IsActive(); };
    bool StopInt3Active() const { return _stopInt3.IsActive(); };

    bool StartItextActive() const { return _startItext.IsActive(); };
    bool StopItextActive() const { return _stopItext.IsActive(); };

    bool StartSscmarkActive() const { return _startSscmark.IsActive(); };
    bool StopSscmarkActive() const { return _stopSscmark.IsActive(); };

    bool StartISAExtensionActive() const { return _startISAExtension.IsActive(); };
    bool StartISACategoryActive() const { return _startISACategory.IsActive(); };
    bool StopISAExtensionActive() const { return _stopISAExtension.IsActive(); };
    bool StopISACategoryActive() const { return _stopISACategory.IsActive(); };
#endif
    bool StartAddressActive() const { return _startAddress.IsActive(); }
    bool StartTIDActive()     const { return _startTID.IsActive(); }

    bool PinPointsActive() const { return _pinpoints.IsActive(); };
    bool IregionsActive() const { return _iregions.IsActive(); }
    bool UniformActive() const { return _uniform.IsActive(); };
    IREGION * CurrentIregion(THREADID tid=0) const { return _iregions.LastTriggeredRegion(tid); };
    UINT32 NumPp(THREADID threadid = 0) const { return _pinpoints.NumPp(threadid); };
    UINT32 PP_Phase(UINT32 pp, THREADID threadid = 0) const { return _pinpoints.PP_Phase(pp);};
    UINT32 PP_Version(THREADID threadid = 0) const { return _pinpoints.PP_Version(threadid);};
    UINT32 PP_Slice(UINT32 pp, THREADID threadid = 0) const {return _pinpoints.PP_Slice(pp);}; 
    UINT32 PP_WarmupFactor(UINT32 pp, THREADID threadid = 0) const {return _pinpoints.PP_WarmupFactor(pp, threadid);}; 
    UINT32 PP_WeightTimesThousand(UINT32 pp, THREADID threadid = 0) const {return _pinpoints.PP_WeightTimesThousand(pp);}; 
    UINT32 CurrentPp(THREADID threadid = 0) const { return _pinpoints.CurrentPp(threadid); };
    UINT32 CurrentPhase(THREADID threadid = 0) const { return _pinpoints.CurrentPhase(threadid); };
    // double CurrentPpWeight(THREADID threadid = 0) const { return _pinpoints.CurrentPpWeight(threadid); };
    UINT32 CurrentPpWeightTimesThousand(THREADID threadid = 0) const { return _pinpoints.CurrentPpWeightTimesThousand(threadid); };
    UINT64 CurrentPpLength(THREADID threadid = 0) const { return _pinpoints.CurrentPpLength(threadid); };
    UINT64 CurrentPpStartIcount(THREADID threadid = 0) const { return _pinpoints.CurrentPpStartIcount(threadid); };

    /* return 1 if the threads should start by default. The controller
     * currently starts thread 0 in this case, legacy. */
    BOOL DefaultStartThread() { return  _default_start_thread; }

  private:
    static VOID LocalHandler(CONTROL_EVENT ev, VOID * val, CONTEXT * ctxt, VOID * ip, THREADID tid)
    {
        CONTROL * control = static_cast<CONTROL*>(val);

        // Notify all interested controls about this event
        control->_length.Event(ev);
        control->_stopAddress.Event(ev);
        control->_fini.Event(ev);

        // Notify the parent
        control->_controlHandler(ev, control->_val, ctxt, ip, tid);
    }

    KNOB_COMMENT _controller_knob_family;
    BOOL _passContext;
    KNOB<string> _outKnob;
    ofstream * _outstream;

    CONTROL_HANDLER _controlHandler;
    VOID * _val;
    BOOL _default_start_thread;
    CONTROL_INIT _init;
    CONTROL_FINI _fini;
    CONTROL_LENGTH _length;
    CONTROL_SKIP _skip;
    CONTROL_START_ADDRESS _startAddress;
    CONTROL_START_THREADID _startTID;
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
    CONTROL_START_INT3 _startInt3;
    CONTROL_STOP_INT3 _stopInt3;

    CONTROL_START_ITEXT _startItext;
    CONTROL_STOP_ITEXT _stopItext;

    CONTROL_START_SSCMARK _startSscmark;
    CONTROL_STOP_SSCMARK _stopSscmark;

    CONTROL_START_ISA_EXTENSION _startISAExtension;
    CONTROL_START_ISA_CATEGORY _startISACategory;
    CONTROL_STOP_ISA_EXTENSION _stopISAExtension;
    CONTROL_STOP_ISA_CATEGORY _stopISACategory;
#endif
    CONTROL_STOP_ADDRESS _stopAddress;
    CONTROL_PINPOINT _pinpoints;
    CONTROL_UNIFORM _uniform;
    CONTROL_IREGIONS _iregions;
};


} //namespace
#endif
