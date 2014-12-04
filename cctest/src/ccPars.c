/*---------------------------------------------------------------------------------------------------------*\
  File:     ccPars.c                                                                    Copyright CERN 2014

  License:  This file is part of cctest.

            cctest is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  Converter controls libraries test program parameter parsing functions

  Author:   Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <float.h>
#include <errno.h>

// Include cctest program header files

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRef.h"
#include "ccRun.h"

// Constants

#define PARS_INT_FORMAT          "% d"
#define PARS_FLOAT_FORMAT        "% .6E"
#define PARS_DOUBLE_FORMAT       "% 20.16E"

/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccParsGet(char *cmd_name, struct ccpars *par, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to interpret arguments remaining on the input line as values belonging to
  a command parameter. It supports different interpretations of multiple arguments, e.g.:

  REF FUNCTION() SINE                   Sets REF FUNCTION(1-16)
  REF FUNCTION(5) PLEP TABLE PPPL       Sets REF FUNCTION(5) (6) and (7)
  TABLE REF(5) 1 2 3 4                  Sets TABLE REF(5)

  If a cycle selector is supplied and the parameter is scalar then multiple values will apply to
  consecutive cycle selectors.
\*---------------------------------------------------------------------------------------------------------*/
{
    int32_t              int_value;
    uint32_t             array_idx;
    uint32_t             cyc_sel;
    uint32_t             cyc_sel_inc  = 0;
    uint32_t             cyc_sel_from = 0;
    uint32_t             cyc_sel_to   = 0;
    uint32_t             num_pars     = 0;
    uint32_t             max_pars;
    double               double_value;
    char                *remaining_arg;
    char                *arg;
    size_t               arg_len;
    struct ccpars_enum  *par_enum;
    struct ccpars_enum  *par_enum_matched;

    // Prepare array index

    array_idx = (cctest.array_idx == CC_NO_INDEX) ? 0 : cctest.array_idx;

    // Prepare cycle selector range

    if(cctest.cyc_sel == CC_NO_INDEX || (par->flags & PARS_CYCLE_SELECTOR) == 0)
    {
        max_pars = par->max_num_elements - array_idx;
    }
    else if(cctest.cyc_sel == CC_ALL_CYCLES)
    {
        cyc_sel_from  = 0;
        cyc_sel_to    = CC_MAX_CYC_SEL;
        max_pars      = par->max_num_elements - array_idx;
    }
    else // cyc_sel provided
    {
        cyc_sel_from = cyc_sel_to = cctest.cyc_sel;

        if(par->max_num_elements == 1)
        {
            cyc_sel_inc = 1;
            max_pars    = CC_NUM_CYC_SELS - cyc_sel_from;
        }
        else
        {
            max_pars = par->max_num_elements - array_idx;
        }
    }

    // Reset errno because strtod does not set it to zero on success

    errno = 0;

    // Try to parse the arguments to set the parameter values

    while((arg = ccTestGetArgument(remaining_line)) != NULL)
    {
        if(num_pars >= max_pars)
        {
            ccTestPrintError("too many values for %s %s (%u max)", cmd_name, par->name, max_pars);
            return(EXIT_FAILURE);
        }

        arg_len = strlen(arg);

        switch(par->type)
        {
        case PAR_UNSIGNED:

            int_value = strtol(arg, &remaining_arg, 10);

            if(*remaining_arg != '\0' || errno != 0 || int_value < 0)
            {
                ccTestPrintError("invalid integer for %s %s[%u]: '%s'",
                        cmd_name, par->name, array_idx, arg);
                return(EXIT_FAILURE);
            }

            for(cyc_sel = cyc_sel_from ; cyc_sel <= cyc_sel_to ; cyc_sel++)
            {
                par->value_p.u[array_idx + cyc_sel * par->max_num_elements] = (uint32_t)int_value;
            }
            break;

        case PAR_FLOAT:

            double_value = strtod(arg, &remaining_arg);

            // Protect against double value that cannot fit in a float

            if(*remaining_arg != '\0' || errno != 0 || double_value >= FLT_MAX || double_value <= -FLT_MAX)
            {
                ccTestPrintError("invalid float for %s %s[%u]: '%s'",
                        cmd_name, par->name, array_idx, arg);
                return(EXIT_FAILURE);
            }

            for(cyc_sel = cyc_sel_from ; cyc_sel <= cyc_sel_to ; cyc_sel++)
            {
                par->value_p.f[array_idx + cyc_sel * par->max_num_elements] = (float)double_value;
            }
            break;

        case PAR_DOUBLE:

            double_value = strtod(arg, &remaining_arg);

            if(*remaining_arg != '\0' || errno != 0)
            {
                ccTestPrintError("invalid double for %s %s[%u]: '%s'",
                        cmd_name, par->name, array_idx, arg);
                return(EXIT_FAILURE);
            }

            for(cyc_sel = cyc_sel_from ; cyc_sel <= cyc_sel_to ; cyc_sel++)
            {
                par->value_p.d[array_idx + cyc_sel * par->max_num_elements] = double_value;
            }
            break;

        case PAR_STRING:

            // Limit string lengths because they are used in path names

            if(arg_len > 32)
            {
                ccTestPrintError("invalid string length (%u) for %s %s[%u]: '%s' (32 max)",
                            arg_len, cmd_name, par->name, array_idx, ccTestAbbreviatedArg(arg));
                return(EXIT_FAILURE);
            }

            // Free and reallocate space for string argument
            // On first call, the pointer is NULL which is ignored by free()

            for(cyc_sel = cyc_sel_from ; cyc_sel <= cyc_sel_to ; cyc_sel++)
            {
                free(par->value_p.s[array_idx + cyc_sel * par->max_num_elements]);
                par->value_p.s[array_idx + cyc_sel * par->max_num_elements] = strcpy(malloc(arg_len+1),arg);
            }
            break;

        case PAR_ENUM:

            // Compare first argument against list of enum strings for the parameter

            for(par_enum = par->ccpars_enum, par_enum_matched = NULL ; par_enum->string != NULL ; par_enum++)
            {
                // If argument matches start or all of an enum string

                if(strncasecmp(par_enum->string, arg, arg_len) == 0)
                {
                    // If first match, remember enum

                    if(par_enum_matched == NULL)
                    {
                        par_enum_matched = par_enum;

                        // If match is exact then break out of search

                        if(strcasecmp(par_enum->string, arg) == 0)
                        {
                            break;
                        }
                    }
                    else // else second match so report error
                    {
                        ccTestPrintError("ambiguous enum for %s %s[%u]: '%s'",
                                         cmd_name, par->name, array_idx, arg);
                        return(EXIT_FAILURE);
                    }
                }
            }

            if(par_enum_matched == NULL)
            {
                ccTestPrintError("unknown enum for %s %s[%u]: '%s'",
                                 cmd_name, par->name, array_idx, ccTestAbbreviatedArg(arg));
                return(EXIT_FAILURE);
            }

            for(cyc_sel = cyc_sel_from ; cyc_sel <= cyc_sel_to ; cyc_sel++)
            {
                par->value_p.u[array_idx + cyc_sel * par->max_num_elements] = par_enum_matched->value;
            }
            break;
        }

        if(cyc_sel_inc == 0)
        {
            array_idx++;

            // Update num_elements if the array length is growing with each new argument

            for(cyc_sel = cyc_sel_from ; cyc_sel <= cyc_sel_to ; cyc_sel++)
            {
                if(array_idx > par->num_elements[cyc_sel])
                {
                    par->num_elements[cyc_sel] = array_idx;
                }
            }
        }
        else
        {
            cyc_sel_from++;
            cyc_sel_to++;
        }

        num_pars++;
    }

    // If parameter length is NOT fixed

    if((par->flags & PARS_FIXED_LENGTH) == 0 && par->max_num_elements > 1)
    {
        for(cyc_sel = cyc_sel_from ; cyc_sel <= cyc_sel_to ; cyc_sel++)
        {
            uint32_t num_elements = par->num_elements[cyc_sel];

            // If array has shrunk then zero the trailing values and update num_elements

            if(array_idx < num_elements &&
              (cctest.array_idx == CC_NO_INDEX || array_idx > (cctest.array_idx + 1)))
            {
                memset(&par->value_p.c[ccpars_sizeof_type[par->type] * (array_idx + cyc_sel * par->max_num_elements)],
                       0,
                       ccpars_sizeof_type[par->type] * (num_elements - array_idx));

                par->num_elements[cyc_sel] = array_idx;
            }
        }
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
char * ccParsEnumString(struct ccpars_enum *par_enum, uint32_t value)
/*---------------------------------------------------------------------------------------------------------*\
  This function will return the string corresponding to the enum
\*---------------------------------------------------------------------------------------------------------*/
{
    while(par_enum->string != NULL && par_enum->value != value)
    {
        par_enum++;
    }

    return(par_enum->string != NULL ? par_enum->string : "invalid");
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccParsPrintElement(FILE *f, struct ccpars *par, uint32_t cyc_sel, uint32_t array_idx)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print an individual element of a parameter with a leading space.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(array_idx < par->num_elements[cyc_sel])
    {
        uint32_t base_idx = cyc_sel * par->max_num_elements;

        switch(par->type)
        {
        case PAR_UNSIGNED:

            fprintf(f, PARS_INT_FORMAT, par->value_p.u[base_idx + array_idx]);
            break;

        case PAR_FLOAT:

            fprintf(f, PARS_FLOAT_FORMAT, par->value_p.f[base_idx + array_idx]);
            break;

        case PAR_DOUBLE:

            fprintf(f, PARS_DOUBLE_FORMAT, par->value_p.d[base_idx + array_idx]);
            break;

        case PAR_STRING:

            fprintf(f, " %s", par->value_p.s[base_idx + array_idx]);
            break;

        case PAR_ENUM:

            fprintf(f, " %s", ccParsEnumString(par->ccpars_enum, par->value_p.u[base_idx + array_idx]));
            break;
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccParsPrint(FILE *f, char *cmd_name, struct ccpars *par, uint32_t cyc_sel, uint32_t array_idx)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print the name and value(s) for one parameter
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t  num_elements;
    uint32_t  num_characters;
    uint32_t  idx_from;
    uint32_t  idx_to;
    uint32_t  idx_local;
    uint32_t  cyc_sel_from;
    uint32_t  cyc_sel_to;
    uint32_t  cyc_sel_local;

    // Determine cycle selector range from cyc_sel

    if((par->flags & PARS_CYCLE_SELECTOR) == 0 || cyc_sel == CC_NO_INDEX)
    {
        cyc_sel_from = cyc_sel_to = 0;
    }
    else
    {
        if(cyc_sel == CC_ALL_CYCLES)
        {
            cyc_sel_from = 0;
            cyc_sel_to   = CC_MAX_CYC_SEL;
        }
        else
        {
            cyc_sel_from = cyc_sel_to = cyc_sel;
        }
    }

    // Print values for each cycle selector within the range2

    for(cyc_sel_local = cyc_sel_from ; cyc_sel_local <= cyc_sel_to ; cyc_sel_local++)
    {
        num_elements = par->num_elements[cyc_sel_local];

        if(num_elements > 0)
        {
            // Print parameter name

            num_characters = fprintf(f,"%-*s %s",CC_MAX_CMD_NAME_LEN,cmd_name,par->name);

            // Print cycle selector if non zero

            if(cyc_sel_local > 0)
            {
                num_characters += fprintf(f, "(%u)", cyc_sel_local);
            }

            // If array index provided, then just display this one element

            if(array_idx != CC_NO_INDEX)
            {
                idx_from = idx_to = array_idx;
                num_characters += fprintf(f, "[%u]", array_idx);
            }
            else // else display all elements
            {
                idx_from = 0;
                idx_to   = num_elements - 1;
            }

            // Pad with spaces to align values colunn

            while(++num_characters < CC_MAX_PAR_NAME_LEN)
            {
                fputc(' ',f);
            }

            // Print the values for the specified array range

            for(idx_local = idx_from ; idx_local <= idx_to ; idx_local++)
            {
                ccParsPrintElement(f, par, cyc_sel_local, idx_local);
            }

            fputc('\n',f);
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccParsPrintAll(FILE *f, char *cmd_name, struct ccpars *par, uint32_t cyc_sel, uint32_t array_idx)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print the name and value(s) for all parameters for one command.
\*---------------------------------------------------------------------------------------------------------*/
{
    while(par->name != NULL)
    {
        ccParsPrint(f, cmd_name, par, cyc_sel, array_idx);
        par++;
    }
}

// EOF
