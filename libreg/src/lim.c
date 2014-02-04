/*---------------------------------------------------------------------------------------------------------*\
  File:     lim.c                                                                       Copyright CERN 2014

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

  Purpose:  Limit functions for field/current/voltage reference and field/current measurement

  Notes:    Voltage reference limits for some 4-quadrant converters need to protect against excessive
            power losses in the output stage when ramping down the current.  This can be done by
            defining an exclusion zone for positive voltages in quadrants 4 and 1 and the software
            will rotate the zone by 180 degrees to define the exclusion zone for negative voltages
            in quadrants 3 and 2.  For example:
                                                    ^ Voltage
                                                    |
                                    +---------------+---------------+  <- V_POS
                                    |excl./         |               |
                                    |zone/          |               |
                                    |   /           |               |
                                    |  /            |               |
                                    | / Quadrant 4  |  Quadrant 1   |
                                    |/              |               |
                                    |               |               |
                                    +---------------+---------------+-> Current
                                    |               |               |
                                    |               |              /|
                                    |   Quadrant 3  |  Quadrant 2 / |
                                    |               |            /  |
                                    |               |           /   |
                                    |               |          /excl|
                                    |               |         / zone|
                                    +---------------+---------------+ <- V_NEG
                                  I_NEG                           I_POS

  Authors:  Quentin.King@cern.ch
            Pierre.Dejoue@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#include <math.h>
#include "libreg/lim.h"

/*---------------------------------------------------------------------------------------------------------*/
void regLimMeasInit(struct reg_lim_meas *lim_meas, float pos_lim, float neg_lim,
                    float low_lim, float zero_lim, uint32_t invert_limits)
/*---------------------------------------------------------------------------------------------------------*\
 This function will initialise the measurement limits based on the pos/neg, zero and low limits supplied.
\*---------------------------------------------------------------------------------------------------------*/
{
    lim_meas->invert_limits   = invert_limits;
    lim_meas->pos_trip        = pos_lim  * (1.0 + REG_LIM_TRIP);
    lim_meas->neg_trip        = neg_lim  * (1.0 + REG_LIM_TRIP);
    lim_meas->low             = low_lim;
    lim_meas->zero            = zero_lim;
    lim_meas->low_hysteresis  = low_lim  * (1.0 - REG_LIM_HYSTERESIS);
    lim_meas->zero_hysteresis = zero_lim * (1.0 - REG_LIM_HYSTERESIS);

    lim_meas->flags.trip = 0;
    lim_meas->flags.low  = 0;
    lim_meas->flags.zero = 0;
}
/*---------------------------------------------------------------------------------------------------------*/
void regLimMeas(struct reg_lim_meas *lim_meas, float meas)
/*---------------------------------------------------------------------------------------------------------*\
 This function will check the measurement against the trip levels and the absolute measurement against
 the low and zero limits with hysteresis to avoid toggling.
\*---------------------------------------------------------------------------------------------------------*/
{
    float abs_meas = fabs(meas);

    if(lim_meas->invert_limits == 0)
    {
        // Trip level - negative limit is only active if less than zero

        if((meas > lim_meas->pos_trip) || (lim_meas->neg_trip < 0.0 && meas < lim_meas->neg_trip))
        {
            lim_meas->flags.trip = 1;
        }
        else
        {
            lim_meas->flags.trip = 0;
        }
    }
    else    // Invert limits before use - this may be necessary if a polarity switch is in the negative position
    {
        // Trip level - negative limit is only active if less than zero

        if((meas < -lim_meas->pos_trip) || (lim_meas->neg_trip < 0.0 && meas > -lim_meas->neg_trip))
        {
            lim_meas->flags.trip = 1;
        }
        else
        {
            lim_meas->flags.trip = 0;
        }
    }
    
    // Zero flag

    if(lim_meas->flags.zero)
    {
        if(abs_meas > lim_meas->zero)
        {
            lim_meas->flags.zero = 0;
        }
    }
    else
    {
        if(abs_meas < lim_meas->zero_hysteresis)
        {
            lim_meas->flags.zero = 1;
        }
    }

    // Low flag

    if(lim_meas->flags.low)
    {
        if(abs_meas > lim_meas->low)
        {
            lim_meas->flags.low = 0;
        }
    }
    else
    {
        if(abs_meas < lim_meas->low_hysteresis)
        {
            lim_meas->flags.low = 1;
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void regLimRefInit(struct reg_lim_ref *lim_ref, float pos_lim, float neg_lim, float rate_lim, 
                   uint32_t invert_limits)
/*---------------------------------------------------------------------------------------------------------*\
 This function will initialise the field/current reference limits.
\*---------------------------------------------------------------------------------------------------------*/
{
    lim_ref->invert_limits = invert_limits;
    lim_ref->rate_clip     = rate_lim * (1.0 + REG_LIM_CLIP);
    lim_ref->max_clip      = pos_lim  * (1.0 + REG_LIM_CLIP);

    // Determine if converter is unipolar or bipolar

    if(neg_lim < 0.0)
    {
        lim_ref->flags.unipolar = 0;
        lim_ref->min_clip = neg_lim * (1.0 + REG_LIM_CLIP);
    }
    else
    {
        lim_ref->flags.unipolar = 1;
        lim_ref->min_clip       = 0.0;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void regLimVrefInit(struct reg_lim_ref *lim_v_ref, float pos_lim, float neg_lim, float rate_lim,
                    float i_quadrants41[2], float v_quadrants41[2], uint32_t invert_limits)
/*---------------------------------------------------------------------------------------------------------*\
 This function will initialise the voltage reference limits.  Voltage reference limits use the same
 structure as field/current limits but have different behaviour.
\*---------------------------------------------------------------------------------------------------------*/
{
    float delta_i_quadrants41;

    lim_v_ref->invert_limits  = invert_limits;
    lim_v_ref->flags.unipolar = (neg_lim > REG_LIM_V_DIODE);    // Set unipolar flag allowing diode voltage
    lim_v_ref->rate_clip      = rate_lim * (1.0 + REG_LIM_CLIP);

    // Set max/min user clip limits
    
    lim_v_ref->max_clip_user = pos_lim * (1.0 + REG_LIM_CLIP);
    lim_v_ref->min_clip_user = neg_lim * (1.0 + REG_LIM_CLIP);
    
    // Quadrants 41 exclusion zone: At least a 1A spread is needed to activate Q41 limiter

    delta_i_quadrants41 = i_quadrants41[1] - i_quadrants41[0];

    if(delta_i_quadrants41 >= 1.0)
    {
        lim_v_ref->i_quadrants41_max = i_quadrants41[1];
        lim_v_ref->dvdi = (v_quadrants41[1] - v_quadrants41[0]) / delta_i_quadrants41;
        lim_v_ref->v0   = (v_quadrants41[0] - lim_v_ref->dvdi * i_quadrants41[0]) * (1.0 + REG_LIM_CLIP);
    }
    else
    {
        lim_v_ref->i_quadrants41_max = -1.0E10;         // Disable Q41 exclusion zone
    }

    // Initialise Vref limits for zero current

    regLimVrefCalc(lim_v_ref, 0.0);
}
/*---------------------------------------------------------------------------------------------------------*/
void regLimVrefCalc(struct reg_lim_ref *lim_v_ref, float i_meas)
/*---------------------------------------------------------------------------------------------------------*\
  This function will use the measured current to work out the voltage limits based on the operating
  zone for the voltage source.  The user defines the exclusion zone for positive voltages in quadrants 41
  and the function rotates the zone to calculate the exclusion zone for negative voltages in quadrants 32.
\*---------------------------------------------------------------------------------------------------------*/
{
    float   v_lim;

    // Invert i_meas when limits are inverted

    if(lim_v_ref->invert_limits != 0)
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
/*---------------------------------------------------------------------------------------------------------*/
float regLimRef(struct reg_lim_ref *lim_ref, float period, float ref, float prev_ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function applies clip and rate limits to the field, current or voltage reference.

  Implementation notes:
    On equipments where the rate limit is several orders of magnitude smaller than the reference limit, it
    is possible that the margin on the rate limit (REG_LIM_CLIP, usually 1 per mil) is too small compared to
    the relative precision of 32-bit floating-points (considered bounded by REG_LIM_FP32_MARGIN, that
    is 2.0E-07 in this library). A consequence of that was observed as a false positive on the rate
    clipping. That can happen if:

       REG_LIM_CLIP * rate_lim * period << LIM_REG_FP32_MARGIN * lim_ref->max_clip

    That is the reason why a margin equal to (LIM_REG_FP32_MARGIN * ref) is kept on the rate clip limit in
    this function. In most cases it is insignificant, but it will prevent the false positive in the rare
    cases mentioned above.
\*---------------------------------------------------------------------------------------------------------*/
{
    float       delta_ref;
    float       rate_lim_ref;
    uint32_t    rate_lim_flag = 0;

    // Clip reference to absolute limits taking into account the invert flag

    if(lim_ref->invert_limits == 0)
    {
        if(ref < lim_ref->min_clip)
        {
            ref = lim_ref->min_clip;
            lim_ref->flags.clip = 1;
        }
        else if(ref > lim_ref->max_clip)
        {
            ref = lim_ref->max_clip;
            lim_ref->flags.clip = 1;
        }
        else
        {
            lim_ref->flags.clip = 0;
        }
    }
    else
    {
        if(ref > -lim_ref->min_clip)
        {
            ref = -lim_ref->min_clip;
            lim_ref->flags.clip = 1;
        }
        else if(ref < -lim_ref->max_clip)
        {
            ref = -lim_ref->max_clip;
            lim_ref->flags.clip = 1;
        }
        else
        {
            lim_ref->flags.clip = 0;
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
                rate_lim_flag = 1;
            }
        }
        else if(delta_ref < 0.0)            // else if change is negative
        {
            rate_lim_ref = prev_ref * (1.0 - REG_LIM_FP32_MARGIN) - lim_ref->rate_clip * period;

            if(ref < rate_lim_ref)
            {
                ref = rate_lim_ref;
                rate_lim_flag = 1;
            }
        }
    }

    lim_ref->flags.rate = rate_lim_flag;

    return(ref);
}
// EOF

