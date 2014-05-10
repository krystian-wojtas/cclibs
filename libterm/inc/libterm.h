/*---------------------------------------------------------------------------------------------------------*\
  File:     libterm.h                                                                   Copyright CERN 2011

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

  Contents: ANSI terminal support library
\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBTERM_H
#define LIBTERM_H

#include <stdio.h>
#include <stdint.h>

// Useful ANSI terminal sequences

#define TERM_ESC                        0x1B
#define TERM_BELL                       '\a'                    // Bell character
#define TERM_BACKSPACE                  '\b'                    // Backspace character
#define TERM_RESET                      "\r      \33c"          // Spaces provide delay before reset
#define TERM_INIT                       "\33[2J  \a \33[?7h \r" // Clear screen, enable line wrap, ring bell
#define TERM_SAVE_POS                   "\33" "7"               // Save cursor position
#define TERM_RESTORE_POS                "\33" "8"               // Restore cursor position
#define TERM_CLR_SCREEN                 "\33[2J"                // Clear complete screen
#define TERM_CLR_UP                     "\33[1J"                // Clear from current line to top of screen
#define TERM_CLR_DOWN                   "\33[J"                 // Clear from current line to bottom of screen
#define TERM_CLR_LINE                   "\33[2K"                // Clear complete line
#define TERM_CLR_LEFT                   "\33[1K"                // Clear from cursor to beginning of line
#define TERM_CLR_RIGHT                  "\33[K"                 // Clear from cursor to end of line
#define TERM_HOME                       "\33[H"                 // Move cursor to top left of screen
#define TERM_SET_SCROLL_LINES           "\33[H\33[%u;%ur"       // Define lines that scroll (as integers)

// Control Sequence introducer (CSI) sequences (see http://en.wikipedia.org/wiki/ANSI_escape_code)

#define TERM_CSI                        "\33["                  // Control sequence introducer
#define TERM_GOTO                       "H"                     // Goto: line;column
#define TERM_LEFT                       "D"                     // Cursor left: columns
#define TERM_RIGHT                      "C"                     // Cursor right: columns
#define TERM_SGR                        "m"                     // Select Graphic Rendition

// Option that can be used between TERM_CSI and TERM_SGR (use TERM_NORMAL to return to normal)

#define TERM_NORMAL                     TERM_CSI TERM_SGR       // Return to normal formatting
#define TERM_BOLD                       ";1"
#define TERM_UNDERLINE                  ";4"
#define TERM_REVERSE                    ";7"
#define TERM_FG_BLACK                   ";30"
#define TERM_FG_RED                     ";31"
#define TERM_FG_GREEN                   ";32"
#define TERM_FG_YELLOW                  ";33"
#define TERM_FG_BLUE                    ";34"
#define TERM_FG_MAGENTA                 ";35"
#define TERM_FG_CYAN                    ";36"
#define TERM_FG_WHITE                   ";37"
#define TERM_BG_BLACK                   ";40"
#define TERM_BG_RED                     ";41"
#define TERM_BG_GREEN                   ";42"
#define TERM_BG_YELLOW                  ";43"
#define TERM_BG_BLUE                    ";44"
#define TERM_BG_MAGENTA                 ";45"
#define TERM_BG_CYAN                    ";46"
#define TERM_BG_WHITE                   ";47"

// Function declarations

#ifdef __cplusplus
extern "C" {
#endif

void     TermLibInit    (FILE *file, void (*term_line)(char *line, uint16_t line_len), char prompt);
void     TermInit       (void);
uint16_t TermChar       (char keyboard_ch);

#ifdef __cplusplus
}
#endif

#endif
// End of file: libterm.h



