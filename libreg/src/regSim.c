/*!
 * @file  regSim.c
 * @brief Converter Control Regulation library voltage source and load simulation functions
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
#include <string.h>
#include <math.h>
#include "libreg/sim.h"

// Define PI for older compilers which do not have PI in math.h

#define PI 3.14159265358979323846264338327950288



// Non-Real-Time Functions - do not call these from the real-time thread or interrupt

void regSimLoadInit(struct reg_sim_load_pars *sim_load_pars, struct reg_load_pars *load_pars, float sim_load_tc_error, float sim_period)
{
    // If Tc error is zero, simply copy load parameters into sim load parameters structure.

    if(sim_load_tc_error == 0.0)
    {
        sim_load_pars->load_pars = *load_pars;
    }

    // else initialise simulated load with distorted load parameters to have required Tc error

    else
    {
        float sim_load_tc_factor = sim_load_tc_error / (sim_load_tc_error + 2.0);

        regLoadInit(&sim_load_pars->load_pars,
                    load_pars->ohms_ser * (1.0 - sim_load_tc_factor),
                    load_pars->ohms_par * (1.0 - sim_load_tc_factor),
                    load_pars->ohms_mag * (1.0 - sim_load_tc_factor),
                    load_pars->henrys   * (1.0 + sim_load_tc_factor),
                    load_pars->gauss_per_amp);

        regLoadInitSat(&sim_load_pars->load_pars,
                       load_pars->sat.henrys * (1.0 + sim_load_tc_factor),
                       load_pars->sat.i_start,
                       load_pars->sat.i_end);
    }

    sim_load_pars->tc_error             = sim_load_tc_error;
    sim_load_pars->period_tc_ratio      = sim_period / sim_load_pars->load_pars.tc;
    sim_load_pars->is_load_undersampled = (sim_load_pars->period_tc_ratio > 3.0);
}



void regSimLoadSetField(struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float b_init)
{
    regSimLoadSetCurrent(pars, vars, regLoadFieldToCurrentRT(&pars->load_pars, b_init));
}



void regSimLoadSetCurrent(struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float i_init)
{
    vars->circuit_voltage = i_init / pars->load_pars.gain2;

    if(pars->is_load_undersampled == false)
    {
        vars->integrator   = vars->circuit_voltage * pars->load_pars.gain1;
        vars->compensation = 0.0;
    }

    regSimLoadRT(pars, vars, 0, vars->circuit_voltage);
}



void regSimLoadSetVoltage(struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float v_init)
{
    if(pars->is_load_undersampled == false)
    {
        vars->integrator      = v_init * pars->load_pars.gain1;
        vars->circuit_voltage = v_init;
        vars->compensation    = 0.0;
    }

    regSimLoadRT(pars, vars, 0, v_init);
}



void regSimVsInit(struct reg_sim_vs_pars *pars, double iter_period, float v_ref_delay_iters, float bandwidth, float z, float tau_zero,
                  float num[REG_N_VS_SIM_COEFFS], float den[REG_N_VS_SIM_COEFFS])
{
    float       natural_freq;
    float       f_pw;
    float       z2;
    float       w;
    float       b;
    float       d;
    float       de;
    float       y;
    uint32_t    i;
    float       sum_num        = 0.0;
    float       sum_den        = 0.0;

    // Save v_ref_delay so that it can be used later by regConvPureDelay()

    pars->v_ref_delay_iters = v_ref_delay_iters;

    // If bandwidth is positive, use Tustin to calculate z-transform for voltage source using 2nd order model

    if(bandwidth > 0.0)
    {
        // Calculate the natural frequency from the bandwidth and damping

        z2 = z * z;
        natural_freq = bandwidth / sqrt(1.0 - 2.0 * z2 + sqrt(2.0 - 4.0 * z2 + 4 * z2 * z2));

        // Calculate the delay for a steady ramp (see doc/pdf/model/FirstSecondOrder.pdf page 36)

        pars->vs_delay_iters = 2.0 * z / (2.0 * PI * natural_freq * iter_period);

        // If voltage source model is too under-sampled then do not attempt Tustin algorithm

        if(pars->vs_delay_iters < REG_VS_SIM_UNDERSAMPLED_THRESHOLD)
        {
            pars->is_vs_undersampled = true;
            pars->gain = 1.0;
        }
        else
        {
            pars->is_vs_undersampled = false;

            // Tustin will match z-transform and s-transform at frequency f_pw

            if(z < 0.7)      // If lightly damped, there is a resonance peak: f_pw = frequency of peak
            {
                f_pw = natural_freq * sqrt(1.0 - 2.0 * z2);
                w  = PI * iter_period * f_pw;
                b  = tan(w) / w;
            }
            else              // else heavily damped, there is no resonance peak: f_pw = 0 (minimizes approximation error)
            {
                w  = 0.0;
                b  = 1.0;
            }

            // Calculate intermediate variables

            d  = 2.0 * tau_zero / (iter_period * b);
            y  = PI * iter_period * b * natural_freq;
            de = 1.0 / (y * y + 2.0 * z * y + 1.0);

            // Numerator (b0, b1, b2, b3) coefficients

            pars->num[0] = (y * y * (1.0 + d)) * de;
            pars->num[1] = (y * y * 2.0) * de;
            pars->num[2] = (y * y * (1.0 - d)) * de;
            pars->num[3] = 0.0;

            // Denominator (a0, a1, a2, a3) coefficient

            pars->den[0] = 1.0;
            pars->den[1] = (y * y * 2.0 - 2.0) * de;
            pars->den[2] = (y * y - 2.0 * z * y + 1.0) * de;
            pars->den[3] = 0.0;

            // Set the gain to 1

            pars->gain = 1.0;
        }
    }
    else
    {
        // Use voltage source model provided in num and den arrays

        memcpy(pars->num, num, sizeof(pars->num));
        memcpy(pars->den, den, sizeof(pars->den));

        pars->is_vs_undersampled = false;

        // Calculate gain of voltage source model and delay for a steady ramp
        // Steady ramp delay = Sum(i.(num[i] - den[i])) / Sum(num[i])

        pars->vs_delay_iters = 0.0;

        for(i = 0 ; i < REG_N_VS_SIM_COEFFS ; i++)
        {
            sum_num += pars->num[i];
            sum_den += pars->den[i];

            pars->vs_delay_iters += (float)i * (pars->num[i] - pars->den[i]);
        }

        // Protect gain against Inf if the denominator is zero

        if(sum_den == 0.0 || sum_num == 0.0)
        {
            pars->gain           = 0.0;
            pars->vs_delay_iters = 0.0;
        }
        else
        {
           pars->gain            = sum_num / sum_den;
           pars->vs_delay_iters /= sum_num;
        }

        // If vs delay is too short, then consider the model to be under sampled

        if(pars->vs_delay_iters < REG_VS_SIM_UNDERSAMPLED_THRESHOLD)
        {
            pars->is_vs_undersampled = true;
        }
    }

    // If model is under sampled, set model to be transparent

    if(pars->is_vs_undersampled)
    {
        pars->den[0] = pars->num[0] = 1.0;
        pars->den[1] = pars->den[2] = pars->den[3] = pars->num[1] = pars->num[2] = pars->num[3] = 0.0;
    }
}



float regSimVsInitHistory(struct reg_sim_vs_pars *pars, struct reg_sim_vs_vars *vars, float v_circuit)
{
    uint32_t        idx;
    float           v_ref;

    // Initialise history arrays for v_ref and v_circuit

    v_ref = v_circuit / pars->gain;

    for(idx = 0 ; idx < REG_N_VS_SIM_COEFFS ; idx++)
    {
        vars->v_ref [idx]    = v_ref;
        vars->v_circuit[idx] = v_circuit;
    }

    return(v_ref);
}

// Real-Time Functions

float regSimVsRT(struct reg_sim_vs_pars *pars, struct reg_sim_vs_vars *vars, float v_ref)
{
    uint32_t    i;
    uint32_t    j;
    float       v_circuit;

    // Shift history of input and output

    for(i = REG_N_VS_SIM_COEFFS-2, j = REG_N_VS_SIM_COEFFS-1 ; j ; i--,j--)
    {
        vars->v_ref [j]    = vars->v_ref [i];
        vars->v_circuit[j] = vars->v_circuit[i];
    }

    vars->v_ref[0] = v_ref;

    v_circuit = pars->num[0] * v_ref;

    for(i = 1 ; i < REG_N_VS_SIM_COEFFS ; i++)
    {
        v_circuit += pars->num[i] * vars->v_ref[i] - pars->den[i] * vars->v_circuit[i];
    }

    if(pars->den[0] != 0.0)     // Protect against divide by zero
    {
        v_circuit /= pars->den[0];
    }

    vars->v_circuit[0] = v_circuit;

    return(v_circuit);
}

float regSimLoadRT(struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, bool is_vs_undersampled, float v_circuit)
{
    float int_gain;
    float increment;
    float prev_integrator;

    /*!
     *<h3>Implementation Notes</h3>
     *
     * On C32 DSP, where there is no native support for 64-bit floating point arithmetic, the use
     * of a compensated summation (like Kahan) is necessary for the accuracy of the load simulation
     * with 32-bit floating-point. However, the C32 has an internal ALU with extended 40-bit floating
     * point arithmetic, and in some conditions the compiler will use the extended precision for all
     * intermediate results. In the case of a Kahan summation, that extended precision would actually
     * break the algorithm and result in a precision no better than a naive summation. The reason for
     * that is if the new integrator value is stored in a register with 40-bit precision, then the
     * compensation:
     * 
     * \f$compensation = (integrator - previous\_integrator) - increment \simeq 0\f$
     * 
     * will be flawed and result in a negligible compensation value, different from the value obtained
     * if the integrator is stored with 32-bit precision. The implementation below stores the new value
     * of the integrator in reg_sim_load_vars::integrator <em>before</em> calculating the compensation,
     * and that is sufficient for the C32 compiler to lower the precision of the integrator to 32 bits.
     */

    // When load is not under sampled the inductive transients are modelled with an integrator

    if(pars->is_load_undersampled == false)
    {
        int_gain = pars->period_tc_ratio / regLoadSatFactorRT(&pars->load_pars,vars->magnet_current);

        // If voltage source simulation is not under sampled use first-order interpolation of voltage

        if(is_vs_undersampled == false)
        {
            increment = int_gain * (pars->load_pars.gain1 * 0.5 * (v_circuit + vars->circuit_voltage) - vars->integrator);
        }
        else // else when voltage source simulation is under sampled use initial voltage for complete sample
        {
            increment = int_gain * (pars->load_pars.gain1 * vars->circuit_voltage - vars->integrator);
        }

        // Computation of the integrator using Kahan Summation

        increment         -= vars->compensation;
        prev_integrator    = vars->integrator;
        vars->integrator   = prev_integrator + increment;
        vars->compensation = (vars->integrator - prev_integrator) - increment;  // Algebraically 0, in fact holds the
                                                                                // floating-point error compensation
        vars->circuit_current = vars->integrator + pars->load_pars.gain0 * v_circuit;
        vars->magnet_current  = vars->integrator * pars->load_pars.ohms1;
    }
    else // else when load is under sampled the inductive transients are ignored and ohms law is used
    {
        vars->circuit_current = v_circuit * pars->load_pars.gain2;
        vars->magnet_current  = vars->circuit_current * pars->load_pars.gain3;
    }

    // Remember load voltage for next iteration

    vars->circuit_voltage = v_circuit;

    // Simulate magnet field based on magnet current

    vars->magnet_field = regLoadCurrentToFieldRT(&pars->load_pars, vars->magnet_current);

    return(vars->circuit_current);
}

// EOF
