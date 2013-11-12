/*---------------------------------------------------------------------------------------------------------*\
  File:     flot.h                                                                      Copyright CERN 2014

  License:  This file is part of cctest.

            cctest is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  Header file for Converter Control library test program to generate html file
            using flot graphing tool

  Authors:  Quentin.King@cern.ch

  Notes:    This file is automatically assembed by the makefile from two header file fragments
            and three html file fragments processed using inc/flot/htmlquote.sh to add and escape
            quotes:

                flot_header.h, flot_footer.h
                flot_part1.html, flot_part2.html, flot_part3.html and flot_part4.html

            DO NOT EDIT flot.h - YOUR CHANGES WILL BE LOST WHEN YOU NEXT RUN MAKE!
\*---------------------------------------------------------------------------------------------------------*/

#ifndef FLOT_H
#define FLOT_H

#define MAX_FLOT_POINTS         100000           // Limit points per signal for FLOT output

static char *flot[] =
{
