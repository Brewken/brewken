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

#include <QDebug>

namespace {
   EnumStringMapping const fieldTypeToName {
      {QObject::tr("Bool"                      ), JsonRecordDefinition::FieldType::Bool                      },
      {QObject::tr("Int"                       ), JsonRecordDefinition::FieldType::Int                       },
      {QObject::tr("UInt"                      ), JsonRecordDefinition::FieldType::UInt                      },
      {QObject::tr("Double"                    ), JsonRecordDefinition::FieldType::Double                    },
      {QObject::tr("String"                    ), JsonRecordDefinition::FieldType::String                    },
      {QObject::tr("Enum"                      ), JsonRecordDefinition::FieldType::Enum                      },
      {QObject::tr("Array"                     ), JsonRecordDefinition::FieldType::Array                     },
      {QObject::tr("Date"                      ), JsonRecordDefinition::FieldType::Date                      },
      {QObject::tr("MeasurementWithUnits"      ), JsonRecordDefinition::FieldType::MeasurementWithUnits      },
      {QObject::tr("OneOfMeasurementsWithUnits"), JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits},
      {QObject::tr("SingleUnitValue"           ), JsonRecordDefinition::FieldType::SingleUnitValue           },
      {QObject::tr("RequiredConstant"          ), JsonRecordDefinition::FieldType::RequiredConstant          }
   };
}

JsonRecordDefinition::JsonRecordDefinition(
   char const * const recordName,
   char const * const namedEntityClassName,
   JsonRecordConstructorWrapper jsonRecordConstructorWrapper,
   std::initializer_list<JsonRecordDefinition::FieldDefinition> fieldDefinitions
) :
   recordName{recordName},
   namedEntityClassName{namedEntityClassName},
   jsonRecordConstructorWrapper{jsonRecordConstructorWrapper},
   fieldDefinitions{fieldDefinitions} {
   return;
}

JsonRecordDefinition::JsonRecordDefinition(
   char const * const recordName,
   char const * const namedEntityClassName,
   JsonRecordConstructorWrapper jsonRecordConstructorWrapper,
   std::initializer_list< std::initializer_list<FieldDefinition> > fieldDefinitionLists
) :
   recordName{recordName},
   namedEntityClassName{namedEntityClassName},
   jsonRecordConstructorWrapper{jsonRecordConstructorWrapper},
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


template<class S>
S & operator<<(S & stream, JsonRecordDefinition::FieldType const fieldType) {
   std::optional<QString> fieldTypeAsString = fieldTypeToName.enumToString(fieldType);
   if (fieldTypeAsString) {
      stream << *fieldTypeAsString;
   } else {
      // This is a coding error, so stop (after logging) on a debug build
      stream << "Unrecognised field type: " << static_cast<int>(fieldType);
      Q_ASSERT(false);
   }
   return stream;
}

//
// Instantiate the above template function for the types that are going to use it
// (This is all just a trick to allow the template definition to be here in the .cpp file and not in the header.)
//
template QDebug & operator<<(QDebug & stream, JsonRecordDefinition::FieldType const fieldType);
template QTextStream & operator<<(QTextStream & stream, JsonRecordDefinition::FieldType const fieldType);
