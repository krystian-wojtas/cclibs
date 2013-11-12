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

// Signal delay structure

struct reg_delay
{
    uint32_t                    undersampled_flag;              // Signal is undersampled flag
    int32_t                     iteration_counter;              // Iteration counter
    int32_t                     buf_index;                      // Index into circular buffer
    int32_t                     delay_int;                      // Integer delay in iteration periods
    float                       delay_frac;                     // Fractional delay in iteration periods
    float                       prev_signal;                    // Signal from previous iteration
    float                       *buf;                           // Pointer to circular buffer for signal
};

// Signal delay functions

#ifdef __cplusplus
extern "C" {
#endif

void     regDelayInitPars       (struct reg_delay *delay, float *buf, float delay_in_iters, uint32_t undersampled_flag);
void     regDelayInitVars       (struct reg_delay *delay, float initial_signal);
uint32_t regDelayCalc           (struct reg_delay *delay, float signal, float *delayed_signal);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_DELAY_H

// EOF
