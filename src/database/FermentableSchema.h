/**
 * database/FermentableSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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
#ifndef FERMENTABLESCHEMA_H
#define FERMENTABLESCHEMA_H

// These will collide, so I define them once in the TableSchemaConst file
// const QString kcolEquipName("name");
// const QString kcolEquipNotes("notes");
//
// Column names
const QString kcolFermType("ftype");
static const QString kcolFermAmount("amount");
// no inventory column
static const QString kcolFermYield("yield");
static const QString kcolFermColor("color");
static const QString kcolFermAddAfterBoil("add_after_boil");
static const QString kcolFermOrigin("origin");
static const QString kcolFermSupplier("supplier");
static const QString kcolFermCoarseFineDiff("coarse_fine_diff");
static const QString kcolFermMoisture("moisture");
static const QString kcolFermDiastaticPower("diastatic_power");
static const QString kcolFermProtein("protein");
static const QString kcolFermMaxInBatch("max_in_batch");
static const QString kcolFermRecommendMash("recommend_mash");
static const QString kcolFermIsMashed("is_mashed");
static const QString kcolFermIBUGalPerLb("ibu_gal_per_lb");
#endif
