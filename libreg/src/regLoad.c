/*!
 * @file  regLoad.c
 * @brief Converter Control Regulation library Load-related functions
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
#include "libreg/load.h"



// Background functions - do not call these from the real-time thread or interrupt

void regLoadInit(struct reg_load_pars *load, float ohms_ser, float ohms_par, float ohms_mag, float henrys, float gauss_per_amp)
{
   // Save the load parameters

    load->ohms_ser      = ohms_ser;
    load->ohms_par      = ohms_par;
    load->ohms_mag      = ohms_mag;
    load->henrys        = henrys;
    load->gauss_per_amp = gauss_per_amp;

    // Calculate load related parameters

    if(ohms_par > 1.0E-10)     // Rp greater than zero (magnet included in circuit)
    {
        load->ohms1 = 1.0 + ohms_ser / ohms_par;
        load->ohms2 = 1.0 + ohms_mag / ohms_par;
        load->ohms  = ohms_mag + ohms_ser / load->ohms1;

        load->gain0 = 1.0 / (ohms_par * load->ohms1);
        load->gain2 = 1.0 / (ohms_ser + ohms_mag / load->ohms2);
        load->gain1 = load->gain2 - load->gain0;
        load->gain3 = 1.0 / load->ohms2;

        load->tc    = load->henrys / load->ohms;
    }
    else                       // Rp is effectively zero (i.e. no magnet in the circuit)
    {
        load->ohms1  = 1.0E30;
        load->ohms2  = 0.0;
        load->ohms   = ohms_ser;

        load->gain0  = 1.0 / ohms_ser;
        load->gain1  = 0.0;
        load->gain2  = load->gain0;
        load->gain3  = 0.0;

        load->tc     = 1.0E-20;
        load->henrys = 0.0;
    }

    // Clip inv_henrys to avoid infinity

    if(load->henrys > 1.0E-20)
    {
        load->inv_henrys  = 1.0 / load->henrys;
    }
    else
    {
        load->inv_henrys  = 1.0E+20;
    }

    // Disable saturation model by default

    load->sat.i_start = 1.0E30;
    load->sat.i_end   = 0.0;
}



void regLoadInitSat(struct reg_load_pars *load, float henrys_sat, float i_sat_start, float i_sat_end)
{
    if(load->henrys > 0.0 && henrys_sat > 0.0 && henrys_sat < load->henrys &&
       i_sat_end > 0.0 && i_sat_end > i_sat_start)
    {
        load->sat.henrys   = henrys_sat;
        load->sat.l_clip   = henrys_sat / load->henrys;
        load->sat.i_start  = i_sat_start;
        load->sat.i_end    = i_sat_end;
        load->sat.i_delta  = i_sat_end - i_sat_start;
        load->sat.l_rate   = (1.0 - load->sat.l_clip) / load->sat.i_delta;
        load->sat.b_end    = 0.5 * load->gauss_per_amp * (i_sat_start + i_sat_end +
                                                          load->sat.i_delta * load->sat.l_clip);
        load->sat.b_factor = 0.5 * (1.0 - load->sat.l_clip) / load->sat.i_delta;
    }
    else  // Disable saturation
    {
        load->sat.i_start = 1.0E30;
        load->sat.i_end   = 0.0;
    }
}



// Real-Time Functions

float regLoadCurrentToFieldRT(struct reg_load_pars *load, float i_meas)
{
    float b_meas;
    float abs_i_meas;
    float di_start;
    float di_end;

    // Field follows a linear - parabola - linear relationship with current due to magnet saturation

    abs_i_meas = fabs(i_meas);

    if(load->sat.i_end <= 0.0 || (di_start = abs_i_meas - load->sat.i_start) < 0.0)
    {
        // Linear

        b_meas = load->gauss_per_amp * abs_i_meas;
    }
    else if((di_end = abs_i_meas - load->sat.i_end) < 0.0)
    {
        // Parabolic

        b_meas = load->gauss_per_amp * (abs_i_meas - load->sat.b_factor * di_start * di_start);
    }
    else
    {
        // Linear

        b_meas = load->gauss_per_amp * load->sat.l_clip * di_end + load->sat.b_end;
    }

    // Return b_meas after adjusting sign to match i_meas

    return(i_meas < 0.0 ? -b_meas : b_meas);
}



float regLoadFieldToCurrentRT(struct reg_load_pars *load, float b_meas)
{
    float i_meas;
    float abs_b_meas;
    float b_sat_start;
    float db_end;
    float quad_a;
    float quad_b;
    float quad_c;

    // Field follows a linear - parabola - linear relationship with curent so this function inverts this
    // relationship to given current as a function of field.

    abs_b_meas = fabs(b_meas);

    b_sat_start = load->gauss_per_amp * load->sat.i_start;

    if(load->sat.i_end <= 0.0 || abs_b_meas <= b_sat_start)
    {
        // Linear

        i_meas = abs_b_meas / load->gauss_per_amp;
    }
    else if((db_end = abs_b_meas - load->sat.b_end) < 0.0)
    {
        // Quadratic: aI^2 + bI + c = 0  ->  I = (-b +/- sqrt(b^2 - 4ac)) / 2a

        quad_a = load->sat.b_factor;
        quad_b = -(2.0 * load->sat.b_factor * load->sat.i_start + 1.0);
        quad_c = load->sat.b_factor * load->sat.i_start * load->sat.i_start + abs_b_meas / load->gauss_per_amp;

        i_meas = (-quad_b - sqrtf(quad_b * quad_b - 4.0 * quad_a * quad_c)) / (2.0 * quad_a);
    }
    else
    {
        // Linear

        i_meas = db_end / (load->gauss_per_amp * load->sat.l_clip);
    }

    // Return i_meas after adjusting sign to match b_meas

    return(b_meas < 0.0 ? -i_meas : i_meas);
}



float regLoadVrefSatRT(struct reg_load_pars *load, float i_meas, float v_ref)
{
    float f = regLoadSatFactorRT(load, i_meas);

    return(f * v_ref + (1.0 - f) * i_meas * load->ohms);
}



float regLoadInverseVrefSatRT(struct reg_load_pars *load, float i_meas, float v_ref_sat)
{
    float f = regLoadSatFactorRT(load, i_meas);

    return((v_ref_sat - (1.0 - f) * i_meas * load->ohms) / f);
}



float regLoadSatFactorRT(struct reg_load_pars *load, float i_meas)
{
    float sat_factor   = 1.0;
    float delta_i_meas = fabs(i_meas) - load->sat.i_start;

    if(delta_i_meas > 0.0)
    {
        sat_factor = 1.0 - delta_i_meas * load->sat.l_rate;

        if(sat_factor < load->sat.l_clip)
        {
            sat_factor = load->sat.l_clip;
        }
    }

    return(sat_factor);
}

// EOF
