/**
 * StyleSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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

#include <QString>
#ifndef _STYLETABLESCHEMA_H
#define _STYLETABLESCHEMA_H

// Columns for the style table
static const QString kcolStyleType("s_type");
static const QString kcolStyleCat("category");
static const QString kcolStyleCatNum("category_number");
static const QString kcolStyleLetter("style_letter");
static const QString kcolStyleGuide("style_guide");
static const QString kcolStyleOGMin("og_min");
static const QString kcolStyleOGMax("og_max");
static const QString kcolStyleFGMin("fg_min");
static const QString kcolStyleFGMax("fg_max");
static const QString kcolStyleIBUMin("ibu_min");
static const QString kcolStyleIBUMax("ibu_max");
static const QString kcolStyleColorMin("color_min");
static const QString kcolStyleColorMax("color_max");
static const QString kcolStyleABVMin("abv_min");
static const QString kcolStyleABVMax("abv_max");
static const QString kcolStyleCarbMin("carb_min");
static const QString kcolStyleCarbMax("carb_max");
static const QString kcolStyleProfile("profile");
static const QString kcolStyleIngreds("ingredients");
static const QString kcolStyleExamples("examples");

// properties for objects

// these are also in the main constants file
static const QString kxmlPropCat("CATEGORY");
static const QString kxmlPropCatNum("CATEGORY_NUMBER");
static const QString kxmlPropLetter("STYLE_LETTER");
static const QString kxmlPropGuide("STYLE_GUIDE");
static const QString kxmlPropOGMin("OG_MIN");
static const QString kxmlPropOGMax("OG_MAX");
static const QString kxmlPropFGMin("FG_MIN");
static const QString kxmlPropFGMax("FG_MAX");
static const QString kxmlPropIBUMin("IBU_MIN");
static const QString kxmlPropIBUMax("IBU_MAX");
static const QString kxmlPropColorMin("COLOR_MIN");
static const QString kxmlPropColorMax("COLOR_MAX");
static const QString kxmlPropCarbMin("CARB_MIN");
static const QString kxmlPropCarbMax("CARB_MAX");
static const QString kxmlPropABVMin("ABV_MIN");
static const QString kxmlPropABVMax("ABV_MAX");
static const QString kxmlPropProfile("PROFILE");
static const QString kxmlPropIngreds("INGREDIENTS");
static const QString kxmlPropExamples("EXAMPLES");

#endif // _STYLETABLESCHEMA_H
