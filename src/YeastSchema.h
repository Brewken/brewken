/**
 * YeastSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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

#ifndef _YEASTTABLESCHEMA_H
#define _YEASTTABLESCHEMA_H

#include <QString>
// Columns for the yeast table
// What isn't here (like name) is defined in TableSchemaConstants
static const QString kcolYeastType("ytype");
static const QString kcolYeastForm("form");
static const QString kcolYeastAmount("amount");
static const QString kcolYeastQuanta("quanta");
static const QString kcolYeastAmtIsWgt("amount_is_weight");
static const QString kcolYeastLab("laboratory");
static const QString kcolYeastProductID("product_id");
static const QString kcolYeastMinTemp("min_temperature");
static const QString kcolYeastMaxTemp("max_temperature");
static const QString kcolYeastFloc("flocculation");
static const QString kcolYeastAtten("attenuation");
static const QString kcolYeastBestFor("best_for");
static const QString kcolYeastTimesCultd("times_cultured");
static const QString kcolYeastMaxReuse("max_reuse");
static const QString kcolYeastAddToSec("add_to_secondary");

// properties for objects
 static char const * const kpropQuanta = "quanta";

// XML properties
// Note -- no XML props for type, form and flocculation
static const QString kxmlPropLab("LABORATORY");
static const QString kxmlPropProductID("PRODUCT_ID");
static const QString kxmlPropMinTemp("MIN_TEMPERATURE");
static const QString kxmlPropMaxTemp("MAX_TEMPERATURE");
static const QString kxmlPropBestFor("BEST_FOR");
static const QString kxmlPropTimesCultd("TIMES_CULTURED");
static const QString kxmlPropMaxReuse("MAX_REUSE");
static const QString kxmlPropAddToSec("ADD_TO_SECONDARY");
static const QString kxmlPropFloc("FLOCCULATION");

#endif // _YEASTTABLESCHEMA_H
