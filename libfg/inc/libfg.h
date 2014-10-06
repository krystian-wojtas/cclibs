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
#include <stdlib.h>     // For NULL

// Constants

#define FG_CLIP_LIMIT_FACTOR    0.001
#define FG_PI                   3.141592653589793238462643383279
#define FG_ERR_DATA_LEN         4

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
    FG_LIMITS_POL_NORMAL,                  //!< Normal limits with no manipulation
    FG_LIMITS_POL_NEGATIVE,                //!< Limits should be inverted
    FG_LIMITS_POL_AUTO                     //!< Limits should be tested based upon the polarity of the reference
};

/*!
 * Diagnostic metadata structure
 */
struct fg_meta
{
    struct
    {
        uint32_t    index;                 //!< Error index from Init function
        float       data[FG_ERR_DATA_LEN]; //!< Error debug data
    } error;                               //!< Used to indicate why the reference function was rejected

    float       duration;                  //!< Function duration

    struct
    {
        float   start;                     //!< Reference value at start of function
        float   end;                       //!< Reference value at the end of the function
        float   min;                       //!< Minimum value of the function
        float   max;                       //!< Maximum value of the function
    } range;
};

/*!
 * Reference limits structure
 */
struct fg_limits
{
    float       pos;                       //!< Positive reference limit
    float       min;                       //!< Minimum absolute reference limit
    float       neg;                       //!< Negative reference limit
    float       rate;                      //!< Rate of change limit
    float       acceleration;              //!< Acceleration limit
};

// External functions

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Reset all the metadata fields.
 *
 * When a function is initialised, a meta structure is filled with a summary of the function including the
 * min/max and start and end values.
 *
 * @param[out] meta       Reference metadata to reset
 * @param[out] local_meta Reference metadata to reset if <em>meta</em> is NULL
 * @param[in]  init_ref   Value for reference start, min and max
 *
 * @return Pointer to the structure that was reset
 */
struct fg_meta *fgResetMeta(struct fg_meta *meta, struct fg_meta *local_meta, float init_ref);

/*!
 * Set the meta min and max fields
 *
 * @param[out] meta Reference metadata to set
 * @param[in]  ref  Value for reference min and max
 */
void fgSetMinMax(struct fg_meta *meta, float ref);

/*!
 * Check function value, rate and acceleration against the supplied limits.
 *
 * When a function is initialised, this is called to check the function value,
 * rate and acceleration against the supplied limits. It will also call a user
 * supplied call back (if supplied) to perform further checks.
 *
 * @param[in]  limits          Limits to check against
 * @param[in]  limits_polarity Invert limits? Only required for unipolar converters
 * @param[in]  ref             Reference level
 * @param[in]  rate            Rate of change
 * @param[in]  acceleration    Acceleration
 * @param[out] meta            Reference metadata structure used to return detailed error codes
 *
 * @retval FG_OK on success
 * @retval FG_OUT_OF_LIMITS if function value is out of limits
 * @retval FG_OUT_OF_RATE_LIMITS if function rate of change is out of limits
 * @retval FG_OUT_OF_ACCELERATION_LIMITS if function acceleration is out of limits
 * @retval other the return value of fg_limits::user_check_limits()
 */
enum fg_error fgCheckRef(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                         float ref, float rate, float acceleration, struct fg_meta *meta);

#ifdef __cplusplus
}
#endif

#endif

// EOF
