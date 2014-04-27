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
static void regDelayCalcDelay(struct reg_delay_pars *delay, uint32_t under_sampled_flag, float delay_iters)
/*---------------------------------------------------------------------------------------------------------*\
  This function converts the delay in iterations into integer and fractional parts, taking into account
  the under_sampled flag. When the signal is under sampled, it is assumed to jump to the final value
  for an iteration immediately - so the delay is round up to the next integer value.
\*---------------------------------------------------------------------------------------------------------*/
{
    float delay_int;      // double variable required for modf() function

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

    // If signal is under sampled then assume it settles immediately

    if(under_sampled_flag)
    {
        // If delay has a fractional part then round down delay

        delay->delay_frac = 0.0;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void regDelayInitDelays(struct reg_delay *delay, uint32_t under_sampled_flag,
                        float delay_1_iters, float delay_2_iters)
/*---------------------------------------------------------------------------------------------------------*\
  This function initialises the reg_delay structure parameters.  The undersampled_flag can be used to
  suppress the linear interpolation between samples.  When set it assumes that the signal settles to
  the new value immediately.
\*---------------------------------------------------------------------------------------------------------*/
{
    regDelayCalcDelay(&delay->delay_1, under_sampled_flag, delay_1_iters);
    regDelayCalcDelay(&delay->delay_2, under_sampled_flag, delay_2_iters);
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
static void regDelayCalcSignal(struct reg_delay *delay, struct reg_delay_pars *pars, float *delayed_signal)
/*---------------------------------------------------------------------------------------------------------*\
  This function calculates the delayed signal value.
\*---------------------------------------------------------------------------------------------------------*/
{
    float s0 = delay->buf[(delay->buf_index - pars->delay_int    ) & REG_DELAY_BUF_INDEX_MASK];
    float s1 = delay->buf[(delay->buf_index - pars->delay_int - 1) & REG_DELAY_BUF_INDEX_MASK];

    *delayed_signal = s0 + pars->delay_frac * (s1 - s0);
}
/*---------------------------------------------------------------------------------------------------------*/
void regDelayCalc(struct reg_delay *delay, float signal, float *delayed_signal_1, float *delayed_signal_2)
/*---------------------------------------------------------------------------------------------------------*\
  This function should be called after the delay structure has been initialised.  It returns a
  status to indicate if the delayed_signal can be used.  This will be zero initially (invalid) until
  the history buffer is full and then it becomes 1, at which point the delayed_signal can be used.
\*---------------------------------------------------------------------------------------------------------*/
{
    delay->buf[++delay->buf_index & REG_DELAY_BUF_INDEX_MASK] = signal;

    regDelayCalcSignal(delay, &delay->delay_1, delayed_signal_1);
    regDelayCalcSignal(delay, &delay->delay_2, delayed_signal_2);
}
// EOF

