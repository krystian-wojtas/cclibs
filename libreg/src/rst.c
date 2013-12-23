/*---------------------------------------------------------------------------------------------------------*\
  File:     rst.c                                                                       Copyright CERN 2014

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

  Purpose:  RST regulation algorithm functions

  Notes:    Libreg uses Landau notation for the RST algorithm.
            In Longchamp notation, R and S are exchanged.
\*---------------------------------------------------------------------------------------------------------*/

#include <math.h>
#include "libreg/rst.h"

#define TWO_PI          6.28318530717958647693

/*---------------------------------------------------------------------------------------------------------*/
static void regRstInitPII(struct reg_rst_pars  *pars,
                          struct reg_load_pars *load,
                          float                 clbw,
                          float                 clbw2,
                          float                 z,
                          float                 clbw3,
                          float                 clbw4,
                          float                 pure_delay)
/*---------------------------------------------------------------------------------------------------------*\
  This function prepares coefficients for the RST regulation algorithm to implement a dead-beat PII
  controller, provided the voltage source bandwidth is more than ten times higher than
  the desired closed loop bandwidth of the controlled quantity (identified by pars->reg_mode).

  It can be used with slow inductive circuits as well as fast or even resistive loads.  For fast loads,
  clbw is set to 1.0E+5 and z to 0.8.  Normally clbw should equal clbw2 and z should be 0.5.

  The pure_delay parameter informs the algorithm of the pure delay around the loop.  This can be
  a dynamic behaviour (i.e. of the voltage source or FIR filter) crudely modelled as a pure delay for low
  frequencies, or it can be real pure delay due to computation or communications delays. A different
  algorithm is used according to the pure delay, however, support for pure delays above 40% of the period
  is restricted to loads without a parallel resistor (i.e. one pole and no zero).

  The calculation of the RST coefficients requires double precision floation point, even though the
  coefficients themselves are stored as single precision.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t alg_index;          // Algorithm index - selected according to pure delay fraction
    double delay_fraction;       // pure delay as a fraction of the period
    double t1;                   // -period / load_Tc
    double a1;                   // -exp(t1)
    double a2;                   // a2 = 1 + a1 = 1 - exp(t1) - use Maclaurin expansion for small t1
    double b0;
    double b1;
    double b0_b1;
    double c1;
    double c2;
    double c3;
    double d1;
    double d2;
    double q1;
    double q2;

    // Calculate a2 = 1 - exp(t1) using Maclaruin series if t1 is small

    t1 = -pars->period / load->tc;
    a1 = -exp(t1);

    if(a1 > -0.99)              // if t1 > 0.01
    {
        a2 = 1.0 + a1;                  // it is okay to use 1 - exp(t1)
    }
    else                        // else use Maclaurin series for exp(t1) = 1 + t1 + t1^2/2! + ...
    {
        a2 = -(t1 * (1.0 + 0.5 * t1));  // This is more precise for small t1
    }

    // Calculate pure delay as a fraction of the period and select the algorithm to used according to the pure delay fraction

    b0_b1 = 0.0;
    delay_fraction =  pure_delay / pars->period;

    if(delay_fraction < 0.401)
    {
        // Option 1 - pure delay fraction < 0.401

        alg_index = 1;
        b0 = load->gain0 + load->gain1 * a2 * (1.0 - delay_fraction);
        b1 = load->gain0 * a1 + load->gain1 * a2 * delay_fraction;
    }
    else if(load->ohms_par < 1.0E6)
    {
        // If pure delay fraction > 0.4 and parallel resistance is significant, then no solution is possible

        return;
    }
    else if(delay_fraction < 1.0)
    {
        // Option 2 - pure delay fraction 0.401 - 0.999

        alg_index = 2;
        b0_b1 = load->gain1 * a2;
        b0 = b0_b1 * (1.0 - delay_fraction);
        b1 = b0_b1 * delay_fraction;
    }
    else if(delay_fraction < 1.401)
    {
        // Option 3 - pure delay fraction 1.0 - 1.401

        alg_index = 3;
        b0_b1 = load->gain1 * a2;
        b0 = b0_b1 * (2.0 - delay_fraction);
        b1 = b0_b1 * (delay_fraction - 1.0);
    }
    else if (delay_fraction < 2.00)
    {
        // Option 4 - pure delay fraction 1.401 - 1.999

        alg_index = 4;
        b0_b1 = load->gain1 * a2;
        b0 = b0_b1 * (2.0 - delay_fraction);
        b1 = b0_b1 * (delay_fraction - 1.0);
    }
    else if (delay_fraction < 2.401)
    {
        // Option 5 - pure delay fraction 2.00 - 2.401

        alg_index = 5;
        b0_b1 = load->gain1 * a2;
        b0 = b0_b1 * (3.0 - delay_fraction);
        b1 = b0_b1 * (delay_fraction - 2.0);
    }
    else
    {
        // If pure delay fraction > 2.401 then no solution is possible

        return;
    }

    // Apply gain to b0 and b1 if regulating field

    if(pars->reg_mode == REG_FIELD)
    {
        b0_b1 *= load->gauss_per_amp;
        b0    *= load->gauss_per_amp;
        b1    *= load->gauss_per_amp;
    }

    // Calculate intermediate values

    c1 = -exp(-pars->period * TWO_PI * clbw);
    q1 =  exp(-pars->period * TWO_PI * clbw2 * z);
    d1 = -2.0 * q1 * cos(pars->period * TWO_PI * clbw2 * sqrt(1.0 - z * z));
    d2 =  q1 * q1;

    // Calculate RST coefficients

    switch(alg_index)
    {
    case 1:                                             // Algorithm 1 : Pure delay fraction 0 - 0.401 (dead-beat)

        pars->rst.r[0] = c1 + d1 - a1 + 2.0;
        pars->rst.r[1] = c1*d1 + d2 + 2.0*a1 - 1.0;
        pars->rst.r[2] = c1*d2 - a1;

        pars->rst.s[0] = b0;
        pars->rst.s[1] = b1 - 2.0*b0;
        pars->rst.s[2] = b0 - 2.0*b1;
        pars->rst.s[3] = b1;

        pars->rst.t[0] = 1.0;
        pars->rst.t[1] = c1 + d1;
        pars->rst.t[2] = c1*d1 + d2;
        pars->rst.t[3] = c1*d2;

        pars->rst.track_delay = pars->period;               // Track delay is exactly 1 period
        break;

    case 2:                                             // Algorithm 2 : Pure delay fraction 0.401 - 0.999 (not dead-beat)

        pars->rst.r[0] = (3*a1 + c1 + d1 + 2*a1*c1 + 2*a1*d1 + a1*d2 - c1*d2 + a1*c1*d1 + 2)/(b0_b1*(a1 + 1)*(a1 + 1)) +
                         (b1*(c1 + 1)*(d1 + d2 + 1))/(b0_b1*b0_b1*(a1 + 1)) + (a1*(a1 - c1)*(a1*a1 - d1*a1 + d2))/((a1 + 1)*(a1 + 1)*(b1 - a1*b0));

        pars->rst.r[1] = (d2 + c1*d1 + 2*c1*d2 + 2*a1*a1*c1 + 2*a1*a1*d1 + a1*a1*d2 + 3*a1*a1 + a1*a1*c1*d1 - 1)/(b0_b1*(a1 + 1)*(a1 + 1)) -
                         (2*a1*(a1 - c1)*(a1*a1 - d1*a1 + d2))/((a1 + 1)*(a1 + 1)*(b1 - a1*b0)) + (b1*(a1 - 1)*(c1 + 1)*(d1 + d2 + 1))/(b0_b1*b0_b1*(a1 + 1));

        pars->rst.r[2] = (a1*(a1 - c1*d2)*b0*b0 + a1*(2*a1 + d2 + c1*d1 - 1)*b0*b1 - a1*(c1 - a1 + d1 + 2)*b1*b1)/(b0_b1*b0_b1*(b1 - a1*b0));

        pars->rst.s[0] = 1.0;

        pars->rst.s[1] = (b0*b0*b1 + 2*b0*b1*b1 + b1*b1*b1)/(b0*b0*b0 + 2*b0*b0*b1 + b0*b1*b1) -
                         (b1*(b1 - b0*c1)*(d2*b0*b0 - d1*b0*b1 + b1*b1))/(b0*b0_b1*b0_b1*(b1 - a1*b0)) - 2;

        pars->rst.s[2] = (2*b1*(b1 - b0*c1)*(d2*b0*b0 - d1*b0*b1 + b1*b1))/(b0*b0_b1*b0_b1*(b1 - a1*b0)) -
                         (2*(b0*b0*b1 + 2*b0*b1*b1 + b1*b1*b1))/(b0*b0*b0 + 2*b0*b0*b1 + b0*b1*b1) + 1;

        pars->rst.s[3] = (b0*b0*b1 + 2*b0*b1*b1 + b1*b1*b1)/(b0*b0*b0 + 2*b0*b0*b1 + b0*b1*b1) -
                         (b1*(b1 - b0*c1)*(d2*b0*b0 - d1*b0*b1 + b1*b1))/(b0*b0_b1*b0_b1*(b1 - a1*b0));

        pars->rst.t[0] = 1.0 / b0_b1;
        pars->rst.t[1] = (c1 + d1) / b0_b1;
        pars->rst.t[2] = (c1*d1 + d2) / b0_b1;
        pars->rst.t[3] = c1*d2 / b0_b1;

        pars->rst.track_delay = 2.0 * pars->period;         // Track delay is about 2 periods (not dead-beat)
        break;

    case 3:                                            // Algorithm 3 : Pure delay fraction 1.0 - 1.401 (dead-beat)

        c2 = exp(-pars->period * TWO_PI * clbw3);
        q1 = 2.0 - a1 + c1 + c2 + d1;

        pars->rst.r[0] = q1*(2.0 - a1) + d2 + c1*c2 + d1*(c1 + c2) + 2.0*a1 - 1.0;
        pars->rst.r[1] = q1*(2.0*a1 - 1.0)  + c1*c2*d1 + d2*(c1 + c2) - a1;
        pars->rst.r[2] = c1*c2*d2 - a1*q1;

        pars->rst.s[0] = b0;
        pars->rst.s[1] = b0*(q1 - 2.0) + b1;
        pars->rst.s[2] = b1*(q1 - 2.0) - b0*(2.0*q1 - 1.0);
        pars->rst.s[3] = b0*q1         - b1*(2.0*q1 - 1.0);
        pars->rst.s[4] = b1*q1;

        pars->rst.t[0] = 1.0;
        pars->rst.t[1] = c1 + c2 + d1;
        pars->rst.t[2] = c1*c2 + d1*(c1 + c2) + d2;
        pars->rst.t[3] = c1*c2*d1 + d2*(c1 + c2);
        pars->rst.t[4] = c1*c2*d2;

        pars->rst.track_delay = 2.0 * pars->period;         // Track delay is exactly 2 periods (dead-beat)
        break;

    case 4:                                             // Algorithm 4 : Pure delay fraction 1.401 - 1.999 (not dead-beat)

        c2 = exp(-pars->period * TWO_PI * clbw3);

        pars->rst.r[0] = (4*a1 + 2*c1 + 2*c2 + 2*d1 + d2 + 3*a1*c1 + 3*a1*c2 + 3*a1*d1 + 2*a1*d2 + c1*c2 + c1*d1 + c2*d1 +
                          2*a1*c1*c2 + 2*a1*c1*d1 + a1*c1*d2 + 2*a1*c2*d1 + a1*c2*d2 - c1*c2*d2 + a1*c1*c2*d1 + 3)/(b0_b1*(a1 + 1)*(a1 + 1)) + 
                          (b1*(c1 + 1)*(c2 + 1)*(d1 + d2 + 1))/(b0_b1*b0_b1*(a1 + 1)) - 
                          (a1*(a1 - c1)*(a1 - c2)*(a1*a1 - d1*a1 + d2))/((a1 + 1)*(a1 + 1)*(b1 - a1*b0));        

        pars->rst.r[1] = (c1*d2 - c2 - d1 - c1 + c2*d2 + 3*a1*a1*c1 + 3*a1*a1*c2 + 3*a1*a1*d1 + 2*a1*a1*d2 + 4*a1*a1 + c1*c2*d1 + 2*c1*c2*d2 +
                          2*a1*a1*c1*c2 + 2*a1*a1*c1*d1 + a1*a1*c1*d2 + 2*a1*a1*c2*d1 + a1*a1*c2*d2 + a1*a1*c1*c2*d1 - 2)/(b0_b1*(a1 + 1)*(a1 + 1)) + 
                         (2*a1*(a1 - c1)*(a1 - c2)*(a1*a1 - d1*a1 + d2))/((a1 + 1)*(a1 + 1)*(b1 - a1*b0)) + 
                         (b1*(a1 - 1)*(c1 + 1)*(c2 + 1)*(d1 + d2 + 1))/(b0_b1*b0_b1*(a1 + 1));

        pars->rst.r[2] = (a1*(2*a1 + a1*c1 + a1*c2 + a1*d1 - a1*a1 - c1*c2*d2)*b0*b0 +
                          a1*(4*a1 - c1 - c2 - d1 + 2*a1*c1 + 2*a1*c2 + 2*a1*d1 + c1*d2 + c2*d2 - 2*a1*a1 + c1*c2*d1 - 2)*b0*b1 - 
                          a1*(2*c1 - 2*a1 + 2*c2 + 2*d1 + d2 - a1*c1 - a1*c2 - a1*d1 + c1*c2 + c1*d1 + c2*d1 + a1*a1 + 3)*b1*b1)/(b0_b1*b0_b1*(b1 - a1*b0));
        
        q1 = 2.0 - a1 + c1 + c2 + d1;
        q2 = (b1*(c1 + c2 + d1 - c1*d2 - c2*d2 - c1*c2*d1 - 2*c1*c2*d2 + 2) + a1*b1*(2*c1 + 2*c2 + 2*d1 + d2 + c1*c2 + c1*d1 + c2*d1 - c1*c2*d2 + 3))/(b0_b1*(a1 + 1)*(a1 + 1)) +
             (b1*b1*(c1 + 1)*(c2 + 1)*(d1 + d2 + 1))/(b0_b1*b0_b1*(a1 + 1)) + 
             (b1*(a1 - c1)*(a1 - c2)*(a1*a1 - d1*a1 + d2))/((a1 + 1)*(a1 + 1)*(b1 - a1*b0));
        
        pars->rst.s[0] = 1.0;
        pars->rst.s[1] = q1 - 2.0;
        pars->rst.s[2] = q2 - 2.0*q1 + 1.0;
        pars->rst.s[3] = q1 - 2.0*q2;
        pars->rst.s[4] = q2;
    
        pars->rst.t[0] = 1.0 / b0_b1;
        pars->rst.t[1] = (c1 + c2 + d1) / b0_b1;
        pars->rst.t[2] = (d2 + c1*c2 + c1*d1 + c2*d1) / b0_b1;
        pars->rst.t[3] = (c1*d2 + c2*d2 + c1*c2*d1) / b0_b1;
        pars->rst.t[4] = c1*c2*d2 / b0_b1;
     
        pars->rst.track_delay = 3.0 * pars->period;         // Track delay is about 3 periods (not dead-beat)
        break;

    case 5:                                            // Algorithm 5 : Pure delay fraction 2.0 - 2.401 (dead-beat)

        c2 = exp(-pars->period * TWO_PI * clbw3);
        c3 = exp(-pars->period * TWO_PI * clbw4);
        q1 = 2.0 - a1 + c1 + c2 + c3 + d1;
        q2 = (2.0 - a1)*q1 + 2.0*a1 - 1 + d2 + c1*c2 + c1*c3 + c2*c3 + c1*d1 + c2*d1 + c3*d1;

        pars->rst.r[0] = -a1 +(2*a1 - 1)*q1 + (2 - a1)*q2 + c1*d2 + c2*d2 + c3*d2 + c1*c2*c3 + c1*c2*d1 + c1*c3*d1 + c2*c3*d1;
        pars->rst.r[1] = (2*a1 - 1)*q2 - a1*q1 + c1*c2*d2 + c1*c3*d2 + c2*c3*d2 + c1*c2*c3*d1;
        pars->rst.r[2] = -a1*q2 + c1*c2*c3*d2;

        pars->rst.s[0] = b0;
        pars->rst.s[1] = b0*(q1 - 2.0)  + b1;
        pars->rst.s[2] = b1*(q1 - 2.0)  - b0*(2.0*q1 - q2 - 1.0);
        pars->rst.s[3] = b0*(q1 -2.0*q2)- b1*(2.0*q1 - q2 - 1.0);
        pars->rst.s[4] = b0*q2 + b1*(q1 - 2.0*q2);
        pars->rst.s[5] = b1*q2;

        pars->rst.t[0] = 1.0;
        pars->rst.t[1] = c1 + c2 + c3 + d1;
        pars->rst.t[2] = d2 + c1*c2 + c1*c3 + c2*c3 + c1*d1 + c2*d1 + c3*d1;
        pars->rst.t[3] = c1*d2 + c2*d2 + c3*d2 + c1*c2*c3 + c1*c2*d1 + c1*c3*d1 + c2*c3*d1;
        pars->rst.t[4] = c1*c2*d2 + c1*c3*d2 + c2*c3*d2 + c1*c2*c3*d1;
        pars->rst.t[5] = c1*c2*c3*d2;

        pars->rst.track_delay = 3.0 * pars->period;         // Track delay is exactly 3 periods (dead-beat)
        break;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void regRstInitPI(struct reg_rst_pars  *pars,
                         struct reg_load_pars *load,
                         float                 clbw)
/*---------------------------------------------------------------------------------------------------------*\
  This function prepares coefficients for the RST regulation algorithm to implement a proportional-integral
  controller.  It can be used with fast slightly inductive circuits.
\*---------------------------------------------------------------------------------------------------------*/
{
    float a1 = -exp(-pars->period * (load->ohms_ser + load->ohms_mag) * load->inv_henrys);
    float b1 = (1.0 + a1) / (load->ohms_ser + load->ohms_mag);
    float c1 = -exp(-pars->period * TWO_PI * clbw);

    if(pars->reg_mode == REG_FIELD)
    {
        b1 *= load->gauss_per_amp;
    }

    pars->rst.r[0] = 1.0 + c1;
    pars->rst.r[1] = a1 * pars->rst.r[0];

    pars->rst.s[0] =  b1;
    pars->rst.s[1] = -b1;

    pars->rst.t[0] = pars->rst.r[0];
    pars->rst.t[1] = pars->rst.r[1];

    pars->rst.track_delay = pars->period;               // Track delay is at least 1 period
}
/*---------------------------------------------------------------------------------------------------------*/
static void regRstInitI(struct reg_rst_pars  *pars,
                        struct reg_load_pars *load,
                        float                 clbw)
/*---------------------------------------------------------------------------------------------------------*\
  This function prepares coefficients for the RST regulation algorithm to implement an Integrator only.
  This can be used with resistive circuits.
\*---------------------------------------------------------------------------------------------------------*/
{
    float b1 = 1.0 / (load->ohms_ser + load->ohms_mag);
    float c1 = -exp(-TWO_PI * pars->period * clbw);

    if(pars->reg_mode == REG_FIELD)
    {
        b1 *= load->gauss_per_amp;
    }

    pars->rst.r[0] = 1.0 + c1;

    pars->rst.s[0] =  b1;
    pars->rst.s[1] = -b1;

    pars->rst.t[0] = 1.0 + c1;

    pars->rst.track_delay = pars->period;               // Track delay is at least 1 period
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t regRstInit(struct reg_rst_pars  *pars,
                    float                 iter_period,
                    uint32_t              period_iters,
                    struct reg_load_pars *load,
                    float                 clbw,
                    float                 clbw2,
                    float                 z,
                    float                 clbw3,
                    float                 clbw4,
                    float                 pure_delay,
                    enum reg_mode         reg_mode,
                    uint32_t              decimate_flag,
                    struct reg_rst       *manual)
/*---------------------------------------------------------------------------------------------------------*\
  This function prepares coefficients for the RST regulation algorithm based on the paper EDMS 686163 by
  Hugues Thiesen with extensions from Martin Veenstra and Michele Martino, to allow a pure loop delay to
  be accommodated. The function returns REG_OK on success and REG_FAULT if s[0] is too small (<1E-10).

  Notes:    The algorithms can calculate the RST coefficients from the clbw/clbw2/z/clbw3/clbw4/pure_delay
            and load parameters. This only works well if the voltage source bandwidth is much faster
            than the closed loop bandwidth (>10x). Three controllers are selectable:  I, PI, PII.
            For the PII, the regulator may or may not be dead-beat according to the pure_delay.

            If the voltage source bandwidth is less than a factor 10 above the closed loop bandwidth
            then the algorithms will not produce good results and the RST coefficients will need
            to be calculated manually using Matlab.

  Implementation notes:
            Computing the T0 correction requires a better precision than 32-bit floating point for
            the intermediate results. This is achieved by using the type double for the local variable
            t0_correction. On TI C32 DSP, 'double' is simply an alias for 'float' i.e. 32-bit FP.
            However that DSP can take advantage of the extended 40-bit FP precision of its ALU,
            providing the code is written in such a way that the C32 compiler produces the most efficient
            assembly code. That usually implies writing the formula on a single line, not relying on loops.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    i;
    double      t0_correction;

    pars->reg_mode      = reg_mode;
    pars->decimate_flag = decimate_flag;
    pars->period_iters  = period_iters;
    pars->period        = iter_period * period_iters;
    pars->freq          = 1.0 / pars->period;

    // if CLBW = 0.0 -> MANUAL RST coefficients

    if(clbw <= 0.0)
    {
        pars->rst = *manual;
    }
    else
    {
        // Reset RST coefficients

        for(i=0 ; i < REG_N_RST_COEFFS ; i++)
        {
            pars->rst.r[i] = pars->rst.s[i] = pars->rst.t[i] = 0.0;
        }

        // Calculate RST coefficients and track delay according to CLBW2 and load inductance

        if(clbw2 > 0.0)                         // If CLBW2 > 0               -> PII regulator (slow inductive load)
        {
            regRstInitPII(pars, load, clbw, clbw2, z, clbw3, clbw4, pure_delay);
        }
        else if(load->henrys >= 1.0E-10)        // If CLBW2 == 0, HENRYS > 0  ->  PI regulator (fast inductive load)
        {
            regRstInitPI(pars, load, clbw);
        }
        else                                    // If CLBW2 == 0, HENRYS == 0 ->   I regulator (resistive load)
        {
            regRstInitI(pars, load, clbw);
        }

        // Use track delay supplied in manual RST structure if it is valid (i.e. >= regulation period)

        if(manual->track_delay >= pars->period)
        {
            pars->rst.track_delay = manual->track_delay;
        }
    }

    // Check that S[0] isn't too small

    if(fabs(pars->rst.s[0]) < 1.0E-10)
    {
        // RST coefficients are invalid and cannot be used

        pars->status = REG_FAULT;

        pars->inv_s0           = 0.0;
        pars->t0_correction    = 0.0;
        pars->inv_corrected_t0 = 0.0;
    }
    else
    {
        // RST coefficients are valid and can be used

        pars->status = REG_OK;

        // Calculate floating point math correction for T coefficients to ensure that Sum(T) == Sum(R)

        t0_correction = 0.0;

        for(i = 0 ; i < REG_N_RST_COEFFS ; i++)
        {
            t0_correction += (double)pars->rst.r[i] - pars->rst.t[i];
        }

        pars->t0_correction    = t0_correction;
        pars->inv_corrected_t0 = 1.0 / (pars->rst.t[0] + t0_correction);
        pars->inv_s0           = 1.0 /  pars->rst.s[0];
    }

    // Return the status

    return(pars->status);
}
/*---------------------------------------------------------------------------------------------------------*/
float regRstCalcAct(struct reg_rst_pars *pars, struct reg_rst_vars *vars, float ref, float meas)
/*---------------------------------------------------------------------------------------------------------*\
  This function returns the actuation based on the supplied reference and measurement using the RST
  parameters.  If the actuation is clipped then regRstCalcRef must be called to re-calculate the
  reference to put in the history.

  Implementation notes:
        Computing the actuation requires a better precision than 32-bit floating point for
        the intermediate results. This is achieved by using the type double for the local variable act.
        On TI C32 DSP, 'double' is simply an alias for 'float' i.e. 32-bit FP. However that DSP can take
        advantage of the extended 40-bit FP precision of its FPU, by defining double to be long double.
\*---------------------------------------------------------------------------------------------------------*/
{
    double      act;
    uint32_t    var_idx;
    uint32_t    par_idx;

    // Return zero immediately if parameters are invalid

    if(pars->status != REG_OK)
    {
        return(0.0);
    }

    // Use RST coefficients to calculate new actuation from reference

    var_idx = vars->history_index;
    act     = (double)pars->t0_correction * ref + 
              (double)pars->rst.t[0] * ref - 
              (double)pars->rst.r[0] * meas;

    for(par_idx = 1 ; par_idx < REG_N_RST_COEFFS ; par_idx++)
    {
        if(var_idx == 0)
        {
            var_idx = REG_N_RST_COEFFS-1;
        }
        else
        {
            var_idx--;
        }

        act += (double)pars->rst.t[par_idx] * vars->ref [var_idx] -
               (double)pars->rst.r[par_idx] * vars->meas[var_idx] -
               (double)pars->rst.s[par_idx] * vars->act [var_idx];
    }
    
    act *= pars->inv_s0;

    // Save latest act, meas and ref in history

    var_idx = vars->history_index;

    vars->ref [var_idx] = ref;
    vars->meas[var_idx] = meas;
    vars->act [var_idx] = act;

    return(act);
}
/*---------------------------------------------------------------------------------------------------------*/
float regRstCalcRef(struct reg_rst_pars *pars, struct reg_rst_vars *vars, float act, float meas)
/*---------------------------------------------------------------------------------------------------------*\
  This function must be called in two situations:

    1. After calling regRstCalcAct() if the actuation has been clipped due to limits in the actuator.

    2. If the system is running with an open loop actuation

  The function saves the new actuation in the RST history and re-calculates the reference which is returned.

  Implementation notes:
        Computing the actuation requires a better precision than 32-bit floating point for
        the intermediate results. This is achieved by using the type double for the local variable ref.
        On TI C32 DSP, 'double' is simply an alias for 'float' i.e. 32-bit FP. However that DSP can take
        advantage of the extended 40-bit FP precision of its FPU, by defining double to be long double.
\*---------------------------------------------------------------------------------------------------------*/
{
    double      ref;
    uint32_t    var_idx;
    uint32_t    par_idx;

    // Return zero immediately if parameters are invalid

    if(pars->status != REG_OK)
    {
        return(0.0);
    }

    // Use RST coefficients to calculate new actuation from reference

    var_idx = vars->history_index;
    ref     = (double)pars->rst.s[0] * act + (double)pars->rst.r[0] * meas;

    for(par_idx = 1 ; par_idx < REG_N_RST_COEFFS ; par_idx++)
    {
        if(var_idx == 0)
        {
            var_idx = REG_N_RST_COEFFS-1;
        }
        else
        {
            var_idx--;
        }

        ref += (double)pars->rst.s[par_idx] * vars->act [var_idx] +
               (double)pars->rst.r[par_idx] * vars->meas[var_idx] -
               (double)pars->rst.t[par_idx] * vars->ref [var_idx];
    }
    
    ref *= pars->inv_corrected_t0;

    // Save latest act, meas and ref in history

    var_idx = vars->history_index;

    vars->act [var_idx] = act;
    vars->meas[var_idx] = meas;
    vars->ref [var_idx] = ref;

    return(ref);
}
/*---------------------------------------------------------------------------------------------------------*/
float regRstHistory(struct reg_rst_vars *vars)
/*---------------------------------------------------------------------------------------------------------*\
  This function must be called after calling regRstCalcAct() to adjust the RST history index.  It returns 
  the average actuation value over the length of the RST history.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t	i;
    float       act_accumulator = 0.0;

    // Adjust var_idx to next free record in the history

    if(++vars->history_index >= REG_N_RST_COEFFS)
    {
        vars->history_index = 0;
    }

    // Calculate the average actuation from the history

    for(i = 0 ; i < REG_N_RST_COEFFS ; i++)
    {
        act_accumulator += vars->act[i];
    }

    return(act_accumulator * (1.0 / REG_N_RST_COEFFS));
}
// EOF

