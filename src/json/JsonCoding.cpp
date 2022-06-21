/*======================================================================================================================
 * json/JsonCoding.cpp is part of Brewken, and is copyright the following authors 2020-2022:
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
#include "json/JsonCoding.h"

#include <algorithm>

#include <QDebug>
#include <QFile>

//
// Private implementation class for JsonCoding
//
class JsonCoding::impl {
public:

   /**
    * Constructor
    */
   impl(JsonCoding & self,
        char const * name,
        char const * const version,
        JsonSchema::Id const schemaId,
        std::initializer_list<JsonRecordType> jsonRecordDefinitions) :
      self{self},
      name{name},
      version{version},
      schemaId{schemaId},
      jsonRecordDefinitions{jsonRecordDefinitions} {
      return;
   }

   /**
    * Destructor
    */
   ~impl() = default;

   /**
    * \brief Validate JSON file against schema, then call other functions to load its contents and store them in the DB
    *
    * \param fileName The JSON file to read
    * \param userMessage Any message that we want the top-level caller to display to the user (either about an error
    *                    or, in the event of success, summarising what was read in) should be appended to this string.
    *
    * \return true if file validated OK (including if there were "errors" that we can safely ignore)
    *         false if there was a problem that means it's not worth trying to read in the data from the file
    */
   bool validateLoadAndStoreInDb(QString const & fileName,
                                 QTextStream & userMessage) {
      return false;
   }

   // Member variables for impl
   JsonCoding & self;
   QString const name;
   QString const version;
   JsonSchema::Id const schemaId;
   QVector<JsonRecordType> const jsonRecordDefinitions;

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

JsonCoding::JsonCoding(char const * name,
                       char const * const version,
                       JsonSchema::Id const schemaId,
                       std::initializer_list<JsonRecordType> jsonRecordDefinitions) :
   pimpl{std::make_unique<impl>(*this, name, version, schemaId, jsonRecordDefinitions)} {
   qDebug() << Q_FUNC_INFO;
   return;
}

// See https://herbsutter.com/gotw/_100/ for why we need to explicitly define the destructor here (and not in the header file)
JsonCoding::~JsonCoding() = default;

bool JsonCoding::validateAgainstSchema(boost::json::value & inputDocument, QTextStream & userMessage) const {
   try {
      JsonSchema const & schema = JsonSchema::instance(this->pimpl->schemaId);
      if (!schema.validate(inputDocument, userMessage)) {
         qWarning() << Q_FUNC_INFO << "Schema validation failed";
         return false;
      }

   } catch (std::exception const & exception) {
      qWarning() <<
         Q_FUNC_INFO << "Caught exception while validating JSON file:" << exception.what();
      userMessage << exception.what();
      return false;
   }
   return true;
}


bool JsonCoding::isKnownJsonRecordType(QString recordName) const {
   auto result = std::find_if(
      this->pimpl->jsonRecordDefinitions.begin(),
      this->pimpl->jsonRecordDefinitions.end(),
      [&recordName](JsonRecordType const & recordType){return recordType.recordName == recordName;}
   );
   return result != this->pimpl->jsonRecordDefinitions.end();
}


/*std::shared_ptr<JsonRecord> JsonCoding::getNewJsonRecord(QString recordName) const {
   JsonCoding::JsonRecordConstructorWrapper constructorWrapper =
      this->pimpl->entityNameToJsonRecordDefinition.value(recordName).constructorWrapper;

   JsonRecordType::FieldDefinitions const * fieldDefinitions =
      this->pimpl->entityNameToJsonRecordDefinition.value(recordName).fieldDefinitions;

   return std::shared_ptr<JsonRecord>(constructorWrapper(recordName, *this, *fieldDefinitions));
}*/

bool JsonCoding::validateLoadAndStoreInDb(QString const & fileName,
                                          QTextStream & userMessage) const {
   return this->pimpl->validateLoadAndStoreInDb(fileName, userMessage);
}
