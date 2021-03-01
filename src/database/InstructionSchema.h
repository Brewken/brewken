/**
 * database/InstructionSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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
#ifndef INSTRUCTIONSCHEMA_H
#define INSTRUCTIONSCHEMA_H

#include <QString>
// Columns for the instruction table
static const QString kcolInstructionDirections("directions");
static const QString kcolInstructionHasTimer("hastimer");
static const QString kcolInstructionTimerValue("timervalue");
static const QString kcolInstructionCompleted("completed");
static const QString kcolInstructionInterval("interval");


static const QString kxmlPropDirections("directions");
static const QString kxmlPropHasTimer("hasTimer");
static const QString kxmlPropTimerValue("timervalue");
static const QString kxmlPropCompleted("completed");
static const QString kxmlPropInterval("interval");


// small cheat here. InstructionInRecipe tables have a spare column. Rather
// than define a unique header file, I am including it here.
static char const * const kpropInstructionNumber = "instruction_number";
static const QString kcolInstructionNumber("instruction_number");
#endif // INSTRUCTIONSCHEMA_H
