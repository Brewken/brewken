/**
 * WaterSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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

#ifndef _WATERSCHEMA_H
#define _WATERSCHEMA_H

#include <QString>
// Columns for the yeast table
// What isn't here (like name) is defined in TableSchemaConstants
static const QString kcolWaterCalcium("calcium");
static const QString kcolWaterBiCarbonate("bicarbonate");
static const QString kcolWaterSulfate("sulfate");
static const QString kcolWaterChloride("chloride");
static const QString kcolWaterSodium("sodium");
static const QString kcolWaterMagnesium("magnesium");
static const QString kcolWaterAlkalinity("alkalinity");
static const QString kcolWaterMashRO("mash_ro");
static const QString kcolWaterSpargeRO("sparge_ro");
static const QString kcolWaterAsHCO3("as_hco3");
static const QString kcolWaterType("wtype");

// properties for objects

// XML properties
static const QString kxmlPropCalcium("CALCIUM");
static const QString kxmlPropBiCarbonate("BICARBONATE");
static const QString kxmlPropSulfate("SULFATE");
static const QString kxmlPropChloride("CHLORIDE");
static const QString kxmlPropSodium("SODIUM");
static const QString kxmlPropMagnesium("MAGNESIUM");

#endif // _WATERSCHEMA_H
