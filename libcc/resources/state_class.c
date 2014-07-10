/*---------------------------------------------------------------------------------------------------------*\
 File:          state51.c

 Purpose:       FGC HC16 Class 51 Software - PC state Functions.

 Author:        Quentin.King@cern.ch

 Notes:         This file contains the functions for the PC state machine.  The generic state
                machine engine is in sta.c.  The states and transitions are defined in states.def.
                These functions are called every five milliseconds.  The first time a function is
                called, the state value in STATE_PC contains the old state and first_f is TRUE.
\*---------------------------------------------------------------------------------------------------------*/

#define DEFPROPS_INC_ALL    // defprops.h

#include <state_class.h>
#include <cmd.h>
#include <fbs_class.h>  // for FAULTS, ST_UNLATCHED, STATE_OP
#include <defprops.h>
#include <definfo.h>    // for FGC_CLASS_ID
#include <dev.h>        // for dev global variable
#include <fbs.h>        // for fbs global variable
#include <dpcom.h>      // for dpcom global variable
#include <crate.h>      // for crate global variable
#include <macros.h>     // for Test(), Set()
#include <sta.h>        // for sta global variable
#include <sta_class.h>  // for vs global variable, DDOP_CMD_RESET, DDOP_CMD_OFF, StaDdip()
#include <dpcls.h>      // for dpcls global variable
#include <cal_class.h>  // for cal global variable, REF_STC_ARMED_FUNC_TYPE, NON_PPM_REG_MODE
#include <log_class.h>  // for timing_log, log_iab global variables, FGC_LAST_LOG
#include <mem.h>        // for MemSetBytes()
#include <ref_class.h>  // for RefClr()
#include <macros.h>     // for Test(), Set(), Clr() macros


/*---------------------------------------------------------------------------------------------------------*/
void StateFO(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  FAULT_OFF state function.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(first_f)
    {
        Clr(sta.flags,(STAF_START      |                // Clear START flag
                       STAF_SLOW_ABORT |                // Clear SLOW_ABORT flag in case it is still set
                       STAF_ABORTING   |                // Clear ABORTING flag in case it is still set
                       STAF_TO_CYCLING));               // Clear TO_CYCLING flag
    }

#if FGC_CLASS_ID == 51
    if(dev.log_pm_state == FGC_LOG_ARMED        &&      // If PM log is still armed, and
       !Test(ST_UNLATCHED,FGC_UNL_LOW_CURRENT) &&       // current is still > 10% of I_POS, and
       (FAULTS & (~FGC_FLT_NO_PC_PERMIT)))              // any fault other than NO_PC_PERMIT is set
    {
        log_iab.run_f    = FALSE;                       // Stop current IAB    logging
        log_iearth.run_f = FALSE;                       // Stop current IEARTH logging
        log_ileads.run_f = FALSE;                       // Stop current ILEADS logging
        log_ireg.run_f  = FALSE;                        // Stop current IREG   logging
        log_thour.run_f  = FALSE;                       // Stop current THOUR  logging
        dev.log_pm_state = FGC_LOG_STOPPING;
    }
#endif

    if(fbs.id &&                                        // If FIP connected, and
      !Test(ST_UNLATCHED,FGC_UNL_POST_MORTEM) &&        // Post mortem not yet requested, and
       dev.log_pm_state == FGC_LOG_STOPPING   &&        // PM log is stopping, and
      (FAULTS & (~FGC_FLT_NO_PC_PERMIT)))               // any fault other than NO_PC_PERMIT is set
    {
        Set(ST_UNLATCHED,FGC_UNL_POST_MORTEM);          // Set post mortem flag
        Set(fbs.u.fieldbus_stat.ack,FGC_SELF_PM_REQ);   // Request self-trig PM dump by GW
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void StateOF(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  OFF state function.
\*---------------------------------------------------------------------------------------------------------*/
{
}
/*---------------------------------------------------------------------------------------------------------*/
void StateFS(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  FAULT_STOPPING state function.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(first_f)                                 // If coming from another state
    {
        if(dev.log_pm_state == FGC_LOG_ARMED)           // If PM log is still armed
        {
#if FGC_CLASS_ID == 51
            log_iab.run_f    = FALSE;                   // Stop current IAB    logging
            log_iearth.run_f = FALSE;                   // Stop current IEARTH logging
            log_ileads.run_f = FALSE;                   // Stop current ILEADS logging
            log_ireg.run_f  = FALSE;                    // Stop current IREG   logging
#else
            log_cycle.run_f  = FALSE;                   // Stop current POPS  logging
#endif
            log_thour.run_f  = FALSE;                   // Stop current THOUR  logging
            dev.log_pm_state = FGC_LOG_STOPPING;        // Set PM log state to STOPPING
        }

        RefClr();                                       // Clear reference including range

        if(Test(sta.inputs,DIG_IP1_VSRUN_MASK16))       // If VS_RUN is still active
        {
            Set(sta.cmd,DDOP_CMD_OFF);                          // Switch off VS_RUN
        }

        dpcom.mcu.diag.freeze_all_dim_logs = TRUE;
    }

    if(!sta.first_faults && FAULTS)             // If first fault(s) have arrived
    {
        sta.first_faults = FAULTS;                      // Save faults which caused the trip
    }

    if(fbs.id &&                                        // If FIP connected, and
      !Test(ST_UNLATCHED,FGC_UNL_POST_MORTEM) &&        // Post mortem not yet requested, and
       dev.log_pm_state == FGC_LOG_STOPPING    &&       // PM log is stopping
      (FAULTS & (~FGC_FLT_NO_PC_PERMIT)))               // any fault other than NO_PC_PERMIT is set
    {
        Set(ST_UNLATCHED,FGC_UNL_POST_MORTEM);                  // Set post mortem flag
        Set(fbs.u.fieldbus_stat.ack,FGC_SELF_PM_REQ);           // Request self-trig PM dump by GW
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void StateSP(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  STOPPING state function.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(first_f)                                         // If coming from another state
    {
        sta.flags |= STAF_STOP;                         // Force STOP flag to be active to stay in STOPPING
                                                        // until VSRUN is OFF and the converter has stopped
    }

    // NB: The 200ms delay below is introduced because the DSP will not switch to the REF_STOPPING reference
    //     (if it ever does) before a certain time and in particular ever since commit
    //     0c7e090ad07171f24524c9ac4edc7107b10e7c7a (Thu, 15 Mar 2012 10:25:13 +0000 (11:25 +0100)) the transition
    //     will occur in the Ref() function on the DSP side, which is executed on a regulation period. With a
    //     maximum regulation period of 100ms, a delay of 200 ms is more than enough for the DSP to react to the
    //     STOPPING state.

    if(sta.time_ms >= 200L                   &&         // If enough time for DSP to register state change and
       Test(sta.inputs,DIG_IP1_VSRUN_MASK16) &&         // converter is still running
       REF_FUNC_TYPE == FGC_REF_NONE)                   // and ref has now finished
    {
        Set(sta.cmd,DDOP_CMD_OFF);                              // Switch off VS_RUN to stop converter
        sta.time_ms = 0L;                                       // Reset state time for simulation of VSPOWERON
    }

    if(FGC_LAST_LOG.run_f &&                            // If logging is active and
       (!Test(sta.inputs,DIG_IP1_VSRUN_MASK16) ||       // VSRUN is off OR
        Test(sta.cmd,DDOP_CMD_OFF)))                    // the converter is being switched off now
    {
        FGC_LAST_LOG.run_f = FALSE;                             // Stop current IREG logging
        sta.flags &= ~STAF_STOP;                                // Clear STOP flag
        RefClr();                                               // Clear reference including range
        dpcom.mcu.diag.freeze_all_dim_logs = TRUE;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void StateST(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  STARTING state function.

  The condition for calibrating the DAC was chosen for these reasons:

  - 1/2Q converters have analogue voltage loops which can wind up their integrators if they see
    the DAC cal voltages (+/-8V).  If I > I_MIN then the software will try to ramp the current
    immediately and this will have a problem with the V LOOP card, so if the circuit is still
    discharging, then DAC calibration is suppressed.  If I < I_MIN then the software leaves
    enough time for the VLOOP to recover and unwind, so the problem is avoided and the DAC calibration
    can take place

  - The RPTK (RF modulator) converters are run in open loop.  They have very poor accuracy current sensors
    which drift and often I>I_MIN even when off, but the DAC calibration must always be done as they run
    in openloop.  So if REF.FUNC.REF_MODE[0] is V, then the DAC calibration will always be done.

  - 4Q converters are not ramped up the way that 1/2Q converters are, so there is never a problem with
    the VLOOP integrator and the DAC cal can always be done.

  - The POPS converter is controlled digitally so the DAC is not used and is not calibrated
\*---------------------------------------------------------------------------------------------------------*/
{
    if(first_f)                                         // If coming from another state
    {
        sta.flags &= ~STAF_STOP;
        sta.first_faults = 0;
        dpcom.mcu.diag.relaunch_dim_logging = TRUE;

        if(   (STATE_OP   != FGC_OP_SIMULATION)                 // If not simulating and
           && (crate.type != FGC_CRATE_TYPE_PC_POPS)            // not POPS converter and
           && (    !dpcls.dsp.unipolar_f                        // Converter is 4Q, or
                || (NON_PPM_REG_MODE == FGC_REG_V)              // Open loop, or
                || !Test(sta.flags,STAF_IMIN)                   // I < I_MIN
              )
           )
        {
            dpcls.mcu.cal.action = CAL_REQ_DAC;                 // Prepare for auto DAC calibration
            cal.seq_idx          = 0;
            STATE_OP             = FGC_OP_CALIBRATING;          // Start auto DAC calibration
        }

        dpcls.mcu.ref.stc_func_type = STC_STARTING;                     // This is because the DSP does not have the
                                                                        // definition of STC_STARTING
    }
    else                                                // else
    {
        if(STATE_OP != FGC_OP_CALIBRATING &&                    // If DAC calibration completed and
           !Test(sta.inputs,DIG_IP1_VSRUN_MASK16))              // VSRUN not yet activated
        {
            sta.flags &= ~STAF_START;                           // Clear START flag
            Set(sta.cmd,(DDOP_CMD_RESET|DDOP_CMD_ON));                  // Pulse Reset and activate VS_RUN
            dev.log_pm_state = FGC_LOG_ARMED;                           // Set PM log state to ARMED
            vs.vsrun_timeout_ms = sta.time_ms + (INT32S)(vs.vsrun_timeout * 1000 + 0.5); // Convert timeout to ms
        }

        if(Test(sta.inputs,DIG_IP1_VSRUN_MASK16) &&             // If VS_RUN is active and
           sta.time_ms > vs.vsrun_timeout_ms)                   // VS_RUN timeout expired
        {
            Set(FAULTS,FGC_FLT_VS_RUN_TO);                                      // Latch Run Timeout Fault
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void StateSA(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  SLOW_ABORT state function.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(first_f)                                 // If coming from another state
    {
        RefClr();                                                       // Clear reference but leaving range
        sta.flags &= ~(STAF_IDLE|STAF_TO_CYCLING|STAF_SLOW_ABORT);      // Clear IDLE, TO_CYCLING and SLOW ABORT flags
    }
    else                                        // else already in this state
    {
        if(!REF_STC_ARMED_FUNC_TYPE &&                          // If RTD ref is blank
           (REF_FUNC_TYPE == FGC_REF_TO_STANDBY ||              // and ref is now running TO_STANDBY
            REF_FUNC_TYPE == FGC_REF_STOPPING))                 // or ref is now running STOPPING
        {
            dpcls.dsp.ref.stc_func_type = STC_SLOW_ABORT;               // Report SLOW_ABORT on RTD
        }

        if(sta.time_ms == 20L &&                        // If still in SLOW_ABORT state after 20ms
           Test(sta.inputs,DIG_IP1_INTLKSPARE_MASK16))      // and INTLKSPARE digital input is active
        {
            Set(fbs.u.fieldbus_stat.ack,FGC_EXT_PM_REQ);        // Trigger an ext post-mortem dump by the GW
        }

        if(sta.time_ms > 200L &&                            // if more than 200ms since entering state
           REF_FUNC_TYPE == FGC_REF_NONE)                   // and ref is back to NONE
        {
            Set(sta.cmd,DDOP_CMD_OFF);                          // Switch off VS_RUN
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void StateTS(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  TO_STANDBY state function.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(first_f)                                 // If coming from another state
    {
        RefClr();                                       // Clear reference except range
        sta.flags |= STAF_TO_STANDBY;                   // Set TO_STANDBY flag to stay in this state
        sta.flags &= ~STAF_ABORTING;                    // Clear ABORTING flag in case it is set
                                                        // (this is to avoid toggling between states AB <--> TS)
    }
    else                                        // else already in this state
    {
        if(!REF_STC_ARMED_FUNC_TYPE &&                          // If RTD ref is blank
           REF_FUNC_TYPE == FGC_REF_TO_STANDBY)                 // and ref is now running TO_STANDBY
        {
            dpcls.dsp.ref.stc_func_type = STC_TO_STANDBY;               // Report TO_STANDBY on RTD
        }

        if(sta.time_ms > 200L)                          // If more than 200ms since entering state
        {
            if(   (Test(sta.flags,STAF_IDLE) &&                         // If going to IDLE
                   Test(sta.flags,STAF_IMIN) &&                         // and I >= IMIN and
                   REF_FUNC_TYPE != FGC_REF_STARTING)                   // ref has finished START state,
               || (REF_FUNC_TYPE == FGC_REF_NONE))                      // or, if ref is back to NONE
            {
                Clr(sta.flags,STAF_TO_STANDBY);                                 // Clear flag to move to IL
            }
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void StateSB(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  ON_STANDBY state function.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(first_f)                                 // If coming from another state
    {
        RefClr();                                       // Clear reference including range
        // Clear flags (set if coming from PPM)
        sta.flags &= ~(STAF_TO_STANDBY | STAF_TO_CYCLING);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void StateIL(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  IDLE state function.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(first_f)                                         // If coming from another state
    {
        RefClr();                                               // Clear reference including range

        // Clear all ref related flags

        sta.flags &= ~(STAF_IDLE | STAF_TO_STANDBY | STAF_ARMED);

        sta.flags &= ~(STAF_RUNNING | STAF_ABORTING);

        dev.log_pm_state = FGC_LOG_ARMED;                       // Set PM log state to ARMED

    }
    else if(REF_FUNC_TYPE == FGC_REF_ARMED)             // else if ref type now ARMED
    {
        sta.flags |= STAF_ARMED;                        // Set ARMED flag to move to ARMED
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void StateAR(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  ARMED state function.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(REF_FUNC_TYPE == FGC_REF_NONE)                   // If function has been aborted
    {
        sta.flags |= STAF_IDLE;                         // Set IDLE flag to move to IDLE state
    }
    else if(REF_FUNC_TYPE != FGC_REF_ARMED)             // else if function has started
    {
        sta.flags |= STAF_RUNNING;                      // Set RUNNING flag to move to RUNNING
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void StateRN(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  RUNNING state function.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(first_f)                                         // If coming from another state (ARMED)
    {
        sta.flags &= ~STAF_ARMED;                       // Clear ARMED flag
    }

    switch(REF_FUNC_TYPE)                               // Check for Run complete or Abort triggered
    {
        case FGC_REF_ABORTING:
            sta.flags |= STAF_ABORTING;
            break;
        case FGC_REF_NONE:
            sta.flags &= ~STAF_RUNNING;
            break;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void StateAB(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  ABORTING state function.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(first_f)                                         // If coming from another state (RUNNING)
    {
        sta.flags &= ~STAF_RUNNING;                     // Clear RUNNING flag
        sta.flags &= ~STAF_TO_STANDBY;                  // Clear TO_STANDBY flag to avoid toggling between TS <--> AB
    }

    if(REF_FUNC_TYPE == FGC_REF_NONE)                   // If abort complete
    {
        sta.flags &= ~STAF_ABORTING;                    // Clear flag to return to IDLE
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void StateTC(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  TO_CYCLING state function.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(first_f)                                 // If coming from TO_CYCLING
    {
        timing_log.out_idx = 0;
        MemSetBytes(&timing_log.length_bp,0,sizeof(timing_log.length_bp));      // Prepare cycle timestamp log
    }

    if(sta.time_ms > 1000L)                     // Wait for 1s
    {
        sta.flags |= STAF_CYCLING;                      // Set CYCLING flag
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void StateCY(BOOLEAN first_f)
/*---------------------------------------------------------------------------------------------------------*\
  CYCLING state function.
\*---------------------------------------------------------------------------------------------------------*/
{
#if FGC_CLASS_ID == 53
    if(first_f)                                         // If coming from TO_CYCLING
    {
        sta.flags &= ~STAF_TO_CYCLING;                          // Clear TO_CYCLING flag
        dev.log_pm_state = FGC_LOG_ARMED;                               // Set PM log state to ARMED
    }

    if(Test(sta.flags,STAF_TO_STANDBY) &&               // If return to ON_STANDBY is requested and
       REF_FUNC_TYPE == FGC_REF_NONE)                   // the reference has finished
    {
        sta.flags &= ~STAF_CYCLING;                             // Clear flag to leave CYCLING state

        if(fbs.id &&                                            // If FIP connected
           dpcls.dsp.cyc.fault.chk != FGC_CYC_FLT_NONE)         // and there is a cycling fault
        {
            log_cycle.run_f  = FALSE;                                   // Stop current POPS  logging
            log_thour.run_f  = FALSE;                                   // Stop current THOUR logging
            dev.log_pm_state = FGC_LOG_STOPPING;                        // Set PM log state to STOPPING

            Set(ST_UNLATCHED,FGC_UNL_POST_MORTEM);                      // Set post mortem flag
            Set(fbs.u.fieldbus_stat.ack,FGC_SELF_PM_REQ);               // Request self-trig PM dump by GW
        }
    }
#endif
}
/*---------------------------------------------------------------------------------------------------------*\
  End of file: state_class.c
\*---------------------------------------------------------------------------------------------------------*/
