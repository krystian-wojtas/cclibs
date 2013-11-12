/*---------------------------------------------------------------------------------------------------------*\
  File:     libcal.h                                                                    Copyright CERN 2011

  Purpose:  Calibration library header file

  Contact:  cclibs-devs@cern.ch

  Authors:  Quentin King

  Notes:    This is based on the theory presented in the calibration theory paper.
            Notes on function arguments:

            int32_t nominal_adc_gain

                Nominal ADC gain - this is the raw value corresponding to a step of
                CAL_V_NOMINAL on the ADC input.  The library uses 32-bit floating point variables
                which means there is no benefit from having a nominal ADC gain of greater than
                about 20 million.

            int32_t v_raw_ave

                When calibrating an ADC or DCCT, a reference voltage or current should be
                acquired by the ADC for about 10,000 samples and the average raw value provided
                to the corresponding calibration function.

            int32_t v_raw_ave[3]

                When calculating the nominal ADC gain or auto-calibrating an ADC, the average
                raw value for an input of zero, +CAL_V_NOMINAL and -CAL_V_NOMINAL should be
                supplied in the v_raw_ave array.  Each average should be of about 10,000 samples.

            struct cal_current *meas

                The structure returns the values associated with a current measurement: i_dcct,
                v_adc and v_raw.

            struct cal_voltage *meas

                The structure returns the values associated with a voltage measurement: v_meas,
                v_adc and v_raw.

            struct cal_event *cal

                This structure is used to hold the calibration errors for an ADC or a DCCT normalised
                to temperature CAL_TEMP_T0.  The event structure also contains a temperature and date/time
                stamp that can optionally be used to record when and at what temperature the last calibration
                was done.  To use this option a non-zero unix_time must be supplied.

            struct cal_limits *limits

                When calculating the calibration factors for a specified temperature the function
                calAdcFactors() or calDcctFactors() can be given a cal_limits structure defining
                warning and fault limits on the normalised calibration errors.  The offset limits
                can be centered on a non-zero value while the gain error limits are symmmetrical around zero.

            enum cal_idx idx

                When calculating the calibration errors for an ADC or DCCT, only one error (offset, positive
                gain error or negative gain error) can be calculated at a time.  The cal_idx argument
                indicates which error is being calibrated: CAL_OFFSET_V, CAL_GAIN_ERR_POS or CAL_GAIN_ERR_NEG

            struct cal_adc *cal_adc

                ADC calibration factors for a specified temperature.  This are based on the normalised
                calibration errors and the temperature coefficients for the ADC.  It should be recalculated
                whenever the temperature changes or the calibration errors are measured.

            struct cal_dcct *cal_dcct

                DCCT calibration factors for a specified temperature.  This are based on the normalised
                calibration errors and the temperature coefficients for the DCCT.  It should be recalculated
                whenever the temperature changes or the calibration errors are measured.

            struct cal_v_meas *cal_v_meas

                Voltage measurement calibration factor for any temperature.  A single gain factor is used
                without offset or temperature compensation.

            float adc_temp_c or dcct_temp_c

                ADC or DCCT temperature measurement in Celcius.

            float adc_temp_coeffs[3] or dcct_temp_coeffs[3]

                Linear temperature coefficients for an ADC or DCCT offset [0], positive gain error [1],
                and negative gain error [2] in ppm/C.  A null pointer can be given if temperature
                compensation is not required.

            float d_adc_temp_coeffs[3] or d_dcct_temp_coeffs[3]

                Second order temperature coefficient for an ADC or DCCT offset [0], positive gain error [1],
                negative gain error [2] in ppm at temp T1.  This parabola is defined so that the
                second order error is zero at temp T0 and T2.  A null pointer can be given if 2nd order
                temperature compensation is not required.

            float v_ref_err_ppm[3]

                When calibrating an ADC the PPM errors normalised to temp T0 in the +/- voltage references
                are supplied in v_ref_err_ppm[1] and v_ref_err_ppm[2].  The voltage references should be
                +/- CAL_V_NOMINAL. There is assumed to be no error in the short circuit used to measure
                the offset so v_ref_err_ppm[0] is not used but is present to keep symmetry with the
                cal_event and calibration factors where there is always a triad of values.

            float vref_temp_coeffs[3]

                If linear temperature coefficients for the voltage reference errors are known then they
                can be supplied in vref_temp_coeffs[1] and vref_temp_coeffs[2].  If temperature compensation
                is not in use then a null pointer can be supplied.

            struct cal_dac *cal_dac

                DAC calibration factors for any temperature.  The structure also contains the raw
                full scale range based on the DAC resolution and the assumption that the DAC register
                takes a signed value.  This is used to clip the calibrated value to avoid wrap-around.

            The DCCT/ADC calibration process works on five different timescales, and it is useful to
            understand which functions are used at which timescale.  The precise periods for these
            timescales depends on the particular application, but broadly for power converter controls
            they are as follows:

                1. Annual

                    Calibration of the on-board voltage reference at a given temperature using an
                    external DVM if present. By knowing the temperature coefficient for the reference
                    the calibration is normalised to T0.

                    Typically, the recalculation of the nominal ADC gain may be done at the same time.

                2. Daily

                    Automatic calibration of the ADCs using the on-board voltage reference if this
                    is supported by the interface hardware.  The process calculates the calibration
                    errors normalised to temperature T0.

                3. ~10 s

                    Measurement of the air temperatures near the ADC and DCCT and electronics.

                4. ~1 s

                    Filtering of the measured temperatures with a simple first order filter to
                    model the thermal inertia of the ADC and DCCT electronics.

                    Calculation of the ADC and DCCT calibration factors for the measured temperature
                    using the calibration errors normalised to T0 (calculated daily), and the
                    temperature coefficients.

                5. 0.05 - 1 ms

                    Use of the raw ADC values with the ADC and DCCT calibration factors (computed
                    every second) to calculate the DCCT currents.

\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBCAL_H
#define LIBCAL_H

#include <stdint.h>

// Library constants

#define CAL_V_NOMINAL           10.0                    // Nominal calibration voltage
#define CAL_TEMP_T0             23.0                    // T0 calibration temperature
#define CAL_TEMP_T1             28.0                    // T1 calibration temperature
#define CAL_TEMP_T2             33.0                    // T2 calibration temperature


// Types

enum cal_idx
{
    CAL_OFFSET_V,
    CAL_GAIN_ERR_POS,
    CAL_GAIN_ERR_NEG,
    CAL_NUM_ERRS
};

// Note: All the fields in the cal event structure are floats so that they can be represented as a single
// float array property.  This is not ideal but simplifies the storage of the FGC calibration data in
// the database.  It means that unix_time must be broken into two floats since one float has
// insufficient resolution to hold a 32 bit integer.

struct cal_event                                        // Calibration event for ADC or DCCT
{                                                       // PPM values are normalised to temp T0
    float               offset_ppm;                     // Voltage offset in PPM of CAL_V_NOMINAL
    float               gain_err_pos_ppm;               // Gain error in ppm of nominal gain for +ve values
    float               gain_err_neg_ppm;               // Gain error in ppm of nominal gain for -ve values
    float               temp_c;                         // Temperature when calibrated
    float               date_days;                      // Calibration date in days since 1970
    float               time_s;                         // Calibration time in seconds since midnight
};

struct cal_limits                                       // Calibration error limits for ADCs or DCCTs
{
    float               nominal_offset_ppm;             // Nominal voltage offset (ppm)
    float               offset_warning_ppm;             // Limits are nominal +/- warning level (ppm)
    float               offset_fault_ppm;               // Limits are nominal +/- fault level (ppm)
    float               gain_err_warning_ppm;           // Limits are +/- warning level (ppm)
    float               gain_err_fault_ppm;             // Limits are +/- fault level (ppm)
};

struct cal_average_v_raw
{
    unsigned            num_samples;                    // Total number of samples to acquire
    unsigned            num_samples_to_acq;             // Down counter of number of samples to acquire
    int32_t             v_raw_0;                        // Initial v_raw value
    int32_t             v_raw_ave;                      // Average v_raw once num_samples_to_acq == 0
};

struct cal_flags                                        // Calibration flags
{
    unsigned            warning;                        // Calibration warning flag
    unsigned            fault;                          // Calibration fault flag
};

struct cal_temp_filter                                  // Temperature filter
{
    float               temp_c;                         // Filtered temperature
    float               factor;                         // Period/time constant
};

struct cal_dac                                          // DAC calibration
{
    float               v_offset;                       // Measured DAC voltage for zero calibration
    float               gain_pos;                       // Gain (raw/V) for positive values
    float               gain_neg;                       // Gain (raw/V) for negative values
    int32_t             max_dac_raw;                    // Max raw value = 2^(resolution-1) - 1
    int32_t             min_dac_raw;                    // Min raw value = -2^(resolution-1)
    float               max_v_dac;                      // Maximum voltage that can be generated
    float               min_v_dac;                      // Minimum voltage that can be generated
};

// Measurement device calibration structures

struct cal_adc                                          // ADC calibration for a particular temperature
{
    int32_t             nominal_gain;                   // Nominal ADC gain (raw/Vnom)
    float               gain;                           // ADC gain (raw/V)
    float               inv_gain;                       // 1 / Nominal ADC gain (V/raw)
    float               offset_v;                       // Offset in voltage
    float               gain_err_pos;                   // Gain error factor for positive values
    float               gain_err_neg;                   // Gain error factor for negative values
    struct cal_flags    flags;                          // Fault and warning flags
};

struct cal_dcct                                         // DCCT calibration for a particular temperature
{
    float               gain;                           // DCCT head gain (V/A)
    float               inv_gain;                       // 1 / DCCT head gain (A/V)
    float               offset_v;                       // Offset in voltage
    float               gain_err_pos;                   // Gain error factor for positive values
    float               gain_err_neg;                   // Gain error factor for negative values
    struct cal_flags    flags;                          // Fault and warning flags
};

struct cal_v_meas                                       // Voltage measurement parameters for any temperature
{
    float               gain;                           // Voltage divider gain (Vadc/Vmeas)
    float               inv_gain;                       // 1 / Voltage divider gain (Vmeas/Vadc)
};

// Calibrated measurement structures

struct cal_current                                      // Acquired current measurement
{
    int32_t             v_raw;                          // Raw integer ADC value
    float               v_adc;                          // Calibrated voltage input to ADC
    float               i_dcct;                         // Calibrated current measured by DCCT
};

struct cal_voltage                                      // Acquired voltage measurement
{
    int32_t             v_raw;                          // Raw integer ADC value
    float               v_adc;                          // Calibrated voltage input to ADC
    float               v_meas;                         // Calibrated voltage measurement
};

// External functions

#ifdef __cplusplus
extern "C" {
#endif

void     calCurrent                 (const struct cal_dcct *cal_dcct, const struct cal_adc *cal_adc,
                                     int32_t v_raw, float i_dcct_sim, unsigned sim_f,
                                     struct cal_current *meas);

void     calVoltage                 (const struct cal_v_meas *cal_v_meas, const struct cal_adc *cal_adc,
                                     int32_t v_raw, float v_meas_sim, unsigned sim_f,
                                     struct cal_voltage *meas);

int32_t  calAdcNominalGain          (int32_t v_offset_raw_ave, int32_t v_pos_raw_ave,
                                     float adc_temp_c,
                                     const float adc_temp_coeffs[CAL_NUM_ERRS],
                                     const float d_adc_temp_coeffs[CAL_NUM_ERRS],
                                     float v_ref_err_ppm, const float *v_ref_temp_coeff);

void     calAdcFactors              (int32_t nominal_adc_gain, const struct cal_event *adc_t0,
                                     float adc_temp_c,
                                     const float adc_temp_coeffs  [CAL_NUM_ERRS],
                                     const float d_adc_temp_coeffs[CAL_NUM_ERRS],
                                     const struct cal_limits *limits, struct cal_adc *cal_adc);

void     calDcctFactors             (float nominal_gain, unsigned primary_turns, float head_err_ppm,
                                     const struct cal_event *dcct_t0,
                                     float dcct_temp_c,
                                     const float dcct_temp_coeffs  [CAL_NUM_ERRS],
                                     const float d_dcct_temp_coeffs[CAL_NUM_ERRS],
                                     const struct cal_limits *limits, struct cal_dcct *cal_dcct);

void     calVoltageDividerFactors   (float              nominal_gain,
                                     float              gain_err_ppm,
                                     struct cal_v_meas *cal_v_meas);

void     calAdcErrors               (const int32_t v_raw_ave[CAL_NUM_ERRS], int32_t nominal_adc_gain,
                                     float adc_temp_c,
                                     const float adc_temp_coeffs  [CAL_NUM_ERRS],
                                     const float d_adc_temp_coeffs[CAL_NUM_ERRS],
                                     const float v_ref_err_ppm    [CAL_NUM_ERRS],
                                     const float v_ref_temp_coeff [CAL_NUM_ERRS],
                                     struct cal_event *adc);

void     calAdcError                (enum cal_idx idx, int32_t v_raw_ave, int32_t nominal_adc_gain,
                                     float adc_temp_c,
                                     const float adc_temp_coeffs  [CAL_NUM_ERRS],
                                     const float d_adc_temp_coeffs[CAL_NUM_ERRS],
                                     const float v_ref_err_ppm    [CAL_NUM_ERRS],
                                     const float v_ref_temp_coeff [CAL_NUM_ERRS],
                                     struct cal_event *adc);

void     calDcctError               (enum cal_idx idx, int32_t v_raw_ave, const struct cal_adc *cal_adc,
                                     float dcct_temp_c,
                                     const float dcct_temp_coeffs  [CAL_NUM_ERRS],
                                     const float d_dcct_temp_coeffs[CAL_NUM_ERRS],
                                     float head_err_ppm, struct cal_event *dcct);

void     calEventStamp              (struct cal_event *event, uint32_t unix_time, float temp_c);

uint32_t calEventUnixtime           (const struct cal_event *event);

unsigned calAverageVraw             (struct cal_average_v_raw *average_v_raw, unsigned num_samples, int32_t v_raw);

void     calTempFilterInit          (struct cal_temp_filter *temp, float period_s, float time_constant_s);

float    calTempFilter              (struct cal_temp_filter *temp, float temp_c);

void     calDacInitRaw              (const struct cal_adc *cal_adc, const int32_t v_raw_ave[CAL_NUM_ERRS],
                                     struct cal_dac *cal_dac, unsigned resolution, int32_t dac_raw);

void     calDacInit                 (const float v_adc[CAL_NUM_ERRS], struct cal_dac *cal_dac,
                                     unsigned resolution, int32_t dac_raw);

int32_t  calDacSet                  (const struct cal_dac *cal_dac, float v_dac);

#ifdef __cplusplus
}
#endif

#endif

/*---------------------------------------------------------------------------------------------------------*\
  End of file: libcal.h
\*---------------------------------------------------------------------------------------------------------*/
