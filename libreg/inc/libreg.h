/*!
 * @file  libreg.h
 * @brief Converter Control Regulation library header file
 *
 * The converter control regulation library provides support for:
 * <ol>
 * <li>Field, Current and voltage limits</li>
 * <li>RST-based regulation</li>
 * <li>Regulation error calculation</li>
 * <li>Voltage source simulation</li>
 * <li>Magnet load definition and simulation</li>
 * </ol>
 * 
 * <h2>Contact</h2>
 *
 * cclibs-devs@cern.ch
 *
 * <h2>Copyright</h2>
 *
 * Copyright CERN 2014. This project is released under the GNU Lesser General
 * Public License version 3.
 * 
 * <h2>License</h2>
 *
 * This file is part of libreg.
 *
 * libreg is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBREG_H
#define LIBREG_H

// Libreg enum constants

/*!
 * DISABLED/ENABLED constants
 */
enum reg_enabled_disabled
{
    REG_DISABLED,                                       //!< Disabled
    REG_ENABLED                                         //!< Enabled
};

/*!
 * Regulation parameters actuation (voltage or current)
 */
enum reg_actuation
{
    REG_VOLTAGE_REF,                                    //!< Actuation is a voltage reference
    REG_CURRENT_REF                                     //!< Actuation is a current reference
};

/*!
 * Regulation parameters source (operational or test)
 */
enum reg_rst_source
{
    REG_OPERATIONAL_RST_PARS,                           //!< Use operational RST parameters
    REG_TEST_RST_PARS                                   //!< Use test RST parameters
};

/*!
 * Regulation error rate control
 */
enum reg_err_rate
{
    REG_ERR_RATE_REGULATION,                            //!< Calculate regulation error at regulation rate
    REG_ERR_RATE_MEASUREMENT                            //!< Calculate regulation error at measurement rate
};

// Include all libreg header files

#include <stdint.h>
#include <stdbool.h>
#include <libreg/delay.h>
#include <libreg/err.h>
#include <libreg/lim.h>
#include <libreg/load.h>
#include <libreg/meas.h>
#include <libreg/rst.h>
#include <libreg/sim.h>
#include <pars.h>
#include <libreg/conv.h>

#endif // LIBREG_H

// EOF
