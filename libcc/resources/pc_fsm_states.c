/*!
 *  @file     pc_fsm_states.c
 *  @defgroup FGC:MCU
 *  @brief    Definition of the states for the Power Converter Finite State Machine.
 *
 *  Notes       : This file contains the functions for the PC state machine.  The generic state
 *                machine engine is in sta.c.  The states and transitions are defined in states.def.
 *                These functions are called every five milliseconds.  The first time a function is
 *                called, the state value in STATE_PC contains the old state and first_f is TRUE.
 *                signals from ACAPULCO
 *
 *     POWER_ON - DIG_IPDIRECT:b0 - DIG_IP1_VSPOWERON_MASK16
 *         0: VS power stage is not powered
 *         1: VS power stage is powered
 *
 *     VS_INIT_OK_NOT - DIG_IPDIRECT:b1 - inv(DIG_IP1_VSREADY_MASK16)
 *         0: VS has finished its initialisation
 *         1: VS is not ready
 *
 *     VS_EXTINTLK - DIG_IPDIRECT:b2 - DIG_IP1_VSEXTINTLK_MASK16
 *         0: VS has no external fault
 *         1: VS has an external fault
 *
 *     VS_FAULT - DIG_IPDIRECT:b3 - DIG_IP1_VSFAULT_MASK16
 *         0: VS has no internal fault
 *         1: VS has an internal fault
 *
 *     FAST_ABORT_MEM - DIG_IPDIRECT:b4 - DIG_IP1_FASTABORT_MASK16
 *     (in the FGC3 extansion panel STATUS is labeled : FAST ABORT)
 *         0: PC_FAST_ABORT signal (from PIC) was not received by the VS
 *         1: VS has received the PC_FAST_ABORT signal from PIC
 *
 *     VS_CABLE_OK_NOT - DIG_IPDIRECT:b5 - DIG_IP1_VSNOCABLE_MASK16
 *     (in the FGC3 extansion panel STATUS is labeled : NO VS CABLE)
 *         0: FGC3-VS cable is connected
 *         1: FGC3-VS cable is disconnected
 *
 *     IMEAS_A_FAULT - DIG_IPDIRECT:b6 - DIG_IP1_DCCTAFLT_MASK16
 *     (in the FGC3 extansion panel STATUS is labeled : IMEAS A ERR)
 *         0: channel A read OK
 *         1: channel A read not OK
 *
 *     IMEAS_B_FAULT - DIG_IPDIRECT:b7 - DIG_IP1_DCCTBFLT_MASK16
 *     (in the FGC3 extansion panel STATUS is labeled : IMEAS B ERR)
 *         0: channel B read OK
 *         1: channel B read not OK
 *
 *     OUTPUT_BLOCK_STATE - DIG_IPDIRECT:b8 - DIG_IP1_OPBLOCKED_MASK16
 *     (in the FGC3 extansion panel STATUS is labeled : OUTPUT BLOCKED)
 *         0: output power stage is enabled
 *         1: output power stage is blocked
 *
 *     PC_PERMIT_NOT - DIG_INTLK.IN:b0 - inv(DIG_IP1_PCPERMIT_MASK16)
 *         0: run of power converter allowed
 *         1: power converter to OFF controlled by the FGC3 (Slow Power Abort)
 */

#define DEFPROPS_INC_ALL
#define PC_FSM_STATES_GLOBALS

// Includes

#include <fgc_pc_fsm_states.h>
#include <pc_state.h>
#include <cmd.h>
#include <fbs_class.h>
#include <defprops.h>
#include <definfo.h>
#include <dev.h>
#include <fbs.h>
#include <DIMsDataProcess.h>
#include <crate.h>
#include <macros.h>
#include <sta.h>
#include <state_manager.h>
#include <dpcls.h>
#include <log_class.h>
#include <mem.h>
#include <cal.h>
#include <macros.h>

// Internal functions declarations

static void StateRefClr(void);

// Internal functions definitions

static void StateRefClr(void)

{
    OS_CPU_SR   cpu_sr;

    OS_ENTER_CRITICAL();

    dpcls.dsp.ref.stc_func_type = 0;

    if(dpcls.mcu.state_pc != FGC_PC_TO_STANDBY &&
       dpcls.mcu.state_pc != FGC_PC_SLOW_ABORT)
    {
        dpcls.dsp.ref.start = 0.0;
        dpcls.dsp.ref.end   = 0.0;
    }

    OS_EXIT_CRITICAL();
}

// External function definitions

void StateFO(BOOLEAN first_f)
{
    if ((dev.log_pm_state == FGC_LOG_ARMED)            // If PM log is still armed, and
        && !Test(ST_UNLATCHED, FGC_UNL_LOW_CURRENT)    // current is still > 10% of I_POS, and
        && (FAULTS & (~FGC_FLT_NO_PC_PERMIT))          // any fault other than NO_PC_PERMIT is set
       )
    {
#if (FGC_CLASS_ID == 61)
        log_iab.run_f    = FALSE;                       // Stop current IAB    logging
        log_ileads.run_f = FALSE;                       // Stop current ILEADS logging
        log_ireg.run_f   = FALSE;                       // Stop current IREG   logging
#endif
        log_iearth.run_f = FALSE;                       // Stop current IEARTH logging
        log_thour.run_f  = FALSE;                       // Stop current THOUR  logging
        dev.log_pm_state = FGC_LOG_STOPPING;
    }

    if (fbs.id                                          // If fieldbus connected, and
        && !Test(ST_UNLATCHED, FGC_UNL_POST_MORTEM)     // Post mortem not yet requested, and
        && (dev.log_pm_state == FGC_LOG_STOPPING)       // PM log is stopping, and
        && (FAULTS & (~FGC_FLT_NO_PC_PERMIT))           // any fault other than NO_PC_PERMIT is set
       )
    {
        Set(ST_UNLATCHED, FGC_UNL_POST_MORTEM);         // Set post mortem flag
        Set(fbs.u.fieldbus_stat.ack, FGC_SELF_PM_REQ);  // Request self-trig PM dump by GW
    }
}

void StateOF(BOOLEAN first_f)
{
    /*
     *  Setting DIG_OP_SET_VSRUNCMD_MASK16 will close the open collector VS_RUN_NOT output (pin D19).
     *  The hardwired logic includes a protection that prevents this action unless the PC_PERMIT_NOT external
     *  interlock is close.
     *  Note that if the external interlock PC_PERMIT_NOT becomes open, VS_RUN_NOT is NOT automatically deactivated,
     *  as this is considered to be a Slow Power Abort condition, and the voltage source must remain ON to
     *  allow for a controlled reduction in the current.
     *
     *  Reading back this bit will show the state of the driver exciting the open collector.
     *  This state is copied to DIG_IP1_VSRUN.
     *
     *  Note: This action will be deactivated automatically by the hardwired logic if FASTABORT or
     *  PWRFAILURE become active.
     *
     *  M32C87 p14.2 must be enable.
     *  RX610  P7.6  must be enable.
     */

    Set(sta.cmd, DDOP_CMD_BLOCKING);
    Set(sta.cmd, DDOP_CMD_OFF);
}

void StateFS(BOOLEAN first_f)
{
    if (first_f)                                        // If coming from another state
    {
        if (dev.log_pm_state == FGC_LOG_ARMED)          // If PM log is still armed
        {
#if (FGC_CLASS_ID == 61)
            log_iab.run_f    = FALSE;                       // Stop current IAB    logging
            log_ileads.run_f = FALSE;                       // Stop current ILEADS logging
            log_ireg.run_f   = FALSE;                       // Stop current IREG   logging
#endif
            log_iearth.run_f = FALSE;                       // Stop current IEARTH logging
            log_thour.run_f  = FALSE;                       // Stop current THOUR  logging
            dev.log_pm_state = FGC_LOG_STOPPING;        // Set PM log state to STOPPING
        }

        StateRefClr();                                  // Clear reference including range

        if (Test(sta.inputs, DIG_IP1_VSRUN_MASK16))     // If VS_RUN is still active
        {
            Set(sta.cmd, DDOP_CMD_OFF);                 // Switch off VS_RUN
        }

        qspi_misc.freeze_all_dim_logs = TRUE;

        // Activate VS output stage blocking (pin C19, OUTPUT_BLOCK) if FS was
        // reached without going through BK. This is possible in case of XXtoFS
        // that is, if FAST_ABORT or PWRFAILURE.

        if (Test(sta.inputs, DIG_IP1_PWRFAILURE_MASK16 | DIG_IP1_FASTABORT_MASK16))
        {
            Set(sta.cmd, DDOP_CMD_BLOCKING);
        }
    }

    if (fbs.id                                          // If FIP connected, and
        && !Test(ST_UNLATCHED, FGC_UNL_POST_MORTEM)     // Post mortem not yet requested, and
        && (dev.log_pm_state == FGC_LOG_STOPPING)       // PM log is stopping
        && (FAULTS & (~FGC_FLT_NO_PC_PERMIT))           // any fault other than NO_PC_PERMIT is set
       )
    {
        Set(ST_UNLATCHED, FGC_UNL_POST_MORTEM);         // Set post mortem flag
        Set(fbs.u.fieldbus_stat.ack, FGC_SELF_PM_REQ);  // Request self-trig PM dump by GW
    }
}

void StateSP(BOOLEAN first_f)
{
    if (first_f)
    {
        Set(sta.cmd, (DDOP_CMD_OFF | DDOP_CMD_BLOCKING));

        if (FGC_LAST_LOG.run_f)
        {
            FGC_LAST_LOG.run_f = FALSE;
            qspi_misc.freeze_all_dim_logs = TRUE;
        }
    }
}

void StateST(BOOLEAN first_f)
{
    /*
     *  The condition for calibrating the DAC was chosen for these reasons:
     *
     *  - 1/2Q converters have analogue voltage loops which can wind up their integrators if they see
     *    the DAC cal voltages (+/-8V).  If I > I_MIN then the software will try to ramp the current
     *    immediately and this will have a problem with the V LOOP card, so if the circuit is still
     *    discharging, then DAC calibration is suppressed.  If I < I_MIN then the software leaves
     *    enough time for the VLOOP to recover and unwind, so the problem is avoided and the DAC calibration
     *    can take place
     *
     *  - The RPTK (RF modulator) converters are run in open loop.  They have very poor accuracy current sensors
     *    which drift and often I>I_MIN even when off, but the DAC calibration must always be done as they run
     *    in openloop.  So if REF.FUNC.REF_MODE[0] is V, then the DAC calibration will always be done.
     *
     *  - 4Q converters are not ramped up the way that 1/2Q converters are, so there is never a problem with
     *    the VLOOP integrator and the DAC cal can always be done.
     *
     *  - The POPS converter is controlled digitally so the DAC is not used and is not calibrated
     */

    if (first_f)                                                // If coming from another state
    {
        qspi_misc.relaunch_dim_logging = TRUE;

        if ((STATE_OP   != FGC_OP_SIMULATION)                   // If not simulating and
            && (!dpcls.dsp.unipolar_f                           // Converter is 4Q, or
                || (NON_PPM_REG_MODE == FGC_REG_V)              // Open loop, or
                || !dpcls.dsp.meas.i_min_f                      // I < I_MIN
               )
           )
        {
            CalInitSequence(&cal_seq_dacs[0],                   // Calibration sequence for internal DACs
                            CAL_DACS,                           // Calibrate  DAC1 and DAC2
                            0);                                 // Last parameter (signal) is not relevant so zero.

            CalRunSequence();                                   // Run the first step of the calibration sequence
        }
    }
    else                                                        // else
    {
        if (STATE_OP != FGC_OP_CALIBRATING                      // If DAC calibration completed and
            && !Test(sta.inputs, DIG_IP1_VSRUN_MASK16)          // VSRUN not yet activated
           )
        {
            Set(sta.cmd, (DDOP_CMD_RESET | DDOP_CMD_ON | DDOP_CMD_BLOCKING));
            dev.log_pm_state = FGC_LOG_ARMED;                   // Set PM log state to ARMED
            vs.vsrun_timeout_ms = sta.time_ms + (INT32U)(vs.vsrun_timeout * 1000 + 0.5); // Convert timeout to ms
        }

        if (Test(sta.inputs, DIG_IP1_VSRUN_MASK16) &&           // If VS_RUN is active and
            (sta.time_ms > vs.vsrun_timeout_ms)                 // VS_RUN timeout expired
           )
        {
            Set(FAULTS, FGC_FLT_VS_RUN_TO);                     // Latch Run Timeout Fault
        }

        // If PC_PERMIT is not present, latch it in FAULTS. This will trigger XXtoFS().

        if (!Test(sta.inputs, DIG_IP1_PCPERMIT_MASK16))
        {
            Set(FAULTS, FGC_FLT_NO_PC_PERMIT);
        }
    }
}

void StateBK(BOOLEAN first_f)
{
    if (first_f)
    {
        // If sta.force_slow_abort_f is set and sta.mode_pc is not OFF, then
        // PC_PERMIT was removed at some point, which triggered XXtoSA() and
        // SAtoBK (when I=Istart). PC_PERMIT being removed must be latched
        // via FAULTS so that the PC ends up in FO via XXtoFS(). Note how at
        // that point mode_pc != OFF, which is OK since the operator must do
        // 's pc OFF' to get out of FO.

        if (sta.mode_pc != FGC_PC_OFF && sta.force_slow_abort_f)
        {
            Set(FAULTS, FGC_FLT_NO_PC_PERMIT);
        }

        // The flag is no longer needed so it can be reset.

        sta.force_slow_abort_f = FALSE;
    }
    else
    {
        // If PC_PERMIT is not present, latch it in FAULTS. This will trigger XXtoFS().

        if (!Test(sta.inputs, DIG_IP1_PCPERMIT_MASK16))
        {
            Set(FAULTS, FGC_FLT_NO_PC_PERMIT);
        }

        // The operator is moving out of BLOCKING. Unblock the PC.

        if (PcStateCmpAbove(sta.mode_pc, FGC_PC_BLOCKING))
        {
            Set(sta.cmd, DDOP_CMD_UNBLOCK);
        }
    }
}

void StateSA(BOOLEAN first_f)
{
    if (first_f)                                                // If coming from another state
    {
        StateRefClr();                                          // Clear reference but leaving range
    }
    else                                                        // else already in this state
    {
        if ((sta.time_ms >= 20L) &&                             // If still in SLOW_ABORT state after 20ms
            !Test(sta.inputs, DIG_IP1_PCPERMIT_MASK16))         // And PC_PERMIT is not present
        {
            fbs.u.fieldbus_stat.ack |= FGC_EXT_PM_REQ;          // Trigger an ext post-mortem dump by the GW
        }

        // Block the converter if we are going straight to OFF
        // (!sta.force_slow_abort_f) or the SLOW_ABORT function has completed.
        // The settling time gives the DSP some time to play the function.

        if (vs.blockable                            &&
            (!sta.force_slow_abort_f                ||
             (sta.time_ms > FGC_REF_SETTLE_TIME_MS  &&
              REF_FUNC_TYPE == FGC_REF_NONE)))
        {
            Set(sta.cmd, DDOP_CMD_BLOCKING);
        }

        if (!REF_STC_ARMED_FUNC_TYPE             &&             // If RTD ref is blank
                (REF_FUNC_TYPE == FGC_REF_TO_STANDBY ||             // and ref is now running TO_STANDBY
                        REF_FUNC_TYPE == FGC_REF_STOPPING))                // or ref is now running STOPPING
        {
            dpcls.dsp.ref.stc_func_type = STC_SLOW_ABORT;       // Report SLOW_ABORT on RTD
        }
    }
}

void StateTS(BOOLEAN first_f)
{
    if (first_f)
    {
        StateRefClr();                                          // Clear reference but leaving range
    }
    else
    {
        if (!REF_STC_ARMED_FUNC_TYPE &&                         // If RTD ref is blank
            REF_FUNC_TYPE == FGC_REF_TO_STANDBY)                // and ref is now running TO_STANDBY
        {
            dpcls.dsp.ref.stc_func_type = STC_TO_STANDBY;       // Report TO_STANDBY on RTD
        }
    }
}

void StateSB(BOOLEAN first_f)
{
    if (first_f)
    {
        StateRefClr();                                           // Clear reference including range
    }
}

void StateIL(BOOLEAN first_f)
{
    if (first_f)
    {
        StateRefClr();                                          // Clear reference including range
        dev.log_pm_state = FGC_LOG_ARMED;                       // Set PM log state to ARMED
    }
}

void StateAR(BOOLEAN first_f)
{
    ; // Does nothing.
}

void StateRN(BOOLEAN first_f)
{
    ; // Does nothing.
}

void StateAB(BOOLEAN first_f)
{
    ; // Does nothing.
}

void StateTC(BOOLEAN first_f)
{
    if (first_f)
    {
        if (STATE_PC == FGC_PC_CYCLING    &&                     // If coming from CYCLING
            REF_FUNC_TYPE == FGC_REF_NONE &&                     // the reference has finished
            fbs.id                        &&                     // If fieldbus connected
            dpcls.dsp.cyc.fault.chk != FGC_CYC_FLT_NONE          // and there is a cycling fault
           )
        {
            log_cycle.run_f  = FALSE;                            // Stop current POPS  logging
            log_thour.run_f  = FALSE;                            // Stop current THOUR logging
            dev.log_pm_state = FGC_LOG_STOPPING;                 // Set PM log state to STOPPING

            Set(ST_UNLATCHED, FGC_UNL_POST_MORTEM);              // Set post mortem flag
            fbs.u.fieldbus_stat.ack |= FGC_SELF_PM_REQ;          // Request self-trig PM dump by GW
        }

        // Prepare cycle time-stamp log
        timing_log.out_idx = 0;
        MemSetBytes(&timing_log.length_bp, 0, sizeof(timing_log.length_bp));
    }
}

#if (FGC_CLASS_ID == 62)
void StateCY(BOOLEAN first_f)
{
    if (first_f)
    {
        // Set PM log state to ARMED

        dev.log_pm_state = FGC_LOG_ARMED;

        vs.vs_ready_timeout_ms = 0;
    }
    else
    {
        // An important gap between two consecutive reference values might get
        // the converter to reset VS_READY until it can provide the requested
        // reference. This is acceptable for a given time as specified by
        // vs.vs_ready_timeout. After this timeout, the converter should trip.

        if (!Test(sta.inputs, DIG_IP1_VSREADY_MASK16))
        {
            if (vs.vs_ready_timeout_ms == 0)
            {
                vs.vs_ready_timeout_ms = sta.time_ms + vs.vs_ready_timeout;
            }
            else
            {
                if (sta.time_ms > vs.vs_ready_timeout_ms)
                {
                    // Trip the converter
                    Set(FAULTS, FGC_FLT_VS_FAULT);
                }
            }
        }
        else
        {
            vs.vs_ready_timeout_ms = 0;
        }

    }
}
#else
void StateCY(BOOLEAN first_f)
{
    if (first_f)
    {
        // Set PM log state to ARMED

        dev.log_pm_state = FGC_LOG_ARMED;
    }
}
#endif

void StatePL(BOOLEAN first_f)
{
    // @TODO
}

void StateEC(BOOLEAN first_f)
{
    // @TODO
}

void StateDT(BOOLEAN first_f)
{
    ; // Does nothing.
}

// EOF
