/*---------------------------------------------------------------------------------------------------------*\
  File:     libcc/ref.h                                                               Copyright CERN 2014

  License:  This file is part of libcc.

            libcc is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  Control Control reference functions library header file

  Contact:  cclibs-devs@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBCC_REF_H
#define LIBCC_REF_H

// Include header files

#include <stdint.h>

// Global power referter cculation structures

struct cc_ref_sim_meas                                ///< Measurement simulation structure
{
    float                       signal;                 ///< Simulated measured signal with noise and tone
};

// Referter control functions

#ifdef __cplusplus
extern "C" {
#endif

float    ccRefPureDelay       (uint32_t cc_period_iters);

#ifdef __cplusplus
}
#endif

#endif // LIBCC_REF_H
// EOF

