/*---------------------------------------------------------------------------------------------------------*\
  File:     term.c                                                                      Copyright CERN 2011

  License:  This file is part of libterm.

            libterm is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  ANSI Terminal support library

  Contact:  cclibs-devs@cern.ch

  Notes:    Libterm provides a simple command shell for use with an ANSI/VT100 terminal.
\*---------------------------------------------------------------------------------------------------------*/

#include <string.h>

#include "libterm.h"

// Private constants (this file only)

#define NUM_LINE_BUFS           16                      // Must be a power of 2 !!!
#define LINE_BUFS_MASK          (NUM_LINE_BUFS-1)       // Mask for circular history buffer
#define MAX_LINE_LEN            78

// Static variables (this file only)

static struct term_static
{
    uint16_t     level;                                         // Keyboard character processing level
    uint16_t     line_idx;                                      // Line editor buffer index
    uint16_t     line_end;                                      // Line editor end of buffer contents index
    uint16_t     cur_line;                                      // Current line in line_bufs[] history
    uint16_t     recall_line;                                   // Recall line in line_bufs[] history
    uint16_t     line_len[NUM_LINE_BUFS];                       // History line lengths
    char         line_buf[MAX_LINE_LEN+2];                      // Edit line buffer
    char         line_bufs[NUM_LINE_BUFS][MAX_LINE_LEN+2];      // Line buffers for history
    FILE        *file;                                          // Stream for writing to the terminal
    void        (*term_line)(char *line, uint16_t line_len);    // User callback for command lines
    char         prompt;                                        // Prompt character
} term_s;

// Static function prototypes (this file only)

static  uint16_t TermLevel0               (char);
static  uint16_t TermLevel1               (char);
static  uint16_t TermLevel2               (char);
static  uint16_t TermLevel3               (char);
static  uint16_t TermLevel4               (char);
static  void     TermInsertChar           (char);
static  void     TermNewline              (void);
static  void     TermCursorLeft           (void);
static  void     TermCursorRight          (void);
static  void     TermStartOfLine          (void);
static  void     TermEndOfLine            (void);
static  void     TermDeleteLine           (void);
static  void     TermDeleteLeft           (void);
static  void     TermDeleteRight          (void);
static  void     TermShiftRemains         (void);
static  void     TermRepeatLine           (void);
static  void     TermPreviousLine         (void);
static  void     TermNextLine             (void);
static  void     TermRecallLine           (uint16_t);

/*---------------------------------------------------------------------------------------------------------*/
void TermLibInit(FILE *file, void (*term_line)(char *line, uint16_t line_len), char prompt)
/*---------------------------------------------------------------------------------------------------------*\
  This function is used to initialise the terminal structure.
\*---------------------------------------------------------------------------------------------------------*/
{
    term_s.term_line = term_line;
    term_s.file      = file;
    term_s.prompt    = prompt;
}
/*---------------------------------------------------------------------------------------------------------*/
void TermInit(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is used to initialise the terminal structure.  Some old terminals take time to reset and
  ingore characters while they are resetting.  So this function sends dummy spaces to give time for the
  reset to complete before the user program sends important characters that must not be ignored.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint16_t    i;

    fputs(TERM_RESET, term_s.file);             // Reset terminal

    for ( i = 0 ; i < 32; i++ )                 // Send spaces to allow time for terminal to reset
    {
        fputc(' ', term_s.file);
    }

    fputs(TERM_INIT, term_s.file);              // Clear screen, enable line wrap, ring bell

    term_s.line_idx = 0;                        // Reset line buffer cursor index
    term_s.line_end = 0;                        // Reset line buffer end index
    term_s.level    = 0;                        // Reset level
}
/*---------------------------------------------------------------------------------------------------------*/
uint16_t TermChar(char keyboard_ch)
/*---------------------------------------------------------------------------------------------------------*/
{
    static uint16_t       (*term_func[])(char) =         // Keyboard character functions
    {
        TermLevel0,                                       // Level 0 : Normal characters
        TermLevel1,                                       // Level 1 : ESC pressed
        TermLevel2,                                       // Level 2 : Cursor keys or all function keys
        TermLevel3,                                       // Level 3 : All function keys except PF1-PF4
        TermLevel4,                                       // Level 4 : PF1-PF4 keys
    };

    // Process keyboard character according to level

    term_s.level = term_func[term_s.level](keyboard_ch);

    // Flush the terminal stream

    fflush(term_s.file);

    // Return the level

    return(term_s.level);
}
/*---------------------------------------------------------------------------------------------------------*/
static uint16_t TermLevel0(char keyboard_ch)
/*---------------------------------------------------------------------------------------------------------*\
  Level 0: The keyboard character is analysed directly.  If the ESC code (0x1B) is received, the level
  changes to 1, otherwise the character is processed and the state remains 0.
\*---------------------------------------------------------------------------------------------------------*/
{
    switch(keyboard_ch)                                 //-[CHARACTER]--ACTION-----------------------------
    {
    case 0x1B:  return(1);                              // [ESC]        Start escape sequence processing

    case 0x7F:                                          // [Delete]     Delete left

    case 0x08:  TermDeleteLeft();            break;     // [Backspace]  Delete left

    case 0x04:  TermDeleteRight();           break;     // [CTRL-D]     Delete right

    case 0x15:  TermDeleteLine();            break;     // [CTRL-U]     Delete line

    case 0x01:  TermStartOfLine();           break;     // [CTRL-A]     Move to start of line

    case 0x05:  TermEndOfLine();             break;     // [CTRL-E]     Move to end of line

    case 0x12:  TermRepeatLine();            break;     // [CTRL-R]     Repeat line

    case 0x0D:  TermNewline();               break;     // [Return]     Carriage return - return line

    default:    TermInsertChar(keyboard_ch); break;     // [others]     Insert character
    }

    return(0);                                  // Continue with level 0
}
/*---------------------------------------------------------------------------------------------------------*/
static uint16_t TermLevel1(char keyboard_ch)
/*---------------------------------------------------------------------------------------------------------*\
  Level 1: The previous character was [ESC].
\*---------------------------------------------------------------------------------------------------------*/
{
    switch(keyboard_ch)                              // Switch according to next code
    {
    case 0x5B:                          return(2);      // [Cursor/Fxx] Change state to analyse

    case 0x4F:                          return(4);      // [PF1-4]      Change state to ignore
    }

    return(0);                                      // All other characters - return to level 0
}
/*---------------------------------------------------------------------------------------------------------*/
static uint16_t TermLevel2(char keyboard_ch)
/*---------------------------------------------------------------------------------------------------------*\
  Level 2: The key pressed was either a cursor key or a function key.  Cursors keys have the code
  sequence "ESC[A" to "ESC[D", while function keys have the code sequence "ESC[???~" where ??? is a
  variable number of alternative codes.
\*---------------------------------------------------------------------------------------------------------*/
{
    switch(keyboard_ch)                              // Switch according to next code
    {
    case 0x41:  TermPreviousLine();      return(0);      // [Up]         Previous history line

    case 0x42:  TermNextLine();          return(0);      // [Down]       Next history line

    case 0x43:  TermCursorRight();       return(0);      // [Right]      Move cursor right

    case 0x44:  TermCursorLeft();        return(0);      // [Left]       Move cursor left
    }

    return(3);                                      // Function key - change to level 3
}
/*---------------------------------------------------------------------------------------------------------*/
static uint16_t TermLevel3(char keyboard_ch)
/*---------------------------------------------------------------------------------------------------------*\
  Level 3:  Orignial Key was a Function with sequence terminated by 0x7E (~).  However, the function will
  also accept a new [ESC] to allow an escape route in case of corrupted reception.
\*---------------------------------------------------------------------------------------------------------*/
{
    switch(keyboard_ch)                             // Switch according to next code
    {
    case 0x1B:                          return(1);      // [ESC]        Escape to level 1

    case 0x7E:                          return(0);      // [~]          Sequence complete - return to state 0
    }

    return(3);                                      // No change to level
}
/*---------------------------------------------------------------------------------------------------------*/
static uint16_t TermLevel4(char keyboard_ch)
/*---------------------------------------------------------------------------------------------------------*\
  Level 4:  The original key was PF1-4 and one more character must be ignored.
\*---------------------------------------------------------------------------------------------------------*/
{
    return(0);                                  // Return to level 0
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermInsertChar(char keyboard_ch)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermLevel0() if a standard character has been received.  It will try to enter
  the character into the current line under the current cursor position.  The rest of the line will be
  moved to the right to make space for the new character.  If the line is full, the bell will ring.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint16_t      i;

    // If character is printable

    if(keyboard_ch >= 0x20)
    {
        if(term_s.line_end == MAX_LINE_LEN)     // If line buffer is full then ring the bell
        {
            fputc(TERM_BELL, term_s.file);
        }
        else
        {
            // Shift the rest of the line by one character and insert the new keyboard character

            for(i = term_s.line_end++ ; i > term_s.line_idx ; i--)
            {
                term_s.line_buf[i] = term_s.line_buf[i-1];
            }

            term_s.line_buf[term_s.line_idx] = keyboard_ch;

            // Print the new character and the rest of the line to the terminal

            for(i = term_s.line_idx++ ; i < term_s.line_end ; i++)
            {
                fputc(term_s.line_buf[i], term_s.file);
            }

            // If cursor is now offset from true position then shift cursor left again

            if((i = term_s.line_end - term_s.line_idx) > 0)
            {
                fprintf(term_s.file,TERM_CSI "%hu" TERM_LEFT, i);
            }
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermNewline(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermLevel0() if Enter or Return have been received.  It skips leading
  white space and enters the line into the command buffer and the line history.  It then calls the users
  callback function to process the buffer.
\*---------------------------------------------------------------------------------------------------------*/
{
    char *      line_p;
    uint16_t    line_len;

    // Nul terminate the line and skip over leading spaces

    line_len = term_s.line_end;
    term_s.line_buf[term_s.line_end] = '\0';

    for(line_p=term_s.line_buf ; *line_p==' ' ; line_p++)
    {
        line_len--;
    }

    // If new line is different then store it in the line history buffer

    if(line_len > 0 &&
       (term_s.line_end != term_s.line_len[(term_s.cur_line-1) & LINE_BUFS_MASK] ||
        memcmp(term_s.line_buf, &term_s.line_bufs[(term_s.cur_line-1) & LINE_BUFS_MASK], term_s.line_end)))
    {
        term_s.line_len[term_s.cur_line] = term_s.line_end;
        memcpy(&term_s.line_bufs[term_s.cur_line], term_s.line_buf, term_s.line_end);
        term_s.cur_line = (term_s.cur_line + 1) & LINE_BUFS_MASK;
    }

    // Call user callback function to process line buffer

    term_s.term_line(line_p, line_len);

    // Reset cursor and end of line indexes

    term_s.line_idx = 0;
    term_s.line_end = 0;
    term_s.recall_line = term_s.cur_line;

    // Move cursor to start of newline and print the prompt character

    printf("\r\n%c", term_s.prompt);
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermCursorLeft(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermLevel2() if the Cursor left key has been pressed.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Move cursor left one column or ring bell if at start of line

    if(term_s.line_idx > 0)
    {
        fputc(TERM_BACKSPACE, term_s.file);
        term_s.line_idx--;
    }
    else
    {
        fputc(TERM_BELL, term_s.file);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermCursorRight(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermLevel2() if the Cursor right key has been pressed.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Move cursor right one column or ring bell if at end of content on the line

    if(term_s.line_idx < term_s.line_end)
    {
        fputs(TERM_CSI TERM_RIGHT, term_s.file);
        term_s.line_idx++;
    }
    else
    {
        fputc(TERM_BELL, term_s.file);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermStartOfLine(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermLevel0() if the Ctrl-A key has been pressed.
\*---------------------------------------------------------------------------------------------------------*/
{
    // If cursor is not already at the start of the line then move cursor left to the start of line

    if(term_s.line_idx > 0)
    {
        fprintf(term_s.file, TERM_CSI "%hu" TERM_LEFT, term_s.line_idx);
        term_s.line_idx = 0;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermEndOfLine(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermLevel0() if the Ctrl-E key has been pressed.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint16_t      shift_right;

    // If cursor is not already at the end of the contents of the line then move cursor to end of contents

    if((shift_right = term_s.line_end - term_s.line_idx) > 0)
    {
        fprintf(term_s.file, TERM_CSI "%hu" TERM_RIGHT, shift_right);
        term_s.line_idx = term_s.line_end;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermDeleteLine(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermLevel0() if the Ctrl-U key has been pressed.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Clear line and print prompt

    fprintf(term_s.file, TERM_CLR_LINE "\r%c", term_s.prompt);

    term_s.line_idx = 0;
    term_s.line_end = 0;
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermDeleteLeft(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermLevel0() if the backspace or delete keys have been pressed.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Delete character to left of cursor or ring bell if at start of line

    if(term_s.line_idx > 0)
    {
        fputc(TERM_BACKSPACE, term_s.file);
        term_s.line_idx--;
        TermShiftRemains();             // Shift remains of the line one character to the left
    }
    else
    {
        fputc(TERM_BELL, term_s.file);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermDeleteRight(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermLevel0() if the Ctrl-D key has been pressed.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Delete to right of cursor or ring bell if at end of content on the line

    if(term_s.line_idx < term_s.line_end)
    {
        TermShiftRemains();             // Shift remains of the line one character to the left
    }
    else
    {
        fputc(TERM_BELL, term_s.file);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermShiftRemains(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermDeleteLeft() and TermDeleteRight() to shift the remains of the line
  one character to the left.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint16_t      i;

    term_s.line_end--;                           // Adjust end of line index

    // For the remainder of the line shift characters in buffer and display it

    for(i = term_s.line_idx ; i < term_s.line_end ; i++)
    {
        fputc( (term_s.line_buf[i] = term_s.line_buf[i+1]), term_s.file);
    }

    // Clear last character and move cursor the required number of columns to the left

    fprintf(term_s.file, " " TERM_CSI "%hu" TERM_LEFT, (1 + term_s.line_end - term_s.line_idx));
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermRepeatLine(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermLevel0() if Ctrl-R is entered.  It recovers the previous line from
  the line history and enters it.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint16_t      recall_line;

    // Calc index of previous line in history and return if empty

    recall_line = (term_s.cur_line - 1) & LINE_BUFS_MASK;

    if(!term_s.line_len[recall_line])
    {
        return;
    }

    // Recall and process line and adjust current line index to avoid a repeat entry in the history

    term_s.cur_line = recall_line;
    TermRecallLine(recall_line);
    TermNewline();
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermPreviousLine(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermLevel2() if cursor up is pressed.  It will recover the previous line from
  the line history buffers.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint16_t      recall_line;

    // If editing current line then save it in the history

    if(term_s.recall_line == term_s.cur_line)
    {
        term_s.line_len[term_s.cur_line] = term_s.line_end;
        memcpy(&term_s.line_bufs[term_s.cur_line], term_s.line_buf, term_s.line_end);
    }

    // Adjust line index to previous line and recall the line if it is not empty

    recall_line = (term_s.recall_line - 1) & LINE_BUFS_MASK;

    if(recall_line == term_s.cur_line ||
       !term_s.line_len[recall_line])
    {
        fputc(TERM_BELL, term_s.file);
    }
    else
    {
        TermRecallLine(recall_line);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermNextLine(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermLevel2() if cursor down is pressed.  It will recover the next line from
  the line history buffers.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Recall next line unless already at the most recent line in the history

    if(term_s.recall_line == term_s.cur_line)
    {
        fputc(TERM_BELL, term_s.file);
    }
    else
    {
        TermRecallLine((term_s.recall_line + 1) & LINE_BUFS_MASK);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void TermRecallLine(uint16_t recall_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from TermRepeatLine(), TermPreviousLine() or TermNextLine() when a history line
  needs to be recalled.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Save recalled line index and recover line length and data

    term_s.recall_line = recall_line;
    term_s.line_idx    = term_s.line_end = term_s.line_len[recall_line];

    memcpy(term_s.line_buf,&term_s.line_bufs[recall_line],term_s.line_end);

    // Nul terminate recalled line and display it following the prompt

    term_s.line_buf[term_s.line_end] = '\0';

    fprintf(term_s.file, TERM_CLR_LINE "\r%c%s", term_s.prompt, term_s.line_buf);
}
/*---------------------------------------------------------------------------------------------------------*\
  End of file: term.c
\*---------------------------------------------------------------------------------------------------------*/
