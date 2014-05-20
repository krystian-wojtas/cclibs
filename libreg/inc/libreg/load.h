/*---------------------------------------------------------------------------------------------------------*\
  File:     libreg.h                                                                    Copyright CERN 2014

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

  Purpose:  Converter Control Regulation library load header file

  Contact:  cclibs-devs@cern.ch

\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBREG_LOAD_H
#define LIBREG_LOAD_H

#include <stdint.h>

// Load structure

struct reg_load_pars
{
    float                       ohms_ser;                       // Load series resistance
    float                       ohms_par;                       // Load parallel resistance
    float                       ohms_mag;                       // Load magnet resistance
    float                       henrys;                         // Load inductance
    float                       inv_henrys;                     // 1.0 / henrys clipped to 1.0E+20 to avoid infinities
    float                       gauss_per_amp;                  // Field to current ratio for the magnet
    float                       ohms;                           // Resistance corresponding to load pole
    float                       tc;                             // Time constant for load pole
    float                       gain0;                          // Load gain 0
    float                       gain1;                          // Load gain 1
    float                       ohms1;                          // Load gain 2
    float                       gain2;                          // Load gain 3 (steady state gain)
    float                       ohms2;                          // Load gain 4
    float                       gain10;                         // Load gain 1/Load gain 0 : Rp insignificance factor
                                                                // if gain10 > ~10 then Rp is insignificant
    struct reg_load_sat
    {
        float                   henrys;                         // Inductance for i > i_sat_end
        float                   i_start;                        // Current measurement at start of saturation
        float                   i_end;                          // Current measurement at end of saturation
        float                   i_delta;                        // i_sat_end - i_sat_start
        float                   b_end;                          // Field at i_sat_end
        float                   b_factor;                       // Parabolic factor for i_sat_start < i < i_sat_end
        float                   l_rate;                         // Inductance droop rate factor (/A)
        float                   l_clip;                         // Clip limit for saturation factor
    } sat;
};

#ifdef __cplusplus
extern "C" {
#endif

// Load functions

void     regLoadInit            (struct reg_load_pars *load, float ohms_ser, float ohms_par, float ohms_mag,
                                 float henrys, float gauss_per_amp);
float    regLoadCurrentToField  (struct reg_load_pars *load, float i_meas);
float    regLoadFieldToCurrent  (struct reg_load_pars *load, float b_meas);
void     regLoadInitSat         (struct reg_load_pars *load, float henrys_sat, float i_sat_start, float i_sat_end);
float    regLoadVrefSat         (struct reg_load_pars *load, float i_meas, float v_ref);
float    regLoadInverseVrefSat  (struct reg_load_pars *load, float i_meas, float v_ref_sat);
float    regLoadCalcSatFactor   (struct reg_load_pars *load, float i_meas);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_LOAD_H

// EOF
