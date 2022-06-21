/*======================================================================================================================
 * json/JsonRecordType.cpp is part of Brewken, and is copyright the following authors 2020-2022:
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
#include "json/JsonRecordType.h"

JsonRecordType::JsonRecordType(char const * const recordName,
                               char const * const namedEntityClassName,
///                               JsonCoding const & jsonCoding,
                               JsonRecordType::FieldDefinitions const & fieldDefinitions) :
   recordName{recordName},
   namedEntityClassName{namedEntityClassName},
///   jsonCoding{jsonCoding},
   fieldDefinitions{fieldDefinitions} {
   return;
}

BtStringConst const & JsonRecordType::getRecordName() const {
   return this->recordName;
}
