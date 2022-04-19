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
        QString const & name,
        JsonSchema const & schema,
        QHash<QString, JsonRecordDefinition> const & entityNameToJsonRecordDefinition) :
      self{self},
      name{name},
      schema{schema},
      entityNameToJsonRecordDefinition{entityNameToJsonRecordDefinition} {
      return;
   }

   /**
    * Destructor
    */
   ~impl() = default;

   /**
    * \brief Validate JSON file against schema, then call other functions to load its contents and store them in the DB
    *
    * \param documentData The contents of the JSON file, which the caller should already have loaded into memory
    * \param fileName Used only for logging / error message
    * \param domErrorHandler The rules for handling any errors encountered in the file - in particular which errors
    *                        should ignored and whether any adjustment needs to be made to the line numbers where
    *                        errors are found when creating user-readable messages.  (This latter is needed because in
    *                        some encodings, eg BeerJSON, we need to modify the in-memory copy of the JSON file before
    *                        parsing it.  See comments in the BeerJSON-specific files for more details.)
    * \param userMessage Any message that we want the top-level caller to display to the user (either about an error
    *                    or, in the event of success, summarising what was read in) should be appended to this string.
    *
    * \return true if file validated OK (including if there were "errors" that we can safely ignore)
    *         false if there was a problem that means it's not worth trying to read in the data from the file
    */
   bool validateLoadAndStoreInDb(QByteArray const & documentData,
                                 QString const & fileName,
                                 QTextStream & userMessage) {
      return false;
   }

   // Member variables for impl
   JsonCoding & self;
   QString name;
   JsonSchema const & schema;
   QHash<QString, JsonRecordDefinition> const entityNameToJsonRecordDefinition;

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

JsonCoding::JsonCoding(QString const & name,
                       JsonSchema const & schema,
                       QHash<QString, JsonRecordDefinition> const & entityNameToJsonRecordDefinition) :
   pimpl{std::make_unique<impl>(*this, name, schema, entityNameToJsonRecordDefinition)} {
   qDebug() << Q_FUNC_INFO;
   return;
}

// See https://herbsutter.com/gotw/_100/ for why we need to explicitly define the destructor here (and not in the header file)
JsonCoding::~JsonCoding() = default;

bool JsonCoding::isKnownJsonRecordType(QString recordName) const {
   return this->pimpl->entityNameToJsonRecordDefinition.contains(recordName);
}


std::shared_ptr<JsonRecord> JsonCoding::getNewJsonRecord(QString recordName) const {
   JsonCoding::JsonRecordConstructorWrapper constructorWrapper =
      this->pimpl->entityNameToJsonRecordDefinition.value(recordName).constructorWrapper;

   JsonRecord::FieldDefinitions const * fieldDefinitions =
      this->pimpl->entityNameToJsonRecordDefinition.value(recordName).fieldDefinitions;

   return std::shared_ptr<JsonRecord>(constructorWrapper(recordName, *this, *fieldDefinitions));
}


bool JsonCoding::validateLoadAndStoreInDb(QByteArray const & documentData,
                                         QString const & fileName,
                                         QTextStream & userMessage) const {
   return this->pimpl->validateLoadAndStoreInDb(documentData, fileName, userMessage);
}
