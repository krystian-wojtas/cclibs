/*!
 * @file  regRst.c
 * @brief Converter Control Regulation library RST regulation algorithm functions
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

#include <stdio.h>
#include <math.h>
#include "libreg/rst.h"

// Constants

#define M_TWO_PI                   (2.0*3.14159265358979323)
#define FLOAT_THRESHOLD            1.0E-10                      //!< Lower bound for s[0]. To allow for floating point rounding errors.
#define REG_AVE_V_REF_LEN          4                            //!< Number of iterations over which to average V_REF
#define REG_TRACK_DELAY_FLTR_TC    100                          //!< Track delay measurement filter time constant (periods)
#define REG_MM_STEPS               20                           //!< Number of steps to cover modulus margin scan

#define REG_JT_OK                                0              //!< Jury's Test Status: Okay
#define REG_JT_S0_IS_ZERO                       -1              //!< Jury's Test Status: S[0] is zero (or negative)
#define REG_JT_SUM_S_IS_NEGATIVE                -2              //!< Jury's Test Status: Sum(S) is negative
#define REG_JT_SUM_EVEN_S_LESS_THAN_SUM_ODD_S   -3              //!< Jury's Test Status: Sum(Even S) < Sum(Odd S)
#define REG_JT_OHMS_PAR_TOO_SMALL               -10             //!< Jury's Test Status: Parallel resistance is too small
#define REG_JT_PURE_DELAY_TOO_LARGE             -11             //!< Jury's Test Status: Pure delay is too large (max is 2.4 periods)

// Macros

#define MINIMUM(A,B)               (A<B?A:B)
#define REG_MM_FREQ(index)         (0.1 + (9.9 / (REG_MM_STEPS*REG_MM_STEPS*REG_MM_STEPS)) * (float)(index*index*index))

// Typedef for complex numbers

typedef struct complex
{
    double  real;
    double  imag;
} complex;

// Static function declarations

static double regVectorMultiply (double *p, double *m, int32_t p_order, int32_t m_idx);
static float  regAbsComplexRatio(double *num, double *den, double k);



// Non-Real-Time Functions - do not call these from the real-time thread or interrupt

static int32_t regJurysTest(struct reg_rst_pars *pars)
{
    int32_t     i;
    int32_t     n;
    int32_t     jury_idx = 0;
    double      d;
    double      a[REG_N_RST_COEFFS];
    double      b[REG_N_RST_COEFFS];
    double      sum_even_s;                     // Sum of even S coefficients: s[0]+s[2]+s[4]+...
    double      sum_odd_s;                      // Sum of odd S coefficients:  s[1]+s[3]+s[5]+...

    // Jury's test -1: s[0] > 0 for stability

    if(pars->rst.s[0] < FLOAT_THRESHOLD)
    {
        return(REG_JT_S0_IS_ZERO);
    }

    // Skip trailing zero s[] coefficients - note that s[0] cannot be zero because of Jury test above

    n = pars->rst_order + 1;

    while(--n > 0 && pars->rst.s[n] == 0.0);

    // Transfer s[] to b[] and sum even and odd coefficients of s[] separately

    for(i = 0, sum_odd_s = 0.0, sum_even_s = 0.0 ; i <= n ; i++)
    {
        b[i] = pars->rst.s[i];

        if((i & 1) == 0)
        {
            sum_even_s += b[i];
        }
        else
        {
            sum_odd_s += b[i];
        }
    }

    // Jury's test -2 : s(1) > 0 for stability - allow for floating point rounding errors

    if((sum_even_s + sum_odd_s) < -FLOAT_THRESHOLD)
    {
        return(REG_JT_SUM_S_IS_NEGATIVE);
    }

    // Jury's test -3 : (-1)^n . s(-1) > 0 for stability

    if(sum_even_s < sum_odd_s)
    {
        return(REG_JT_SUM_EVEN_S_LESS_THAN_SUM_ODD_S);
    }

    // Run Jury Stability Test

    do
    {
        for(i = 0 ; i <= n ; i++)
        {
            a[i] = b[i];
        }

        d = a[n] / a[0];

        for(i = 0 ; i < n ; i++)
        {
            b[i] = a[i] - d * a[n - i];
        }

        // Jury's tests 1 - (n-2) : First element of every row of Jury's array > 0 for stability

        jury_idx++;

        if(b[0] <= 0.0)
        {
            return(jury_idx);
        }

    } while(--n > 2);

    // All roots lie in the unit circle

    return(REG_JT_OK);
}



static float regModulusMargin(struct reg_rst_pars *pars)
{
    int32_t     frequency_index;
    int32_t     frequency_index_step;                       // +/-1
    float       frequency_fraction;                         // 1 = regulation frequency, 0.5 = Nyquist
    float       frequency_fraction_for_min_abs_S_p_y;
    float       abs_S_p_y;                                  // Current value of the sensitivity function

    // For algorithm 1 (dead-beat, 1 period delay), the modulous margin comes at the Nyquist

    if(pars->alg_index == 1)
    {
        pars->modulus_margin      = regAbsComplexRatio(pars->asbr, pars->as, 0.5);
        pars->modulus_margin_freq = 0.5 / pars->reg_period;
    }
    else // for algorithms 2-5, scan for minimum abs_S_p_y (this is the modulus margin)
    {
        // Limit scan frequency range from 0.1 x min_auxpole_hz to 10 x min_auxpole_hz.
        // Frequency steps size follows a cubic because abs_S_p_y changes more quickly at lower
        // frequencies. Start the scan in the middle of the range and stop scan at the minimum
        // abs_S_p_y or the Nyquist.

        // Start in the middle of the linearised range

        frequency_index = REG_MM_STEPS / 2;
        frequency_fraction_for_min_abs_S_p_y =
        frequency_fraction = pars->min_auxpole_hz * pars->reg_period * REG_MM_FREQ(frequency_index);

        // If this frequency is over the Nyquist, report a bad modulus margin (0)

        if(frequency_fraction > 0.5)
        {
            return(0.0);
        }

        // Evaluate abs_S_p_y on cons

        pars->modulus_margin = regAbsComplexRatio(pars->asbr, pars->as, frequency_fraction);
        frequency_index--;
        frequency_fraction = pars->min_auxpole_hz * pars->reg_period * REG_MM_FREQ(frequency_index);
        abs_S_p_y = regAbsComplexRatio(pars->asbr, pars->as, frequency_fraction);

        if(abs_S_p_y < pars->modulus_margin)
        {
            frequency_index_step = -1;
        }
        else
        {
            abs_S_p_y = pars->modulus_margin;
            frequency_fraction = frequency_fraction_for_min_abs_S_p_y;
            frequency_index_step = 1;
            frequency_index++;
        }

        frequency_index += frequency_index_step;

        do
        {
            pars->modulus_margin = abs_S_p_y;
            frequency_fraction_for_min_abs_S_p_y = frequency_fraction;

            frequency_fraction = pars->min_auxpole_hz * pars->reg_period * REG_MM_FREQ(frequency_index);

            abs_S_p_y = regAbsComplexRatio(pars->asbr, pars->as, frequency_fraction);

            frequency_index += frequency_index_step;

        } while(frequency_index >= 0 && frequency_index <= REG_MM_STEPS && frequency_fraction < 0.5 && abs_S_p_y < pars->modulus_margin);

        pars->modulus_margin_freq = frequency_fraction_for_min_abs_S_p_y / pars->reg_period;
    }

    return(pars->modulus_margin);
}



static float regAbsComplexRatio(double *num, double *den, double k)
{
    int32_t     idx;
    double      cosine;
    double      sine;
    double      w;
    complex     num_exp = { 0.0, 0.0 };
    complex     den_exp = { 0.0, 0.0 };

    for(idx = 0 ; idx < REG_N_RST_COEFFS; idx++)
    {
        w      = M_TWO_PI * (double)(idx + 1) * k;
        cosine = cos(w);
        sine   = sin(w);

        num_exp.real += num[idx] * cosine;
        num_exp.imag -= num[idx] * sine;

        den_exp.real += den[idx] * cosine;
        den_exp.imag -= den[idx] * sine;
    }

    return(sqrt(num_exp.real * num_exp.real + num_exp.imag * num_exp.imag) /
           sqrt(den_exp.real * den_exp.real + den_exp.imag * den_exp.imag));
}



/*
 * This function prepares coefficients for the RST regulation algorithm. It chooses the algorithm to use
 * based on pars->pure_delay_iters. It is divided into 5 ranges, three result in dead-beat PII
 * controllers and two in non-dead-beat PII controllers. To work the bandwidth of the voltage source and
 * FIR filter notches must be at least ten times the frequency of the auxiliary poles because they
 * not included in the load model.
 * 
 * It can be used with slow inductive circuits as well as fast or even resistive loads. For fast loads,
 * auxpole1_hz is set to 1.0E+5 and z to 0.8. Normally auxpole1_hz should equal auxpoles2_hz and z should be 0.5.
 * 
 * The pars->pure_delay_periods parameter informs the algorithm of the pure delay around the loop. This can be
 * a dynamic behaviour (i.e. of the voltage source or filters) crudely modelled as a pure delay for low
 * frequencies, or it can be real pure delay due to computation or communications delays.
 * 
 * Support for pure delays above 40% of the period is restricted to loads without a parallel resistor
 * (i.e. one pole and no zero).
 * 
 * The calculation of the RST coefficients requires double precision floating point, even though the
 * coefficients themselves are stored as single precision.
 */
static int32_t regRstInitPII(struct reg_rst_pars  *pars,
                             struct reg_load_pars *load,
                             float                 auxpole1_hz,
                             float                 auxpoles2_hz,
                             float                 auxpoles2_z,
                             float                 auxpole4_hz,
                             float                 auxpole5_hz)
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

    t1 = -pars->reg_period / load->tc;
    a1 = -exp(t1);

    if(a1 > -0.99)              // if t1 > 0.01
    {
        a2 = 1.0 + a1;                  // it is okay to use 1 - exp(t1)
    }
    else                        // else use Maclaurin series for exp(t1) = 1 + t1 + t1^2/2! + ...
    {
        a2 = -(t1 * (1.0 + 0.5 * t1));  // This is more precise for small t1
    }

    b0_b1 = load->gain1 * a2;

    // Identify minimum frequency of an auxiliary pole

    pars->min_auxpole_hz = MINIMUM(auxpole1_hz, auxpoles2_hz);

    // Select the algorithm to use according to the pure delay

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

        return(REG_JT_OHMS_PAR_TOO_SMALL);
    }
    else if(pars->pure_delay_periods < 1.0)
    {
        // Option 2 - pure delay 0.401 - 0.999

        pars->alg_index = 2;
        b0 = b0_b1 * (1.0 - pars->pure_delay_periods);
        b1 = b0_b1 * pars->pure_delay_periods;
    }
    else if(pars->pure_delay_periods < 1.401)
    {
        // Option 3 - pure delay 1.0 - 1.401

        pars->alg_index = 3;
        b0 = b0_b1 * (2.0 - pars->pure_delay_periods);
        b1 = b0_b1 * (pars->pure_delay_periods - 1.0);
        pars->min_auxpole_hz = MINIMUM(pars->min_auxpole_hz, auxpole4_hz);
    }
    else if (pars->pure_delay_periods < 2.00)
    {
        // Option 4 - pure delay 1.401 - 1.999

        pars->alg_index = 4;
        b0 = b0_b1 * (2.0 - pars->pure_delay_periods);
        b1 = b0_b1 * (pars->pure_delay_periods - 1.0);
    }
    else if (pars->pure_delay_periods < 2.401)
    {
        // Option 5 - pure delay 2.00 - 2.401

        pars->alg_index = 5;
        b0 = b0_b1 * (3.0 - pars->pure_delay_periods);
        b1 = b0_b1 * (pars->pure_delay_periods - 2.0);
    }
    else
    {
        // If pure delay > 2.401 then no solution is possible

        return(REG_JT_PURE_DELAY_TOO_LARGE);
    }

    // Apply gain to b0 and b1 if regulating field

    if(pars->reg_mode == REG_FIELD)
    {
        // If parallel resistor is significant, stop now because we don't handle it correctly.

        if(load->ohms_par < 1.0E6)
        {
            return(REG_JT_OHMS_PAR_TOO_SMALL);
        }

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

    c1 = -exp(-pars->reg_period * M_TWO_PI * auxpole1_hz);
    q1 = -exp(-pars->reg_period * M_TWO_PI * auxpoles2_hz * auxpoles2_z);
    d1 = 2.0 * q1 * cos(pars->reg_period * M_TWO_PI * auxpoles2_hz * sqrt(1.0 - auxpoles2_z * auxpoles2_z));
    d2 = q1 * q1;

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
        r_idx = 4;
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
        r_idx = 4;
        s_idx = 5;
        break;

    case 3:                 // Algorithm 3 : Pure delay fraction 1.0 - 1.401 : dead-beat (2)

        pars->min_auxpole_hz = MINIMUM(pars->min_auxpole_hz, auxpole4_hz);
        c2 = -exp(-pars->reg_period * M_TWO_PI * auxpole4_hz);
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
        r_idx = 5;
        s_idx = 7;
        break;

    case 4:                 // Algorithm 4 : Pure delay fraction 1.401 - 1.999 : not dead-beat

        pars->min_auxpole_hz = MINIMUM(pars->min_auxpole_hz, auxpole4_hz);
        c2 = -exp(-pars->reg_period * M_TWO_PI * auxpole4_hz);

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
        r_idx = 5;
        s_idx = 7;
        break;

    case 5:                 // Algorithm 5 : Pure delay fraction 2.0 - 2.401 : dead-beat (3)

        pars->min_auxpole_hz = MINIMUM(pars->min_auxpole_hz, auxpole4_hz);
        pars->min_auxpole_hz = MINIMUM(pars->min_auxpole_hz, auxpole5_hz);
        c2 = -exp(-pars->reg_period * M_TWO_PI * auxpole4_hz);
        c3 = -exp(-pars->reg_period * M_TWO_PI * auxpole5_hz);
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
        r_idx = 6;
        s_idx = 9;
        break;
    }

    // Calculate A.S and A.S + B.R to allow Modulus Margin to be calculated later

    for(idx = REG_N_RST_COEFFS-1 ; idx > s_idx ; idx --)
    {
        pars->as  [idx] = 0.0;
        pars->asbr[idx] = 0.0;
    }

    for(idx = s_idx ; s_idx >= 0 || r_idx >= 0 ; s_idx--, r_idx--, idx--)
    {
        pars->as  [idx] = regVectorMultiply(pars->a, pars->rst.s, 1, s_idx);
        pars->asbr[idx] = regVectorMultiply(pars->b, pars->rst.r, 1, r_idx) + pars->as[idx];
    }

    return(REG_JT_OK);
}

static double regVectorMultiply(double *p, double *m, int32_t p_order, int32_t m_idx)
{
    int32_t    p_idx;
    double     product = 0.0;

    for(p_idx = 0 ; m_idx >= 0 && p_idx <= p_order; m_idx--, p_idx++)
    {
        product += p[p_idx] * m[m_idx];
    }

    return(product);
}



/*
 * This function prepares coefficients for the RST regulation algorithm to implement a proportional-integral
 * controller. It can be used with fast slightly inductive circuits.
 */
static void regRstInitPI(struct reg_rst_pars  *pars,
                         struct reg_load_pars *load,
                         float                 auxpole1_hz)
{
    float a1 = -exp(-pars->reg_period * (load->ohms_ser + load->ohms_mag) * load->inv_henrys);
    float b1 = (1.0 + a1) / (load->ohms_ser + load->ohms_mag);
    float c1 = -exp(-pars->reg_period * M_TWO_PI * auxpole1_hz);

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



/*
 * This function prepares coefficients for the RST regulation algorithm to implement an Integrator only.
 * This can be used with resistive circuits.
 */
static void regRstInitI(struct reg_rst_pars  *pars,
                        struct reg_load_pars *load,
                        float                 auxpole1_hz)
{
    float b1 = 1.0 / (load->ohms_ser + load->ohms_mag);
    float c1 = -exp(-M_TWO_PI * pars->reg_period * auxpole1_hz);

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



/*
 * Calculate coefficients for open loop reference difference equation.
 */
static inline void regRstInitOpenLoop(struct reg_rst_pars *pars, struct reg_load_pars *load)
{
    // Backwards EULER

    const double s1 = -load->henrys / (pars->reg_period * load->ohms_par);
    const double t1 = -load->henrys / pars->reg_period * (1.0 + load->ohms_ser / load->ohms_par);
    const double s0 =  load->ohms_mag / load->ohms_par +  1.0 - s1;
    const double t0 =  load->ohms_mag + load->ohms_ser * (1.0 + load->ohms_mag/load->ohms_par) - t1;

    pars->openloop_forward.ref[0] =  t0/s0;
    pars->openloop_forward.ref[1] =  t1/s0;
    pars->openloop_forward.act[1] = -s1/s0;

    pars->openloop_reverse.ref[1] = -t1/t0;
    pars->openloop_reverse.act[0] =  s0/t0;
    pars->openloop_reverse.act[1] =  s1/t0;

   // If the regulation mode is CURRENT, we can use the reference current value as provided.
   // If it is FIELD, the current is calculated as I = B/G where G is the field-to-current
   // ratio for the magnet.

    if(pars->reg_mode == REG_FIELD)
    {
        pars->openloop_forward.act[0] /= load->gauss_per_amp;
        pars->openloop_forward.act[1] /= load->gauss_per_amp;
        pars->openloop_reverse.act[0] *= load->gauss_per_amp;
        pars->openloop_reverse.act[1] *= load->gauss_per_amp;
    }
}



enum reg_status regRstInit(struct reg_rst_pars  *pars,
                           uint32_t              reg_period_iters,
                           double                reg_period,
                           struct reg_load_pars *load,
                           float                 auxpole1_hz,
                           float                 auxpoles2_hz,
                           float                 auxpoles2_z,
                           float                 auxpole4_hz,
                           float                 auxpole5_hz,
                           float                 pure_delay_periods,
                           float                 track_delay_periods,
                           enum reg_mode         reg_mode,
                           struct reg_rst       *manual)
{
    uint32_t    i;
    double      t0_correction;

    pars->reg_mode             = reg_mode;
    pars->inv_reg_period_iters = 1.0 / (float)reg_period_iters;
    pars->reg_period           = reg_period;
    pars->alg_index            = 0;
    pars->dead_beat            = 0;
    pars->pure_delay_periods   = pure_delay_periods;
    pars->modulus_margin       = 0.0;
    pars->jurys_result         = REG_JT_OK;

    // if AUXPOLE1 = 0.0 -> MANUAL RST coefficients

    if(auxpole1_hz <= 0.0)
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

        // Calculate RST coefficients and track delay according to AUXPOLES2_HZ and load inductance

        if(auxpoles2_hz > 0.0)                         // If AUXPOLES2_HZ > 0               -> PII regulator (slow inductive load)
        {
            pars->jurys_result = regRstInitPII(pars, load, auxpole1_hz, auxpoles2_hz, auxpoles2_z, auxpole4_hz, auxpole5_hz);
        }
        else if(load->henrys >= 1.0E-10)               // If AUXPOLES2_HZ <= 0, HENRYS > 0  ->  PI regulator (fast inductive load)
        {
            regRstInitPI(pars, load, auxpole1_hz);
        }
        else                                           // If AUXPOLES2_HZ <= 0, HENRYS <= 0 ->   I regulator (resistive load)
        {
            regRstInitI(pars, load, auxpole1_hz);
        }
    }

    // Determine the highest order of the RST polynomials

    i = REG_N_RST_COEFFS;

    while(--i > 0 && pars->rst.r[i] == 0.0 && pars->rst.s[i] == 0.0 && pars->rst.t[i] == 0.0);

    pars->rst_order = i;

    //Check that s polynomial is stable using Jury test

    if(pars->jurys_result == REG_JT_OK)
    {
        pars->jurys_result = regJurysTest(pars);
    }

    if(pars->jurys_result != REG_JT_OK)
    {
        // RST coefficients are invalid and cannot be used

        pars->status = REG_FAULT;

        pars->inv_s0           = 0.0;
        pars->t0_correction    = 0.0;
        pars->inv_corrected_t0 = 0.0;
        pars->rst_order        = 0;
    }
    else
    {
        // Calculate floating point math correction for T coefficients to ensure that Sum(T) == Sum(R)

        t0_correction = 0.0;

        for(i = 0 ; i <= pars->rst_order ; i++)
        {
            t0_correction += (double)pars->rst.r[i] - (double)pars->rst.t[i];
        }

        pars->t0_correction    = t0_correction;
        pars->inv_corrected_t0 = 1.0 / (t0_correction + (double)pars->rst.t[0]);
        pars->inv_s0           = 1.0 /  pars->rst.s[0];

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

            // Calculate and check Modulus Margin

            if(regModulusMargin(pars) < REG_MM_WARNING_THRESHOLD)
            {
                pars->status = REG_WARNING;
            }
            else
            {
                pars->status = REG_OK;
            }
        }
    }

    // Calculate coefficients for open loop difference equation.

    regRstInitOpenLoop(pars, load);

    // Return the status

    return(pars->status);
}



void regRstInitHistory(struct reg_rst_vars *vars, float ref, float openloop_ref, float act)
{
    uint32_t    var_idx;

    for(var_idx = 0 ; var_idx <= REG_RST_HISTORY_MASK ; var_idx++)
    {
        vars->openloop_ref[var_idx] = openloop_ref;
        vars->ref         [var_idx] = ref;
        vars->meas        [var_idx] = ref;
        vars->act         [var_idx] = act;
    }

    vars->history_index = 0;
}



// Real-Time Functions

void regRstInitRefRT(struct reg_rst_pars *pars, struct reg_rst_vars *vars, float rate)
{
    double      meas;
    double      ref_offset;
    uint32_t    var_idx;
    uint32_t    par_idx;
    uint32_t    rst_order = pars->rst_order;

    // Return zero immediately if parameters are invalid

    if(pars->status == REG_FAULT)
    {
        return;
    }

    // Use RST coefficients to calculate new actuation from reference

    ref_offset = rate * pars->track_delay_periods * pars->reg_period;

    var_idx = vars->history_index;

    vars->ref[var_idx] = vars->meas[var_idx] + ref_offset;

    meas = pars->rst.t[0]      * (double)vars->ref [var_idx] -
           pars->rst.s[0]      * (double)vars->act [var_idx] +
           pars->t0_correction * (double)vars->ref [var_idx];

    for(par_idx = 1 ; par_idx <= rst_order ; par_idx++)
    {
        var_idx = (var_idx - 1) & REG_RST_HISTORY_MASK;

        vars->ref[var_idx] = vars->meas[var_idx] + ref_offset;

        meas += pars->rst.t[par_idx] * (double)vars->ref [var_idx] -
                pars->rst.s[par_idx] * (double)vars->act [var_idx] -
                pars->rst.r[par_idx] * (double)vars->meas[var_idx];
    }

    vars->openloop_ref[vars->history_index] = vars->ref[vars->history_index];

    vars->meas[vars->history_index] = meas / pars->rst.r[0];
}



float regRstCalcActRT(struct reg_rst_pars *pars, struct reg_rst_vars *vars, float ref, bool is_openloop)
/*!
 * <h3>Implementation Notes</h3>
 *
 * Computing the actuation requires a better precision than 32-bit floating point for the
 * intermediate results. This is achieved by using the type double for the local variable
 * act. On TI C32 DSP, double is simply an alias for float, <em>i.e.</em>, 32-bit floating
 * point. However that DSP can take advantage of the extended 40-bit precision of its FPU,
 * by defining double to be long double.
 */
{
    double      act;
    uint32_t    var_idx;
    uint32_t    par_idx;

    // Return zero immediately if parameters are invalid

    if(pars->status == REG_FAULT)
    {
        return(0.0);
    }

    // Calculate actuation based on openloop flag from regRstCalcRefRT() on previous iteration

    if(is_openloop)
    {
        // Store the reference in openloop history

        vars->openloop_ref[vars->history_index] = ref;

        // Use openloop coefficients to calculate new openloop actuation from reference

        var_idx = (vars->history_index - 1) & REG_RST_HISTORY_MASK;

        // Calculate open loop actuation

        act = pars->openloop_forward.ref[0] * (double)ref +
              pars->openloop_forward.ref[1] * (double)vars->openloop_ref[var_idx] +
              pars->openloop_forward.act[1] * (double)vars->act[var_idx];
    }
    else
    {
        uint32_t rst_order = pars->rst_order;

        var_idx = vars->history_index;

        // Store the reference in RST history

        vars->ref[var_idx] = ref;

        // Use RST coefficients to calculate new actuation from reference

        act     = pars->rst.t[0]      * (double)ref -
                  pars->rst.r[0]      * (double)vars->meas[var_idx] +
                  pars->t0_correction * (double)ref;

        for(par_idx = 1 ; par_idx <= rst_order ; par_idx++)
        {
            var_idx = (var_idx - 1) & REG_RST_HISTORY_MASK;

            act += pars->rst.t[par_idx] * (double)vars->ref [var_idx] -
                   pars->rst.r[par_idx] * (double)vars->meas[var_idx] -
                   pars->rst.s[par_idx] * (double)vars->act [var_idx];
        }
    
        act *= pars->inv_s0;
    }

    return(act);
}



void regRstCalcRefRT(struct reg_rst_pars *pars, struct reg_rst_vars *vars, float act, bool is_limited, bool is_openloop)
/*!
 * <h3>Implementation Notes</h3>
 *
 * Computing the actuation requires a better precision than 32-bit floating point for the
 * intermediate results. This is achieved by using the type double for the local variable
 * ref. On TI C32 DSP, double is simply an alias for float, <em>i.e.</em>, 32-bit floating
 * point. However that DSP can take advantage of the extended 40-bit precision of its FPU,
 * by defining double to be long double.
 */
{
    double      ref;
    uint32_t    var_idx;
    uint32_t    var_idx0;
    uint32_t    par_idx;

    // Return zero immediately if parameters are invalid

    if(pars->status == REG_FAULT)
    {
        return;
    }

    // If we are limited, we need to calculate both the closed-loop and open-loop reference values for the
    // history. If we are not limited, we only need to calculate the one we are not.

    var_idx0 = vars->history_index;

    if(is_limited || !is_openloop)
    {
        // Use openloop coefficients to calculate new openloop reference from actuation

        var_idx = (var_idx0 - 1) & REG_RST_HISTORY_MASK;

        // Calculate and save openloop_ref in history

        vars->openloop_ref[var_idx0] = pars->openloop_reverse.act[0] * (double)act +
                                       pars->openloop_reverse.act[1] * (double)vars->act[var_idx] +
                                       pars->openloop_reverse.ref[1] * (double)vars->openloop_ref[var_idx];
    }

    if(is_limited || is_openloop)
    {
        uint32_t rst_order = pars->rst_order;

        // Use RST coefficients to back-calculate reference from actuation

        var_idx = var_idx0;

        ref = pars->rst.s[0] * (double)act + pars->rst.r[0] * (double)vars->meas[var_idx];

        for(par_idx = 1 ; par_idx <= rst_order ; par_idx++)
        {
            var_idx = (var_idx - 1) & REG_RST_HISTORY_MASK;

            ref += pars->rst.s[par_idx] * (double)vars->act [var_idx] +
                   pars->rst.r[par_idx] * (double)vars->meas[var_idx] -
                   pars->rst.t[par_idx] * (double)vars->ref [var_idx];
        }

        ref *= pars->inv_corrected_t0;

        // Save closed loop ref in history

        vars->ref[var_idx0] = ref;
    }

    // Save act in history

    vars->act[var_idx0] = act;
}



float regRstTrackDelayRT(struct reg_rst_vars *vars)
{
    float    meas_track_delay_periods   = 0.0;
    float    delta_ref                  = regRstDeltaRefRT(vars);
    uint32_t var_idx                    = vars->history_index;

    // Measure track delay if reference is changing

    if(fabs(delta_ref) > 1.0E-4)
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

    return(meas_track_delay_periods);
}



float regRstDelayedRefRT(struct reg_rst_pars *pars, struct reg_rst_vars *vars, uint32_t iteration_index)
{
    int32_t  delay_int;
    float    ref_delay_periods;
    float    float_delay_int;
    float    delay_frac;
    float    ref1;
    float    ref2;

    // Adjust ref delay to account for the acquisition iteration time between regulation iterations

    ref_delay_periods = pars->ref_delay_periods - (float)iteration_index * pars->inv_reg_period_iters;

    // Convert track delay to integer and fractional parts

    if(ref_delay_periods <= 0.0)
    {
        // If ref_delay_periods is zero or less, just return most recent reference value

        return(vars->ref[vars->history_index]);
    }

    delay_frac = modff(ref_delay_periods, &float_delay_int);
    delay_int  = (int32_t)float_delay_int;

    if(delay_int < (REG_RST_HISTORY_MASK - 1))
    {
        // Extract references for the period containing the delayed reference

        ref1 = vars->ref[(vars->history_index - delay_int    ) & REG_RST_HISTORY_MASK];
        ref2 = vars->ref[(vars->history_index - delay_int - 1) & REG_RST_HISTORY_MASK];

        // Return interpolated delayed reference value

        return(ref1 + delay_frac * (ref2 - ref1));
    }

    // else delay_int over-runs the end of the history buffer so return the oldest reference value

    return(vars->ref[(vars->history_index + 1) & REG_RST_HISTORY_MASK]);
}



float regRstAverageVrefRT(struct reg_rst_vars *vars)
{
    uint32_t    i;
    uint32_t    var_idx  = vars->history_index;
    float       sum_vref = 0.0;

    for(i = 0 ; i < REG_AVE_V_REF_LEN ; i++, var_idx--)
    {
        sum_vref += vars->act [var_idx & REG_RST_HISTORY_MASK];
    }

    return(sum_vref * (1.0 / REG_AVE_V_REF_LEN));
}

// EOF
