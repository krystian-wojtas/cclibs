#!/usr/bin/awk -f
#
# pars.awk
# Converter Control Regulation library parameter header file generator
#
# All libreg parameters are defined in libreg/parameteres/pars.csv and this
# script translates the csv file into a header file that declares and defines
# structures that describe all the parameters.
#
# Contact
#
# cclibs-devs@cern.ch
#
# Copyright
#
# Copyright CERN 2014. This project is released under the GNU Lesser General
# Public License version 3.
#
# License
#
# This file is part of libreg.
#
# libreg is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
# for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Notes
#
# All libreg variables are address by pointers, so initialization of values 
# cannot be done by using the linker. Instead, this script creates a static
# function in the generated header file, regConvParsInit(), which initializes
# the conv->pars and conv->pars_values structures. This is called by regConvInit().
#
# There is a restriction in the current implementation which means that parameters
# that are arrays can only be initialized with the same value in all elements.

BEGIN {

# Set field separater to comma to read csv file

    FS = ","

# Identify the columns in the csv file

    par_group_column  = 1
    par_name_column   = 2
    par_type_column   = 3
    par_length_column = 4
    par_init_column   = 5
    par_flags_column  = 6

# Read heading line from stdin

    getline

    n_columns = NF
    n_pars    = 0
    
# Create REG_LOAD_SELECT as the first flag

    n_flags = 1    
    flag[0] = "REG_LOAD_SELECT"
    
# Create the rest of the flags from the column headings

    for(i=par_flags_column ; i <= n_columns ; i++)
    {
        flag[n_flags++] = $i
    }

# Read parameter definitions from stdin

    while(getline > 0)
    {
        # Skip blank lines

        if(NF == 0) continue

        # Stop if non-blank lines do not have the correct number of colums

        if(NF != n_columns)
        {
            printf "Error in line %d : %d column(s) read while %d expected\n", NR, NF, n_columns >> "/dev/stderr"
            exit -1
        }

        # Detect if parameter is an array based on LOAD SELECT
        
        if($par_length_column == "REG_N_LOADS")
        {
            # Set REG_LOAD_SELECT flag to inform regConvParsCheck() that this parameter is based on LOAD SELECT
                        
            par_flags[n_pars] = flag[0]
            
            # libreg will only keep the scalar value corresponding to LOAD SELECT so change length to 1

            $par_length_column = "1"
        }
        else
        {
            par_flags[n_pars] = "0"
        }

        # Save contents

        par_variable[n_pars] = tolower($par_group_column) "_" tolower($par_name_column)
        par_type[n_pars]     = $par_type_column
        par_length[n_pars]   = $par_length_column
        par_init[n_pars]     = $par_init_column
        
        # Interpret flag specifiers (YES or NO)
        # Note: flag 0 (REG_LOAD_SELECT) is created internally by pars.awk so flags from pars.csv start from index 1

        for(i=1; i < n_flags ; i++)
        {
            f = $(par_flags_column+i-1)

            if(f == "YES")
            {
                par_flags[n_pars] = par_flags[n_pars] "|" flag[i]
            }
            else if(f != "NO")
            {
                printf "Error in line %d : column %d (%s) must be YES or NO\n", NR, par_flags_column+i, f >> "/dev/stderr"
                exit -1
            }
        }

        n_pars++
    }

# Generate parameter header file inc/pars.h 

    of = "inc/pars.h"   # Set output file (of) to pars.h

    print "/*!"                                                                                  > of
    print " * @file  pars.h"                                                                     > of
    print " * @brief Converter Control Regulation library generated parameters header file"      > of
    print " *"                                                                                   > of
    print " * IMPORTANT - DO NOT EDIT - This file is generated from libreg/parameters/pars.csv"  > of
    print " *"                                                                                   > of
    print " * All libreg parameters are defined in pars.csv and this is transformed into"        > of
    print " * this header file by libreg/parameters/pars.awk."                                   > of
    print " *"                                                                                   > of
    print " * <h2>Contact</h2>"                                                                  > of
    print " *"                                                                                   > of
    print " * cclibs-devs@cern.ch"                                                               > of
    print " *"                                                                                   > of
    print " * <h2>Copyright</h2>"                                                                > of
    print " *"                                                                                   > of
    print " * Copyright CERN 2014. This project is released under the GNU Lesser General"        > of
    print " * Public License version 3."                                                         > of
    print " *"                                                                                   > of
    print " * <h2>License</h2>"                                                                  > of
    print " *"                                                                                   > of
    print " * This file is part of libreg."                                                      > of
    print " *"                                                                                   > of
    print " * libreg is free software: you can redistribute it and/or modify it under the"       > of
    print " * terms of the GNU Lesser General Public License as published by the Free"           > of
    print " * Software Foundation, either version 3 of the License, or (at your option)"         > of
    print " * any later version."                                                                > of
    print " *"                                                                                   > of
    print " * This program is distributed in the hope that it will be useful, but WITHOUT"       > of
    print " * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or"             > of
    print " * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License"       > of
    print " * for more details."                                                                 > of
    print " *"                                                                                   > of
    print " * You should have received a copy of the GNU Lesser General Public License"          > of
    print " * along with this program.  If not, see <http://www.gnu.org/licenses/>."             > of
    print " *"                                                                                   > of
    print " * <h2>Parameter initialization template</h2>"                                        > of
    print " *"                                                                                   > of
    print " * To use libreg, the calling program should initialize the parameter value"          > of
    print " * pointers to access the parameter variables in the calling program. Here"           > of
    print " * is an example template for all the libreg parameters, which assumes that"          > of
    print " * the reg_conv struct is called conv. Copy this into your initialization"            > of
    print " * function and for each parameter that you want to control, replace"                 > of
    print " * REG_PAR_NOT_USED with the address of your variable.\n"                             > of

    for(i=0 ; i < n_pars ; i++)
    {
        printf "    regConvParInitPointer(&conv,%-30s,REG_PAR_NOT_USED);\n", par_variable[i]     > of
    }

    print "\n * If a parameter is not used by your program, you can delete the macro"            > of
    print " * and the default value from pars.csv will be used instead."                         > of
    print " * If you want a fixed value for a parameter that is different to the"                > of
    print " * default value in pars.csv, you can set it using another macro:"                    > of
    print " *"                                                                                   > of
    print " *  regConvParInitValue(&conv,{parameter_name},{array_index},{initial_value});"       > of
    print " *"                                                                                   > of
    print " * For example, if a given application will always work with the actuation"           > of
    print " * set to REG_CURRENT_REF, then this can be done using:\n"                            > of
    print "    regConvParInitValue(&conv,global_actuation,0,REG_CURRENT_REF);"                   > of
    print " */\n"                                                                                > of
    print "#ifndef LIBREG_PARS_H"                                                                > of
    print "#define LIBREG_PARS_H\n"                                                              > of
    print "#define REG_N_PARS                    ", n_pars                                       > of
    print "#define REG_PAR_NOT_USED              (void*)0\n"                                     > of
                                           
    print "#define regConvParInitPointer(conv,par_name,value_p)        (conv)->pars.par_name.value=value_p"             > of
    print "#define regConvParInitValue(conv,par_name,index,init_value) (conv)->par_values.par_name[index]=init_value\n" > of

    for(i=0 ; i < n_flags ; i++)
    {                                                                             
        printf "#define %-40s(1<<%d)\n", flag[i], i                                              > of
    }

    print "\nstruct reg_par"                                                                     > of
    print "{"                                                                                    > of
    print "    void                     *value;"                                                 > of
    print "    void                     *copy_of_value;"                                         > of
    print "    uint32_t                  size_in_bytes;"                                         > of
    print "    uint32_t                  sizeof_type;"                                           > of
    print "    uint32_t                  flags;"                                                 > of
    print "};\n"                                                                                 > of
    print "struct reg_pars"                                                                      > of
    print "{"                                                                                    > of

    for(i=0 ; i < n_pars ; i++)
    {
        printf "    struct reg_par            %s;\n", par_variable[i]                            > of
    }

    print "};\n"                                                                                 > of
    print "struct reg_par_values"                                                                > of
    print "{"                                                                                    > of

    for(i=0 ; i < n_pars ; i++)
    {
        printf "    %-25s %-30s[%s];\n", par_type[i], par_variable[i], par_length[i]             > of
    }

    print "};\n"                                                                                 > of
    print "#endif // LIBREG_PARS_H\n"                                                            > of
    print "// EOF"                                                                               > of
  
    close(of)
      
# Generate parameter initialisation function in inc/init_pars.h
    
    of = "inc/init_pars.h"    # Set output file (of) to init_pars.h
    
    print "/*!"                                                                                  > of
    print " * @file  init_pars.h"                                                                > of
    print " * @brief Libreg generated parameter initialisation header file"                      > of
    print " *"                                                                                   > of
    print " * IMPORTANT - DO NOT EDIT - This file is generated from libreg/parameters/pars.csv"  > of
    print " *"                                                                                   > of
    print " * All libreg parameters are defined in pars.csv and this is transformed into"        > of
    print " * this header file (and pars.h) by libreg/parameters/pars.awk."                      > of
    print " *"                                                                                   > of
    print " * <h2>Contact</h2>"                                                                  > of
    print " *"                                                                                   > of
    print " * cclibs-devs@cern.ch"                                                               > of
    print " *"                                                                                   > of
    print " * <h2>Copyright</h2>"                                                                > of
    print " *"                                                                                   > of
    print " * Copyright CERN 2014. This project is released under the GNU Lesser General"        > of
    print " * Public License version 3."                                                         > of
    print " *"                                                                                   > of
    print " * <h2>License</h2>"                                                                  > of
    print " *"                                                                                   > of
    print " * This file is part of libreg."                                                      > of
    print " *"                                                                                   > of
    print " * libreg is free software: you can redistribute it and/or modify it under the"       > of
    print " * terms of the GNU Lesser General Public License as published by the Free"           > of
    print " * Software Foundation, either version 3 of the License, or (at your option)"         > of
    print " * any later version."                                                                > of
    print " *"                                                                                   > of
    print " * This program is distributed in the hope that it will be useful, but WITHOUT"       > of
    print " * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or"             > of
    print " * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License"       > of
    print " * for more details."                                                                 > of
    print " *"                                                                                   > of
    print " * You should have received a copy of the GNU Lesser General Public License"          > of
    print " * along with this program.  If not, see <http://www.gnu.org/licenses/>."             > of
    print " */\n"                                                                                > of
    print "#ifndef LIBREG_INIT_PARS_H"                                                           > of
    print "#define LIBREG_INIT_PARS_H\n"                                                         > of
    
    print "static void regConvParsInit(struct reg_conv *conv)"                                   > of
    print "{"                                                                                    > of
    print "    uint32_t i;"                                                                      > of


    for(i=0 ; i < n_pars ; i++)
    {
        printf "\n    conv->pars.%s.copy_of_value = &conv->par_values.%s;\n", par_variable[i], par_variable[i]  > of
        printf   "    conv->pars.%s.size_in_bytes = sizeof(conv->par_values.%s);\n", par_variable[i], par_variable[i]    > of
        printf   "    conv->pars.%s.sizeof_type   = sizeof(%s);\n", par_variable[i], par_type[i]                         > of
        printf   "    conv->pars.%s.flags         = %s;\n", par_variable[i], par_flags[i]                                > of
        
        if(par_length[i] == 1)
        {
            printf "    conv->par_values.%s[0]      = %s;\n", par_variable[i], par_init[i]                             > of
        }
        else
        {
            printf "    for(i=0;i<%s;i++) conv->par_values.%s[i]=%s;\n", par_length[i], par_variable[i], par_init[i]   > of
        }
    }

    print "}\n"                                                                                  > of
    print "#endif // LIBREG_INIT_PARS_H\n"                                                       > of
    print "// EOF"                                                                               > of
    close(of)
    
    exit 0
}
# EOF
