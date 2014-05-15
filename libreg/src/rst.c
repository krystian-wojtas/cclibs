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

// Constants

#define TWO_PI          6.28318530717958647693

/*---------------------------------------------------------------------------------------------------------*/
static double RegVectorMultiply(double *p, double *m, int32_t p_order, int32_t m_idx)
/*---------------------------------------------------------------------------------------------------------*/
{
    int32_t    p_idx;
    double     product = 0.0;

    for(p_idx = 0 ; m_idx >= 0 && p_idx <= p_order; m_idx--, p_idx++)
    {
        product += p[p_idx] * m[m_idx];
    }

    return(product);
}
/*---------------------------------------------------------------------------------------------------------*/
static void regRstInitPII(struct reg_rst_pars  *pars,
                          struct reg_load_pars *load,
                          float                 clbw,
                          float                 clbw2,
                          float                 z,
                          float                 clbw3,
                          float                 clbw4)
/*---------------------------------------------------------------------------------------------------------*\
  This function prepares coefficients for the RST regulation algorithm. It chooses the algorithm to use
  based on pars->pure_delay_iters. It is divided into 5 ranges, two result in dead-beat PII
  controllers and two non-dead-beat PII controllers. To work the bandwidth of the voltage source and
  FIR filter notches must be at least ten times the bandwidth because they not included in the load model.

  It can be used with slow inductive circuits as well as fast or even resistive loads.  For fast loads,
  clbw is set to 1.0E+5 and z to 0.8.  Normally clbw should equal clbw2 and z should be 0.5.

  The pure_delay_iters parameter informs the algorithm of the pure delay around the loop.  This can be
  a dynamic behaviour (i.e. of the voltage source or filters) crudely modelled as a pure delay for low
  frequencies, or it can be real pure delay due to computation or communications delays.

  Support for pure delays above 40% of the period is restricted to loads without a parallel resistor
  (i.e. one pole and no zero).

  The calculation of the RST coefficients requires double precision floating point, even though the
  coefficients themselves are stored as single precision.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    idx;
    int32_t     s_idx = 0;
    int32_t     r_idx = 0;
    double      t1;                   // -period / load_Tc
    double      a1;                   // -exp(t1)
    double      a2;                   // a2 = 1 + a1 = 1 - exp(t1) - use Maclaurin expansion for small t1
    double      b0;
    double      b1;
    double      b0_b1;
    double      c1;
    double      c2;
    double      c3;
    double      d1;
    double      d2;
    double      q1;
    double      q2;

    // Calculate a2 = 1 - exp(t1) using Maclaurin series if t1 is small

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

    // Select the algorithm to use according to the pure delay

    b0_b1 = 0.0;

    if(pars->pure_delay_periods < 0.401)
    {
        // Option 1 - pure delay < 0.401

        pars->alg_index = 1;
        b0 = load->gain0 + load->gain1 * a2 * (1.0 - pars->pure_delay_periods);
        b1 = load->gain0 * a1 + load->gain1 * a2 * pars->pure_delay_periods;
    }
    else if(load->ohms_par < 1.0E6)
    {
        // If pure delay > 0.401 and parallel resistance is significant, then no solution is possible

        return;
    }
    else if(pars->pure_delay_periods < 1.0)
    {
        // Option 2 - pure delay 0.401 - 0.999

        pars->alg_index = 2;
        b0_b1 = load->gain1 * a2;
        b0 = b0_b1 * (1.0 - pars->pure_delay_periods);
        b1 = b0_b1 * pars->pure_delay_periods;
    }
    else if(pars->pure_delay_periods < 1.401)
    {
        // Option 3 - pure delay 1.0 - 1.401

        pars->alg_index = 3;
        b0_b1 = load->gain1 * a2;
        b0 = b0_b1 * (2.0 - pars->pure_delay_periods);
        b1 = b0_b1 * (pars->pure_delay_periods - 1.0);
    }
    else if (pars->pure_delay_periods < 2.00)
    {
        // Option 4 - pure delay 1.401 - 1.999

        pars->alg_index = 4;
        b0_b1 = load->gain1 * a2;
        b0 = b0_b1 * (2.0 - pars->pure_delay_periods);
        b1 = b0_b1 * (pars->pure_delay_periods - 1.0);
    }
    else if (pars->pure_delay_periods < 2.401)
    {
        // Option 5 - pure delay 2.00 - 2.401

        pars->alg_index = 5;
        b0_b1 = load->gain1 * a2;
        b0 = b0_b1 * (3.0 - pars->pure_delay_periods);
        b1 = b0_b1 * (pars->pure_delay_periods - 2.0);
    }
    else
    {
        // If pure delay > 2.401 then no solution is possible

        return;
    }

    // Apply gain to b0 and b1 if regulating field

    if(pars->reg_mode == REG_FIELD)
    {
        b0_b1 *= load->gauss_per_amp;
        b0    *= load->gauss_per_amp;
        b1    *= load->gauss_per_amp;
    }

    // Save plant coefficients

    pars->b[0] = b0;
    pars->b[1] = b1;

    pars->a[0] = 1.0;
    pars->a[1] = a1;

    // Calculate intermediate values

    c1 = -exp(-pars->period * TWO_PI * clbw);
    q1 =  exp(-pars->period * TWO_PI * clbw2 * z);
    d1 = -2.0 * q1 * cos(pars->period * TWO_PI * clbw2 * sqrt(1.0 - z * z));
    d2 =  q1 * q1;

    // Calculate RST coefficients

    switch(pars->alg_index)
    {
    case 1:                 // Algorithm 1 : Pure delay fraction 0 - 0.401 : dead-beat (1)

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

        pars->dead_beat = 1;
        r_idx = 3;
        s_idx = 5;
        break;

    case 2:                 // Algorithm 2 : Pure delay fraction 0.401 - 0.999 : not dead-beat

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

        pars->dead_beat = 0;
        r_idx = 3;
        s_idx = 5;
        break;

    case 3:                 // Algorithm 3 : Pure delay fraction 1.0 - 1.401 : dead-beat (2)

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

        pars->dead_beat = 2;
        r_idx = 3;
        s_idx = 7;
        break;

    case 4:                 // Algorithm 4 : Pure delay fraction 1.401 - 1.999 : not dead-beat

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

        pars->dead_beat = 0;
        r_idx = 3;
        s_idx = 7;
        break;

    case 5:                 // Algorithm 5 : Pure delay fraction 2.0 - 2.401 : dead-beat (3)

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

        pars->dead_beat = 3;
        r_idx = 3;
        s_idx = 9;
        break;
    }

    for(idx = 0 ; s_idx >= 0 || r_idx >= 0 ; s_idx--, r_idx--, idx++)
    {
        pars->asbr[idx] = RegVectorMultiply(pars->a, pars->rst.s, 1, s_idx) +
                          RegVectorMultiply(pars->b, pars->rst.r, 2, r_idx);
    }

    // Calculate AS x BR

    //to check that A*S+B*R=z*(z+c1)*(z^2+d1*z+d2)*(z+b1/b0)
    //A*S+B*R
    //(5th) [1  0  0  0  0  0][s0] + [0  0  0  0  0  0 ][0]
    //(4th) [a1 1  0  0  0  0][s1] + [0  0  0  0  0  0 ][0]
    //(3rd) [0  a1 1  0  0  0][s2] + [0  0  b0 0  0  0 ][r0]
    //(2nd) [0  0  a1 1  0  0][s3] + [0  0  b1 b0 0  0 ][r1]
    //(1st) [0  0  0  a1 1  0][s4] + [0  0  0  b1 b0 0 ][r2] s4=0
    //(0th) [0  0  0  0  a1 1][s5] + [0  0  0  0  b1 b0][r3] s5=0, r3=0


    //to check that A*S+B*R=z^2*(z+c1)*(z^2+d1*z+d2)
    //A*S+B*R
    //(5th) [1  0  0  0  0  0][s0] + [0  0  0  0  0  0 ][0]
    //(4th) [a1 1  0  0  0  0][s1] + [0  0  0  0  0  0 ][0]
    //(3rd) [0  a1 1  0  0  0][s2] + [0  0  b0 0  0  0 ][r0]
    //(2nd) [0  0  a1 1  0  0][s3] + [0  0  b1 b0 0  0 ][r1]
    //(1st) [0  0  0  a1 1  0][s4] + [0  0  0  b1 b0 0 ][r2] s4=0
    //(0th) [0  0  0  0  a1 1][s5] + [0  0  0  0  b1 b0][r3] s5=0, r3=0


    //to check that A*S+B*R=z^2*(z+c1)*(z+c2)*(z^2+d1*z+d2)*(z+b1/b0)
    //A*S+B*R
    //(7th) [1  0  0  0  0  0  0  0][s0] + [0 0 0 0 0  0  0  0 ][0]
    //(6th) [a1 1  0  0  0  0  0  0][s1] + [0 0 0 0 0  0  0  0 ][0]
    //(5th) [0  a1 1  0  0  0  0  0][s2] + [0 0 0 0 0  0  0  0 ][0]
    //(4th) [0  0  a1 1  0  0  0  0][s3] + [0 0 0 0 0  0  0  0 ][0]
    //(3rd) [0  0  0  a1 1  0  0  0][s4] + [0 0 0 0 b0 0  0  0 ][r0]
    //(2nd) [0  0  0  0  a1 1  0  0][s5] + [0 0 0 0 b1 b0 0  0 ][r1] s5=0
    //(1st) [0  0  0  0  0  a1 1  0][s6] + [0 0 0 0 0  b1 b0 0 ][r2] s6=0
    //(0th) [0  0  0  0  0  0  a1 1][s7] + [0 0 0 0 0  0  b1 b0][r3] s7=0, r3=0


    //to check that A*S+B*R=z^3*(z+c1)*(z+c2)*(z^2+d1*z+d2)
    //A*S+B*R
    //(7th) [1  0  0  0  0  0  0  0][s0] + [0 0 0 0 0  0  0  0 ][0]
    //(6th) [a1 1  0  0  0  0  0  0][s1] + [0 0 0 0 0  0  0  0 ][0]
    //(5th) [0  a1 1  0  0  0  0  0][s2] + [0 0 0 0 0  0  0  0 ][0]
    //(4th) [0  0  a1 1  0  0  0  0][s3] + [0 0 0 0 0  0  0  0 ][0]
    //(3rd) [0  0  0  a1 1  0  0  0][s4] + [0 0 0 0 b0 0  0  0 ][r0]
    //(2nd) [0  0  0  0  a1 1  0  0][s5] + [0 0 0 0 b1 b0 0  0 ][r1] s5=0
    //(1st) [0  0  0  0  0  a1 1  0][s6] + [0 0 0 0 0  b1 b0 0 ][r2] s6=0
    //(0th) [0  0  0  0  0  0  a1 1][s7] + [0 0 0 0 0  0  b1 b0][r3] s7=0, r3=0


    //to check that A*S+B*R=z^3*(z+c1)*(z+c2)*(z+c3)*(z^2+d1*z+d2)*(z+b1/b0)
    //A*S+B*R
    //(9th) [1  0  0  0  0  0  0  0  0 0][s0] + [0 0 0 0 0 0 0  0  0  0 ][0]
    //(8th) [a1 1  0  0  0  0  0  0  0 0][s1] + [0 0 0 0 0 0 0  0  0  0 ][0]
    //(7th) [0  a1 1  0  0  0  0  0  0 0][s2] + [0 0 0 0 0 0 0  0  0  0 ][0]
    //(6th) [0  0  a1 1  0  0  0  0  0 0][s3] + [0 0 0 0 0 0 0  0  0  0 ][0]
    //(5th) [0  0  0  a1 1  0  0  0  0 0][s4] + [0 0 0 0 0 0 0  0  0  0 ][0]
    //(4th) [0  0  0  0  a1 1  0  0  0 0][s5] + [0 0 0 0 0 0 0  0  0  0 ][0]
    //(3rd) [0  0  0  0  0  a1 1  0  0 0][s6] + [0 0 0 0 0 0 b0 0  0  0 ][r0] s6=0
    //(2nd) [0  0  0  0  0  0  a1 1  0 0][s7] + [0 0 0 0 0 0 b1 b0 0  0 ][r1] s7=0
    //(1st) [0  0  0  0  0  0  0  a1 1 0][s8] + [0 0 0 0 0 0 0  b1 b0 0 ][r2] s8=0, r3=0
    //(0th) [0  0  0  0  0  0  0  0 a1 1][s9] + [0 0 0 0 0 0 0  0  b1 b0][r3] s9=0, r3=0



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

    pars->alg_index = 10;

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

    pars->alg_index = 20;

    if(pars->reg_mode == REG_FIELD)
    {
        b1 *= load->gauss_per_amp;
    }

    pars->rst.r[0] = 1.0 + c1;

    pars->rst.s[0] =  b1;
    pars->rst.s[1] = -b1;

    pars->rst.t[0] = 1.0 + c1;
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t regRstInit(struct reg_rst_pars  *pars,
                    double                iter_period,
                    uint32_t              period_iters,
                    struct reg_load_pars *load,
                    float                 clbw,
                    float                 clbw2,
                    float                 z,
                    float                 clbw3,
                    float                 clbw4,
                    float                 pure_delay_periods,
                    float                 track_delay_periods,
                    enum reg_mode         reg_mode,
                    struct reg_rst       *manual)
/*---------------------------------------------------------------------------------------------------------*\
  This function prepares coefficients for the RST regulation algorithm based on the paper EDMS 686163 by
  Hugues Thiesen with extensions from Martin Veenstra and Michele Martino, to allow a pure loop delay to
  be accommodated. The function returns REG_OK on success and REG_FAULT if s[0] is too small (<1E-10).

  Notes:    The algorithms can calculate the RST coefficients from the parameters
            clbw/clbw2/z/clbw3/clbw4/pure_delay_periods and the load parameters.
            This only works well if the voltage source bandwidth and FIR notch are much faster
            than the closed loop bandwidth (>10x). Three controllers are selectable:  I, PI, PII.
            For the PII, the regulator may or may not be dead-beat according to the pure delay.

            If the voltage source bandwidth is less than a factor 10 above the closed loop bandwidth
            then the algorithms will not produce good results and the RST coefficients will need
            to be calculated manually using Matlab.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    i;
    double      t0_correction;

    pars->reg_mode           = reg_mode;
    pars->period_iters       = period_iters;
    pars->inv_period_iters   = 1.0 / (float)period_iters;
    pars->period             = iter_period * (double)period_iters;
    pars->alg_index          = 0;
    pars->dead_beat          = 0;
    pars->pure_delay_periods = pure_delay_periods;

    // if CLBW = 0.0 -> MANUAL RST coefficients

    if(clbw <= 0.0)
    {
        pars->rst = *manual;
    }
    else
    {
        // Reset R, S, T, A, B and ASBR coefficients

        for(i=0 ; i < REG_N_RST_COEFFS ; i++)
        {
            pars->rst.r[i] = pars->rst.s[i] = pars->rst.t[i] = pars->a[i] = pars->b[i] = pars->asbr[i] = 0.0;
        }

        // Calculate RST coefficients and track delay according to CLBW2 and load inductance

        if(clbw2 > 0.0)                         // If CLBW2 > 0               -> PII regulator (slow inductive load)
        {
            regRstInitPII(pars, load, clbw, clbw2, z, clbw3, clbw4);
        }
        else if(load->henrys >= 1.0E-10)        // If CLBW2 <= 0, HENRYS > 0  ->  PI regulator (fast inductive load)
        {
            regRstInitPI(pars, load, clbw);
        }
        else                                    // If CLBW2 <= 0, HENRYS <= 0 ->   I regulator (resistive load)
        {
            regRstInitI(pars, load, clbw);
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
            t0_correction += (double)pars->rst.r[i] - (double)pars->rst.t[i];
        }

        pars->t0_correction    = t0_correction;
        pars->inv_corrected_t0 = 1.0 / (t0_correction + (double)pars->rst.t[0]);
        pars->inv_s0           = 1.0 /  pars->rst.s[0];
    }

    // Set track_delay to parameter supplied if manual RST coefficients used, or I or PI algorithm

    if(pars->alg_index == 0 || pars->alg_index >= 10)
    {
        pars->track_delay_periods = track_delay_periods;
    }
    else // else for PII the track delay can be calculated
    {
        if(pars->dead_beat > 0)
        {
            pars->track_delay_periods = (float)pars->dead_beat;
        }
        else
        {
            pars->track_delay_periods = 1.0 + pure_delay_periods;
        }
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
        var_idx = (var_idx - 1) & REG_RST_HISTORY_MASK;

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
        var_idx = (var_idx - 1) & REG_RST_HISTORY_MASK;

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
void regRstMeasTrackDelay(struct reg_rst_vars *vars, float period, float max_ref_rate)
/*---------------------------------------------------------------------------------------------------------*\
  This function must be called after calling regRstCalcAct() and BEFORE calling regRstHistory() to measure
  the track delay.
\*---------------------------------------------------------------------------------------------------------*/
{
    float    meas_track_delay_periods   = 0.0;
    float    delta_ref                  = regRstDeltaRef(vars);
    uint32_t var_idx                    = vars->history_index;

    // Measure track delay if reference is changing

    if(delta_ref != 0.0)
    {
        meas_track_delay_periods = 1.0 +
                ((vars->ref[(var_idx - 1) & REG_RST_HISTORY_MASK]) - vars->meas[var_idx]) / delta_ref;

        // Clip to sane range to handle when delta_ref is small

        if(meas_track_delay_periods < 0.5)
        {
            meas_track_delay_periods = 0.5;
        }
        else if(meas_track_delay_periods > 3.5)
        {
            meas_track_delay_periods = 3.5;
        }
    }

    vars->meas_track_delay_periods = meas_track_delay_periods;
}
/*---------------------------------------------------------------------------------------------------------*/
void regRstHistory(struct reg_rst_vars *vars)
/*---------------------------------------------------------------------------------------------------------*\
  This function must be called after calling regRstCalcAct() and regRstMeasTrackDelay() to adjust the
  RST history index.
\*---------------------------------------------------------------------------------------------------------*/
{
    vars->history_index = (vars->history_index + 1) & REG_RST_HISTORY_MASK;

    vars->delayed_ref_index = 0;
}
/*---------------------------------------------------------------------------------------------------------*/
float regRstPrevRef(struct reg_rst_vars *vars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will return the reference for the previous iteration.  It should be called after
  regRstHistory() has been called.
\*---------------------------------------------------------------------------------------------------------*/
{
    return(vars->ref[(vars->history_index - 1) & REG_RST_HISTORY_MASK]);
}
/*---------------------------------------------------------------------------------------------------------*/
float regRstDeltaRef(struct reg_rst_vars *vars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will return the change of reference for the previous iteration.  It should be called after
  regRstHistory() has been called.
\*---------------------------------------------------------------------------------------------------------*/
{
    return(vars->ref[(vars->history_index - 1) & REG_RST_HISTORY_MASK] -
           vars->ref[(vars->history_index - 2) & REG_RST_HISTORY_MASK]);
}
/*---------------------------------------------------------------------------------------------------------*/
float regRstDelayedRef(struct reg_rst_pars *pars, struct reg_rst_vars *vars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will return the reference delayed by ref_delay_periods.  It should be called after
  regRstHistory() has been called.  It can be called every acquisition iteration between regulation
  iterations, or just on the regulation iterations, as required.
\*---------------------------------------------------------------------------------------------------------*/
{
    int32_t  delay_int;
    float    ref_delay_periods = pars->ref_delay_periods;
    float    float_delay_int;
    float    delay_frac;
    float    ref1;
    float    ref2;

    // Clip track delay

    if(ref_delay_periods < 1.0)
    {
        ref_delay_periods = 1.0;
    }
    else if(ref_delay_periods > (REG_RST_HISTORY_MASK - 1.0))
    {
        ref_delay_periods = (REG_RST_HISTORY_MASK - 1.0);
    }

    // Adjust track delay to account for the acquisition iteration time between regulation iterations

    ref_delay_periods -= (float)vars->delayed_ref_index++ * pars->inv_period_iters;

    // Convert track delay to integer and fractional parts

    delay_frac = modff(ref_delay_periods, &float_delay_int);

    delay_int  = 1 + (int32_t)float_delay_int;  // Add 1 because history_index has been incremented

    // Extract references for the period containing the delayed reference

    ref1 = vars->ref[(vars->history_index - delay_int    ) & REG_RST_HISTORY_MASK];
    ref2 = vars->ref[(vars->history_index - delay_int - 1) & REG_RST_HISTORY_MASK];

    // Return interpolated delayed reference value

    return(ref1 + delay_frac * (ref2 - ref1));
}
/*---------------------------------------------------------------------------------------------------------*/
float regRstAverageVref(struct reg_rst_vars *vars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will return the average RST actuation (V_REF) over the past few iterations.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    i;
    uint32_t    var_idx  = vars->history_index;
    float       sum_vref = 0.0;

    for(i = 0 ; i < REG_AVE_V_REF_LEN ; i++)
    {
        var_idx = (var_idx - 1) & REG_RST_HISTORY_MASK;

        sum_vref += vars->act [var_idx];
    }

    return(sum_vref * (1.0 / REG_AVE_V_REF_LEN));
}
// EOF

