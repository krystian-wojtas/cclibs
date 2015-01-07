/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/ccFlot.h                                                         Copyright CERN 2014

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

  Purpose:  Header file for cctest program flot chart webpage creation functions
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCFLOT_H
#define CCFLOT_H

#include <stdint.h>

#include "ccPars.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCFLOT_EXT
#else
#define CCFLOT_EXT extern
#endif

// Constants

#define FLOT_PATH               "../.."

// Globals

CCFLOT_EXT uint32_t flot_index;      // Index into flot buffers

// Function declarations

void     ccFlot              (FILE *f, char *filename);

#endif

// EOF
