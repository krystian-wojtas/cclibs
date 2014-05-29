/*---------------------------------------------------------------------------------------------------------*\
  File:     libreg/delay.h                                                              Copyright CERN 2014

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

  Purpose:  Converter Control Regulation library delay functions header file

  Contact:  cclibs-devs@cern.ch

\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBREG_DELAY_H
#define LIBREG_DELAY_H

#include <stdint.h>

#define REG_DELAY_BUF_INDEX_MASK    31

// Signal delay structure

struct reg_delay
{
    int32_t                     buf_index;                         ///< Index into circular buffer
    float                       buf[REG_DELAY_BUF_INDEX_MASK+1];   ///< Circular buffer for signal
    int32_t                     delay_int;                         ///< Integer delays in iteration periods
    float                       delay_frac;                        ///< Fractional delays in iteration periods
};

// Signal delay functions

#ifdef __cplusplus
extern "C" {
#endif

void    regDelayInitDelay   (struct reg_delay *delay, float delay_iters);
void    regDelayInitVars    (struct reg_delay *delay, float initial_signal);
float   regDelayCalc        (struct reg_delay *delay, float signal, uint32_t under_sampled_flag);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_DELAY_H

// EOF
