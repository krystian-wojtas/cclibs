/*!
 * @file  meas.h
 * @brief Converter Control Regulation library measurement-related functions
 *
 * Provides a basic two-stage cascaded box-car filter with extrapolation to
 * compensate for the delay.
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

#ifndef LIBREG_MEAS_H
#define LIBREG_MEAS_H

// Include header files

#include <stdint.h>

// Constants

#define REG_MEAS_RATE_BUF_MASK      3                            //!< Rate will use linear regression through 4 points

// Enum constants

/*!
 * Parameter value to select field measurement for regulation
 */
enum reg_meas_select
{
    REG_MEAS_UNFILTERED,                                         //!< Use hardware delay only
    REG_MEAS_FILTERED,                                           //!< Include FIR filter delay
    REG_MEAS_EXTRAPOLATED,                                       //!< Extrapolate delay
    REG_MEAS_NUM_SIGNALS                                         //!< Number of options in reg_meas_select
};

enum reg_meas_status
{
    REG_MEAS_SIGNAL_OK,
    REG_MEAS_SIGNAL_INVALID
};

// Measurement structures

struct reg_meas_signal
{
    float                 signal;                                //!< Measurement signal
    enum reg_meas_status  status;                                //!< Measurement signal status
};

/*!
 * Measurement filter parameters and variables
 */
struct reg_meas_filter
{
    uint32_t              enable;                                //!< Enable filter control

    uint32_t              extrapolation_len_iters;               //!< Extrapolation length (normally regulation period)
    uint32_t              extrapolation_index;                   //!< Index to oldest sample in extrapolation buffer

    uint32_t              fir_length[2];                         //!< FIR filter length for two cascaded stages
    uint32_t              fir_index[2];                          //!< Index to oldest sample in FIR buffers
    int32_t               fir_accumulator[2];                    //!< FIR filter accumulator for two cascaded stages

    int32_t              *fir_buf[2];                            //!< Pointers to buffers for two cascaded FIR stages
    float                *extrapolation_buf;                     //!< Pointer to buffer for extrapolation stage

    float                 max_meas_value;                        //!< Maximum value that can be filtered
    float                 float_to_integer;                      //!< Factor to convert unfiltered measurement to integer
    float                 integer_to_float;                      //!< Factor to converter integer to filtered measurement
    float                 extrapolation_factor;                  //!< Extrapolation factor

    enum reg_meas_select  reg_select;                            //!< Regulation measurement selector
    float                 delay_iters[REG_MEAS_NUM_SIGNALS];     //!< Delay for each signal in iterations. See also #REG_MEAS_NUM_SIGNALS
    float                 signal[REG_MEAS_NUM_SIGNALS];          //!< Array of measurement with different filtering. See also #REG_MEAS_NUM_SIGNALS
};

/*!
 * Measurement rate estimate structure
 */
struct reg_meas_rate
{
    uint32_t              iter_counter;                          //!< Iteration counter
    uint32_t              history_index;                         //!< Index of most recent sample in history buffer
    float                 history_buf[REG_MEAS_RATE_BUF_MASK+1]; //!< History buffer. See also #REG_MEAS_RATE_BUF_MASK
    float                 estimate;                              //!< Estimated rate using linear regression through 4 samples
};

/*!
 * Noise and tone generator structure
 */
struct reg_noise_and_tone
{
    uint32_t              iter_counter;                           //!< Iteration counter for simulated tone
    uint32_t              tone_half_period_iters;                 //!< Tone half period in iterations
    uint32_t              tone_toggle;                            //!< Tone toggle (0,1,0,1,...)
    float                 tone_amp;                               //!< Tone amplitude
    float                 noise_pp;                               //!< Simulated measurement peak-peak noise level
};

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Pass memory allocated for the measurement filter buffer into the filter data
 * structure. The buffer is used for both FIR filter stages and the extrapolation
 * history, so it must be long enough to cover all three requirements:
 * reg_meas_filter::fir_length[0] + reg_meas_filter::fir_length[1] + reg_meas_filter::extrapolation_len_iters
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    filter                     Measurement filter object. Buffer is allocated to reg_meas_filter::fir_buf
 * @param[in]     buf                        Pointer to buffer of sufficient size
 */
void regMeasFilterInitBuffer(struct reg_meas_filter *filter, int32_t *buf);

/*!
 * Initialise the FIR measurement filter.
 *
 * The measurement filter buffer must be allocated (using regMeasFilterInitBuffer())
 * before calling this function.
 *
 * Scale factors for the FIR filters are calculated from the positive/negative
 * limits for the measurement being filtered. This is necessary because the
 * filter must work with integers to avoid rounding errors.
 * 
 * The filter history buffers are initialised using reg_meas_filter::signal[#REG_MEAS_UNFILTERED],
 * then the filter will be restarted. This can be done in the background while
 * the filter is being called in real-time, but the filter will be bypassed
 * during the initialisation process, so expect a perturbation and potential
 * instability.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in,out] filter                     Measurement filter object to initialise
 * @param[in]     fir_length                 Two-dimensional array containing the lengths of the FIR filter stages
 * @param[in]     extrapolation_len_iters    Extrapolation buffer length (number of iterations). Normally equal to the regulation period.
 * @param[in]     pos                        Positive limit
 * @param[in]     neg                        Negative limit
 * @param[in]     meas_delay_iters           Delay in "unfiltered" measurement, i.e. the hardware filtering delay
 */
void regMeasFilterInit(struct reg_meas_filter *filter, uint32_t fir_length[2],
                       uint32_t extrapolation_len_iters, float pos, float neg, float meas_delay_iters);

/*!
 * Set the selector of the regulation measurement.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    filter                     Measurement filter object to update
 * @param[in]     reg_select                 Filter selector
 */
void regMeasSetRegSelect(struct reg_meas_filter *filter, enum reg_meas_select reg_select);

/*!
 * Set the noise and tone characteristics for a simulated measurement.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    noise_and_tone             Noise and tone object to update
 * @param[in]     noise_pp                   Simulated measurement peak-peak noise level
 * @param[in]     tone_amp                   Tone amplitude
 * @param[in]     tone_half_period_iters     Tone half period (in iterations)
 */
void regMeasSetNoiseAndTone(struct reg_noise_and_tone *noise_and_tone, float noise_pp,
                            float tone_amp, uint32_t tone_half_period_iters);

/*!
 * Filter the measurement with a two-stage cascaded box car filter and extrapolate
 * to estimate the measurement without the measurement and FIR filtering delays.
 * If the filter is not running then the output is simply the unfiltered input.
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in,out] filter                     Measurement filter object to update
 */
void regMeasFilterRT(struct reg_meas_filter *filter);

/*!
 * Generate a tone. The tone is simulated using the sum of white noise (generated
 * using a simple pseudo-random number generator) and a square wave. The frequency
 * of the tone is defined by reg_noise_and_tone::tone_half_period_iters.
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in,out] noise_and_tone             Noise and tone object (maintains state of pseudo-random number sequence)
 * @returns Sum of noise and tone values
 */
float regMeasNoiseAndToneRT(struct reg_noise_and_tone *noise_and_tone);

/*!
 * Calculate the estimated measurement rate by least-squares regression across
 * the last four saved values. The filtered measurement is stored in the rate
 * estimation history at the regulation period.
 *
 * This is a Real-Time function (thread safe).
 *
 * <h3>Linear Least Squares theory</h3>
 *
 * \f[
 * b=\frac{(\sum x)(\sum xy)-(\sum y)(\sum x^2)}{(\sum x)^2-n(\sum x^2)} \\
 * \f]
 * \f[
 * m=\frac{(\sum y)(\sum x)-n(\sum xy)}{(\sum x)^2-n(\sum x^2)}
 * \f]
 * \f$y = m \cdot x + b\f$ and \f$x = 2t\f$, therefore \f$y = 2m \cdot t + b\f$.
 *
 * If \f$x\f$ is symmetrical about zero, \f$\sum x = 0\f$, and therefore:
 * \f[
 * b = \frac{\sum y}{n}
 * \f]
 * \f[
 * 2m = \frac{2 \sum xy}{\sum x^2}
 * \f]
 *
 * @param[in,out] meas_rate                  Measurement rate estimate object to update
 * @param[in]     filtered_meas              Filtered measurement for the specified period
 * @param[in]     period                     Regulation period 
 * @param[in]     period_iters               Regulation period in iterations
 */
void regMeasRateRT(struct reg_meas_rate *meas_rate, float filtered_meas, float period, int32_t period_iters);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_MEAS_H

// EOF
