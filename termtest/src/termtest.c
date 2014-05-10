/*---------------------------------------------------------------------------------------------------------*\
  File:     termtest.c                                                                  Copyright CERN 2011

  License:  This program is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  Test program for libterm

  Contact:  cclibs-devs@cern.ch

  Authors:  Quentin.King@cern.ch

  Notes:    As well as testing libterm, this program provides an example of how it can be used with an
            ANSI standard terminal to use part of the terminal window as a shell (with scrolling) and
            part for static information.  This uses the ability to save the cursor position, then
            move and write a field, and then restore the cursor position.  The program also shows how
            to use the terminal control sequences that can set text or background colour, bold and
            underline.  These use the sequency TERM_CSI + formatting codes + TERM_SGR.
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "libterm.h"                    // Include libterm header file

// Constants

#define PROMPT          '>'             // Prompt can only be a single character

// Global variables

struct termios stdin_config;            // Original stdin configuration used by ResetStdinConfig()

/*---------------------------------------------------------------------------------------------------------*/
void ResetStdinConfig(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    TermInit();                                  // Initialise terminal on stdout (clear screen, etc...)

    tcsetattr(STDIN_FILENO, 0, &stdin_config);   // Restore stdin configuation
}
/*---------------------------------------------------------------------------------------------------------*/
void ResetTerm(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    TermInit();                                 // Initialise terminal on stdout (clear screen, etc...)

    printf(TERM_SET_SCROLL_LINES, 1, 21);       // Set scroll zone to be from lines 1 to 21

    // Print example formatting and help information in the scolled zone and end with prompt

    printf(TERM_CSI TERM_BOLD       TERM_UNDERLINE  TERM_SGR "LibTerm Test Program\n\n\r" TERM_NORMAL);

    printf(TERM_CSI TERM_FG_BLACK   TERM_BG_WHITE   TERM_SGR "BLACK   TEXT          "   TERM_NORMAL);
    printf(TERM_CSI                 TERM_BG_BLACK   TERM_SGR "BLACK   BACKGROUND\n\r"   TERM_NORMAL);

    printf(TERM_CSI TERM_FG_RED                     TERM_SGR "RED     TEXT          "   TERM_NORMAL);
    printf(TERM_CSI TERM_FG_BLACK   TERM_BG_RED     TERM_SGR "RED     BACKGROUND\n\r"   TERM_NORMAL);

    printf(TERM_CSI TERM_FG_GREEN                   TERM_SGR "GREEN   TEXT          "   TERM_NORMAL);
    printf(TERM_CSI TERM_BG_GREEN   TERM_FG_BLACK   TERM_SGR "GREEN   BACKGROUND\n\r"   TERM_NORMAL);

    printf(TERM_CSI TERM_FG_YELLOW                  TERM_SGR "YELLOW  TEXT          "   TERM_NORMAL);
    printf(TERM_CSI TERM_BG_YELLOW  TERM_FG_BLACK   TERM_SGR "YELLOW  BACKGROUND\n\r"   TERM_NORMAL);

    printf(TERM_CSI TERM_FG_BLUE                    TERM_SGR "BLUE    TEXT          "   TERM_NORMAL);
    printf(TERM_CSI TERM_BG_BLUE    TERM_FG_BLACK   TERM_SGR "BLUE    BACKGROUND\n\r"   TERM_NORMAL);

    printf(TERM_CSI TERM_FG_MAGENTA                 TERM_SGR "MAGENTA TEXT          "   TERM_NORMAL);
    printf(TERM_CSI TERM_FG_BLACK   TERM_BG_MAGENTA TERM_SGR "MAGENTA BACKGROUND\n\r"   TERM_NORMAL);

    printf(TERM_CSI TERM_FG_CYAN                    TERM_SGR "CYAN    TEXT          "   TERM_NORMAL);
    printf(TERM_CSI TERM_BG_CYAN    TERM_FG_BLACK   TERM_SGR "CYAN    BACKGROUND\n\r"   TERM_NORMAL);

    printf(TERM_CSI TERM_FG_WHITE                   TERM_SGR "WHITE   TEXT          "   TERM_NORMAL);
    printf(TERM_CSI TERM_BG_WHITE   TERM_FG_BLACK   TERM_SGR "WHITE   BACKGROUND\n\n\r" TERM_NORMAL);

    printf("CTRL-A : Start of line             Left arrow:  Move cursor left\n\r");
    printf("CTRL-E : Start of line             Right arrow: Move current right\n\r");
    printf("CTRL-R : Repeat last line          Up arrow:    Previous line from history\n\r");
    printf("CTRL-U : Clear line                Down arrow:  Next line from history\n\r");
    printf("CTRL-D : Delete right              ESC ESC:     Reset terminal\n\r");
    printf("CTRL-C : Quit termtest             Enter:       Process line\n\n\r");

    fputc(PROMPT,stdout);

    // Prepare information zone in non-scolled lines at the bottom of the terminal

    printf(TERM_SAVE_POS TERM_CSI "22;1" TERM_GOTO);  // Save cursor and jump to info zone (from lines 22-24)

    printf("+---------+---------+---------+---------+---------+---------+---------+---------");
    printf(TERM_CSI "23;1" TERM_GOTO "Keyboard character:                     Line length:" TERM_RESTORE_POS);
}
/*---------------------------------------------------------------------------------------------------------*/
void ProcessLine(char *line, uint16_t line_len)
/*---------------------------------------------------------------------------------------------------------*/
{
    // Report line length in info zone

    printf(TERM_SAVE_POS TERM_CSI "23;54" TERM_GOTO TERM_CSI TERM_BOLD TERM_SGR "%3hu", line_len);

    // Report line buffer in info zone

    printf(TERM_CSI "24;1" TERM_GOTO TERM_CLR_LINE "%s" TERM_RESTORE_POS, line);
}
/*---------------------------------------------------------------------------------------------------------*/
int main(int argc, char **argv)
/*---------------------------------------------------------------------------------------------------------*/
{
    int            keyboard_ch;
    uint16_t       term_level;
    struct termios stdin_config_raw;

    // Configure stdin to receive keyboard characters one at a time and without echo

    tcgetattr(STDIN_FILENO, &stdin_config);

    if(atexit(ResetStdinConfig))         // Link ResetStdinConfig() to be called on exit()
    {
        fprintf(stderr,"atexit failed\n");
        exit(1);
    }

    stdin_config_raw = stdin_config;      // Keep copy of original configuration for ResetStdinConfig()

    cfmakeraw(&stdin_config_raw);
    tcsetattr(STDIN_FILENO, 0, &stdin_config_raw);

    // Initialise libterm for stdout and reset the terminal display

    TermLibInit(stdout, ProcessLine, PROMPT);

    ResetTerm();

    // Loop forever to process keyboard characters

    for(;;)
    {
        // Wait for the next keyboard character

        fflush(stdout);

        keyboard_ch = getchar();

        // Report character value in the info zone on the terminal

        printf(TERM_SAVE_POS TERM_CSI "23;21" TERM_GOTO TERM_CSI TERM_BOLD TERM_SGR "%3u" TERM_RESTORE_POS, keyboard_ch);

        // Catch CTRL-C to exit

        if(keyboard_ch == 0x03)
        {
            printf("\nExiting\n");
            exit(0);
        }

        // Give characters to libterm to be processed

        term_level = TermChar(keyboard_ch);

        // Check for ESC pressed twice - this will appear as ESC at terminal level zero

        if(term_level == 0 && keyboard_ch == TERM_ESC)
        {
            ResetTerm();
        }

        // The terminal level is a state machine used in TermChar to process keyboard character
        // sequences that start with ESC (0x1B) which can correspond to cursor keys and function keys.
        // term_level goes from 0-4.  When the first ESC is received it becomes 1.  This can be due
        // to the user pressing the ESC key or pressing a cursor or function key (in which case other
        // characters will follow).
        // If the user presses ESC twice, then term_level will go to 1 and then back to zero.  So
        // this is a convenient way to capture a double ESC as a signal from the user that they want
        // to refresh the terminal.
    }
}
/*---------------------------------------------------------------------------------------------------------*\
  End of file: termtest.c
\*---------------------------------------------------------------------------------------------------------*/

