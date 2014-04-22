/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/ccInit.h                                                         Copyright CERN 2014

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

  Purpose:  Header file for ccInit.c

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCINIT_H
#define CCINIT_H

// Function declarations

uint32_t ccInitRun        (void);
uint32_t ccInitLoad       (void);
uint32_t ccInitFunctions  (void);
uint32_t ccInitLimits     (void);
uint32_t ccInitRegulation (void);
uint32_t ccInitSimulation (void);

#endif
// EOF
