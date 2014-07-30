/*!
 *  @file     pc_fsm.c
 *  @defgroup FGC:MCU
 *  @brief    Defines PC_FSM_GLOBALS to define variables in fgc_pc_fsm.h.
 *
 *            Definition for ValidTargetState() and helper functions. For an explanation on the
 *            conditions needed to transition to a given target state, refer to:
 *            http://cs-ccr-www3.cern.ch/~poccdev/git/gitweb.cgi?p=projects/fgc/
 *                   doc.git;a=tree;f=state_pc;h=b9d2ce7c82768ecf2b93f8ad50b450696f9c2e19;hb=HEAD
 */

#define PC_FSM_GLOBALS

// Includes

#include <fgc_pc_fsm.h>
#include <pc_state.h>
#include <fgc_errs.h>
#include <macros.h>
#include <fbs_class.h>
#include <pub.h>
#include <dpcls.h>
#include <dev.h>
#include <log.h>
#include <prop.h>

// Internal functions declaration

/*!
 * Do not reset if a FW DIODE or FABORT_UNSAFE fault was received. This condition
 * is not valid for class 53 however.
 *
 * @retval FGC_FW_DIODE_FAULT if FW DIODE in fault; FGC_FABORT_UNSAFE if not safe;
 *         FGC_OK_NO_RSP if ok.
 */
static INLINE INT16U RestartIfPossible(void);

/*!
 * Check if PM logging is active in the GW and the PM flag is still set.
 *
 * @retval FGC_LOG_WAITING if PM is active. FGC_OK_NO_RSP otherwise.
 */
static INLINE INT16U CheckPostMortemActive(void);

/*!
 * Check if the device is not PPM and NON-PPM reference (0) is not armed.
 *
 * @retval FGC_OK_NO_RSP if ok. FGC_BAD_STATE otherwise.
 */
static INLINE INT16U CheckNoPPMNoRef0(void);

/*!
 * Verifies that the target state is valid based on the current state FAULT_OFF.
 *
 * @retval FGC_OK_NO_RSP if ok. FGC_BAD_STATE otherwise.
 */
static INLINE INT16U CheckPCPermit(void);

/*!
 * Returns True if the access has been granted and I > Iaccess.
 *
 * @retval True if access is granted. False otherwise.
 */
static INLINE BOOLEAN SectorAccessInterlock(void);

/*!
 * Verifies that the target_state sent by the operator is valid based on the current
 * state and status of the system.
 *
 * @param current_state Current state.
 * @param target_state State to which the operator wants to transition to.
 *
 * @retval FGC_OK_NO_RSP if ok. FGC_BAD_STATE otherwise.
 */
static INT16U PcFsmValidteState(INT16U current_state, INT16U target_state);

/*
 * The functions below verify that the target state is valid based on the current state.
 * They return FGC_OK_NO_RSP if ok. FGC_BAD_STATE otherwise.
 */
static INT16U PcFsmValidateFO(INT16U target_state);
static INT16U PcFsmValidateOF(INT16U target_state);
static INT16U PcFsmValidateFS(INT16U target_state);
static INT16U PcFsmValidateSP(INT16U target_state);
static INT16U PcFsmValidateST(INT16U target_state);
static INT16U PcFsmValidateSA(INT16U target_state);
static INT16U PcFsmValidateTS(INT16U target_state);
static INT16U PcFsmValidateSB(INT16U target_state);
static INT16U PcFsmValidateIL(INT16U target_state);
static INT16U PcFsmValidateTC(INT16U target_state);
static INT16U PcFsmValidateAR(INT16U target_state);
static INT16U PcFsmValidateRN(INT16U target_state);
static INT16U PcFsmValidateAB(INT16U target_state);
static INT16U PcFsmValidateCY(INT16U target_state);
static INT16U PcFsmValidatePL(INT16U target_state);
static INT16U PcFsmValidateBK(INT16U target_state);
static INT16U PcFsmValidateEC(INT16U target_state);
static INT16U PcFsmValidateDT(INT16U target_state);

// Internal function definitions

static INLINE INT16U RestartIfPossible(void)
{
#if (FGC_CLASS_ID == 61 || FGC_CLASS_ID == 62)

    if (vs.fw_diode == FGC_VDI_FAULT)
    {
        return FGC_FW_DIODE_FAULT;
    }

    if (vs.fabort_unsafe == FGC_VDI_FAULT)
    {
        return FGC_FABORT_UNSAFE;
    }

#endif

    Set(sta.cmd, DDOP_CMD_RESET);

    return FGC_OK_NO_RSP;
}

static INLINE INT16U CheckPostMortemActive(void)
{
    return ((dev.pm_enabled_f && Test(ST_UNLATCHED, FGC_UNL_POST_MORTEM)) ?
            FGC_LOG_WAITING : FGC_OK_NO_RSP);
}

static INLINE INT16U CheckNoPPMNoRef0(void)
{
    return ((!DEVICE_PPM && dpcls.dsp.ref.func.type[0] != FGC_REF_NONE) ?
            FGC_BAD_STATE : FGC_OK_NO_RSP);
}

static INLINE INT16U CheckPCPermit(void)
{
    return (!Test(sta.inputs, DIG_IP1_PCPERMIT_MASK16) ?
            FGC_BAD_STATE : FGC_OK_NO_RSP);
}

static INLINE BOOLEAN SectorAccessInterlock(void)
{
    return (fbs.sector_access == FGC_CTRL_ENABLED &&
            (INT16U)dpcls.dsp.meas.i_access_f);
}

static INT16U PcFsmValidateFO(INT16U target_state)
{
    return ((target_state == FGC_PC_OFF) ? RestartIfPossible() : FGC_BAD_STATE);
}

static INT16U PcFsmValidateOF(INT16U target_state)
{
    INT16U retval;

    if (target_state == FGC_PC_OFF)
    {
        retval = RestartIfPossible();
    }
    else if (target_state == FGC_PC_BLOCKING   ||
             target_state == FGC_PC_ON_STANDBY ||
             target_state == FGC_PC_DIRECT)
    {
        retval = CheckPostMortemActive();
    }
    else if (target_state == FGC_PC_IDLE)
    {
        retval = (CheckNoPPMNoRef0() != FGC_OK_NO_RSP ? FGC_BAD_STATE : CheckPostMortemActive());
    }
    else if (target_state == FGC_PC_CYCLING)
    {
        retval = (DEVICE_CYC ? CheckPostMortemActive() : FGC_BAD_STATE);
    }
    else
    {
        retval = FGC_BAD_STATE;
    }

    return retval;
}

static INT16U PcFsmValidateFS(INT16U target_state)
{
    return FGC_BAD_STATE;
}

static INT16U PcFsmValidateST(INT16U target_state)
{
    return ((target_state == FGC_PC_SLOW_ABORT) ? FGC_BAD_STATE :
            ((target_state == FGC_PC_IDLE) ? CheckNoPPMNoRef0() : FGC_OK_NO_RSP)
           );
}

static INT16U PcFsmValidateSP(INT16U target_state)
{
    return ((target_state == FGC_PC_OFF) ? FGC_OK_NO_RSP : FGC_BAD_STATE);
}

static INT16U PcFsmValidateBK(INT16U target_state)
{
    return ((target_state == FGC_PC_CYCLING && !DEVICE_CYC) ? FGC_BAD_STATE :
            ((target_state == FGC_PC_IDLE) ? CheckNoPPMNoRef0() : FGC_OK_NO_RSP)
           );
}

static INT16U PcFsmValidateSA(INT16U target_state)
{
    INT16U retval;

    if (target_state == FGC_PC_BLOCKING ||
        target_state == FGC_PC_ON_STANDBY)
    {
        retval = CheckPCPermit();
    }
    else if (target_state == FGC_PC_IDLE)
    {
        retval = (CheckNoPPMNoRef0() != FGC_OK_NO_RSP ? FGC_BAD_STATE : CheckPCPermit());
    }
    else if (target_state == FGC_PC_CYCLING ||
             target_state == FGC_PC_DIRECT)
    {
        retval = FGC_BAD_STATE;
    }
    else
    {
        retval = FGC_OK_NO_RSP;
    }

    return retval;
}

static INT16U PcFsmValidateTS(INT16U target_state)
{
    return ((target_state == FGC_PC_CYCLING && !DEVICE_CYC) ? FGC_BAD_STATE :
            ((target_state == FGC_PC_IDLE) ? CheckNoPPMNoRef0() : FGC_OK_NO_RSP)
           );
}

static INT16U PcFsmValidateSB(INT16U target_state)
{
    INT16U retval;

    if (target_state == FGC_PC_BLOCKING ||
        target_state == FGC_PC_IDLE     ||
        target_state == FGC_PC_DIRECT)
    {
        retval = CheckPostMortemActive();
    }
    else if (target_state == FGC_PC_CYCLING)
    {
        retval = (!DEVICE_CYC ? FGC_BAD_STATE : CheckPostMortemActive());
    }
    else
    {
        retval = FGC_OK_NO_RSP;
    }

    return retval;
}

static INT16U PcFsmValidateIL(INT16U target_state)
{
    return (target_state == FGC_PC_CYCLING && !DEVICE_CYC) ?
           FGC_BAD_STATE : FGC_OK_NO_RSP;
}

static INT16U PcFsmValidateTC(INT16U target_state)
{
    return (target_state == FGC_PC_IDLE ? FGC_BAD_STATE : FGC_OK_NO_RSP);
}

static INT16U PcFsmValidateAR(INT16U target_state)
{
    return (target_state == FGC_PC_CYCLING && !DEVICE_CYC) ?
           FGC_BAD_STATE : FGC_OK_NO_RSP;
}

static INT16U PcFsmValidateRN(INT16U target_state)
{
    return (target_state == FGC_PC_CYCLING && !DEVICE_CYC) ?
           FGC_BAD_STATE : FGC_OK_NO_RSP;
}

static INT16U PcFsmValidateAB(INT16U target_state)
{
    return (target_state == FGC_PC_CYCLING && !DEVICE_CYC) ?
           FGC_BAD_STATE : FGC_OK_NO_RSP;
}

static INT16U PcFsmValidateCY(INT16U target_state)
{
    return FGC_OK_NO_RSP;
}

static INT16U PcFsmValidatePL(INT16U target_state)
{
    return ((target_state == FGC_PC_OFF      ||
             target_state == FGC_PC_BLOCKING ||
             target_state == FGC_PC_CYCLING) ?
            FGC_OK_NO_RSP : FGC_BAD_STATE);
}

static INT16U PcFsmValidateEC(INT16U target_state)
{
    return ((target_state == FGC_PC_OFF        ||
             target_state == FGC_PC_SLOW_ABORT ||
             target_state == FGC_PC_ON_STANDBY ||
             target_state == FGC_PC_IDLE) ?
            FGC_OK_NO_RSP : FGC_BAD_STATE);
}

static INT16U PcFsmValidateDT(INT16U target_state)
{
    return ((target_state == FGC_PC_CYCLING && !DEVICE_CYC) ? FGC_BAD_STATE :
            ((target_state == FGC_PC_IDLE) ? CheckNoPPMNoRef0() : FGC_OK_NO_RSP)
           );
}

static INT16U PcFsmValidteState(INT16U current_state, INT16U target_state)
{
    // List of the local precondition functions: more efficient.

    static INT16U(*validation_funcs[])(INT16U) =
    {
        PcFsmValidateFO, PcFsmValidateOF, PcFsmValidateFS, PcFsmValidateSP, PcFsmValidateST,
        PcFsmValidateSA, PcFsmValidateTS, PcFsmValidateSB, PcFsmValidateIL, PcFsmValidateTC,
        PcFsmValidateAR, PcFsmValidateRN, PcFsmValidateAB, PcFsmValidateCY, PcFsmValidatePL,
        PcFsmValidateBK, PcFsmValidateEC, PcFsmValidateDT
    };

    if (current_state >= PC_FSM_NUM_STATES)
    {
        return FGC_BAD_STATE;
    }

    if (vs.blockable == FGC_CTRL_DISABLED &&
        target_state == FGC_PC_BLOCKING)
    {
        return FGC_BAD_STATE;
    }

    // Do not allow transitioning above ON_STANDBY when the access
    // interlock has been asserted.

    if (PcStateCmpAboveEqual(target_state, FGC_PC_ON_STANDBY) &&
        SectorAccessInterlock())
    {
        return FGC_BAD_STATE;
    }

    return validation_funcs[current_state](target_state);
}

// External function definitions

void PcFsmProcessState(void)
{
    // pc_simplifed maps the current operational state (index of array)
    // to the corresponding simplified state (value in the array).

    static INT8U const pc_simplified[] = { FGC_PC_SIMPLIFIED_FAULT,    // FO
                                           FGC_PC_SIMPLIFIED_OFF,      // OF
                                           FGC_PC_SIMPLIFIED_FAULT,    // FS
                                           FGC_PC_SIMPLIFIED_OFF,      // SP
                                           FGC_PC_SIMPLIFIED_OFF,      // ST
                                           FGC_PC_SIMPLIFIED_ON,       // SA
                                           FGC_PC_SIMPLIFIED_ON,       // TS
                                           FGC_PC_SIMPLIFIED_ON,       // SB
                                           FGC_PC_SIMPLIFIED_ON,       // IL
                                           FGC_PC_SIMPLIFIED_ON,       // TC
                                           FGC_PC_SIMPLIFIED_ON,       // AR
                                           FGC_PC_SIMPLIFIED_ON,       // RN
                                           FGC_PC_SIMPLIFIED_ON,       // AB
                                           FGC_PC_SIMPLIFIED_ON,       // CY
                                           FGC_PC_SIMPLIFIED_ON,       // PL
                                           FGC_PC_SIMPLIFIED_BLOCKING, // BK
                                           FGC_PC_SIMPLIFIED_ON,       // EC
                                           FGC_PC_SIMPLIFIED_ON        // DT
                                         };

    INT8U  const * transition;
    INT16U         num_trans;

    // Set flag to move to or stay in ON_STANDBY if:
    // not in simulation and the access interlock has been raised.
    // For now, this is only meaningful for the LHC.

    sta.force_to_standby_f = ((sta.mode_op != FGC_OP_SIMULATION ||
                               !vs.sim_intlks)                  &&
                              SectorAccessInterlock());

    // TO_STANDBY request from DSP

    if (dpcls.dsp.cyc.to_standby_f)
    {
        if (STATE_PC == FGC_PC_CYCLING)
        {
            sta.force_to_standby_f = TRUE;
        }
        else if (STATE_PC == FGC_PC_ON_STANDBY)
        {
            dpcls.dsp.cyc.to_standby_f = FALSE;
            sta.force_to_standby_f = FALSE;
        }
    }

    // If PC_PERMIT is not present and PC state is above SLOW_ABORT, transition
    // to SLOW_ABORT to gracefully bring I to Istart.

    if (!Test(sta.inputs, DIG_IP1_PCPERMIT_MASK16) &&
        PcStateAbove(FGC_PC_SLOW_ABORT))
    {
        sta.force_slow_abort_f = TRUE;
    }

    // Run PC state machine

    transition = pc_states[STATE_PC].trans;

    // Scan transitions

    for (num_trans = pc_states[STATE_PC].n_trans;
         num_trans && !pc_transitions[*transition].condition();
         num_trans--, transition++)
    {
        ; // Do nothing.
    }

    if (num_trans)
    {
        sta.time_ms = 0L;
        num_trans = pc_transitions[*transition].next_state;
        dpcls.mcu.state_pc = num_trans;

        pc_states[num_trans].state_func(TRUE);

        // Update state machine after first call to StateXX() allowing these
        // functions to retrieve the old state.

        STATE_PC = num_trans;

        // Update and publish the simplified PC state only if it changed.

        if (sta.state_pc_simplified != pc_simplified[num_trans])
        {
            sta.state_pc_simplified = pc_simplified[num_trans];
            PubProperty(&PROP_STATE_PC_SIMPLIFIED, NON_PPM_USER, TRUE);
        }
    }
    else
    {
        pc_states[STATE_PC].state_func(FALSE);
    }
}

INT16U PcFsmSetState(struct cmd * c, INT8U target_state)
{
    INT16U errnum = 0;
    INT16U current_state;

    current_state = STATE_PC;

    errnum = PcFsmValidteState(current_state, target_state);

    if (errnum == FGC_OK_NO_RSP)
    {
        // If the current state is OFF and the target state is valid, then we
        // are leaving the OFF state. This is the time to restart the logging.

        if (current_state == FGC_PC_OFF)
        {
            LogStartAll();
        }

#if (FGC_CLASS_ID == 61)

        // If the target state is IDLE:
        //   When in ARMED, clear reference.
        //   When in RUNNING, set the abort reference to run now.
        //   When in SLOW_ABORT, set the abort reference to run now.
        //   When in TO_STANDBY, set the abort reference to run now.
        // Note that this cannot be done in StateAB() because in the case of
        // asynchronous abort events, the abort reference would be set twice.
        // Using REF is the only way to transition to IDLE since mode_pc is
        // already IDLE.

        if (target_state == FGC_PC_IDLE)
        {
            if (current_state == FGC_PC_SLOW_ABORT ||
                current_state == FGC_PC_TO_STANDBY ||
                current_state == FGC_PC_RUNNING)
            {
                // The statement below triggers the DSP to run the ABORT function.

                dpcom.mcu.evt.abort_event_delay = 1;
            }

            if (current_state == FGC_PC_ARMED)
            {
                RefArm(c, NON_PPM_USER, FGC_REF_NONE, STC_NONE);
            }
        }

#endif

        // If the current state is SA and the user wants to transition to
        // OFF, SB or IL, then reset the force_slow_abort_f flag.
        // IMPORTANT: this check must precede the one below, which might set
        // target_state to FGC_PC_OFF.

        if (current_state == FGC_PC_SLOW_ABORT  &&
            (target_state != FGC_PC_OFF        ||
             target_state != FGC_PC_ON_STANDBY ||
             target_state != FGC_PC_IDLE)
           )
        {
            sta.force_slow_abort_f = FALSE;
        }

        // SLOW_ABORT is a transitional state to OFF. That is why sta.mode_pc
        // is set to OFF. sta.force_slow_abort_f is asserted for the state
        // machine to distinguish from s pc OFF and be able to do the
        // transitions SA->BK->SP->OFF

        if (target_state == FGC_PC_SLOW_ABORT)
        {
            target_state = FGC_PC_OFF;
            sta.force_slow_abort_f = TRUE;
        }

        // Update mode.pc only if the value changed.

        if (sta.mode_pc != target_state)
        {
            sta.mode_pc = target_state;
            PubProperty(&PROP_MODE_PC, NON_PPM_USER, TRUE);
        }
    }

    return (errnum);
}

// EOF
