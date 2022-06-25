/*======================================================================================================================
 * json/JsonRecordDefinition.cpp is part of Brewken, and is copyright the following authors 2020-2022:
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
#include "json/JsonRecordDefinition.h"

JsonRecordDefinition::JsonRecordDefinition(
   char const * const recordName,
   char const * const namedEntityClassName,
   std::initializer_list<JsonRecordDefinition::FieldDefinition> fieldDefinitions
) :
   recordName{recordName},
   namedEntityClassName{namedEntityClassName},
   fieldDefinitions{fieldDefinitions} {
   return;
}

JsonRecordDefinition::JsonRecordDefinition(
   char const * const recordName,
   char const * const namedEntityClassName,
   std::initializer_list< std::initializer_list<FieldDefinition> > fieldDefinitionLists
) :
   recordName{recordName},
   namedEntityClassName{namedEntityClassName},
   fieldDefinitions{} {
   // This is a bit clunky, but it works and the inefficiency is a one-off cost at start-up
   for (auto const & list : fieldDefinitionLists) {
      // After you've initialised a const, you can't modify it, even in the constructor, unless you cast away the
      // constness (is that a word?) via a pointer or reference to tell the compiler you really do want to modify the
      // member variable.
      std::vector<JsonRecordDefinition::FieldDefinition> & myFieldDefinitions =
         const_cast<std::vector<JsonRecordDefinition::FieldDefinition> &>(this->fieldDefinitions);
      // You can't do the following with QVector, which is why we're using std::vector here
      myFieldDefinitions.insert(myFieldDefinitions.end(), list.begin(), list.end());
   }
   return;
}
