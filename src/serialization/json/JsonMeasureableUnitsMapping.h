/*======================================================================================================================
 * serialization/json/JsonMeasureableUnitsMapping.h is part of Brewken, and is copyright the following authors 2022-2023:
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
#ifndef SERIALIZATION_JSON_JSONMEASUREABLEUNITSMAPPING_H
#define SERIALIZATION_JSON_JSONMEASUREABLEUNITSMAPPING_H
#pragma once

#include <map>
#include <string_view>

#include "serialization/json/JsonXPath.h"
#include "measurement/Unit.h"


/**
 * \brief Maps a set of BeerJSON "measurable units" to our internal data structures (\c Measurement::Unit in particular)
 *
 *        In BeerJSON at least, a lot of values are given as value:unit pairs.  (This contrasts with BeerXML and our
 *        internal storage, where everything is STORED in standard, usually SI, units and conversion to other units is
 *        only done for display and entry.)
 *
 * \c nameToUnit tells us how to map the string unit value to one of our \c Measurement::Unit constants, all of which
 *               should be for the same \c Measurement::PhysicalQuantity
 * \c unitField is the key used to pull out the string value representing the units of the measurement, usually "unit"
 *              in BeerJSON
 * \c valueField is the key used to pull out the double value representing the measurement itself
 */
struct JsonMeasureableUnitsMapping {
   // We could use boost::bimap here, but it doesn't support brace initializer lists, which is a bit tiresome.  Given
   // that the size of this mapping is always small (<20 entries), even doing linear search is not going to be that
   // complicated.
   //
   // We use std::string_view here rather than QString because there's less conversion to do when working with
   // Boost.JSON.  We use std::map rather than QMap because it's easier to search both sides of the map (ie search by
   // value as well as search by key).
   std::map<std::string_view, Measurement::Unit const *> nameToUnit;

   JsonXPath const unitField = JsonXPath{"unit"};
   JsonXPath const valueField = JsonXPath{"value"};

   Measurement::PhysicalQuantity getPhysicalQuantity() const;

   /**
    * \brief For a given \c Measurement::Unit, return the name.  Caller's responsibility to ensure this mapping holds
    *        units of the corresponding \c Measurement::PhysicalQuantity
    */
   std::string_view getNameForUnit(Measurement::Unit const & unitToMatch) const;
};

#endif
