/*======================================================================================================================
 * serialization/SerializationRecordDefinition.h is part of Brewken, and is copyright the following authors 2020-2023:
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
#ifndef SERIALIZATION_SERIALIZATIONRECORDDEFINITION_H
#define SERIALIZATION_SERIALIZATIONRECORDDEFINITION_H
#pragma once

#include "utils/BtStringConst.h"
#include "utils/TypeLookup.h"

/**
 * \brief Common base class for \c XmlRecordDefinition and \c JsonRecordDefinition
 *
 * TODO: We could probably do some templating to bring more up into this class
 */
class SerializationRecordDefinition {
public:
   SerializationRecordDefinition(char       const * const recordName,
                                 TypeLookup const * const typeLookup,
                                 char       const * const namedEntityClassName) :
      m_recordName{recordName},
      m_typeLookup{typeLookup},
      m_namedEntityClassName{namedEntityClassName} {
      return;
   }
   ~SerializationRecordDefinition() = default;

   BtStringConst const      m_recordName;

   TypeLookup const * const m_typeLookup;

   // The name of the class of object contained in this type of record, eg "Hop", "Yeast", etc.
   // Blank for the root record (which is just a container and doesn't have a NamedEntity).
   BtStringConst const m_namedEntityClassName;
};

#endif
