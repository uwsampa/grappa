////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////
#ifndef DICT_OUT_HPP
#define DICT_OUT_HPP

#include <queue>
#include <string>
#include <iostream>
#include <sstream>
#include <stdint.h>

// ruby symbol map ":"

#define DICT_ADD(d, var) (d).add(#var, var)

class Entry {
    private:
        std::string name_;
        std::string value_;
    public:
        Entry( std::string name, std::string value ) 
            : name_( name )
            , value_( value )
        { }

        std::string name( ) {
            return name_;
        }

        std::string value( ) {
            return value_;
        }

};

//TODO use existing output formatter library?

class DictOut {
    private:
        std::queue<Entry*> outputs;
        const std::string sym_mapsto;

        const std::string entryString( Entry* e ) {
            std::stringstream ss;
            ss << "\"" << e->name() << "\""
               << sym_mapsto 
               << e->value();
            return ss.str();
        }

    public:
        DictOut( const std::string map_symbol=":")
            : outputs( )
            , sym_mapsto( map_symbol ) 
        { }

        ~DictOut( ) {
            if (!outputs.empty()) 
                std::cerr << "Warning: did not output DictOut" << std::endl;
        }

        void add( std::string name, std::string value ) {
            outputs.push( new Entry( name, value ) );
        }

        void add( std::string name, double value ) {
            std::stringstream ssv;
            ssv << value;
            outputs.push( new Entry( name, ssv.str() ));
        }
        
        void add( std::string name, uint64_t value ) {
            std::stringstream ssv;
            ssv << value;
            outputs.push( new Entry( name, ssv.str() ));
        }

//        void add( std::string name, int value ) {
//            std::stringstream ssv;
//            ssv << value;
//            outputs.push( new Entry( name, ssv.str() ));
//        }
//        
        void add( std::string name, int64_t value ) {
            std::stringstream ssv;
            ssv << value;
            outputs.push( new Entry( name, ssv.str() ));
        }

        const std::string toString( ) {
            std::stringstream ss;

            ss << "{";

            int size = outputs.size();
	    for( int i = 0; i != size; ++i ) {
                Entry* e = outputs.front();
                outputs.pop();
                ss << entryString( e );
                if( i != size-1 ) ss << ", ";
                delete e;
            }
            
            ss << "}";

            return ss.str();
        }

};


#endif // DICT_OUT_HPP
