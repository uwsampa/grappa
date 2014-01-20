////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
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
