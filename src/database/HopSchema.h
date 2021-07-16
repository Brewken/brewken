/**
 * database/HopSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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
#ifndef HOPSCHEMA_H
#define HOPSCHEMA_H

#include <QString>

// Columns for the hop table
static const QString kcolHopForm("form");
static const QString kcolHopType("htype");
static const QString kcolHopOrigin("origin");
static const QString kcolHopAlpha("alpha");
static const QString kcolHopAmount("amount");
static const QString kcolHopBeta("beta");
static const QString kcolHopHSI("hsi");
static const QString kcolHopHumulene("humulene");
static const QString kcolHopCaryophyllene("caryophyllene");
static const QString kcolHopCohumulone("cohumulone");
static const QString kcolHopMyrcene("myrcene");

#endif
