/*---------------------------------------------------------------------------------------------------------*\
  File:     cal.c                                                                       Copyright CERN 2011

  Purpose:  Current and voltage measurement and DAC calibration library

  Contact:  cclibs-devs@cern.ch

  Authors:  Quentin King
\*---------------------------------------------------------------------------------------------------------*/

#include "math.h"
#include "libcal.h"

/*---------------------------------------------------------------------------------------------------------*/
static void calCheckLimits(struct cal_event        *cal,                // Will be modified if fault limits exceeded!
                           const struct cal_limits *limits,
                           struct cal_flags        *flags)              // Returned fault and warning flags
/*---------------------------------------------------------------------------------------------------------*\
  Timescale: 1s (called from calAdcFactors() and calDcctFactors())

  This function will check the calibration values against the supplied warning and fault limits.
  If the fault limits are exceeded the calibration values are SET TO THE NOMINAL VALUES and the fault flag
  is set to 1.  If the warning limits are exceeded then the warning flag is set to 1.
\*---------------------------------------------------------------------------------------------------------*/
{
    float offset_ppm_error;
    float gain_err_pos_ppm_error;
    float gain_err_neg_ppm_error;

    if(!limits)                               // If limits are not supplied, reset the flags and return
    {
        flags->fault   = 0;
        flags->warning = 0;
        return;
    }

    offset_ppm_error       = fabs(cal->offset_ppm - limits->nominal_offset_ppm);
    gain_err_pos_ppm_error = fabs(cal->gain_err_pos_ppm);
    gain_err_neg_ppm_error = fabs(cal->gain_err_neg_ppm);

    // Check fault limits

    flags->fault = (offset_ppm_error       > limits->offset_fault_ppm   ||
                    gain_err_pos_ppm_error > limits->gain_err_fault_ppm ||
                    gain_err_neg_ppm_error > limits->gain_err_fault_ppm);

    // If fault limits exceeded then set nominal calibration errors

    if(flags->fault)
    {
        cal->offset_ppm = limits->nominal_offset_ppm;
        cal->gain_err_pos_ppm = 0.0;
        cal->gain_err_neg_ppm = 0.0;
    }

     // Check warning limits

    flags->warning = (offset_ppm_error       > limits->offset_warning_ppm   ||
                      gain_err_pos_ppm_error > limits->gain_err_warning_ppm ||
                      gain_err_neg_ppm_error > limits->gain_err_warning_ppm);
}
/*---------------------------------------------------------------------------------------------------------*/
static float calTempCompensation(enum cal_idx idx,
                                 float        temp_c,
                                 const float  temp_coeffs[CAL_NUM_ERRS],      // Pass NULL if not required
                                 const float  d_temp_coeffs[CAL_NUM_ERRS])    // Pass NULL if not required
/*---------------------------------------------------------------------------------------------------------*\
  Timescales: Annual, Daily, 1s (called from many Libcal functions)

  This function calculates the temperature compensation in ppm using the parabolic formula defined
  AB-PO Tech. Note 11.  The calling function can pass NULL for the temp_coeffs and/or d_temp_coeffs
  according to whether 2nd order, 1st order or no temperature compensation is in use.  The function
  returns the compensation value in ppm for the supplied idx (CAL_OFFSET_V, CAL_GAIN_ERR_POS or
  CAL_GAIN_ERR_NEG).
\*---------------------------------------------------------------------------------------------------------*/
{
    if(d_temp_coeffs)           // if 2nd order temp coeffs defined
    {
        return((temp_c - CAL_TEMP_T0) * (temp_coeffs[idx] + d_temp_coeffs[idx] * (CAL_TEMP_T2 - temp_c) *
                                (1.0/((CAL_TEMP_T1 - CAL_TEMP_T0) * (CAL_TEMP_T2 - CAL_TEMP_T1)))));
    }
    else if(temp_coeffs)        // else if 1st order temp coeffs defined
    {
        return((temp_c - CAL_TEMP_T0) * temp_coeffs[idx]);
    }
                                // else no temp coeffs defined
    return(0.0);
}
/*---------------------------------------------------------------------------------------------------------*/
void calCurrent(const struct cal_dcct    *cal_dcct,           // DCCT calibration factors
                const struct cal_adc     *cal_adc,            // ADC calibration factors
                int32_t                   v_raw,              // ADC raw value
                float                     i_dcct_sim,         // Simulated dcct current (if sim_f is non zero)
                unsigned                  sim_f,              // Simulation flag
                struct cal_current       *meas)               // Returned current/voltage values
/*---------------------------------------------------------------------------------------------------------*\
  Timescale: milliseconds

  This function uses the supplied ADC and DCCT calibration factors to translate:

	v_raw -> v_adc -> v_dcct -> i_dcct

  or if simulating the current, then it runs the reverse action:

	i_dcct_sim -> v_dcct -> v_adc -> v_raw

  The results are returned in the meas structure.  Note that v_dcct is a mathematical construct and
  doesn't correspond to a real world voltage.  It is therefore not included in the meas structure.
\*---------------------------------------------------------------------------------------------------------*/
{
    float       v_adc;
    float       v_dcct;
    float       i_dcct;

    if(sim_f == 0)	                // Not simulating current: v_raw -> v_adc -> v_dcct -> i_dcct
    {
    // Vadc = f(Vraw)

        v_adc = cal_adc->inv_gain * (float)v_raw *
               (1.0 - (v_raw < 0 ? cal_adc->gain_err_neg : cal_adc->gain_err_pos)) - cal_adc->offset_v;

    // Vdcct = f(Vadc)

        v_dcct = v_adc *
                (1.0 - (v_adc < 0.0 ? cal_dcct->gain_err_neg : cal_dcct->gain_err_pos)) - cal_dcct->offset_v;

    // Idcct = f(Vdcct)

        i_dcct = cal_dcct->inv_gain * v_dcct;
    }
    else                        // Simulating current: i_dcct_sim -> v_dcct -> v_adc -> v_raw
    {
        i_dcct = i_dcct_sim;

    // Vdcct = f(Idcct)

        v_dcct = cal_dcct->gain * i_dcct;

    // Vadc = f(Vdcct)

        v_adc = cal_dcct->offset_v + v_dcct *
                (1.0 + (v_dcct < 0.0 ? cal_dcct->gain_err_neg : cal_dcct->gain_err_pos));

    // Vraw = f(Vadc)

        v_raw = (int32_t)((float)cal_adc->gain * (cal_adc->offset_v +
                           v_adc * (1.0 + (v_adc < 0.0 ? cal_adc->gain_err_neg: cal_adc->gain_err_pos))));
    }

// Return results (excluding fictional v_dcct)

    meas->v_raw  = v_raw;
    meas->v_adc  = v_adc;
    meas->i_dcct = i_dcct;
}
/*---------------------------------------------------------------------------------------------------------*/
void calVoltage(const struct cal_v_meas  *cal_v_meas,         // Voltage measurement calibration factors
                const struct cal_adc     *cal_adc,            // ADC calibration factors
                int32_t                   v_raw,              // ADC raw value
                float                     v_meas_sim,         // Simulated voltage measurement (if sim_f is non zero)
                unsigned                  sim_f,              // Simulation flag
                struct cal_voltage       *meas)               // Returned voltage values
/*---------------------------------------------------------------------------------------------------------*\
  Timescale: milliseconds

  This function uses the supplied ADC and voltage measurement calibration factors to translate:

	v_raw -> v_adc -> v_meas

  or if simulating the voltage, then it runs the reverse action:

	v_meas_sim -> v_adc -> v_raw

  The results are returned in the meas structure.
\*---------------------------------------------------------------------------------------------------------*/
{
    float       v_adc;
    float       v_meas;

    if(sim_f == 0)	                // Not simulating current: v_raw -> v_adc -> v_meas
    {
    // Vadc = f(Vraw)

        v_adc = cal_adc->inv_gain * (float)v_raw *
               (1.0 - (v_raw < 0 ? cal_adc->gain_err_neg : cal_adc->gain_err_pos)) - cal_adc->offset_v;

    // Vmeas = f(Vadc)

        v_meas = cal_v_meas->inv_gain * v_adc;
    }
    else                        // Simulating current: v_meas_sim -> v_adc -> v_raw
    {
        v_meas = v_meas_sim;

    // Vadc = f(Vmeas)

        v_adc = cal_v_meas->gain * v_meas;

    // Vraw = f(Vadc)

        v_raw = (int32_t)((float)cal_adc->gain * (cal_adc->offset_v +
                           v_adc * (1.0 + (v_adc < 0.0 ? cal_adc->gain_err_neg: cal_adc->gain_err_pos))));
    }

// Return results

    meas->v_raw = v_raw;
    meas->v_adc = v_adc;
    meas->v_meas = v_meas;
}
/*---------------------------------------------------------------------------------------------------------*/
int32_t calAdcNominalGain(int32_t       v_offset_raw_ave,                // Average Vraw when measuring zero volts
                          int32_t       v_pos_raw_ave,                   // Average Vraw when measuring +Vref
                          float         adc_temp_c,                      // Temperature now
                          const float   adc_temp_coeffs[CAL_NUM_ERRS],   // Pass NULL if not required
                          const float   d_adc_temp_coeffs[CAL_NUM_ERRS], // Pass NULL if not required
                          float         v_ref_err_ppm,                   // Error in +Vref
                          const float  *v_ref_temp_coeff)                // Temperature coefficient for +Vref
/*---------------------------------------------------------------------------------------------------------*\
  This function returns the nominal ADC gain.  This is the gain that will result in the temperature
  normalised positive gain error being zero.  It should be called before calling calAdcFactors() the first
  time and then again if the ADC has drifted too far or the ADC filter gain has changed.
\*---------------------------------------------------------------------------------------------------------*/
{
    int32_t delta_v_raw_ave = v_pos_raw_ave - v_offset_raw_ave;

    return(delta_v_raw_ave -
           (int32_t)((float)delta_v_raw_ave * 1.0E-6 *
                    (v_ref_err_ppm +
                     calTempCompensation(CAL_OFFSET_V,     adc_temp_c, v_ref_temp_coeff, 0) +
                     calTempCompensation(CAL_GAIN_ERR_POS, adc_temp_c, adc_temp_coeffs, d_adc_temp_coeffs))));
}
/*---------------------------------------------------------------------------------------------------------*/
void calAdcFactors(int32_t                  nominal_adc_gain,                // Nominal ADC gain (raw/Vnom)
                   const struct cal_event  *adc_t0,                          // Calibration at temperature T0
                   float                    adc_temp_c,                      // Temperature now
                   const float              adc_temp_coeffs[CAL_NUM_ERRS],   // Pass NULL if not required
                   const float              d_adc_temp_coeffs[CAL_NUM_ERRS], // Pass NULL if not required
                   const struct cal_limits *limits,                          // Pass NULL if not required
                   struct cal_adc          *cal_adc)                         // Returned cal for temperature now
/*---------------------------------------------------------------------------------------------------------*\
  Timescale: ~1 s

  This function calculates the calibration factors that can be used with calCurrent() and calVoltage()
  for an ADC channel at the specified temperature using the calibration errors at temp T0 and the temperature
  coefficients for the ADC.  The calibration factors are needed by the calCurrent() function and should
  be recalculated whenever the temperature changes, or at regular intervals (e.g. at 1Hz).

  If temperature compensation is not in use then this only needs to be called if the ADC is recalibrated
  giving a new nominal ADC gain and/or new calibration errors.
\*---------------------------------------------------------------------------------------------------------*/
{
    struct cal_event   cal_adc_t0;  // Local copy of ADC cal errors at T0 - may be modified by limits check

    cal_adc_t0            = *adc_t0;                                 // Take local copy of ADC cal event
    cal_adc->nominal_gain = nominal_adc_gain;                        // Save nominal ADC gain (Raw/Vnom)
    cal_adc->gain         = (float)nominal_adc_gain / CAL_V_NOMINAL; // Calc ADC gain (raw/V)
    cal_adc->inv_gain     = 1.0 / cal_adc->gain;                     // Calc inverse ADC gain (V/raw)

    // Check limits if limits != NULL, else reset the flags

    calCheckLimits(&cal_adc_t0, limits, &cal_adc->flags);       // If fault limits are exceeded then the
                                                                // cal errors are reset to nominal values!


    // Calculate ADC calibration factors for specified temperature

    cal_adc->offset_v     = 1.0E-6 * CAL_V_NOMINAL * (cal_adc_t0.offset_ppm +
                            calTempCompensation(CAL_OFFSET_V, adc_temp_c, adc_temp_coeffs, d_adc_temp_coeffs));

    cal_adc->gain_err_pos = 1.0E-6 * (cal_adc_t0.gain_err_pos_ppm +
                            calTempCompensation(CAL_GAIN_ERR_POS, adc_temp_c, adc_temp_coeffs, d_adc_temp_coeffs));

    cal_adc->gain_err_neg = 1.0E-6 * (cal_adc_t0.gain_err_neg_ppm +
                            calTempCompensation(CAL_GAIN_ERR_NEG, adc_temp_c, adc_temp_coeffs, d_adc_temp_coeffs));
}
/*---------------------------------------------------------------------------------------------------------*/
void calDcctFactors(float                     nominal_gain,                     // Nominal DCCT gain (A/V/Primary Turn)
                    unsigned                  primary_turns,                    // Number of primary turns
                    float                     head_err_ppm,                     // Head error in ppm of V/A
                    const struct cal_event   *dcct_t0,                          // Calibration at temperature T0
                    float                     dcct_temp_c,                      // Temperature now
                    const float               dcct_temp_coeffs[CAL_NUM_ERRS],   // Pass NULL if not required
                    const float               d_dcct_temp_coeffs[CAL_NUM_ERRS], // Pass NULL if not required
                    const struct cal_limits  *limits,                           // Pass NULL if not required
                    struct cal_dcct          *cal_dcct)                         // Returned cal for temperature now
/*---------------------------------------------------------------------------------------------------------*\
  Timescale: ~1 s

  This function calculates the calibration factors that can be used with calCurrent() for a DCCT measurement
  at the specified temperature using the calibration errors normalised to temp T0 and the temperature
  coefficients for the DCCT.  The calibration factors are needed by the calCurrent() function and should
  be recalculated whenever the temperature changes, or at regular intervals (e.g. at 1Hz).

  If temperature compensation is not in use then this only needs to be called if the DCCT is recalibrated
  giving new normalised calibration errors.
\*---------------------------------------------------------------------------------------------------------*/
{
    struct cal_event   cal_dcct_t0;  // Local copy of DCCT cal errors at T0 - may be modified by limits check

    cal_dcct_t0        = *dcct_t0;                                       // Take local copy of ADC cal event
    cal_dcct->gain     = (1.0 + 1.0E-6 * head_err_ppm) * primary_turns / nominal_gain;   // Head gain (V/A)
    cal_dcct->inv_gain = 1.0 / cal_dcct->gain;                                           // (A/V)

    // Check limits if limits != NULL, else reset the flags

    calCheckLimits(&cal_dcct_t0, limits, &cal_dcct->flags);

    // Calculate DCCT calibration factors for specified temperature

    cal_dcct->offset_v     = 1.0E-6 * CAL_V_NOMINAL * (cal_dcct_t0.offset_ppm +
                             calTempCompensation(CAL_OFFSET_V, dcct_temp_c, dcct_temp_coeffs, d_dcct_temp_coeffs));

    cal_dcct->gain_err_pos = 1.0E-6 * (cal_dcct_t0.gain_err_pos_ppm +
                             calTempCompensation(CAL_GAIN_ERR_POS, dcct_temp_c, dcct_temp_coeffs, d_dcct_temp_coeffs));

    cal_dcct->gain_err_neg = 1.0E-6 * (cal_dcct_t0.gain_err_neg_ppm +
                             calTempCompensation(CAL_GAIN_ERR_NEG, dcct_temp_c, dcct_temp_coeffs, d_dcct_temp_coeffs));
}
/*---------------------------------------------------------------------------------------------------------*/
void calVoltageDividerFactors(float              nominal_gain,      // Nominal Vmeas gain (Vmeas/Vadc)
                              float              gain_err_ppm,      // Vmeas gain error in ppm of Vmeas/Vadc
                              struct cal_v_meas *cal_v_meas)        // Returned calibration for any temp
/*---------------------------------------------------------------------------------------------------------*\
  Timescale: On change of nominal gain or gain error (generally fixed after manufacture)

  This function calculates the calibration factors that can be used with calVoltage() for a voltage divider
  measurement.  The temperature coefficient is not taken into account so this only needs to be called if
  the nominal gain or gain error changes.
\*---------------------------------------------------------------------------------------------------------*/
{
    cal_v_meas->inv_gain = nominal_gain * (1.0 + 1.0E-6 * gain_err_ppm);   // Inverse Vmeas gain (Vmeas/Vadc)
    cal_v_meas->gain     = 1.0 / cal_v_meas->inv_gain;                     // (Vadc/Vmeas)
}
/*---------------------------------------------------------------------------------------------------------*/
void calAdcErrors(const int32_t     v_raw_ave        [CAL_NUM_ERRS],
                  int32_t           nominal_adc_gain,
                  float             adc_temp_c,
                  const float       adc_temp_coeffs  [CAL_NUM_ERRS],  // Pass NULL if not required
                  const float       d_adc_temp_coeffs[CAL_NUM_ERRS],  // Pass NULL if not required
                  const float       v_ref_err_ppm    [CAL_NUM_ERRS],
                  const float       v_ref_temp_coeff [CAL_NUM_ERRS],  // Pass NULL if not required
                  struct cal_event *adc)                              // Returned cal_event structure
/*---------------------------------------------------------------------------------------------------------*\
  Timescale: Daily

  This function is a wrapper for calAdcError() if the raw measurements are available for all three
  calibrations (zero, positive CAL_V_NOMINAL and negative CAL_V_NOMINAL).
\*---------------------------------------------------------------------------------------------------------*/
{
    int32_t        cal_idx;

    for(cal_idx=0 ; cal_idx < CAL_NUM_ERRS ;  cal_idx++)
    {
        calAdcError((enum cal_idx)cal_idx, v_raw_ave[cal_idx], nominal_adc_gain,
                    adc_temp_c, adc_temp_coeffs, d_adc_temp_coeffs,
                    v_ref_err_ppm, v_ref_temp_coeff, adc);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void calAdcError(enum cal_idx      idx,
                 int32_t           v_raw_ave,
                 int32_t           nominal_adc_gain,
                 float             adc_temp_c,
                 const float       adc_temp_coeffs  [CAL_NUM_ERRS],   // Pass NULL if not required
                 const float       d_adc_temp_coeffs[CAL_NUM_ERRS],   // Pass NULL if not required
                 const float       v_ref_err_ppm    [CAL_NUM_ERRS],
                 const float       v_ref_temp_coeff [CAL_NUM_ERRS],   // Pass NULL if not required
                 struct cal_event *adc)                               // Returned cal_event structure
/*---------------------------------------------------------------------------------------------------------*\
  Timescale: Daily

  This function is used to calculate one calibration value for the ADC (i.e. either offset, positive gain
  error or negative gain error).  The raw value from the ADC should be averaged over about 10000 samples
  while measuring the appropriate calibration voltage (zero, positive CAL_V_NOMINAL or negative
  CAL_V_NOMINAL).  The error and the linear temperature coefficient for the voltage reference should
  be supplied if known.

  Likewise, the linear or 1st or 2nd order temperature coefficients for the ADC can be provided if known.
  If temperature coefficients are not used then a NULL pointer should be sent by the calling function.

  The function calculates the relevant ADC calibration error in PPM, normalised to temperature T0.
\*---------------------------------------------------------------------------------------------------------*/
{
    float inv_nominal_adc_gain_10E6 = 1.0E6 / (float)nominal_adc_gain;

    switch(idx)                 // Switch on calibration index
    {
    case CAL_OFFSET_V:

        adc->offset_ppm = (float)v_raw_ave * inv_nominal_adc_gain_10E6
                        - calTempCompensation(CAL_OFFSET_V, adc_temp_c, adc_temp_coeffs, d_adc_temp_coeffs);
        break;

    case CAL_GAIN_ERR_POS:

        adc->gain_err_pos_ppm = (float)(v_raw_ave - nominal_adc_gain) * inv_nominal_adc_gain_10E6
                              - calTempCompensation(CAL_GAIN_ERR_POS, adc_temp_c, adc_temp_coeffs, d_adc_temp_coeffs)
                              - adc->offset_ppm
                              - calTempCompensation(CAL_OFFSET_V,     adc_temp_c, adc_temp_coeffs, d_adc_temp_coeffs)
                              - v_ref_err_ppm[CAL_GAIN_ERR_POS]
                              - calTempCompensation(CAL_GAIN_ERR_POS, adc_temp_c, v_ref_temp_coeff, 0);
        break;

    case CAL_GAIN_ERR_NEG:

        adc->gain_err_neg_ppm = (float)(-v_raw_ave - nominal_adc_gain) * inv_nominal_adc_gain_10E6
                              - calTempCompensation(CAL_GAIN_ERR_NEG, adc_temp_c, adc_temp_coeffs, d_adc_temp_coeffs)
                              + adc->offset_ppm
                              + calTempCompensation(CAL_OFFSET_V,     adc_temp_c, adc_temp_coeffs, d_adc_temp_coeffs)
                              - v_ref_err_ppm[CAL_GAIN_ERR_NEG]
                              - calTempCompensation(CAL_GAIN_ERR_NEG, adc_temp_c, v_ref_temp_coeff, 0);
        break;

    default: break;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void calDcctError(enum cal_idx            idx,
                  int32_t                 v_raw_ave,
                  const struct cal_adc   *cal_adc,                          // Calibrate ADC before DCCT
                  float                   dcct_temp_c,
                  const float             dcct_temp_coeffs  [CAL_NUM_ERRS], // Pass NULL if not required
                  const float             d_dcct_temp_coeffs[CAL_NUM_ERRS], // Pass NULL if not required
                  float                   head_err_ppm,                     // Head error in ppm of V/A
                  struct cal_event       *dcct)                             // Returned cal_event structure
/*---------------------------------------------------------------------------------------------------------*\
  Timescale: Daily

  This function will calculate the DCCT offset or gain error (for positive or negative values).  The
  ADC must be calibrated first and the ADC calibration factors in cal_adc must be correct.

  The offset is calculated from v_adc while the gain errors directly use v_raw_ave.  This is because the
  subtraction between v_raw_ave and cal_adc->nominal_gain MUST be done in integer as 32-bit floating point is
  not sufficiently accurate once v_raw > 10,000,000.
\*---------------------------------------------------------------------------------------------------------*/
{
    float       v_adc;
    float       inv_adc_gain;

    // Calculate specified DCCT calibration error (ppm) normalised to T0

    inv_adc_gain = cal_adc->inv_gain * 1.0E6 / CAL_V_NOMINAL;

    switch(idx)                 // Switch on calibration index
    {
    case CAL_OFFSET_V:

        v_adc = cal_adc->inv_gain * (float)v_raw_ave *
                (1.0 - (v_raw_ave < 0 ? cal_adc->gain_err_neg : cal_adc->gain_err_pos)) - cal_adc->offset_v;

        dcct->offset_ppm       = v_adc * 1.0E6 / CAL_V_NOMINAL
                               - calTempCompensation(CAL_OFFSET_V, dcct_temp_c, dcct_temp_coeffs, d_dcct_temp_coeffs);
        break;

    case CAL_GAIN_ERR_POS:

        dcct->gain_err_pos_ppm = (float)(v_raw_ave - cal_adc->nominal_gain) * inv_adc_gain
                               - dcct->offset_ppm
                               - cal_adc->offset_v * 1.0E6 / CAL_V_NOMINAL
                               - cal_adc->gain_err_pos * 1.0E6
                               - head_err_ppm
                               - calTempCompensation(CAL_OFFSET_V,     dcct_temp_c, dcct_temp_coeffs, d_dcct_temp_coeffs)
                               - calTempCompensation(CAL_GAIN_ERR_POS, dcct_temp_c, dcct_temp_coeffs, d_dcct_temp_coeffs);
        break;

    case CAL_GAIN_ERR_NEG:

        dcct->gain_err_neg_ppm = (float)(-v_raw_ave - cal_adc->nominal_gain) * inv_adc_gain
                               + dcct->offset_ppm
                               + cal_adc->offset_v * 1.0E6 / CAL_V_NOMINAL
                               - cal_adc->gain_err_neg * 1.0E6
                               - head_err_ppm
                               + calTempCompensation(CAL_OFFSET_V,     dcct_temp_c, dcct_temp_coeffs, d_dcct_temp_coeffs)
                               - calTempCompensation(CAL_GAIN_ERR_NEG, dcct_temp_c, dcct_temp_coeffs, d_dcct_temp_coeffs);
        break;

    default:  break;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void calEventStamp(struct cal_event *event, uint32_t unix_time, float temp_c)
/*---------------------------------------------------------------------------------------------------------*\
  Timescale: Daily

  This function will store the temperature and date/time in the calibration event structure.
\*---------------------------------------------------------------------------------------------------------*/
{
    event->temp_c    = temp_c;
    event->date_days = (float)(unix_time / 86400);
    event->time_s    = (float)(unix_time % 86400);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t calEventUnixtime(const struct cal_event *event)
/*---------------------------------------------------------------------------------------------------------*\
  This function will convert the event timestamp into unixtime.
\*---------------------------------------------------------------------------------------------------------*/
{
    return((uint32_t)event->date_days * 86400 + (uint32_t)event->time_s);
}
/*---------------------------------------------------------------------------------------------------------*/
unsigned calAverageVraw(struct cal_average_v_raw *average_v_raw, unsigned num_samples, int32_t v_raw)
/*---------------------------------------------------------------------------------------------------------*\
  This function calculates the average v_raw value for large number of samples.  To initialise an
  average the function must be called with num_samples set to the number of samples to average:

        calAverageVraw(&cal_average_v_raw, 200, 0);                 // First call

  Then the function should be called for this number of acquisitions with num_samples set to zero
  and v_raw set to the raw ADC value. The function will return the number of acquisitions remaining.
  Once this reaches zero the average v_raw value will be available in average_v_raw->v_raw:

        remaining = calAverageVraw(&cal_average_v_raw, 0, v_raw);   // Subsequent calls, until remaining == 0

\*---------------------------------------------------------------------------------------------------------*/
{
    if(num_samples)                             // If number of samples to average is non-zero
    {
        average_v_raw->num_samples        = num_samples;         // Initialise average
        average_v_raw->num_samples_to_acq = 0;
    }
    else                                        // else
    {
        if(!average_v_raw->num_samples_to_acq)          // if first sample
        {
            average_v_raw->v_raw_0            = v_raw;
            average_v_raw->v_raw_ave          = 0;
            average_v_raw->num_samples_to_acq = average_v_raw->num_samples - 1;
        }
        else                                            // else
        {
            average_v_raw->v_raw_ave += (v_raw - average_v_raw->v_raw_0);   // Accumulate v_raw

            if(--average_v_raw->num_samples_to_acq == 0)             // If last sample, calculate average v_raw
            {
                average_v_raw->v_raw_ave /= (int32_t)average_v_raw->num_samples;
                average_v_raw->v_raw_ave += average_v_raw->v_raw_0;
            }
        }
    }

    return(average_v_raw->num_samples_to_acq);
}
/*---------------------------------------------------------------------------------------------------------*/
void calTempFilterInit(struct cal_temp_filter *temp_filter, float period_s, float time_constant_s)
/*---------------------------------------------------------------------------------------------------------*\
  This function should be called once to prepare the temperature filter structure
\*---------------------------------------------------------------------------------------------------------*/
{
    temp_filter->temp_c = CAL_TEMP_T0;                          // Initialise the temperature filter to T0
    temp_filter->factor = period_s / time_constant_s;           // Calculate filter factor
}
/*---------------------------------------------------------------------------------------------------------*/
float calTempFilter(struct cal_temp_filter *temp_filter, float temp_c)
/*---------------------------------------------------------------------------------------------------------*\
  Timescale: ~1 s

  This function is used to filter a temperature measurement using a first order filter with time the
  time constant given to calTemperatureInit().  Typically the temperature might be measured every 10s, but this
  function is called every second and the time constant is chosen to model the thermal inertia of the
  ADC or DCCT circuit board (60-100s is typical).
\*---------------------------------------------------------------------------------------------------------*/
{
    temp_filter->temp_c += (temp_c - temp_filter->temp_c) * temp_filter->factor;        // 1st order filter

    return(temp_filter->temp_c);                // Return filtered temperature
}
/*---------------------------------------------------------------------------------------------------------*/
void calDacInitRaw(const struct cal_adc *cal_adc, const int32_t v_raw_ave[CAL_NUM_ERRS],
                   struct cal_dac *cal_dac, unsigned resolution, int32_t dac_raw)
/*---------------------------------------------------------------------------------------------------------*\
  This function will initialise the DAC calibration parameters based on the raw calibration measurements made
  using the supplied +/- dac_raw values (and zero).
\*---------------------------------------------------------------------------------------------------------*/
{
    unsigned    i;
    float       v_adc[3];

    // Convert ADC raw averages into volts

    for(i=0 ; i < 3 ; i++)
    {
        v_adc[i] = cal_adc->inv_gain * (float)v_raw_ave[i] *
               (1.0 - (v_raw_ave[i] < 0 ? cal_adc->gain_err_neg : cal_adc->gain_err_pos)) - cal_adc->offset_v;
    }

    // Calculate DAC calibration gains

    calDacInit(v_adc, cal_dac, resolution, dac_raw);
}
/*---------------------------------------------------------------------------------------------------------*/
void calDacInit(const float v_adc[CAL_NUM_ERRS], struct cal_dac *cal_dac, unsigned resolution, int32_t dac_raw)
/*---------------------------------------------------------------------------------------------------------*\
  This function will initialise the DAC calibration parameters based on the calibration measurements made
  using the supplied +/- d values (and zero).
\*---------------------------------------------------------------------------------------------------------*/
{
    // Calculate DAC calibration gains

    cal_dac->v_offset = v_adc[CAL_OFFSET_V];
    cal_dac->gain_pos = (float)dac_raw / (v_adc[CAL_GAIN_ERR_POS] - v_adc[CAL_OFFSET_V]);
    cal_dac->gain_neg = (float)dac_raw / (v_adc[CAL_OFFSET_V] - v_adc[CAL_GAIN_ERR_NEG]);

    // Calculate DAC raw value range

    cal_dac->min_dac_raw = -(1 << (resolution-1));
    cal_dac->max_dac_raw = -(cal_dac->min_dac_raw + 1);

    // Calculate DAC voltage range

    cal_dac->max_v_dac = cal_dac->v_offset + (float)cal_dac->max_dac_raw / cal_dac->gain_pos;
    cal_dac->min_v_dac = cal_dac->v_offset + (float)cal_dac->min_dac_raw / cal_dac->gain_neg;
}
/*---------------------------------------------------------------------------------------------------------*/
int32_t calDacSet(const struct cal_dac *cal_dac, float v_dac)
/*---------------------------------------------------------------------------------------------------------*\
  This function converts the DAC voltage parameter into a raw value to be send to the DAC which it returns.
  The DAC calibration is defined by two linear gains, one for positive values and the other
  for negative values.  No temperature compensation is supported.

                V_RAW = (V_DAC - V_OFFSET) x GAIN     <-->      V_DAC = V_OFFSET + V_RAW / GAIN
\*---------------------------------------------------------------------------------------------------------*/
{
    int32_t      dac_raw;

    // Calculate raw DAC value based on calibration

    v_dac -= cal_dac->v_offset;

    if(v_dac >= 0.0)
    {
        dac_raw = v_dac * cal_dac->gain_pos;
    }
    else
    {
        dac_raw = v_dac * cal_dac->gain_neg;
    }

    // Clip raw value to raw DAC range

    if(dac_raw > cal_dac->max_dac_raw)
    {
	dac_raw = cal_dac->max_dac_raw;
    }
    else if(dac_raw < cal_dac->min_dac_raw)
    {
	dac_raw = cal_dac->min_dac_raw;
    }

    return(dac_raw);
}
/*---------------------------------------------------------------------------------------------------------*\
  End of file: cal.c
\*---------------------------------------------------------------------------------------------------------*/

