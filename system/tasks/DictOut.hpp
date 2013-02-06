// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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
