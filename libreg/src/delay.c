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
void regDelayInitPars(struct reg_delay *delay, float *buf, float delay_in_iters, uint32_t undersampled_flag)
/*---------------------------------------------------------------------------------------------------------*\
  This function initialises the reg_delay structure parameters.  The buffer pointered to by buf must have
  a length of (int)delay_in_iters.  If delay_in_iters is less than 1 then buf can be NULL.
  If buf is NULL then the buffer pointer is preserved so once a buffer is linked the function can be called
  with NULL without losing it. The undersampled_flag can be used to suppress the linear interpolation
  between sampled.  When set it assumes that the signal settles to the new value immediately.
\*---------------------------------------------------------------------------------------------------------*/
{
    double      delay_int;		// double variable required for modf() function

    // Remember undersampled_flag

    delay->undersampled_flag = undersampled_flag;

    // Protect against negative delays

    if(delay_in_iters < 0.0)
    {
        delay_in_iters = 0.0;
        buf            = 0;
    }

    // Calculate delay integer and fractional parts

    delay->delay_frac = modf(delay_in_iters, &delay_int);

    delay->delay_int  = (int32_t)delay_int;

    // If signal is undersampled then assume it settles immediately

    if(undersampled_flag)
    {
        // If delay has a fractional part then round up delay to next integer

        if(delay->delay_frac > 0.0)
        {
            delay->delay_frac = 0.0;
            delay->delay_int++;
        }
    }

    // Connect the buffer if available - this will preserve an existing buffer pointer if NULL passed

    if(buf)
    {
        delay->buf = buf;
    }

    // Initialise the delay structure variables

    regDelayInitVars(delay, 0.0);
}
/*---------------------------------------------------------------------------------------------------------*/
void regDelayInitVars(struct reg_delay *delay, float initial_signal)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to initialise the reg_delay structure variables.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    i;

    delay->buf_index = 0;

    // Initialise history to value of supplied signal

    delay->prev_signal = initial_signal;

    for(i=0 ; i < delay->delay_int ; i++)
    {
        delay->buf[i] = initial_signal;
    }

    // Iteration counter starts negative to add extra periods before delayed signal is valid

    delay->iteration_counter = -2;
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t regDelayCalc(struct reg_delay *delay, float signal, float *delayed_signal)
/*---------------------------------------------------------------------------------------------------------*\
  This function should be called after the delay structure has been initialised.  It returns a
  status to indicate if the delayed_signal can be used.  This will be zero initialy (invalid) until
  the history buffer is full and then it becomes 1, at which point the delayed_signal can be used.
\*---------------------------------------------------------------------------------------------------------*/
{
    float interpolated_signal;

    // Interpolate using fractional delay

    interpolated_signal = signal + (delay->prev_signal - signal) * delay->delay_frac;
    delay->prev_signal  = signal;

    // Use circular buffer for the integer delay

    if(delay->delay_int > 0)
    {
        *delayed_signal = delay->buf[delay->buf_index];

        delay->buf[delay->buf_index] = interpolated_signal;

        if(++delay->buf_index >= delay->delay_int)
        {
            delay->buf_index = 0;
        }
    }
    else
    {
        *delayed_signal = interpolated_signal;
    }

    // Return validity flag - only valid once circular buffer is full

    if(delay->iteration_counter >= delay->delay_int)
    {
        return(1);                              // delayed_signal is now valid
    }

    delay->iteration_counter++;
    return(0);                          // delayed_signal is not yet valid
}
// EOF

