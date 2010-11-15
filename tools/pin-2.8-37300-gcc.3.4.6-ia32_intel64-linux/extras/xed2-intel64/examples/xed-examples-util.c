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
/// @file xed-examples-util.cpp
/// @author Mark Charney   <mark.charney@intel.com>

#include "xed-interface.h"
#include "xed-get-time.h"
#include "xed-examples-util.h"
#include <string.h> //strlen, memcmp, memset
#if defined(__APPLE__) || defined(__linux__) || defined(__linux) || \
        defined(__FreeBSD__)
# include <unistd.h>
# include <sys/mman.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
#endif
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include "xed-portability.h"
#include "xed-util.h"

#define XED_TMP_BUF_LEN (1024*4)




void xed_decode_file_info_init(xed_decode_file_info_t* p,
                               char* arg_input_file_name,
                               xed_state_t arg_dstate,
                               int arg_ninst,
                               int arg_decode_only,
                               int arg_64b, 
                               char* arg_target_section, 
                               xed_bool_t arg_use_binary_mode, 
                               xed_int64_t arg_addr_start,
                               xed_int64_t arg_addr_end,
                               void (*arg_late_init)(xed_decoded_inst_t*),
                               xed_bool_t arg_xml_format,
                               xed_int64_t arg_fake_base,
                               xed_bool_t arg_resync)

{
    p->input_file_name = arg_input_file_name;
    p->dstate = arg_dstate;
    p->ninst = arg_ninst;
    p->decode_only = arg_decode_only;
    p->sixty_four_bit = arg_64b;

    p->target_section = arg_target_section;
    p->use_binary_mode =arg_use_binary_mode;
    p->addr_start = arg_addr_start;
    p->addr_end = arg_addr_end;
    p->late_init = arg_late_init;
    p->xml_format = arg_xml_format;
    p->fake_base = arg_fake_base;
    p->resync = arg_resync;
}

typedef struct {
    xed_uint64_t  total_time ;
    xed_uint64_t  total_insts ;
    xed_uint64_t  total_ilen ;
    xed_uint64_t  total_olen ;
    xed_uint64_t  total_shorter ;
    xed_uint64_t  total_longer ;
    xed_uint64_t  bad_times ;
    xed_uint64_t  reset_counter;
} xed_decode_stats_t;

void xed_decode_stats_reset(xed_decode_stats_t* p, xed_uint64_t t1, 
                            xed_uint64_t t2) {
    if (t2 > t1) 
        p->total_time += (t2-t1);
    else
        p->bad_times++;
    p->total_insts++;
    p->reset_counter++;
    if (p->reset_counter == 50) {
        if (CLIENT_VERBOSE1) 
            printf("\n\nRESETTING STATS\n\n");
        // to ignore startup transients paging everything in.
        p->total_insts=0;
        p->total_time=0;
    }
}

void xed_decode_stats_zero(xed_decode_stats_t* p)    {
    p->total_time = 0;
    p->total_insts = 0;
    p->total_ilen = 0;
    p->total_olen = 0;
    p->total_shorter = 0;
    p->total_longer = 0;
    p->bad_times = 0;
    p->reset_counter = 0;
}

static xed_decode_stats_t xed_stats;
int xed_syntax = 0;
int intel_syntax = 1;
int att_syntax = 0;
int client_verbose=0; 

////////////////////////////////////////////////////////////////////////////

static char xed_toupper(char c) {
    if (c >= 'a' && c <= 'z')
        return c-'a'+'A';
    return c;
}

char* xed_upcase_buf(char* s) {
    xed_uint_t len = XED_STATIC_CAST(xed_uint_t,strlen(s));
    xed_uint_t i;
    for(i=0 ; i < len ; i++ ) 
        s[i] = XED_STATIC_CAST(char,xed_toupper(s[i]));
    return s;
}

static xed_uint8_t convert_nibble(xed_uint8_t x) {
    // convert ascii nibble to hex
    xed_uint8_t rv = 0;
    if (x >= '0' && x <= '9') 
        rv = x - '0';
    else if (x >= 'A' && x <= 'F') 
        rv = x - 'A' + 10;
    else if (x >= 'a' && x <= 'f') 
        rv = x - 'a' + 10;
    else    {
        printf("Error converting hex digit. Nibble value 0x%x\n", x);
        exit(1);
    }
    return rv;
}


xed_int64_t xed_atoi_hex(char* buf) {
    xed_int64_t o=0;
    xed_uint_t i;
    xed_uint_t len = XED_STATIC_CAST(xed_uint_t,strlen(buf));
    for(i=0; i<len ; i++) 
        o = o*16 + convert_nibble(buf[i]);
    return o;
}

xed_int64_t xed_atoi_general(char* buf, int mul) {
    /*      mul should be 1000 or 1024     */
    char* q;
    xed_int64_t b;

    char* p = buf;
    while(*p && isspace(*p))
    {
        p++;
    }
    // exclude hex; octal works just fine
    q = p;
    if (*q == '-' || *q == '+')
    {
        q++;
    }
    if (*q=='0' && (q[1]=='x' || q[1]=='X'))
    {
        return xed_strtoll(buf,0);
    }

    b = xed_strtoll(buf,0);
    if (p)
    {
        while(*p && (*p == '-' || *p == '+'))
        {
            p++;
        }
        while(*p && isdigit(*p))
        {
            p++;
        }

        if (*p != 0)
        {
            if (*p == 'k' || *p == 'K')
            {
                b = b * mul;
            }
            else if (*p == 'm' || *p == 'M')
            {
                b = b * mul * mul;
            }
            else if (*p == 'g' || *p == 'G' || *p == 'b' || *p == 'B')
            {
                b = b * mul * mul * mul;
            }
        }
    }
    return b;
}

static char nibble_to_ascii_hex(xed_uint8_t i) {
    if (i<10) return i+'0';
    if (i<16) return i-10+'A';
    return '?';
}
void xed_print_hex_line(char* buf, const xed_uint8_t* array, const int length, 
                        const int buflen) {
  int n = length;
  int i=0;
  if (length == 0)
      n = XED_MAX_INSTRUCTION_BYTES;
  assert(buflen >= (2*n+1)); /* including null */
  for( i=0 ; i< n; i++)     {
      buf[2*i+0] = nibble_to_ascii_hex(array[i]>>4);
      buf[2*i+1] = nibble_to_ascii_hex(array[i]&0xF);
  }
  buf[2*i]=0;
}



void xedex_derror(const char* s) {
    printf("[XED CLIENT ERROR] %s\n",s);
    exit(1);
}

void xedex_dwarn(const char* s) {
    printf("[XED CLIENT WARNING] %s\n",s);
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////




void xed_print_decode_stats()
{
    double cpi;
    xed_int64_t growth;
    printf("#Total decode cycles:        " XED_FMT_LU "\n", 
           xed_stats.total_time);
    printf("#Total instructions decoded: " XED_FMT_LU "\n", 
           xed_stats.total_insts);
#if defined(_MSC_VER) 
#  if  _MSC_VER==1200
#    define XCAST(x) XED_STATIC_CAST(xed_int64_t,x)
#  else 
#    define XCAST(x) (x)
#  endif
#else
# define XCAST(x) (x)
#endif
    cpi  =  1.0 * XCAST(xed_stats.total_time) / XCAST(xed_stats.total_insts);
    printf("#Total cycles/instructions decoded: %f\n" , cpi);

    printf("#Bad times: " XED_FMT_LU "\n", xed_stats.bad_times);
    printf("#Total input length bytes: " XED_FMT_LU "\n", 
           xed_stats.total_ilen );
    printf("#Total output length bytes: " XED_FMT_LU "\n",
           xed_stats.total_olen );
    printf("#Growth bytes: " XED_FMT_LU "\n", xed_stats.total_longer );
    printf("#Shrinkage bytes: " XED_FMT_LU "\n", xed_stats.total_shorter );
    growth = xed_stats.total_olen - xed_stats.total_ilen;
    printf("#Growth/Shrinkage  bytes: " XED_FMT_LD "\n", growth );
    if (xed_stats.total_ilen)    {
        double pct_growth = \
               100.0 * growth / (double) XCAST(xed_stats.total_ilen);
        printf("#Code size growth percent: %f\n", pct_growth);
    }
}


void
xed_map_region(const char* path,
               void** start,
               unsigned int* length)
{
#if defined(_WIN32) 
    FILE* f;
    size_t t,ilen;
    xed_uint8_t* p;
#if defined(XED_MSVC8_OR_LATER)
    errno_t err;
    fprintf(stderr,"#Opening %s\n", path);
    err = fopen_s(&f,path,"rb");
#else
    int err=0;
    fprintf(stderr,"#Opening %s\n", path);
    f = fopen(path,"rb");
    err = (f==0);
#endif
    if (err != 0) {
        fprintf(stderr,"ERROR: Could not open %s\n", path);
        exit(1);
    }
    err =  fseek(f, 0, SEEK_END);
    if (err != 0) {
        fprintf(stderr,"ERROR: Could not fseek %s\n", path);
        exit(1);
    }
    ilen = ftell(f);
    fprintf(stderr,"#Trying to read " XED_FMT_SIZET "\n", ilen);
    p = (xed_uint8_t*)malloc(ilen);
    t=0;
    err = fseek(f,0, SEEK_SET);
    if (err != 0) {
        fprintf(stderr,"ERROR: Could not fseek to start of file %s\n", path);
        exit(1);
    }
    
    while(t < ilen) {
        size_t n;
        if (feof(f)) {
            fprintf(stderr, "#Read EOF. Stopping.\n");
            break;
        }
        n = fread(p+t, 1, ilen-t,f);
        t = t+n;
        fprintf(stderr,"#Read " XED_FMT_SIZET " of " XED_FMT_SIZET " bytes\n", 
                t, ilen);
        if (ferror(f)) {
            fprintf(stderr, "Error in file read. Stopping.\n");
            break;
        }
    }
    fclose(f);
    *start = p;
    *length = (unsigned int)ilen;
    
#else 
    int ilen,fd;
    fd = open(path, O_RDONLY);
    if (fd == -1)   {
        printf("Could not open file: %s\n" , path);
        exit(1);
    }
    ilen = lseek(fd, 0, SEEK_END); // find the size.
    if (ilen == -1)
        xedex_derror("lseek failed");
    else 
        *length = (unsigned int) ilen;

    lseek(fd, 0, SEEK_SET); // go to the beginning
    *start = mmap(0,
                  *length,
                  PROT_READ|PROT_WRITE,
                  MAP_PRIVATE,
                  fd,
                  0);
    if (*start == (void*) -1)
        xedex_derror("could not map region");
#endif
    if (CLIENT_VERBOSE1)
        printf("Mapped " XED_FMT_U " bytes!\n", *length);
}


////////////////////////////////////////////////////////////////////////////

static int all_zeros(xed_uint8_t* p, unsigned int len) {
    unsigned int i;
    for( i=0;i<len;i++) 
        if (p[i]) 
            return 0;
    return 1;
}

int
fn_disassemble_xed(xed_syntax_enum_t syntax,
                   char* buf,
                   int buflen,
                   xed_decoded_inst_t* xedd, 
                   xed_uint64_t runtime_instruction_address,
                   void* caller_data) {   

    char buffer[XED_TMP_BUF_LEN];
    int blen= buflen;
    int ok = xed_format_context(syntax, xedd, 
                                buffer, XED_TMP_BUF_LEN,
                                runtime_instruction_address, caller_data);
    if (ok)
        blen = xed_strncpy(buf,buffer,buflen);
    else    {
        blen = buflen;
        blen = xed_strncpy(buf,"Error disassembling ",blen);
        blen = xed_strncat(buf, xed_syntax_enum_t2str(syntax),blen);
        blen = xed_strncat(buf," syntax.",blen);
    }
    return blen;
}


void disassemble(char* buf,
                 int buflen,
                 xed_decoded_inst_t* xedd,
                 xed_uint64_t runtime_instruction_address,
                 void* caller_data)
{
    int blen = buflen;
    if (xed_syntax)    {
        blen = fn_disassemble_xed(XED_SYNTAX_XED, buf, blen,
                                  xedd, runtime_instruction_address,
                                  caller_data);
        if (att_syntax || intel_syntax)
            blen = xed_strncat(buf, " | ",blen);
    }
    if (att_syntax)    {
        char* xbuf = buf+strlen(buf);
        blen = fn_disassemble_xed(XED_SYNTAX_ATT, xbuf, blen, 
                                  xedd, runtime_instruction_address, 
                                  caller_data);
        if (intel_syntax)
            blen = xed_strncat(buf, " | ",blen);
    }
    if (intel_syntax) {
        char* ybuf = buf+strlen(buf);
        blen = fn_disassemble_xed(XED_SYNTAX_INTEL, ybuf, blen,
                                  xedd, runtime_instruction_address, 
                                  caller_data);
    }
}

void xed_decode_error(
    xed_uint64_t runtime_instruction_address,
    xed_uint64_t offset, 
    const xed_uint8_t* ptr, 
    xed_error_enum_t xed_error) {
    char buf[XED_HEX_BUFLEN];
    printf("ERROR: %s Could not decode at offset: 0x" 
           XED_FMT_LX " PC: 0x" XED_FMT_LX ": [", 
           xed_error_enum_t2str(xed_error),
           offset,
           runtime_instruction_address);

    xed_print_hex_line(buf, ptr, 15,XED_HEX_BUFLEN);
    printf("%s]\n",buf);
}

///////////////////////////////////////////////////////////////////////////
// 2007-07-02

static void print_hex_line(const xed_uint8_t* p, unsigned int length) {
        char buf[XED_HEX_BUFLEN];
        unsigned int lim = XED_HEX_BUFLEN/2;
        if (length < lim)
            lim = length;
        xed_print_hex_line(buf,p, lim, XED_HEX_BUFLEN); 
        printf("%s\n", buf);
}

xed_uint_t disas_decode_binary(const xed_state_t* dstate,
                               const xed_uint8_t* hex_decode_text,
                               const unsigned int bytes,
                               xed_decoded_inst_t* xedd,
                               xed_uint64_t runtime_address) {
    xed_uint64_t t1,t2;
    xed_error_enum_t xed_error;
    xed_bool_t okay;

    if (CLIENT_VERBOSE) {
        print_hex_line(hex_decode_text, bytes);
    }
    t1 = xed_get_time();
    xed_error = xed_decode(xedd, hex_decode_text, bytes);
    t2 = xed_get_time();
    okay = (xed_error == XED_ERROR_NONE);
    if (CLIENT_VERBOSE3) {
        xed_uint64_t delta = t2-t1;
        printf("Decode time = " XED_FMT_LU "\n", delta);
    }
    if (okay)     {

        if (CLIENT_VERBOSE1) {
            char tbuf[XED_TMP_BUF_LEN];
            xed_decoded_inst_dump(xedd,tbuf,XED_TMP_BUF_LEN);
            printf("%s\n",tbuf);
        }
        if (CLIENT_VERBOSE) {
            char buf[XED_TMP_BUF_LEN];
            if (xed_decoded_inst_valid(xedd)) 
            {
                printf( "ICLASS: %s   CATEGORY: %s   EXTENSION: %s  IFORM: %s"
                        "   ISA_SET: %s\n", 
                xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(xedd)),
                xed_category_enum_t2str(xed_decoded_inst_get_category(xedd)),
                xed_extension_enum_t2str(xed_decoded_inst_get_extension(xedd)),
                xed_iform_enum_t2str(xed_decoded_inst_get_iform_enum(xedd)),
                xed_isa_set_enum_t2str(xed_decoded_inst_get_isa_set(xedd)));
            }
            memset(buf,0,XED_TMP_BUF_LEN);
            disassemble(buf,XED_TMP_BUF_LEN, xedd, runtime_address,0);
            printf("SHORT: %s\n", buf);
        }
        return 1;
    }
    else {
        xed_decode_error(0, 0, hex_decode_text, xed_error);
        return 0;
    }
    (void) dstate; // pacify compiler
}

xed_uint_t disas_decode_encode_binary(const xed_state_t* dstate,
                                      const xed_uint8_t* decode_text_binary,
                                      const unsigned int bytes,
                                      xed_decoded_inst_t* xedd, 
                                      xed_uint64_t runtime_address)   {
    // decode then encode
    unsigned int retval_olen = 0;
    // decode it...
    xed_bool_t decode_okay =  disas_decode_binary(dstate, decode_text_binary,
                                                  bytes, xedd, 
                                                  runtime_address);
    if (decode_okay)     {
        xed_error_enum_t encode_okay;
        unsigned int enc_olen, ilen = XED_MAX_INSTRUCTION_BYTES;
        xed_uint8_t array[XED_MAX_INSTRUCTION_BYTES];
        // they are basically the same now
        xed_encoder_request_t* enc_req = xedd; 
        // convert decode structure to proper encode structure
        xed_encoder_request_init_from_decode(xedd);
        
        // encode it again...
        encode_okay =  xed_encode(enc_req, array, ilen, &enc_olen);
        if (encode_okay != XED_ERROR_NONE) {
            if (CLIENT_VERBOSE) {
                char buf[XED_TMP_BUF_LEN];
                char buf2[XED_TMP_BUF_LEN];
                int blen=XED_TMP_BUF_LEN;
                xed_encode_request_print(enc_req, buf, XED_TMP_BUF_LEN);
                blen = xed_strncpy(buf2,"Could not re-encode: ", blen);
                blen = xed_strncat(buf2, buf, blen);
                blen = xed_strncat(buf2,"\nError code was: ",blen);
                blen = xed_strncat(buf2,
                                   xed_error_enum_t2str(encode_okay),blen);
                blen = xed_strncat(buf2, "\n",blen);
                xedex_dwarn(buf2);
            }
        }
        else         {
            retval_olen = enc_olen;
            // See if it matched the original...
            if (CLIENT_VERBOSE) {
                char buf[XED_HEX_BUFLEN];
                xed_uint_t dec_length; 
                xed_print_hex_line(buf,array, enc_olen, XED_HEX_BUFLEN);
                printf("Encodable! %s\n",buf);
                dec_length = xed_decoded_inst_get_length(xedd);
                if ((enc_olen != dec_length ||
                     memcmp(decode_text_binary, array, enc_olen)  )) {
                    char buf2[XED_TMP_BUF_LEN];
                    char buf3[XED_TMP_BUF_LEN];
                    printf("Discrepenacy after re-encoding. dec_len= " 
                           XED_FMT_U " ", dec_length);
                    xed_print_hex_line(buf, decode_text_binary, 
                                       dec_length,XED_HEX_BUFLEN);
                    printf("[%s] ", buf);
                    printf("enc_olen= " XED_FMT_U "", enc_olen);
                    xed_print_hex_line(buf, array, enc_olen, XED_HEX_BUFLEN);
                    printf(" [%s] ", buf);
                    printf("for instruction: ");
                    xed_decoded_inst_dump(xedd, buf3,XED_TMP_BUF_LEN);
                    printf("%s\n", buf3);
                    printf("vs Encode  request: ");
                    xed_encode_request_print(enc_req, buf2, XED_TMP_BUF_LEN);
                    printf("%s\n", buf2);
                }
                else 
                    printf("Identical re-encoding\n");
            }
        }
    }
    return retval_olen;
}



///////////////////////////////////////////////////////////////////////////

void xed_disas_test(xed_disas_info_t di) {

    static int first = 1;
    xed_uint64_t errors = 0;
    unsigned int m;
    unsigned char* z;
    unsigned char* zlimit;
    unsigned int length;
    
    int skipping;
    int last_all_zeros;
    unsigned int i;

    int okay;
    xed_decoded_inst_t xedd;

    xed_uint64_t runtime_instruction_address;
//#define     XED_USE_DECODE_CACHE
#if defined(XED_USE_DECODE_CACHE)
    xed_decode_cache_t cache;
    xed_uint32_t n_cache_entries = 16*1024;
    xed_decode_cache_entry_t* cache_entries = 
        (xed_decode_cache_entry_t*) malloc(n_cache_entries * 
                                           sizeof(xed_decode_cache_entry_t));
    xed_decode_cache_initialize(&cache, cache_entries, n_cache_entries);
#endif

    if (first) {
        xed_decode_stats_zero(&xed_stats);
        first = 0;
    }

    m = di.ninst; // number of things to decode
    z = di.a;
  
    if (di.runtime_vaddr_disas_start) 
        if (di.runtime_vaddr_disas_start > di.runtime_vaddr)
            z = (di.runtime_vaddr_disas_start - di.runtime_vaddr) + di.a;

    zlimit = 0;
    if (di.runtime_vaddr_disas_end) {
        if (di.runtime_vaddr_disas_end > di.runtime_vaddr)
            zlimit = (di.runtime_vaddr_disas_end - di.runtime_vaddr) + di.a;
        else  /* end address is before start of this region -- skip it */
            goto finish;
    } 


    if (z >= di.q)   /* start pointer  is after end of section */
        goto finish;

    // for skipping long strings of zeros
    skipping = 0;
    last_all_zeros = 0;
    for( i=0; i<m;i++) 
    {
        int ilim,elim;
        if (zlimit && z >= zlimit) {
            if (di.xml_format == 0)
                printf("# end of range.\n");
            break;
        }
        if (z >= di.q) {
            if (di.xml_format == 0)
                printf("# end of text section.\n");
            break;
        }

        /* if we get near the end of the section, clip the itext length */
        ilim = 15;
        elim = di.q - z;
        if (elim < ilim) 
           ilim = elim;

        if (CLIENT_VERBOSE3) {
            printf("\n==============================================\n");
            printf("Decoding instruction " XED_FMT_U "\n", i);
            printf("==============================================\n");
        }
    
        // if we get two full things of 0's in a row, start skipping.
        if (all_zeros((xed_uint8_t*) z, ilim)) 
        {
            if (skipping) {
                z = z + ilim;
                continue;
            }
            else if (last_all_zeros) { 
                printf("...\n");
                z = z + ilim;
                skipping = 1;
                continue;
            }
            else
                last_all_zeros = 1;
        }
        else
        {
            skipping = 0;
            last_all_zeros = 0;
        }

        runtime_instruction_address =  ((xed_uint64_t)(z-di.a)) + 
                                       di.runtime_vaddr;
         
        if (CLIENT_VERBOSE3) {
            char tbuf[XED_HEX_BUFLEN];
            printf("Runtime Address " XED_FMT_LX ,runtime_instruction_address);
            xed_print_hex_line(tbuf, (xed_uint8_t*) z, ilim,XED_HEX_BUFLEN);
            printf(" [%s]\n", tbuf);
        }
        okay = 0;
        xed_decoded_inst_zero_set_mode(&xedd, di.dstate);
        if (di.late_init) 
            (*di.late_init)(&xedd);

        length = 0;
        if ( di.decode_only )
        {
            xed_uint64_t t1 = xed_get_time();
            xed_uint64_t t2;
            
            xed_error_enum_t xed_error;
#if defined(XED_USE_DECODE_CACHE)
            xed_error = xed_decode_cache(&xedd, 
                                    XED_REINTERPRET_CAST(const xed_uint8_t*,z),
                                    ilim, 
                                    &cache);
#else
            xed_error = xed_decode(&xedd, 
                                   XED_REINTERPRET_CAST(const xed_uint8_t*,z),
                                   ilim);
#endif
            t2 = xed_get_time();
            okay = (xed_error == XED_ERROR_NONE);
            xed_decode_stats_reset(&xed_stats, t1, t2);
           
            length = xed_decoded_inst_get_length(&xedd);
            if (okay && length == 0) {
                printf("Zero length on decoded instruction!\n");
                xed_decode_error( runtime_instruction_address, 
                                  z-di.a, z, xed_error);
                xedex_derror("Dieing");
            }

            if (di.resync && di.symfn) {
                xed_bool_t resync = 0;
                unsigned int x;
                for(x=1;x<length;x++) {
                    char* name = (*di.symfn)(runtime_instruction_address+x, 
                                             di.caller_symbol_data);
                    if (name) {
                        char buf[XED_HEX_BUFLEN];
                        /* bad news. We found a symbol in the middle of an
                         * instruction. That probably means decoding is
                         * messed up.  This usually happens because of
                         * data-in the code/text section.  We should reject
                         * the current instruction and pick up at the
                         * symbol address. */
                        printf("ERROR: found symbol in the middle of"
                               " an instruction. Resynchronizing...\n");
                        printf("ERROR: Rejecting: [");
                        xed_print_hex_line(buf, z, x, XED_HEX_BUFLEN);
                        printf("%s]\n",buf);

                        z += x;
                        resync = 1;
                        break;
                    }
                }
                if (resync)
                    continue;
            }

            xed_stats.total_ilen += length;

            if (okay)  {
                if (CLIENT_VERBOSE1) {
                    char tbuf[XED_TMP_BUF_LEN];
                    xed_decoded_inst_dump(&xedd,tbuf, XED_TMP_BUF_LEN);
                    printf("%s\n",tbuf);
                }
                if (CLIENT_VERBOSE)  {
                    char buffer[XED_TMP_BUF_LEN];
                    unsigned int dec_len;
                    unsigned int sp;
                    if (di.symfn) {
                        char* name = (*di.symfn)(runtime_instruction_address, 
                                                 di.caller_symbol_data);
                        if (name) {
                            if (di.xml_format) 
                                printf("\n<SYM>%s</SYM>\n", name);
                            else
                                printf("\nSYM %s:\n", name);
                        }
                    }
                    if (di.xml_format) {
                        printf("<ASMLINE>\n"); 
                        printf("  <ADDR>" XED_FMT_LX "</ADDR>\n", 
                               runtime_instruction_address);
                        printf("  <CATEGORY>%s</CATEGORY>\n", 
                               xed_category_enum_t2str(
                                        xed_decoded_inst_get_category(&xedd)));
                        printf("  <EXTENSION>%s</EXTENSION>\n",
                               xed_extension_enum_t2str(
                                       xed_decoded_inst_get_extension(&xedd)));
                        printf("  <ITEXT>");
                        dec_len = xed_decoded_inst_get_length(&xedd);
                        xed_print_hex_line(buffer, (xed_uint8_t*) z, 
                                           dec_len, XED_TMP_BUF_LEN);
                        printf("%s</ITEXT>\n",buffer);
                        buffer[0]=0;
                        disassemble(buffer,XED_TMP_BUF_LEN, 
                                    &xedd, runtime_instruction_address, 
                                    di.caller_symbol_data);
                        printf( "  %s\n",buffer);
                        printf("</ASMLINE>\n"); 
                    }
                    else {
                        printf("XDIS " XED_FMT_LX ": ",
                               runtime_instruction_address);
                        printf("%-9s ", xed_category_enum_t2str(
                                        xed_decoded_inst_get_category(&xedd)));
                        printf("%-5s ", xed_extension_enum_t2str(
                                       xed_decoded_inst_get_extension(&xedd)));
                        dec_len = xed_decoded_inst_get_length(&xedd);
                        xed_print_hex_line(buffer, (xed_uint8_t*) z, 
                                           dec_len, XED_HEX_BUFLEN);
                        printf("%s",buffer);
                        // pad out the instruction bytes
                        for ( sp=dec_len; sp < 12; sp++) 
                            printf("  ");
                        printf(" ");
                        buffer[0]=0;
                        disassemble(buffer,XED_TMP_BUF_LEN,
                                    &xedd, 
                                    runtime_instruction_address, 
                                    di.caller_symbol_data);
                        printf( "%s\n",buffer);
                    }
                }
            }
            else {
                errors++;
                xed_decode_error( runtime_instruction_address, z-di.a, z, 
                                  xed_error);
                // just give a length of 1B to see if we can restart decode...
                length = 1;
            }
        }
        else
        {
            xed_uint64_t t1 = xed_get_time();
            xed_uint64_t t2;
            unsigned int olen  = 0;
            olen  = disas_decode_encode_binary(di.dstate, 
                                        XED_REINTERPRET_CAST(const xed_uint8_t*,z),
                                               ilim,
                                               &xedd, 
                                               runtime_instruction_address);
            t2=xed_get_time();
            okay = (olen != 0);
            xed_decode_stats_reset(&xed_stats, t1, t2);
            if (!okay)  {
                errors++;
                printf("-- Could not decode/encode at offset: %d\n" ,
                       (int)(z-di.a));
                // just give a length of 1B to see if we can restart decode...
                length = 1;
                //exit(1);
            }        
            else {
                length = xed_decoded_inst_get_length(&xedd);
                xed_stats.total_ilen += length;
                xed_stats.total_olen += olen;
                if (length > olen)
                    xed_stats.total_shorter += (length - olen);
                else
                    xed_stats.total_longer += (olen - length);
            }

        }

        z = z + length;
    }
   
    if (di.xml_format == 0) {
        printf( "# Errors: " XED_FMT_LU "\n", errors);
    }
finish:

#if defined(XED_USE_DECODE_CACHE)
    if (di.xml_format == 0) {
        printf( "# Cache refs:            " XED_FMT_LU12 "\n", 
                cache.misses + cache.hits);
        printf( "# Cache hits:            " XED_FMT_LU12 "\n", 
                cache.hits);
        printf( "# Cache found_something: " XED_FMT_LU12 "\n", 
                cache.found_something);
        printf( "# Cache miscompares:     " XED_FMT_LU12 "\n", 
                cache.miscompares);
        printf( "# Cache hit_rate: %.2f\n", 
                100.0*cache.hits/(cache.misses+cache.hits));
    }
    free(cache_entries);
#endif
    ;
}


xed_uint8_t
convert_ascii_nibble(char c)
{
  if (c >= '0' && c <= '9') {
    return c-'0';
  }
  else if (c >= 'a' && c <= 'f') {
    return c-'a' + 10;
  }
  else if (c >= 'A' && c <= 'F') {
    return c-'A' + 10;
  }
  else {
      char buffer[XED_HEX_BUFLEN];
      char* x;
      xed_strncpy(buffer,"Invalid character in hex string: ", XED_HEX_BUFLEN);
      x= buffer+strlen(buffer);
      *x++ = c;
      *x++ = 0;
      xedex_derror(buffer);
      return 0;
  }
}



xed_uint64_t convert_ascii_hex_to_int(const char* s) {
    xed_uint64_t retval = 0;
    const char* p = s;
    while (*p) {
        retval  =  (retval << 4) + convert_ascii_nibble(*p);
        p++;
    }
    return retval;
}


xed_uint8_t convert_ascii_nibbles(char c1, char c2) {
    xed_uint8_t a = convert_ascii_nibble(c1) * 16 + convert_ascii_nibble(c2);
    return a;
}

unsigned int
xed_convert_ascii_to_hex(const char* src, xed_uint8_t* dst, 
                         unsigned int max_bytes)
{
    unsigned int j;
    unsigned int p = 0;
    unsigned int i = 0;

    const unsigned int len = XED_STATIC_CAST(unsigned int,strlen(src));
    if ((len & 1) != 0) 
        xedex_derror("test string was not an even number of nibbles");
    
    if (len > (max_bytes * 2) ) 
        xedex_derror("test string was too long");

    for( j=0;j<max_bytes;j++) 
        dst[j] = 0;

    for(;i<len/2;i++) {
        if (CLIENT_VERBOSE3) 
            printf("Converting %c & %c\n", src[p], src[p+1]);
        dst[i] = convert_ascii_nibbles(src[p], src[p+1]);
        p=p+2;
    }
    return i;
}


static xed_int64_t
convert_base10(const char* buf)
{
    xed_int64_t v = 0;
    xed_int64_t sign = 1;
    int len = XED_STATIC_CAST(int,strlen(buf));
    int i; 
    for(i=0;i<len;i++)
    {
        char c = buf[i];
        if (i == 0 && c == '-')
        {
            sign = -1;
        }
        else if (c >= '0' && c <= '9')
        {
            unsigned int digit = c - '0';
            v = v*10 + digit;
        }
        else if (c == '_') /* skip underscores */
            continue; 
        else
        {
            break;
        }
    }
    return v*sign;
}

static xed_int64_t
convert_base16(const char* buf)
{
    xed_int64_t v = 0;
    int len = XED_STATIC_CAST(int,strlen(buf));
    int start =0 ;
    int i;
    if (len > 2 && buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X'))
    {
        start = 2;
    }
    for(i=start;i<len;i++)
    {
        char c = buf[i];
        if (c >= '0' && c <= '9')
        {
            unsigned int digit = c - '0';
            v = v*16 + digit;
        }
        else if (c >= 'A' && c <= 'F')
        {
            unsigned int digit = c - 'A' + 10;
            v = v*16 + digit;
        }
        else if (c >= 'a' && c <= 'f')
        {
            unsigned int digit = c - 'a' + 10;
            v = v*16 + digit;
        }
        else if (c == '_') /* skip underscores */
            continue;
        else
        {
            break;
        }
    }
    return v;
}

static xed_int64_t
xed_internal_strtoll(const char* buf, int base)
{
    switch(base)
    {
      case 0:
        if (strlen(buf) > 2 && buf[0] == '0' && 
            (buf[1] == 'x' || buf[1] == 'X'))
        {
            return convert_base16(buf);
        }
        return convert_base10(buf);
      case 10:
        return convert_base10(buf);
      case 16:
        return convert_base16(buf);
      default:
        assert(0);
    }
    return 0;
}


xed_int64_t xed_strtoll(const char* buf, int base)
{
    xed_int64_t t;
    // strtoll is missing on some compilers and buggy on some platforms
    t =  xed_internal_strtoll(buf,base);
   return t;
}



////////////////////////////////////////////////////////////////////////////
//Local Variables:
//pref: "xed-examples-util.H"
//End:
