/*---------------------------------------------------------------------------------------------------------*\
  File:         transitions_class.h

  Contents:

  Notes:


\*---------------------------------------------------------------------------------------------------------*/

#ifndef TRANSITIONS_CLASS_H      // header encapsulation
#define TRANSITIONS_CLASS_H

#ifdef TRANSITIONS_CLASS_GLOBALS
    #define TRANSITIONS_CLASS_VARS_EXT
#else
    #define TRANSITIONS_CLASS_VARS_EXT extern
#endif
//-----------------------------------------------------------------------------------------------------------

#include <cc_types.h>           // basic typedefs
#include <state_class.h>        // for StateXX ...
#include <defconst.h>

//-----------------------------------------------------------------------------------------------------------

#define ArrayLen(arr)           ( sizeof arr / sizeof arr[0] )

// Transition function constants

// each one has it corresponding function
enum state_pc_transitions
{
    tr_OFtoFO,          //  0 off to fault (off)
    tr_FStoFO,          //  1 fault (stopping) to fault (off)
    tr_FOtoOF,          //  2 fault (off) to off
    tr_SPtoOF,          //  3 stopping to off
    tr_STtoFS,          //  4 starting to fault (stopping)
    tr_XXtoFS,          //  5 ??? to fault (stopping)
    tr_STtoSP,          //  6 starting to stopping
    tr_XXtoSP,          //  7 ??? to stopping
    tr_OFtoST,          //  8 off to starting
    tr_XXtoSA,          //  9 ??? to slow abort
    tr_STtoTS,          // 10 starting to to_standBy
    tr_XXtoTS,          // 11 ??? to to_standBy
    tr_TStoSB,          // 12 to_standBy to standBy
    tr_TStoAB,          // 13 to_standBy to aborting
    tr_SBtoIL,          // 14 standBy to idle
    tr_ARtoIL,          // 15 armed to idle
    tr_RNtoIL,          // 16 running to idle
    tr_ABtoIL,          // 17 aborting to idle
    tr_SAtoAB,          // 18 slow_abort to aborting
    tr_ILtoTC,          // 19 idle to to_cycling
    tr_ILtoAR,          // 20 idle to armed
    tr_ARtoRN,          // 21 armed to running
    tr_RNtoAB,          // 22 running to aborting
    tr_SBtoTC,          // 23 standBy to to_cycling
    tr_TCtoCY,          // 24 to_cycling to cycling (running pulse to pulse modulation)
};

struct transition
{
    INT16U      (*condition)(void);
    INT8U       next_state;
};

struct state
{
    void        (*state_func)(BOOLEAN); // state function
    INT8U       n_trans;                // number of possible transitions from this state
    INT8U       *trans;                 // list of possible transitions (bifurcations) from this state
};

//-----------------------------------------------------------------------------------------------------------

// Transition functions

INT16U OFtoFO    (void);         //  0
INT16U FStoFO    (void);         //  1
INT16U FOtoOF    (void);         //  2
INT16U SPtoOF    (void);         //  3
INT16U STtoFS    (void);         //  4
INT16U XXtoFS    (void);         //  5
INT16U STtoSP    (void);         //  6
INT16U XXtoSP    (void);         //  7
INT16U OFtoST    (void);         //  8
INT16U XXtoSA    (void);         //  9
INT16U STtoTS    (void);         // 10
INT16U XXtoTS    (void);         // 11
INT16U TStoSB    (void);         // 12
INT16U TStoAB    (void);         // 13
INT16U SBtoIL    (void);         // 14
INT16U ARtoIL    (void);         // 15
INT16U RNtoIL    (void);         // 16
INT16U ABtoIL    (void);         // 17
INT16U SAtoAB    (void);         // 18
INT16U ILtoTC    (void);         // 19
INT16U ILtoAR    (void);         // 20
INT16U ARtoRN    (void);         // 21
INT16U RNtoAB    (void);         // 22
INT16U SBtoTC    (void);         // 23
INT16U TCtoCY    (void);         // 24

//-----------------------------------------------------------------------------------------------------------

//  State variables & functions(14)

// the order is based in enum pc_state_machine_states !!!!

TRANSITIONS_CLASS_VARS_EXT char pc_str[]
#ifdef TRANSITIONS_CLASS_GLOBALS
= "FOOFFSSPSTSATSSBILTCARRNABCY"
#endif
;

// the order is based in enum state_pc_transitions !!!!

TRANSITIONS_CLASS_VARS_EXT struct transition pc_transitions[]
#ifdef TRANSITIONS_CLASS_GLOBALS
= {
    {OFtoFO, FGC_PC_FLT_OFF     },          //  0     tr_OFtoFO      off to fault (off)
    {FStoFO, FGC_PC_FLT_OFF     },          //  1     tr_FStoFO      fault (stopping) to fault (off)
    {FOtoOF, FGC_PC_OFF         },          //  2     tr_FOtoOF      fault (off) to off
    {SPtoOF, FGC_PC_OFF         },          //  3     tr_SPtoOF      stopping to off
    {STtoFS, FGC_PC_FLT_STOPPING},          //  4     tr_STtoFS      starting to fault (stopping)
    {XXtoFS, FGC_PC_FLT_STOPPING},          //  5     tr_XXtoFS      ??? to fault (stopping)
    {STtoSP, FGC_PC_STOPPING    },          //  6     tr_STtoSP      starting to stopping
    {XXtoSP, FGC_PC_STOPPING    },          //  7     tr_XXtoSP      ??? to stopping
    {OFtoST, FGC_PC_STARTING    },          //  8     tr_OFtoST      off to starting
    {XXtoSA, FGC_PC_SLOW_ABORT  },          //  9     tr_XXtoSA      ??? to slow abort
    {STtoTS, FGC_PC_TO_STANDBY  },          // 10     tr_STtoTS      starting to to_standBy
    {XXtoTS, FGC_PC_TO_STANDBY  },          // 11     tr_XXtoTS      ??? to to_standBy
    {TStoSB, FGC_PC_ON_STANDBY  },          // 12     tr_TStoSB      to_standBy to on_standBy
    {TStoAB, FGC_PC_ABORTING    },          // 13     tr_TStoAB      to_standBy to aborting
    {SBtoIL, FGC_PC_IDLE        },          // 14     tr_SBtoIL      standBy to idle
    {ARtoIL, FGC_PC_IDLE        },          // 15     tr_ARtoIL      armed to idle
    {RNtoIL, FGC_PC_IDLE        },          // 16     tr_RNtoIL      running to idle
    {ABtoIL, FGC_PC_IDLE        },          // 17     tr_ABtoIL      aborting to idle
    {SAtoAB, FGC_PC_ABORTING    },          // 18     tr_SAtoAB      slow abort to aborting
    {ILtoTC, FGC_PC_TO_CYCLING  },          // 29     tr_ILtoTC      idle to to_cycling
    {ILtoAR, FGC_PC_ARMED       },          // 20     tr_ILtoAR      idle to armed
    {ARtoRN, FGC_PC_RUNNING     },          // 21     tr_ARtoRN      armed to running
    {RNtoAB, FGC_PC_ABORTING    },          // 22     tr_RNtoAB      running to aborting
    {SBtoTC, FGC_PC_TO_CYCLING  },          // 23     tr_SBtoTC      on_standby to to_cycling
    {TCtoCY, FGC_PC_CYCLING     },          // 24     tr_TCtoCY      to_cycling to cycling (running pulse to pulse modulation)
}
#endif
;

// the 1st transitions checked is the rightmost and the last checked is the leftmost
// the first one that meet the requirements is the one chosen

#ifdef TRANSITIONS_CLASS_GLOBALS
INT8U trans_FO[] = { tr_FOtoOF,};
INT8U trans_OF[] = { tr_OFtoFO, tr_OFtoST,};
INT8U trans_FS[] = { tr_FStoFO,};
INT8U trans_SP[] = { tr_XXtoFS, tr_SPtoOF,};
INT8U trans_ST[] = { tr_STtoFS, tr_STtoSP, tr_STtoTS, };
INT8U trans_SA[] = { tr_XXtoFS, tr_XXtoSP, tr_SAtoAB,};
INT8U trans_TS[] = { tr_XXtoFS, tr_XXtoSP, tr_XXtoSA, tr_TStoAB, tr_TStoSB,};
INT8U trans_SB[] = { tr_XXtoFS, tr_XXtoSP, tr_XXtoSA, tr_SBtoIL, tr_SBtoTC,};
INT8U trans_IL[] = { tr_XXtoFS, tr_XXtoSP, tr_XXtoSA, tr_XXtoTS, tr_ILtoAR, tr_ILtoTC,};
INT8U trans_TC[] = { tr_XXtoFS, tr_XXtoSP, tr_XXtoSA, tr_XXtoTS, tr_TCtoCY,};
INT8U trans_AR[] = { tr_XXtoFS, tr_XXtoSP, tr_XXtoSA, tr_XXtoTS, tr_ARtoIL, tr_ARtoRN,};
INT8U trans_RN[] = { tr_XXtoFS, tr_XXtoSP, tr_XXtoSA, tr_XXtoTS, tr_RNtoIL, tr_RNtoAB,};
INT8U trans_AB[] = { tr_XXtoFS, tr_XXtoSP, tr_XXtoSA, tr_XXtoTS, tr_ABtoIL,};
INT8U trans_CY[] = { tr_XXtoFS, tr_XXtoSP, tr_XXtoSA, tr_XXtoTS,};
#endif

// the order is based in enum pc_state_machine_states !!!!
TRANSITIONS_CLASS_VARS_EXT struct state pc_states[]       // The order must match the state constant values in the XML
#ifdef TRANSITIONS_CLASS_GLOBALS
= {
    {StateFO, ArrayLen(trans_FO), trans_FO},    // FGC_PC_FLT_OFF
    {StateOF, ArrayLen(trans_OF), trans_OF},    // FGC_PC_OFF
    {StateFS, ArrayLen(trans_FS), trans_FS},    // FGC_PC_FLT_STOPPING
    {StateSP, ArrayLen(trans_SP), trans_SP},    // FGC_PC_STOPPING
    {StateST, ArrayLen(trans_ST), trans_ST},    // FGC_PC_STARTING
    {StateSA, ArrayLen(trans_SA), trans_SA},    // FGC_PC_SLOW_ABORT
    {StateTS, ArrayLen(trans_TS), trans_TS},    // FGC_PC_TO_STANDBY
    {StateSB, ArrayLen(trans_SB), trans_SB},    // FGC_PC_ON_STANDBY
    {StateIL, ArrayLen(trans_IL), trans_IL},    // FGC_PC_IDLE
    {StateTC, ArrayLen(trans_TC), trans_TC},    // FGC_PC_TO_CYCLING
    {StateAR, ArrayLen(trans_AR), trans_AR},    // FGC_PC_ARMED
    {StateRN, ArrayLen(trans_RN), trans_RN},    // FGC_PC_RUNNING
    {StateAB, ArrayLen(trans_AB), trans_AB},    // FGC_PC_ABORTING
    {StateCY, ArrayLen(trans_CY), trans_CY},    // FGC_PC_CYCLING
}
#endif
;
//-----------------------------------------------------------------------------------------------------------

#endif  // TRANSITIONS_CLASS_H end of header encapsulation
/*---------------------------------------------------------------------------------------------------------*\
  End of file: transitions_class.h
\*---------------------------------------------------------------------------------------------------------*/
