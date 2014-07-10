/*---------------------------------------------------------------------------------------------------------*\
  File:         state_class.h

  Purpose:

  Author:       Quentin.King@cern.ch

  Notes:
\*---------------------------------------------------------------------------------------------------------*/

#ifndef STATE_CLASS_H      // header encapsulation
#define STATE_CLASS_H

#ifdef STATE_CLASS_GLOBALS
    #define STATE_CLASS_VARS_EXT
#else
    #define STATE_CLASS_VARS_EXT extern
#endif

//-----------------------------------------------------------------------------------------------------------

#include <cc_types.h>           // basic typedefs

//-----------------------------------------------------------------------------------------------------------

// PC State Flags constants

#define STAF_TO_STANDBY         0x0001
#define STAF_IDLE               0x0002
#define STAF_ARMED              0x0008
#define STAF_RUNNING            0x0010
#define STAF_ABORTING           0x0020
#define STAF_TO_CYCLING         0x0040
#define STAF_IMIN               0x0080
#define STAF_START              0x0100
#define STAF_STOP               0x0200
#define STAF_SLOW_ABORT         0x0400
#define STAF_CYCLING            0x0800

//-----------------------------------------------------------------------------------------------------------

void StateFO     (BOOLEAN first_f);      //  0
void StateOF     (BOOLEAN first_f);      //  1
void StateFS     (BOOLEAN first_f);      //  2
void StateSP     (BOOLEAN first_f);      //  3
void StateST     (BOOLEAN first_f);      //  4
void StateSA     (BOOLEAN first_f);      //  5
void StateTS     (BOOLEAN first_f);      //  6
void StateSB     (BOOLEAN first_f);      //  7
void StateIL     (BOOLEAN first_f);      //  8
void StateTC     (BOOLEAN first_f);      //  9
void StateAR     (BOOLEAN first_f);      // 10
void StateRN     (BOOLEAN first_f);      // 11
void StateAB     (BOOLEAN first_f);      // 12
void StateCY     (BOOLEAN first_f);      // 13

//-----------------------------------------------------------------------------------------------------------

#endif  // STATE_CLASS_H end of header encapsulation
/*---------------------------------------------------------------------------------------------------------*\
  End of file: state_class.h
\*---------------------------------------------------------------------------------------------------------*/
