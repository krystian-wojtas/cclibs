/*!
 * @file      libfg.h
 * @brief     Function Generation library top-level header file
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
 * This file is part of libfg.
 *
 * libfg is free software: you can redistribute it and/or modify it under the
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

#ifndef LIBFG_H
#define LIBFG_H

#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>                         // For NULL

// Constants

#define FG_CLIP_LIMIT_FACTOR    0.001       //!< Scale factor for user limits
#define FG_ERR_DATA_LEN         4           //!< meta::error.data array length

/*!
 * FG Library error numbers
 */
enum fg_error
{
    FG_OK,
    FG_BAD_ARRAY_LEN,
    FG_BAD_PARAMETER,
    FG_INVALID_TIME,
    FG_OUT_OF_LIMITS,
    FG_OUT_OF_RATE_LIMITS,
    FG_OUT_OF_ACCELERATION_LIMITS,
};

/*!
 * Polarity of limits
 */
enum fg_limits_polarity
{
    FG_LIMITS_POL_NORMAL,                   //!< Normal limits with no manipulation
    FG_LIMITS_POL_NEGATIVE,                 //!< Limits should be inverted
    FG_LIMITS_POL_AUTO                      //!< Limits should be tested based upon the polarity of the reference
};

/*!
 * Diagnostic meta data structure
 */
struct fg_meta
{
    enum fg_error   fg_error;               //!< Function error number
    bool            invert_limits;          //!< Limits inverted flag

    struct
    {
        uint32_t    index;                  //!< Error index from Init function
        float       data[FG_ERR_DATA_LEN];  //!< Error debug data
    } error;                                //!< Used to indicate why the reference function was rejected

    float           duration;               //!< Function duration (not including delay)

    struct
    {
        float       start;                  //!< Reference value at start of function
        float       end;                    //!< Reference value at the end of the function
        float       min;                    //!< Minimum value of the function
        float       max;                    //!< Maximum value of the function
    } range;
};

/*!
 * Reference limits structure
 */
struct fg_limits
{
    float           pos;                    //!< Positive reference limit
    float           min;                    //!< Minimum absolute reference limit
    float           neg;                    //!< Negative reference limit
    float           rate;                   //!< Rate of change limit
    float           acceleration;           //!< Acceleration limit
};

// External functions

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Reset all the meta data fields.
 *
 * When a function is initialized, a meta data structure is filled with a summary of the function's
 * characteristics, including the duration, min/max and start and end values. Normally, the
 * <em>meta</em> parameter is received from the calling application and it may be NULL. The libfg
 * Init function passes a pointer to transient meta structure on the stack in <em>local_meta</em>
 * and this is used when the application's pointer is NULL. This simplifies the code in libfg because
 * a meta structure is always available.
 *
 * @param[out] meta       Pointer to fg_meta data structure.
 * @param[out] local_meta Pointer to fg_meta data structure to use if <em>meta</em> is NULL.
 * @param[in]  init_ref   Initial value for the start, min and max.
 *
 * @return Pointer to the fg_meta structure that was reset (<em>meta</em> or <em>local_meta</em> if meta is NULL).
 */
struct fg_meta *fgResetMeta(struct fg_meta *meta, struct fg_meta *local_meta, float init_ref);

/*!
 * Set the meta min and max fields
 *
 * @param[out] meta      Pointer to fg_meta structure.
 * @param[in]  ref       Value for reference to check against meta min and max.
 */
void fgSetMinMax(struct fg_meta *meta, float ref);

/*!
 * Check function reference value, rate and acceleration against the supplied limits.
 *
 * When a function is initialized, this is called to check the function value,
 * rate and acceleration (in that order) against the supplied limits. The function
 * returns the status of the first limit that is exceeded and the return value
 * indicates the type of error (REF, RATE, ACCELERATION). The meta::error.data array
 * provides extra information about limit:
 * <PRE>
 * |RETURN VALUE                 |ERROR.DATA[0]|ERROR.DATA[1]|ERROR.DATA[2]       |
 * |-----------------------------|-------------|-------------|--------------------|
 * |FG_OUT_OF_LIMITS             |ref          |min          |max                 |
 * |FG_OUT_OF_RATE_LIMITS        |rate         |rate_limit*  |limits->rate        |       
 * |FG_OUT_OF_ACCELERATION_LIMITS|acceleration |acc_limit**  |limits->acceleration|
 * </PRE>
 *
 * rate_limit = (1.0 + FG_CLIP_LIMIT_FACTOR) * limits::rate
 * acc_limit  = (1.0 + FG_CLIP_LIMIT_FACTOR) * limits::acceleration
 *
 * @param[in]  limits           Pointer to fg_limits structure.
 * @param[in]  limits_polarity  Control of limits inversion.
 *                              Important for unipolar converters with a polarity switch.
 * @param[in]  ref              Reference value.
 * @param[in]  rate             Rate of change of reference.
 * @param[in]  acceleration     Acceleration of reference.
 * @param[out] meta             Pointer to fg_meta structure used to return detailed error codes.
 *
 * @retval FG_OK on success.
 * @retval FG_OUT_OF_LIMITS if function value is out of limits.
 * @retval FG_OUT_OF_RATE_LIMITS if function rate of change is out of limits.
 * @retval FG_OUT_OF_ACCELERATION_LIMITS if function acceleration is out of limits.
 */
enum fg_error fgCheckRef(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                         float ref, float rate, float acceleration, struct fg_meta *meta);

#ifdef __cplusplus
}
#endif

#endif

// EOF
