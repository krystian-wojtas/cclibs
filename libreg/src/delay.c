/*---------------------------------------------------------------------------------------------------------*\
  File:     delay.c                                                                     Copyright CERN 2014

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

  Purpose:  Signal delay functions

            These functions use a circular buffer and linear interpolation to provide a programmable
            delay line for signals.  It is used by the regulation error calculation functions and
            can be used to simulate measurement filter delays etc.
\*---------------------------------------------------------------------------------------------------------*/

#include <math.h>
#include "libreg/delay.h"

/*---------------------------------------------------------------------------------------------------------*/
void regDelayInitDelay(struct reg_delay *delay, float delay_iters)
/*---------------------------------------------------------------------------------------------------------*\
  This function initialises the reg_delay structure parameters.  The undersampled_flag can be used to
  suppress the linear interpolation between samples.  When set it assumes that the signal settles to
  the new value immediately.
\*---------------------------------------------------------------------------------------------------------*/
{
    float delay_int;

    // Clip delay

    if(delay_iters < 0.0)
    {
        delay_iters = 0.0;
    }
    else if(delay_iters > (REG_DELAY_BUF_INDEX_MASK - 1.0))
    {
        delay_iters = REG_DELAY_BUF_INDEX_MASK - 1.0;
    }

    // Calculate integer and fractional parts of the delay in iterations

    delay->delay_frac = modff(delay_iters, &delay_int);
    delay->delay_int  = (int32_t)delay_int;
}
/*---------------------------------------------------------------------------------------------------------*/
void regDelayInitVars(struct reg_delay *delay, float initial_signal)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to initialise the reg_delay structure variables.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    i;

    for(i=0 ; i <= REG_DELAY_BUF_INDEX_MASK ; i++)
    {
        delay->buf[i] = initial_signal;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
float regDelayCalc(struct reg_delay *delay, float signal, uint32_t under_sampled_flag)
/*---------------------------------------------------------------------------------------------------------*\
  This function should be called after the delay structure has been initialised.
\*---------------------------------------------------------------------------------------------------------*/
{
    float s0;
    float s1;

    delay->buf[++delay->buf_index & REG_DELAY_BUF_INDEX_MASK] = signal;

    s0 = delay->buf[(delay->buf_index - delay->delay_int    ) & REG_DELAY_BUF_INDEX_MASK];
    s1 = delay->buf[(delay->buf_index - delay->delay_int - 1) & REG_DELAY_BUF_INDEX_MASK];

    if(under_sampled_flag == 0)
    {
        return(s0 + delay->delay_frac * (s1 - s0));
    }

    // When under-sampled, jump to final value at the start of each period

    return(s0);
}
// EOF

