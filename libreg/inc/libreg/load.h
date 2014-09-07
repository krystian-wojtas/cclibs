/*!
 * @file  load.h
 * @brief Converter Control Regulation library Load-related functions
 *
 * <h2>Load Model</h2>
 *
 * \image html  LoadModel.png
 * \image latex LoadModel.png "Load Model" width=0.5\textwidth
 *
 * The load model used by libreg is based on a magnet with inductance \f$L\f$ and
 * resistance \f$R_m\f$ (zero for superconducting magnets). This is associated
 * with a parallel damping resistor \f$R_p\f$ and a series resistor \f$R_s\f$
 * which corresponds to the cables or bus bars leading to the magnet. In many
 * cases multiple magnets are connected in series but for low frequencies their
 * individual impedances can be combined.
 *
 * The gain response of this model is first order:
 *
 * \image html  GainResponse.png
 * \image latex GainResponse.png "Gain Response" width=0.5\textwidth
 *
 * <h2>Saturation Model</h2>
 *
 * Non-superconducting magnets may be affected by saturation of their iron yokes,
 * resulting in a reduction in their inductance as the current increases. This
 * can be by as much as 60% which can destabilise the current regulation if it is
 * not compensated. Libreg supports compensation of saturation using a simple
 * linear model:
 *
 * \image html  MagnetSaturationModel.png
 * \image latex MagnetSaturationModel.png "Magnet Saturation Model" width=0.5\textwidth
 *
 * The saturation is summarised by three parameters: \f$L_{sat}\f$, \f$I_{sat\_start}\f$
 * and \f$I_{sat\_end}\f$. Despite being rather crude, this model works very well
 * and hides the non-linearity of the load from the RST regulation algorithm.
 *
 * Note that magnet saturation is not a problem when regulating the magnetic field. In
 * this case it is a second-order effect and can be ignored.
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

#ifndef LIBREG_LOAD_H
#define LIBREG_LOAD_H

#include <stdint.h>

/*!
 * Load structure
 */
struct reg_load_pars
{
    float                       ohms_ser;                       //!< Load series resistance \f$R_s\f$
    float                       ohms_par;                       //!< Load parallel resistance \f$R_p\f$
    float                       ohms_mag;                       //!< Load magnet resistance \f$R_m\f$
    float                       henrys;                         //!< Load inductance \f$L\f$
    float                       inv_henrys;                     //!< 1.0 / henrys. Clipped to 1.0E+20 to avoid infinities.
    float                       gauss_per_amp;                  //!< Field-to-current ratio for the magnet
    float                       ohms;                           //!< Resistance corresponding to load pole
    float                       tc;                             //!< Time constant for load pole
    float                       gain0;                          //!< Load gain 0
    float                       gain1;                          //!< Load gain 1
    float                       ohms1;                          //!< Load gain 2
    float                       gain2;                          //!< Load gain 3 (steady state gain)
    float                       ohms2;                          //!< Load gain 4

    //! \f$R_p\f$ insignificance factor = gain1/gain0. gain10 is not used in the
    //! library but indicates to the application whether the parallel resistance
    //! is significant. If gain10 > 10 then \f$R_p\f$ is insignificant.
    float                       gain10;
                                                          
    /*!
     * Saturation structure
     */
    struct
    {
        float                   henrys;                         //!< Inductance for i > i_sat_end
        float                   i_start;                        //!< Current measurement at start of saturation
        float                   i_end;                          //!< Current measurement at end of saturation
        float                   i_delta;                        //!< i_sat_end - i_sat_start
        float                   b_end;                          //!< Field at i_sat_end
        float                   b_factor;                       //!< Parabolic factor for i_sat_start < i < i_sat_end
        float                   l_rate;                         //!< Inductance droop rate factor (/A)
        float                   l_clip;                         //!< Clip limit for saturation factor
    } sat;
};

#ifdef __cplusplus
extern "C" {
#endif

// Load functions

/*!
 * Initialise the load structure. The saturation model is disabled by default.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    load             Load structure to initialise
 * @param[in]     ohms_ser         Load series resistance
 * @param[in]     ohms_par         Load parallel resistance. Setting to zero means there is no magnet in the circuit.
 * @param[in]     ohms_mag         Load magnet resistance
 * @param[in]     henrys           Load inductance
 * @param[in]     gauss_per_amp    Field-to-current ratio for the magnet
 */
void regLoadInit(struct reg_load_pars *load, float ohms_ser, float ohms_par, float ohms_mag,
                 float henrys, float gauss_per_amp);

/*!
 * Process the magnet saturation parameters and calculate the linear model slope.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in,out] load             Load structure to store saturation parameters in. Saturation is enabled if reg_load_pars::henrys > henrys_sat.
 * @param[in]     henrys_sat       \f$L_{sat}\f$ from the saturation model.
 * @param[in]     i_sat_start      \f$i_{sat\_start}\f$ from the saturation model.
 * @param[in]     i_sat_end        \f$i_{sat\_end}\f$ from the saturation model. Must be greater than i_sat_start.
 */
void regLoadInitSat(struct reg_load_pars *load, float henrys_sat, float i_sat_start, float i_sat_end);

/*!
 * Estimate the field based on current. The field follows a linear-parabola-linear
 * relationship to current due to magnet saturation.
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in]     load             Load parameters and saturation model
 * @param[in]     i_meas           Measured current value
 * @returns Estimated field value
 */
float regLoadCurrentToFieldRT(struct reg_load_pars *load, float i_meas);

/*!
 * Estimate the current based on the field according to the saturation model of the
 * magnet. As the field follows a linear-parabola-linear relationship to current,
 * this function inverts this relationship to obtain current as a function of field.
 *
 * <strong>Note:</strong> This function requires a sqrt() call so may take a long
 * time, depending upon the floating-point support of the CPU.
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in]     load             Load parameters and saturation model
 * @param[in]     b_meas           Measured field value
 * @returns Estimated current value
 */
float regLoadFieldToCurrentRT(struct reg_load_pars *load, float b_meas);

/*!
 * This function helps to linearise the effects of magnet saturation when regulating
 * current. This is not required if regulating field.
 * 
 * This assumes that:
 *
 * \f[
 * L =\left\{\begin{array}{lll}
 *  & L & \textrm{if } I_{meas} \leq I_{sat\_start}\\ 
 *  & L \times f & \textrm{if } I_{meas} > I_{sat\_start} \land I_{meas} < I_{sat\_end}\\ 
 *  & L_{sat} & \textrm{if } I_{meas} \geq I_{sat\_end}
 *  \end{array}\right.
 * \f]
 *
 * where \f$f = (1.0 - L_{rate}) \times (|I_{meas}| - I_{sat\_start})\f$ and
 * \f[
 * L_{rate} = \frac{1.0 - \frac{L_{sat}}{L}}{I_{sat\_end}-I_{sat\_start}}
 * \f].
 *
 * Given \f$V_{ref} = I \cdot R + L \cdot \frac{dI}{dt}\f$, then for \f$|I_{meas}| > I_{sat\_start}\f$:
 * \f[
 * V_{ref\_sat} = I \cdot R + f \cdot L \cdot \frac{dI}{dt} = f \cdot V_{ref} + (1-f) \cdot I \cdot R
 * \f]
 * 
 * This is a Real-Time function (thread safe).
 *
 * @param[in]     load              Load parameters and saturation model
 * @param[in]     i_meas            Measured current value
 * @param[in]     v_ref             Voltage reference
 * @returns Voltage reference after compensation for magnet saturation
 */
float regLoadVrefSatRT(struct reg_load_pars *load, float i_meas, float v_ref);

/*!
 * This function is the inverse of regLoadVrefSatRT().
 * \f[
 * V_{ref} = \frac{V_{ref\_sat} - (1-f) \cdot I \cdot R}{f}
 * \f]
 * 
 * This is a Real-Time function (thread safe).
 *
 * @param[in]     load              Load parameters and saturation model
 * @param[in]     i_meas            Measured current value
 * @param[in]     v_ref_sat         Voltage reference after compensation for magnet saturation
 * @returns Uncompensated voltage reference
 */
float regLoadInverseVrefSatRT(struct reg_load_pars *load, float i_meas, float v_ref_sat);

/*!
 * Calculate the saturation factor f for the load for the given measured current. This function uses
 * the simple linear saturation model defined in reg_load: \f$L = L_0\f$. See regLoadVrefSatRT() for
 * equations.
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in]     load              Load parameters and saturation model
 * @param[in]     i_meas            Measured current value
 * @returns Saturation factor
 */
float regLoadSatFactorRT(struct reg_load_pars *load, float i_meas);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_LOAD_H

// EOF
