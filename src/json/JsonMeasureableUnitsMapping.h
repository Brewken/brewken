/*======================================================================================================================
 * json/JsonMeasureableUnitsMapping.h is part of Brewken, and is copyright the following authors 2022:
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
#ifndef JSON_JSONMEASUREABLEUNITSMAPPING_H
#define JSON_JSONMEASUREABLEUNITSMAPPING_H
#pragma once

#include <QMap>
#include <QString>

#include "measurement/Unit.h"

/**
 * \brief Maps a set of BeerJSON "measurable units" to our internal data structures (\c Measurement::Unit in particular)
 *
 *        In BeerJSON at least, a lot of values are given as value:unit pairs.  (This contrasts with BeerXML and our
 *        internal storage, where everything is STORED in standard, usually SI, units and conversion to other units is
 *        only done for display and entry.)
 *
 * \c scaleKey is the key used to pull out the string value representing the units of the measurement, usually "unit" in
 *             BeerJSON
 * \c scaleValue is the key used to pull out the double value representing the measurement itself
 * \c scaleToUnits tells us how to map the string unit value to one of our \c Measurement::Unit constants
 */
struct JsonMeasureableUnitsMapping {
   char const * const unitField;  // Usually "unit"
   char const * const valueField; // Usually "value"
   QMap<QString, Measurement::Unit const *> nameToUnit;
};

#endif
