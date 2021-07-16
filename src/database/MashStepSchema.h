/**
 * database/MashStepSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
 *   • Mik Firestone <mikfire@gmail.com>
 *
 * Brewken is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Brewken is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef MASHSTEPSCHEMA_H
#define MASHSTEPSCHEMA_H

#include <QString>
// Columns for the mash table
static const QString kcolMashstepType("mstype");
static const QString kcolMashstepInfuseAmt("infuse_amount");
static const QString kcolMashstepStepTemp("step_temp");
static const QString kcolMashstepStepTime("step_time");
static const QString kcolMashstepRampTime("ramp_time");
static const QString kcolMashstepEndTemp("end_temp");
static const QString kcolMashstepInfuseTemp("infuse_temp");
static const QString kcolMashstepDecoctAmt("decoction_amount");
static const QString kcolMashstepStepNumber("step_number");
#endif
