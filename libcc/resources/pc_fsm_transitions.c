/*!
 *  @file     pc_fsm_transitions.c
 *  @defgroup FGC:MCU
 *  @brief    File providing the definition of the transitions for the Power
 *            Converter Finite State Machine.
 *
 *  The functions TCtoCY() and STtoBK() are implemented either in
 *  pc_fsm_transitions_common.c or pc_fsm_transitions_class.c.
 */

#define PC_FSM_TRANSITIONS_GLOBALS

// Includes

#include <fgc_pc_fsm_transitions.h>
#include <fgc_pc_fsm_states.h>
#include <cc_types.h>
#include <state_manager.h>
#include <sta.h>
#include <dev.h>
#include <memmap_mcu.h>
#include <fbs_class.h>
#include <fbs.h>
#include <cal.h>
#include <macros.h>
#include <dpcls.h>

// Internal functions declaration

/*!
 * Returns True if the device is not blockable or the device is blocked and
 * the unblock command is not requested. Returns False otherwise.
 *
 * @retval True if blocked. False otherwise.
 */
static INLINE BOOLEAN DeviceBlocked(void);

/*!
 * Returns True if the device is not blockable or the device is unblocked and
 * the unblock command is requested. Returns FALSE otherwise.
 *
 * @retval True if unblocked. False otherwise.
 */
static INLINE BOOLEAN DeviceUnblocked(void);

// Internal function definitions

static INLINE BOOLEAN DeviceBlocked(void)
{
    return (!vs.blockable ||
            Test(sta.inputs, DIG_IP1_OPBLOCKED_MASK16));
}

static INLINE BOOLEAN DeviceUnblocked(void)
{
    return (!vs.blockable ||
            !Test(sta.inputs, DIG_IP1_OPBLOCKED_MASK16));
}

// External function definitions

BOOLEAN FOtoOF(void)
{
    /*
     * Condition(s): No faults present and all permits asserted.
     *
     * The actual state means that the PC is OFF but one or more faults were
     * latched so a reset of fault is expected before doing this check.
     *
     * DIG_IP1_PWRFAILURE_MASK16 == 1 active when:
     *   VSFAULT present | VSEXTINTLK present |
     *   (DCCTAFLT and DCCTBFLT both present) |
     *   (DIG_OP_RST_FGCOKCMD executed)
     *
     *  Which is equivalent to:
     *  FGC_FLT_VS_FAULT | FGC_FLT_VS_EXTINTLOCK | FGC_FLT_I_MEAS |
     *  (read DIG_OP_SET_FGCFLTCMD_MASK16 == 0)
     *
     *  Checking against FGC_FLT_FAST_ABORT and not against DIG_IP1_FASTABORT_MASK16
     *  Checking against FGC_FLT_NO_PC_PERMIT and not against DIG_IP1_PCPERMIT_MASK16
     */

    return (!Test(sta.inputs, DIG_IP1_PWRFAILURE_MASK16) &&
            !Test(FAULTS, FGC_FLT_FAST_ABORT | FGC_FLT_NO_PC_PERMIT)
           );
}

BOOLEAN OFtoFO(void)
{
    /*
     *  Condition(s): Any fault present or the permits not asserted.
     *
     *  DIG_IP1_PWRFAILURE_MASK16 == 1 active when:
     *        VSFAULT present | VSEXTINTLK present |
     *        (DCCTAFLT and DCCTBFLT both present) |
     *        (DIG_OP_RST_FGCOKCMD executed)
     *
     *  Which is equivalent to:
     *  FGC_FLT_VS_FAULT | FGC_FLT_VS_EXTINTLOCK | FGC_FLT_I_MEAS |
     *  (read DIG_OP_SET_FGCFLTCMD_MASK16 == 0)
     *
     *  Checking against FGC_FLT_FAST_ABORT and not against DIG_IP1_FASTABORT_MASK16
     *  Checking against FGC_FLT_NO_PC_PERMIT and not against DIG_IP1_PCPERMIT_MASK16
     */

    return (Test(sta.inputs, DIG_IP1_PWRFAILURE_MASK16) ||
            Test(FAULTS, FGC_FLT_FAST_ABORT | FGC_FLT_NO_PC_PERMIT));
}

BOOLEAN OFtoST(void)
{
    /*
     *  Condition(s): BLOCKING   >cmd: s pc bk
     *                ON_STANDBY >cmd: s pc sb
     *                IDLE       >cmd: s pc il
     *                CYCLING    >cmd: s pc cy
     *                DIRECT     >cmd: s pc dt
     */

    return (FGC_PC_BLOCKING   == sta.mode_pc ||
            FGC_PC_ON_STANDBY == sta.mode_pc ||
            FGC_PC_IDLE       == sta.mode_pc ||
            FGC_PC_CYCLING    == sta.mode_pc ||
            FGC_PC_DIRECT     == sta.mode_pc
           );
}

BOOLEAN FStoFO(void)
{
    /*
     *  Condition(s): The VS power stage is not powered (the power converter is OFF).
     *
     *  PC is going OFF in fast mode, the current is not following the FGC reference.
     *
     *  DIG_IP1_VSPOWERON_MASK16
     *     0: VS power stage is not powered
     *     1: VS power stage is powered
     *
     *  DIG_IP1_VSREADY_MASK16
     *    0: VS is not ready
     *    1: VS has finished its initialisation
     *
     *  DIG_IP1_VSRUN_MASK8 is the readback of the DIG_OP_SET_VSRUNCMD
     *    0: VSRUN command is inactive
     *    1: VSRUN command is active
     */

    return (!Test(sta.inputs, DIG_IP1_VSPOWERON_MASK16) &&
            !Test(sta.inputs, DIG_IP1_VSREADY_MASK16)   &&
            !Test(sta.inputs, DIG_IP1_VSRUN_MASK16));
}

BOOLEAN SPtoOF(void)
{
    /*
     *  Condition(s): The VS power stage is not powered (the power converter is OFF).
     *
     *  DIG_IP1_VSPOWERON_MASK16
     *     0: VS power stage is not powered
     *     1: VS power stage is powered
     *
     *  DIG_IP1_VLOOPOK_MASK16
     *    0: VS is not ready
     *    1: VS has finished its initialisation
     *
     *  DIG_IP1_VSRUN_MASK8 is the readback of the DIG_OP_SET_VSRUNCMD
     *        0: VSRUN command is inactive
     *        1: VSRUN command is active
     *
     *  NOTE: delay of 100 ms. to prevent an unlikely race condition
     *        when ST->SP if s pc OFF.
     */

    return (!Test(sta.inputs, DIG_IP1_VSPOWERON_MASK16) &&
            !Test(sta.inputs, DIG_IP1_VSREADY_MASK16)   &&
            !Test(sta.inputs, DIG_IP1_VSRUN_MASK16)     &&
            sta.time_ms > 100L);
}

#if (FGC_CLASS_ID == 62)
BOOLEAN STtoBK(void)
{
    /*
     *  Condition(s): The VS power stage is powered (the power converter is ON).
     *                And the device is not blocked.
     */

    return (Test(sta.inputs, DIG_IP1_VSPOWERON_MASK16) &&
            DeviceBlocked());
}
#else
BOOLEAN STtoBK(void)
{
    /*
     *  Condition(s): The VS power stage is powered (the power converter is ON).
     *                And the device is not blocked.
     */

    return (Test(sta.inputs, DIG_IP1_VSPOWERON_MASK16) &&
            Test(sta.inputs, DIG_IP1_VSREADY_MASK16)   &&
            DeviceBlocked());
}
#endif

BOOLEAN STtoSP(void)
{
    /*
     *  Condition(s): The operator wants to go to: OFF >cmd: s pc of
     */

    return (FGC_PC_OFF == sta.mode_pc);
}

BOOLEAN BKtoSP(void)
{
    /*
     *  Condition(s): The operator wants to go to: OFF >cmd: s pc of
     *
     *  Checking for errors conditions is not needed because XXtoFS() is
     *  always invoked before BKtoSP().
     */

    return (sta.mode_pc == FGC_PC_OFF);
}

BOOLEAN BKtoTS(void)
{
    /*
     *  Condition(s): ON_STANDBY >cmd: s pc sb
     *                IDLE       >cmd: s pc il
     *                CYCLING    >cmd: s pc cy
     *                                             DIRECT     >cmd: s pc dt
     *                And if the device is unblocked.
     */

    return (DeviceUnblocked()                 &&
            (sta.mode_pc == FGC_PC_IDLE       ||
             sta.mode_pc == FGC_PC_CYCLING    ||
             sta.mode_pc == FGC_PC_DIRECT     ||
             sta.mode_pc == FGC_PC_ON_STANDBY));
}

BOOLEAN SAtoBK(void)
{
    /*
     *  Condition(s): force_slow_abort_f = FALSE
     *                And cmd: s pc off.
     *                Or I = Istart
     *                And the device is blocked.
     */

    return (DeviceBlocked()                &&
            ((sta.mode_pc == FGC_PC_OFF    &&
              !sta.force_slow_abort_f)     ||
             REF_FUNC_TYPE == FGC_REF_NONE));
}

BOOLEAN SAtoTS(void)
{
    /*
     *  Condition(s): If the operator wants to go to: ON_STANDBY >cmd: s pc sb
     *                And force_slow_abort_f = FALSE
     *
     *  Note: there is no need to check for PC_PERMIT because force_slow_abort_f
     *        is reset only if >cmd: s pc SB, which is not allowed if PC_PERMIT
     *        is not asserted.
     */

    return (sta.mode_pc == FGC_PC_ON_STANDBY && !sta.force_slow_abort_f);
}

BOOLEAN SAtoAB(void)
{
    /*
     *  Condition(s): The operator wants to go to: IDLE >cmd: s pc il
     *                And force_slow_abort_f = FALSE
     *
     *  Note: there is no need to check for PC_PERMIT because force_slow_abort_f
     *        is reset only if >cmd: s pc IL, which is not allowed if PC_PERMIT
     *        is not asserted.
     */

    return (sta.mode_pc == FGC_PC_IDLE && !sta.force_slow_abort_f);
}

BOOLEAN TStoSB(void)
{
    /*
     *  Condition(s): The standby reference has been reached.
     *                 And FGC_PC_TS_SETTLE_TIME has elapsed.
     */

    return (REF_FUNC_TYPE == FGC_REF_NONE &&
            sta.time_ms    > FGC_REF_SETTLE_TIME_MS);
}

BOOLEAN TStoAB(void)
{
    /*
     *  Condition(s): The operator wants to go to: IDLE >cmd: s pc il
     *                And the current is above Imin.
     *
     *  There are two scenarios that lead to this transition:
     *  1) Whilst in TS, the user issues s pc IL.
     *  2) The FGC is powered ON when the circuit is already energized. If the
     *     user wanted to go to IDLE, the chain of transitions would be
     *     OFF->ST->BK->TS.
     *  From TS the path to IL diverges depending on I:
     *  a) I  > Imin TS->AB->IL or it is 4Q, then TS->AB->IL
     *  b) I <= Imin and unipolar in current (1Q, 2Q), then TS->SB->IL
     *  For case a), bringing the current down to Imin must be aborted via the
     *  AB state.
     */

    return (sta.mode_pc == FGC_PC_IDLE &&
            ((INT16U)dpcls.dsp.meas.i_min_f ||
             !dpcls.dsp.unipolar_f));
}

BOOLEAN SBtoIL(void)
{
    /*
     *  Condition(s): The operator wants to go to: IDLE >cmd: s pc il
     *                And the reason to force standby is no longer valid,
     *                which includes the sector access interlock not set
     *                or the DSP not requesting to go to SB.
     */

    return (sta.mode_pc == FGC_PC_IDLE &&
            sta.force_to_standby_f == FALSE);
}

BOOLEAN SBtoTC(void)
{
    /*
     *  Condition(s): The operator wants to go to: CYCLING >cmd: s pc cy
     *                And the reason to force standby is no longer valid,
     *                which includes the sector access interlock not set
     *                or the DSP not requesting to go to SB.
     */

    return (sta.mode_pc == FGC_PC_CYCLING &&
            sta.force_to_standby_f == FALSE);
}

BOOLEAN ILtoTC(void)
{
    /*
     *  Condition(s): The operator wants to go to: CYCLING >cmd: s pc cy
     *                And coasting is no longer required. TBD how.
     */

    return (sta.mode_pc == FGC_PC_CYCLING);
}

BOOLEAN ILtoAR(void)
{
    /*
     *  Condition(s): A new reference has been armed.
     */

    return (REF_FUNC_TYPE == FGC_REF_ARMED);
}

BOOLEAN TCtoIL(void)
{
    /*
     *  Condition(s): The operator wants to go to: IDLE >cmd: s pc il
     */

    return (sta.mode_pc == FGC_PC_IDLE);
}

#if (FGC_CLASS_ID == 62)
BOOLEAN TCtoCY(void)
{
    /*
     *  Condition(s): The operator wants to go to: CYCLING >cmd: s pc cy
     *                VS_READY has been asserted, that is, the converter can
     *                reach the requested reference within the current cycle.
     */

    return (sta.mode_pc == FGC_PC_CYCLING &&
            Test(sta.inputs, DIG_IP1_VSREADY_MASK16));
}
#else
BOOLEAN TCtoCY(void)
{
    /*
     *  Condition(s): The operator wants to go to: CYCLING >cmd: s pc cy
     *                Currently, a one second delay is added but eventually a
     *                more sophisticated technique to synchronize with ref and
     *                time has to be implemented.
     */

    return (sta.mode_pc == FGC_PC_CYCLING &&
            sta.time_ms > 1000L);
}
#endif

BOOLEAN ARtoIL(void)
{
    /*
     *  Condition(s): The operator wants to go to: IDLE >cmd: s pc il
     */

    return (REF_FUNC_TYPE == FGC_REF_NONE);
}

BOOLEAN ARtoRN(void)
{
    /*
     *  Condition(s): The reference is no longer armed nor none.
     */

    return (REF_FUNC_TYPE != FGC_REF_ARMED &&
            REF_FUNC_TYPE != FGC_REF_NONE);
}

BOOLEAN RNtoIL(void)
{
    /*
     *  Condition(s): The reference value has been reached.
     */

    return (REF_FUNC_TYPE == FGC_REF_NONE);
}

BOOLEAN RNtoAB(void)
{
    /*
     *  Condition(s): Aborting reference armed.
     */

    return (REF_FUNC_TYPE == FGC_REF_ABORTING);
}

BOOLEAN ABtoTS(void)
{
    /*
     *  Condition(s): I < Imin for 1/2Q converters.
     *
     *  After transitioning from TS->AB - see comments in TStoAB() to understand
     *  the need for this transition - the state AB might end up bringing I below
     *  Imin. If this is the case, the current must be brought back up to Imin via
     *  TS, at which point the transition to IDLE is done via AB->TS->SB->IL.
     */

    return (!(INT16U)dpcls.dsp.meas.i_min_f &&
            dpcls.dsp.unipolar_f            &&
            REF_FUNC_TYPE == FGC_REF_NONE);
}

BOOLEAN ABtoIL(void)
{
    /*
     *  Condition(s): The operator wants to go to: IDLE >cmd: s pc il
     *                And the aborting is complete -> reference is none
     */

    return (sta.mode_pc == FGC_PC_IDLE &&
            REF_FUNC_TYPE == FGC_REF_NONE);
}

BOOLEAN CYtoIL(void)
{
    /*
     *  Condition(s): The operator wants to go to: IDLE >cmd: s pc il
     */

    return (sta.mode_pc == FGC_PC_IDLE);
}

BOOLEAN CYtoEC(void)
{
    /*
     *  Condition(s): TBD
     */

    return FALSE;
}

BOOLEAN PLtoOF(void)
{
    /*
     *  Condition(s): The polarity switch has complete and mode.pc is FGC_PC_OFF.
     */

    return (dpcls.mcu.vs.polarity.state != FGC_POL_SWITCH_MOVING &&
            sta.mode_pc == FGC_PC_OFF);
}

BOOLEAN PLtoBK(void)
{
    /*
     *  Condition(s): The polarity switch has complete and mode.pc is FGC_PC_BLOCKING.
     */

    return (dpcls.mcu.vs.polarity.state != FGC_POL_SWITCH_MOVING &&
            sta.mode_pc == FGC_PC_BLOCKING);
}

BOOLEAN PLtoTC(void)
{
    /*
     *  Condition(s): TBD
     *
     *  Note: Class 53 (LHC) has no polarity switching capabilities.
     */

    return FALSE;
}

BOOLEAN ECtoIL(void)
{
    /*
     *  Condition(s): The operator wants to go to: IDLE >cmd: s pc il
     */

    return (sta.mode_pc == FGC_PC_IDLE);
}

BOOLEAN ECtoCY(void)
{
    /*
     *  Condition(s): Reference has reached the operational reference.
     */

    return FALSE;
}

BOOLEAN XXtoFS(void)
{
    /*
     *  Actual state: FGC_PC_STOPPING,   FGC_PC_STARTING,   FGC_PC_BLOCKING
     *                FGC_PC_TO_STANDBY, FGC_PC_ON_STANDBY, FGC_PC_IDLE,
     *                FGC_PC_TO_CYCLING, FGC_PC_ARMED,      FGC_PC_RUNNING,
     *                FGC_PC_ABORTING,   FGC_PC_CYCLING,    FGC_PC_POL_SWITCHING,
     *                FGC_PC_DIRECT
     *  Condition(s): FAST_ABORT issued or power failure flag set.
     *                Or no PC_PERMIT when the converter is OFF.
     *
     *  DIG_IP1_PWRFAILURE_MASK16 == 1 active
     *  When:  VSFAULT present or VSEXTINTLK present or DCCTAFLT and DCCTBFLT both present
     *         or DIG_OP_RST_FGCOKCMD executed
     *
     *  Equivalent to:
     *  FGC_FLT_VS_FAULT or FGC_FLT_VS_EXTINTLOCK or FGC_FLT_I_MEAS or read
     *  DIG_OP_SET_FGCFLTCMD_MASK16 == 0
     *
     *  DIG_IP1_FASTABORT_MASK16
     *    0: PC_FAST_ABORT signal (from the PIC) was not received by the VS
     *    1: VS has received the PC_FAST_ABORT signal from the PIC
     */

    BOOLEAN failure = Test(sta.inputs, DIG_IP1_PWRFAILURE_MASK16 |
                           DIG_IP1_FASTABORT_MASK16);

    BOOLEAN no_pc_permit = Test(FAULTS, FGC_FLT_NO_PC_PERMIT);

    return (failure || no_pc_permit);
}

BOOLEAN XXtoSA(void)
{
    /*
     *  Actual state: FGC_PC_TO_STANDBY, FGC_PC_ON_STANDBY, FGC_PC_IDLE,
     *                FGC_PC_TO_CYCLING, FGC_PC_ARMED,      FGC_PC_RUNNING,
     *                FGC_PC_ABORTING,   FGC_PC_CYCLING,    FGC_PC_DIRECT
     *
     *  Condition(s): The operator wants to go to: SLOW_ABORT >cmd: s pc sa
     *                                             OFF        >cmd: s pc of
     *                                             BLOCKING   >cmd: s pc bk
     *                Or PC_PERMIT is no longer granted.
     *
     *  NOTE: sta.force_slow_abort_f is set if PC PERMIT is not present of s pc sa.
     */

    return (sta.force_slow_abort_f          ||
            sta.mode_pc == FGC_PC_BLOCKING  ||
            sta.mode_pc == FGC_PC_OFF);
}

BOOLEAN XXtoTS(void)
{
    /*
     *  Actual state: FGC_PC_IDLE,    FGC_PC_TO_CYCLING, FGC_PC_ARMED,
     *                FGC_PC_RUNNING, FGC_PC_ABORTING,   FGC_PC_CYCLING,
     *                FGC_PC_DIRECT
     *  Condition(s): The operator wants to go to: ON_STANDBY >cmd: s pc sb
     *                Or force_to_standby_f = TRUE
     *                Or whilst in DIRECT:  IDLE    >cmd: s pc il
     *                                      CYCLING >cmd: s pc cy.
     */

    return (sta.mode_pc == FGC_PC_ON_STANDBY ||
            sta.force_to_standby_f           ||
            (STATE_PC == FGC_PC_DIRECT       &&
             (sta.mode_pc == FGC_PC_IDLE     ||
              sta.mode_pc == FGC_PC_CYCLING)));
}

BOOLEAN XXtoTC(void)
{
    /*
     *  Actual state: FGC_PC_ARMED, FGC_PC_RUNNING, FGC_PC_ABORTING
     *  Condition(s): The operator wants to go to: SLOW_ABORT >cmd: s pc cy
     */

    return (sta.mode_pc == FGC_PC_CYCLING);
}

BOOLEAN XXtoPL(void)
{
    /*
     *  Actual state: FGC_PC_OFF, FGC_PC_BLOCKING, FGC_PC_TO_CYCLING,
     *                FGC_PC_CYCLING
     *  Condition(s): TBD
     */

    return (dpcls.mcu.vs.polarity.state == FGC_POL_SWITCH_MOVING);
}

BOOLEAN XXtoDT(void)
{
    /*
     *  Actual state: FGC_PC_ON_STANDBY, FGC_PC_IDLE,    FGC_PC_TO_CYCLING,
     *                FGC_PC_ARMED       FGC_PC_RUNNING, FGC_PC_ABORTING,
     *                FGC_PC_TO_CYCLING
     *  Condition(s): The operator wants to go to: DIRECT >cmd: s pc dt
     */

    return (sta.mode_pc == FGC_PC_DIRECT);
}

// EOF
