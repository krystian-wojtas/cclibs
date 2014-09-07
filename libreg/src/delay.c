/*!
 * @file  delay.c
 * @brief Converter Control Regulation library signal delay functions
 *
 * <h2>Copyright</h2>
 *
 * Copyright CERN 2014. This project is released under the GNU Lesser General
 * Public License version 3.
 * 
 * <h2>License</h2>
 *
 * This file is part of libreg.
 *
 * libreg is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include "libreg/delay.h"

// Non-Real-Time Functions: do not call these from the real-time thread or interrupt

void regDelayInitDelay(struct reg_delay *delay, float delay_iters)
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

void regDelayInitVars(struct reg_delay *delay, float initial_signal)
{
    uint32_t    i;

    for(i=0 ; i <= REG_DELAY_BUF_INDEX_MASK ; i++)
    {
        delay->buf[i] = initial_signal;
    }
}

// Real-Time Functions

float regDelaySignalRT(struct reg_delay *delay, float signal, uint32_t under_sampled_flag)
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
