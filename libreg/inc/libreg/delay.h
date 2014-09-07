/*!
 * @file  delay.h
 * @brief Converter Control Regulation library signal delay functions
 *
 * These functions use a circular buffer and linear interpolation to provide a programmable
 * delay line for signals. It is used by the regulation error calculation functions and
 * can be used to simulate measurement filter delays <em>etc.</em>
 *
 * <h2>Contact</h2>
 *
 * cclibs-devs@cern.ch
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

#ifndef LIBREG_DELAY_H
#define LIBREG_DELAY_H

#include <stdint.h>

/*!
 * Size of circular buffer. Specified as a mask, so value must be of the form \f$2^n-1\f$.
 */
#define REG_DELAY_BUF_INDEX_MASK    31

/*!
 * Signal delay structure
 */
struct reg_delay
{
    int32_t                     buf_index;                         //!< Index into circular buffer
    float                       buf[REG_DELAY_BUF_INDEX_MASK+1];   //!< Circular buffer for signal. See also #REG_DELAY_BUF_INDEX_MASK
    int32_t                     delay_int;                         //!< Integer delays in iteration periods
    float                       delay_frac;                        //!< Fractional delays in iteration periods
};

// Signal delay functions

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Initialise reg_delay structure delay
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    delay                 data struct to initialise
 * @param[in]     delay_iters           no. of iterations to delay. Can be fractional.
 *                                      Value is clipped to the range (0,#REG_DELAY_BUF_INDEX_MASK-1.0)
 */
void regDelayInitDelay(struct reg_delay *delay, float delay_iters);

/*!
 * Initialise reg_delay structure buffer
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    delay                 data struct to initialise
 * @param[in]     initial_signal        Value to assign to all elements of reg_delay::buf
 */
void regDelayInitVars(struct reg_delay *delay, float initial_signal);

/*!
 * Assign signal value to next slot in reg_delay::buf
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in,out] delay                 delay struct must be initialised before calling this function
 * @param[in]     signal                Value to assign to next slot in reg_delay::buf
 * @param[in]     under_sampled_flag    If set (non-zero), suppress the linear
 *                                      interpolation between samples; assume
 *                                      that the signal settles to the new value
 *                                      immediately.
 * @returns Signal value after delay
 */
float regDelaySignalRT(struct reg_delay *delay, float signal, uint32_t under_sampled_flag);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_DELAY_H

// EOF
