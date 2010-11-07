/*
 * ===========================================================================
 *
 *      Filename:  asciiTable.h
 *
 *      Description:  Module to create and print a ascii table
 *
 *      Version:  2.0
 *      Created:  12.10.2010
 *
 *      Author:  Jan Treibig (jt), jan.treibig@gmail.com
 *      Company:  RRZE Erlangen
 *      Project:  likwid
 *      Copyright:  Copyright (c) 2010, Jan Treibig
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License, v2, as
 *      published by the Free Software Foundation
 *     
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *     
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * ===========================================================================
 */

#ifndef ASCIITABLE_H
#define ASCIITABLE_H
#include <types.h>
#include <bstrlib.h>

extern TableContainer* asciiTable_allocate(int numRows,int numColumns, bstrList* headerLabels);
extern void asciiTable_free(TableContainer* container);
extern void asciiTable_insertRow(TableContainer* container, int row,  bstrList* fields);
extern void asciiTable_appendRow(TableContainer* container, bstrList* fields);
extern void asciiTable_setCurrentRow(TableContainer* container, int row);
extern void asciiTable_print(TableContainer* container);

#endif /*ASCIITABLE_H*/
