/*
 * ===========================================================================
 *
 *      Filename:  textcolor.h
 *
 *      Description:  Header File textcolor Module. 
 *                    Allows toggling of terminal escape sequences for 
 *                    colored text.
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

#ifndef TEXTCOLOR_H
#define TEXTCOLOR_H

#define RESET     0
#define BRIGHT    1
#define DIM       2
#define UNDERLINE 3
#define BLINK     4
#define REVERSE   7
#define HIDDEN    8

#define BLACK     0
#define RED       1
#define GREEN     2
#define YELLOW    3
#define BLUE      4
#define MAGENTA   5
#define CYAN      6
#define WHITE     7

static void color_on(int attr, int fg);
static void color_reset(void);

static void color_on(int attr, int fg)
{
    char command[13];

    sprintf(command, "%c[%d;%dm", 0x1B, attr, fg + 30);
    printf("%s", command);
}

static void color_reset()
{
    char command[13];

    sprintf(command, "%c[%dm", 0x1B, 0);
    printf("%s", command);

}

#endif /*TEXTCOLOR_H*/
