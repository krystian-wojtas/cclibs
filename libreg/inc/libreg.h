/*---------------------------------------------------------------------------------------------------------*\
  File:     libreg.h                                                                    Copyright CERN 2014

  License:  This file is part of libreg.

            libreg is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  Converter Control Regulation library header file

  Contact:  cclibs-devs@cern.ch

  Notes:    The regulation library provides support for:

                1. Field, Current and voltage limits
                2. RST based regulation (Landau notation)
                3. Regulation error calculation
                4. Voltage source simulation
                5. Magnet load definition and simulation

\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBREG_H
#define LIBREG_H

// Include header files

#include <stdint.h>
#include <libreg/delay.h>
#include <libreg/err.h>
#include <libreg/lim.h>
#include <libreg/load.h>
#include <libreg/meas.h>
#include <libreg/rst.h>
#include <libreg/sim.h>
#include <libreg/conv.h>

#endif // LIBREG_H
// EOF

