/*!
 * @file   regLim.c
 * @brief  Converter Control Regulation library limit functions for field/current/voltage reference and field/current measurement
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
#include "libreg.h"

// Non-Real-Time Functions - do not call these from the real-time thread or interrupt

void regLimMeasInit(struct reg_lim_meas *lim_meas, float pos_lim, float neg_lim, float low_lim, float zero_lim)
{
    lim_meas->pos_trip        = pos_lim  * (1.0 + REG_LIM_TRIP);
    lim_meas->neg_trip        = neg_lim  * (1.0 + REG_LIM_TRIP);
    lim_meas->low             = low_lim;
    lim_meas->zero            = zero_lim;
    lim_meas->low_hysteresis  = low_lim  * (1.0 - REG_LIM_HYSTERESIS);
    lim_meas->zero_hysteresis = zero_lim * (1.0 - REG_LIM_HYSTERESIS);

    lim_meas->flags.trip = false;
    lim_meas->flags.low  = false;
    lim_meas->flags.zero = false;
}



void regLimRmsInit(struct reg_lim_rms *lim_rms, float rms_warning, float rms_fault, float rms_tc, double iter_period)
{
    if(rms_tc > 0.0)
    {
        lim_rms->meas2_filter_factor     = iter_period / rms_tc;
        lim_rms->rms2_fault              = rms_fault * rms_fault;
        lim_rms->rms2_warning            = rms_warning * rms_warning;
        lim_rms->rms2_warning_hysteresis = lim_rms->rms2_warning * (1.0 - 2.0 * REG_LIM_HYSTERESIS);
    }
    else
    {
        lim_rms->meas2_filter_factor = 0.0;
    }

    lim_rms->flags.fault   = false;
    lim_rms->flags.warning = false;
}



void regLimRefInit(struct reg_lim_ref *lim_ref, float pos_lim, float min_lim, float neg_lim,
                  float rate_lim, float acceleration_lim, float closeloop)
{
    // Keep raw limits as they are used by libcc

    lim_ref->min          = min_lim;
    lim_ref->pos          = pos_lim;
    lim_ref->neg          = neg_lim;
    lim_ref->rate         = rate_lim;
    lim_ref->acceleration = acceleration_lim;

    // Set clip limits by expanding the user limits

    lim_ref->rate_clip = rate_lim * (1.0 + REG_LIM_CLIP);
    lim_ref->max_clip  = pos_lim  * (1.0 + REG_LIM_CLIP);

    // Determine if reference is unipolar or bipolar

    if(neg_lim < 0.0)
    {
        lim_ref->flags.unipolar = false;
        lim_ref->min_clip       = neg_lim * (1.0 + REG_LIM_CLIP);
        lim_ref->closeloop      = -1.0E30;
    }
    else
    {
        lim_ref->flags.unipolar = true;
        lim_ref->min_clip       = 0.0;
        lim_ref->closeloop      = closeloop;
    }
}



void regLimVrefInit(struct reg_lim_ref *lim_v_ref, float pos_lim, float neg_lim, float rate_lim,
                    float acceleration_lim, float i_quadrants41[2], float v_quadrants41[2])
{
    float delta_i_quadrants41;

    // Keep pos limit as it is used by libcc for pre-function ramps

    lim_v_ref->min          = 0.0;
    lim_v_ref->pos          = pos_lim;
    lim_v_ref->rate         = rate_lim;
    lim_v_ref->acceleration = acceleration_lim;

    // Expand user clip limits

    lim_v_ref->rate_clip     = rate_lim * (1.0 + REG_LIM_CLIP);
    lim_v_ref->max_clip_user = pos_lim  * (1.0 + REG_LIM_CLIP);

    // Determine if converter is unipolar or bipolar in voltage

    if(neg_lim < 0.0)
    {
        lim_v_ref->flags.unipolar = false;
        lim_v_ref->min_clip_user  = neg_lim * (1.0 + REG_LIM_CLIP);
    }
    else
    {
        lim_v_ref->flags.unipolar = true;
        lim_v_ref->min_clip_user  = 0.0;
    }

    // Disable Q41 exclusion zone before changing to avoid real-time thread having inconsistent values

    lim_v_ref->i_quadrants41_max = -1.0E10;

    // Quadrants 41 exclusion zone: At least a 1A spread is needed to activate Q41 limiter

    delta_i_quadrants41 = i_quadrants41[1] - i_quadrants41[0];

    if(delta_i_quadrants41 >= 1.0)
    {
        lim_v_ref->dvdi = (v_quadrants41[1] - v_quadrants41[0]) / delta_i_quadrants41;
        lim_v_ref->v0   = (v_quadrants41[0] - lim_v_ref->dvdi * i_quadrants41[0]) * (1.0 + REG_LIM_CLIP);

        // Enable quadrants 41 exclusiong after setting v0 and dvdi

        lim_v_ref->i_quadrants41_max = i_quadrants41[1];
    }

    // Initialise Vref limits for zero current

    regLimVrefCalcRT(lim_v_ref, 0.0);
}



// Real-Time Functions

void regLimMeasRT(struct reg_lim_meas *lim_meas, float meas)
{
    float abs_meas = fabs(meas);

    // Invert measurement if limits are inverted

    if(lim_meas->invert_limits == REG_ENABLED)
    {
        meas = -meas;
    }

    // Trip level - negative limit is only active if less than zero

    if((meas > lim_meas->pos_trip) || (lim_meas->neg_trip < 0.0 && meas < lim_meas->neg_trip))
    {
        lim_meas->flags.trip = true;
    }
    else
    {
        lim_meas->flags.trip = false;
    }

    // Zero flag

    if(lim_meas->zero > 0.0)
    {
        if(lim_meas->flags.zero == REG_ENABLED)
        {
            if(abs_meas > lim_meas->zero)
            {
                lim_meas->flags.zero = false;
            }
        }
        else
        {
            if(abs_meas < lim_meas->zero_hysteresis)
            {
                lim_meas->flags.zero = true;
            }
        }
    }

    // Low flag

    if(lim_meas->low > 0.0)
    {
        if(lim_meas->flags.low)
        {
            if(abs_meas > lim_meas->low)
            {
                lim_meas->flags.low = false;
            }
        }
        else
        {
            if(abs_meas < lim_meas->low_hysteresis)
            {
                lim_meas->flags.low = true;
            }
        }
    }
}



void regLimMeasRmsRT(struct reg_lim_rms *lim_rms, float meas)
{
    if(lim_rms->meas2_filter_factor > 0.0)
    {
        // Use first order filter on measurement squared

        lim_rms->meas2_filter += (meas * meas - lim_rms->meas2_filter) * lim_rms->meas2_filter_factor;

        // Apply trip limit if defined

        if(lim_rms->rms2_fault > 0.0 && lim_rms->meas2_filter > lim_rms->rms2_fault)
        {
            lim_rms->flags.fault = true;
        }
        else
        {
            lim_rms->flags.fault = false;
        }

        // Apply warning limit if defined (with hysteresis)

        if(lim_rms->rms2_warning > 0.0)
        {
            if(lim_rms->flags.warning == false)
            {
                if(lim_rms->meas2_filter > lim_rms->rms2_warning)
                {
                    lim_rms->flags.warning = true;
                }
            }
            else
            {
                if(lim_rms->meas2_filter < lim_rms->rms2_warning_hysteresis)
                {
                    lim_rms->flags.warning = false;
                }
            }
        }
    }
}



void regLimVrefCalcRT(struct reg_lim_ref *lim_v_ref, float i_meas)
{
    float   v_lim;

    // Invert i_meas when limits are inverted

    if(lim_v_ref->invert_limits == REG_ENABLED)
    {
        i_meas = -i_meas;
    }

    // Calculate max positive voltage (Quadrants 41)

    lim_v_ref->max_clip = lim_v_ref->max_clip_user;

    if(i_meas < lim_v_ref->i_quadrants41_max)
    {
        v_lim = lim_v_ref->v0 + lim_v_ref->dvdi * i_meas;

        if(v_lim < 0.0)
        {
            v_lim = 0.0;
        }

        if(v_lim < lim_v_ref->max_clip)
        {
            lim_v_ref->max_clip = v_lim;
        }
    }

    // Calculate min negative voltage (Quadrants 32 uses the Q41 limits rotated by 180 degrees)

    lim_v_ref->min_clip = lim_v_ref->min_clip_user;

    if(i_meas > -lim_v_ref->i_quadrants41_max)
    {
        v_lim = -lim_v_ref->v0 + lim_v_ref->dvdi * i_meas;

        if(v_lim > 0.0)
        {
            v_lim = 0.0;
        }

        if(v_lim > lim_v_ref->min_clip)
        {
            lim_v_ref->min_clip = v_lim;
        }
    }
}



float regLimRefRT(struct reg_lim_ref *lim_ref, float period, float ref, float prev_ref)
/*! 
 * <h3>Implementation Notes</h3>
 *
 * On equipment where the rate limit is several orders of magnitude smaller than the
 * reference limit, it is possible that #REG_LIM_CLIP (margin on the rate limit,
 * usually 1 per mil) is too small compared to the relative precision of 32-bit floating-point
 * arithmetic (considered bounded by #REG_LIM_FP32_MARGIN = 2.0E-07 in this library).
 * A consequence of that was observed as a false positive on the rate clipping. That
 * can happen if:
 * 
 * #REG_LIM_CLIP \f$\times\f$ reg_lim_ref::rate_clip \f$\times\f$ period \f$\ll\f$ #REG_LIM_FP32_MARGIN \f$\times\f$ reg_lim_ref::max_clip
 * 
 * That is the reason why a margin equal to (#REG_LIM_FP32_MARGIN \f$\times\f$ prev_ref)
 * is kept on the rate clip limit in this function. In most cases it is insignificant,
 * but it will prevent the false positive in the rare cases mentioned above.
 */
{
    float   delta_ref;
    float   rate_lim_ref;
    bool    rate_lim_flag = false;

    // Clip reference to absolute limits taking into account the invert flag

    if(lim_ref->invert_limits == REG_DISABLED)
    {
        if(ref < lim_ref->min_clip)
        {
            ref = lim_ref->min_clip;
            lim_ref->flags.clip = true;
        }
        else if(ref > lim_ref->max_clip)
        {
            ref = lim_ref->max_clip;
            lim_ref->flags.clip = true;
        }
        else
        {
            lim_ref->flags.clip = false;
        }
    }
    else
    {
        if(ref > -lim_ref->min_clip)
        {
            ref = -lim_ref->min_clip;
            lim_ref->flags.clip = true;
        }
        else if(ref < -lim_ref->max_clip)
        {
            ref = -lim_ref->max_clip;
            lim_ref->flags.clip = true;
        }
        else
        {
            lim_ref->flags.clip = false;
        }
    }

    // Clip reference to rate of change limits if rate limit is non-zero

    if(lim_ref->rate_clip > 0.0)
    {
        delta_ref = ref - prev_ref; // Requested change in reference

        if(delta_ref > 0.0)         // If change is positive
        {
            rate_lim_ref = prev_ref * (1.0 + REG_LIM_FP32_MARGIN) + lim_ref->rate_clip * period;

            if(ref > rate_lim_ref)
            {
                ref = rate_lim_ref;
                rate_lim_flag = true;
            }
        }
        else if(delta_ref < 0.0)            // else if change is negative
        {
            rate_lim_ref = prev_ref * (1.0 - REG_LIM_FP32_MARGIN) - lim_ref->rate_clip * period;

            if(ref < rate_lim_ref)
            {
                ref = rate_lim_ref;
                rate_lim_flag = true;
            }
        }
    }

    lim_ref->flags.rate = rate_lim_flag;

    return(ref);
}

// EOF
