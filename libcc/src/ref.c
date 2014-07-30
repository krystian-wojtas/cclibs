/*---------------------------------------------------------------------------------------------------------*\
  File:     ref.c                                                                        Copyright CERN 2014

  License:  This file is part of libcc.

            libcc is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  This provides a higher level access to libreg with functions that combine all the elements
            needed to regulate current or field in converter.
\*---------------------------------------------------------------------------------------------------------*/

#include <string.h>
#include "libcc.h"

//-----------------------------------------------------------------------------------------------------------
// Non-Real-Time Functions - do not call these from the real-time thread or interrupt
//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------
// Real-Time Functions
//-----------------------------------------------------------------------------------------------------------
enum reg_mode ccRef(float *ref)
{
    return(REG_VOLTAGE);
}
// EOF
