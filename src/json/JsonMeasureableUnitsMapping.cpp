/*======================================================================================================================
 * json/JsonMeasureableUnitsMapping.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "json/JsonMeasureableUnitsMapping.h"

#include <stdexcept>

#include <QDebug>

std::string_view JsonMeasureableUnitsMapping::getNameForUnit(Measurement::Unit const & unitToMatch) const {
   // We could use std::find_if here with a lambda, but a loop with structured bindings is more concise in this instance
   for (auto const [unitName, unit] : this->nameToUnit) {
      if (unit == &unitToMatch) {
         return unitName;
      }
   }

   // It's almost certainly a coding error if we get here - because we should always have a mapping for a Unit we use.
   qCritical() <<
      Q_FUNC_INFO << "No name found for Unit" << unitToMatch << "while searching mapping for" <<
      this->getPhysicalQuantity();
   throw std::invalid_argument("Unit not found in JsonMeasureableUnitsMapping");
}

Measurement::PhysicalQuantity JsonMeasureableUnitsMapping::getPhysicalQuantity() const {
   // We assume that each mapping only holds Units corresponding to one PhysicalQuantity, so it suffices to return the
   // PhysicalQuantity of the first element in the map`
   return this->nameToUnit.begin()->second->getPhysicalQuantity();
}
