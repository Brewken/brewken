/*======================================================================================================================
 * json/JsonSingleUnitSpecifier.h.h is part of Brewken, and is copyright the following authors 2022:
 *   • Matt Young <mfsy@yahoo.com>
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
 =====================================================================================================================*/
#ifndef JSON_JSONSINGLEUNITSPECIFIER_H
#define JSON_JSONSINGLEUNITSPECIFIER_H
#pragma once

#include <QMap>
#include <QString>

/**
 * \brief Defines the expected unit for an JSON value:unit pairs that only ever uses one unit (eg percentages in
 *        BeerJSON).
 *
 *        The advantage of using this struct is that it allows us to assert that the unit we're reading is what we
 *        expected, which should catch programming errors (eg if we try to read a percentage field into a pH attribute).
 *
 * \c expectedUnit tells us what we are expecting the unit to be
 * \c unitField is the key used to pull out the string value representing the units of the measurement, usually "unit" in
 *              BeerJSON
 * \c valueField is the key used to pull out the double value representing the measurement itself
 */
struct JsonSingleUnitSpecifier {
   QString expectedUnit;
   char const * const unitField = "unit";
   char const * const valueField = "value";
};

#endif
