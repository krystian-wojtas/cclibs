/*---------------------------------------------------------------------------------------------------------*\
  File:     load.c                                                                      Copyright CERN 2011

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

  Purpose:  Load related functions

  Authors:  Quentin King
            Martin Veenstra
            Hugues Thiesen

  Notes:    The load model is shown in this figure:

                               O--[OHMS_SER]---+--[OHMS_MAG]--[HENRYS]--+--O
                                               |                        |
                                               +-------[OHMS_PAR]-------+

            A simple linear magnet saturation model is supported:

                                            L
                                            ^
                                            |
                                   HENRYS ->|-----------
                                            |           \
                                            |            \
                                            |             \
                                            |              \
                                            |               \
                                            |                --- <- HENRYS_SAT
                                            |
                                            |
                                           0+-----------+----+--> I
                                            0           ^    ^
                                                        |    |
                                                        |I_SAT_END
                                                        |
                                                   I_SAT_START
\*---------------------------------------------------------------------------------------------------------*/

#include <math.h>
#include "libreg/load.h"

/*---------------------------------------------------------------------------------------------------------*/
void regLoadInit(struct reg_load_pars *load, float ohms_ser, float ohms_par, float ohms_mag, float henrys,
                 float gauss_per_amp)
/*---------------------------------------------------------------------------------------------------------*\
  This function will initialise the load structure with the specified load parameters
\*---------------------------------------------------------------------------------------------------------*/
{
   // Save the load parameters

    load->ohms_ser  = ohms_ser;
    load->ohms_par  = ohms_par;
    load->ohms_mag  = ohms_mag;
    load->henrys    = henrys;
    load->gauss_per_amp = gauss_per_amp;

    // Calculate load related parameters

    if(ohms_par > 1.0E-10)     // Rp greater than zero (magnet included in circuit)
    {
        load->gain2 = 1.0 + ohms_ser / ohms_par;
        load->gain0 = 1.0 / (ohms_par * load->gain2);
        load->gain3 = 1.0 / (ohms_ser + ohms_mag / (1.0 + ohms_mag / ohms_par));
        load->gain1 = load->gain3 - load->gain0;
        load->ohms  = ohms_mag + ohms_ser / load->gain2;
        load->tc    = load->henrys / load->ohms;
    }
    else                       // Rp is effectively zero (i.e. no magnet in the circuit)
    {
        load->gain0 = 1.0 / ohms_ser;
        load->gain1 = 0.0;
        load->gain3 = load->gain0;
        load->ohms  = ohms_ser;
        load->tc    = 1.0E-20;
    }

    // gain10 is not used in the library but indicates to the application the significance of
    // the parallel resistance.  If gain10 > 10 then the parallel resistor is not significant.

    load->gain10 = load->gain1 / load->gain0;

    // Clip inv_henrys to avoid infinity

    if(henrys > 1.0E-20)
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
/*---------------------------------------------------------------------------------------------------------*/
float regLoadCurrentToField(struct reg_load_pars *load, float i_meas)
/*---------------------------------------------------------------------------------------------------------*\
  This function estimates the field based on current.
\*---------------------------------------------------------------------------------------------------------*/
{
    float b_meas;
    float abs_i_meas;
    float di_start;
    float di_end;

    // Field follows a linear - parabola - linear relationship with curent due to magnet saturation

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

    // Return b meas after adjust sign to match i_meas

    return(i_meas < 0.0 ? -b_meas : b_meas);
}
/*---------------------------------------------------------------------------------------------------------*/
float regLoadFieldToCurrent(struct reg_load_pars *load, float b_meas)
/*---------------------------------------------------------------------------------------------------------*\
  This function estimates the current based on the field according to the saturation model of the magnet.
  Beware: This function requires a sqrt() call so it may take a long time depending upon the floating-point
  support of the CPU.
\*---------------------------------------------------------------------------------------------------------*/
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

        i_meas = (-quad_b - sqrt(quad_b * quad_b - 4.0 * quad_a * quad_c)) / (2.0 * quad_a);
    }
    else
    {
        // Linear

        i_meas = db_end / (load->gauss_per_amp * load->sat.l_clip);
    }

    // Return b meas after adjust sign to match i_meas

    return(b_meas < 0.0 ? -i_meas : i_meas);
}
/*---------------------------------------------------------------------------------------------------------*/
void regLoadInitSat(struct reg_load_pars *load, float henrys_sat, float i_sat_start, float i_sat_end)
/*---------------------------------------------------------------------------------------------------------*\
 This function processes the magnet saturation parameters and calculates the linear model slope.
\*---------------------------------------------------------------------------------------------------------*/
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
/*---------------------------------------------------------------------------------------------------------*/
float regLoadVrefSat(struct reg_load_pars *load, float i_meas, float v_ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function will help to linearise the effects of magnet saturation when regulating current.  This is
  not required if regulating field.

  This assumes that:

        L = load->henrys                        for i_meas <= load->sat.i_start
        L = load->henrys x f                    for i_meas  > load->sat.i_start && meas < load->sat.i_end
        L = load->sat.henrys                    for i_meas  > load->sat.i_end

  where f = (1.0 - load->sat.l_rate) * (|i_meas| - load->sat.i_start)

  where load->sat.l_rate = (1.0 - henrys_sat/henrys) / (load->sat.i_end - load->sat.i_start)

  Given v_ref = I.R + L.dI/dt then for |i_meas| > load->sat.i_start:

        v_ref_sat = I.R + f.L.dI/dt = f.v_ref + (1 - f).I.R
\*---------------------------------------------------------------------------------------------------------*/
{
    float f = regLoadCalcSatFactor(load, i_meas);

    return(f * v_ref + (1.0 - f) * i_meas * load->ohms);
}
/*---------------------------------------------------------------------------------------------------------*/
float regLoadInverseVrefSat(struct reg_load_pars *load, float i_meas, float v_ref_sat)
/*---------------------------------------------------------------------------------------------------------*\
  This function does the opposite of regLoadVrefSat:

               v_ref_sat - (1 - f)I.R
       v_ref = ----------------------
                          f
\*---------------------------------------------------------------------------------------------------------*/
{
    float f = regLoadCalcSatFactor(load, i_meas);

    return((v_ref_sat - (1.0 - f) * i_meas * load->ohms) / f);
}
/*---------------------------------------------------------------------------------------------------------*/
float regLoadCalcSatFactor(struct reg_load_pars *load, float i_meas)
/*---------------------------------------------------------------------------------------------------------*\
  This function calculates the saturation factor f for the load for the given measured current.  Is uses
  the simple linear saturation model defined in reg_load: L = L0 . regLoadCalcSatFactor(&load,i_meas)
\*---------------------------------------------------------------------------------------------------------*/
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
/*---------------------------------------------------------------------------------------------------------*\
  End of file: load.c
\*---------------------------------------------------------------------------------------------------------*/
