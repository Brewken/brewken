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

#include "json/JsonUtils.h"

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
        std::initializer_list<JsonRecordDefinition> jsonRecordDefinitions) :
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

   // Member variables for impl
   JsonCoding & self;
   QString const name;
   QString const version;
   JsonSchema::Id const schemaId;
   QVector<JsonRecordDefinition> const jsonRecordDefinitions;

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

JsonCoding::JsonCoding(char const * name,
                       char const * const version,
                       JsonSchema::Id const schemaId,
                       std::initializer_list<JsonRecordDefinition> jsonRecordDefinitions) :
   pimpl{std::make_unique<impl>(*this, name, version, schemaId, jsonRecordDefinitions)} {
   qDebug() << Q_FUNC_INFO;
   return;
}

// See https://herbsutter.com/gotw/_100/ for why we need to explicitly define the destructor here (and not in the header file)
JsonCoding::~JsonCoding() = default;

bool JsonCoding::isKnownJsonRecordDefinition(QString recordName) const {
   auto result = std::find_if(
      this->pimpl->jsonRecordDefinitions.begin(),
      this->pimpl->jsonRecordDefinitions.end(),
      [&recordName](JsonRecordDefinition const & recordDefn){return recordDefn.recordName == recordName;}
   );
   return result != this->pimpl->jsonRecordDefinitions.end();
}

JsonRecordDefinition const & JsonCoding::getRoot() const {
   // The root element is the one with no corresponding named entity
   auto root = std::find_if(
      this->pimpl->jsonRecordDefinitions.cbegin(),
      this->pimpl->jsonRecordDefinitions.cend(),
      [](JsonRecordDefinition const & recordDefn){return recordDefn.namedEntityClassName == "";}
   );
   // It's a coding error if we didn't find a root element
   Q_ASSERT(root != this->pimpl->jsonRecordDefinitions.end());
   return *root;
}


/*std::shared_ptr<JsonRecord> JsonCoding::getNewJsonRecord(QString recordName) const {
   JsonCoding::JsonRecordConstructorWrapper constructorWrapper =
      this->pimpl->entityNameToJsonRecordDefinition.value(recordName).constructorWrapper;

   JsonRecordDefinition::FieldDefinitions const * fieldDefinitions =
      this->pimpl->entityNameToJsonRecordDefinition.value(recordName).fieldDefinitions;

   return std::shared_ptr<JsonRecord>(constructorWrapper(recordName, *this, *fieldDefinitions));
}*/

bool JsonCoding::validateLoadAndStoreInDb(boost::json::value & inputDocument,
                                          QTextStream & userMessage) const {
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

   qDebug() << Q_FUNC_INFO << "Schema validation succeeded";


   // Now we've loaded the JSON document into memory and determined that it's valid against its schema, we need to
   // extract the data from it
   //
   // Per https://www.json.org/json-en.html, a JSON object is an unordered set of name/value pairs, so there's no
   // constraint about what order we parse things
   //

   //
   // Look at the root object first
   //
   JsonRecordDefinition const & root = this->getRoot();
   qDebug() << Q_FUNC_INFO << "Looking at field definitinos of root element (" << root.recordName << ")";

   for (auto & fieldDefinition : root.fieldDefinitions) {
      qDebug() << Q_FUNC_INFO << "Looking at" << fieldDefinition.xPath;
      // .:TODO:. IMPLEMENT THIS!
   }


   // TBD the rest of this stuff is old diagnostic and can probably be binned once we have the main implementation done

   // We're expecting the root of the JSON document to be an object named "beerjson".  This should have been
   // established by the validation above.
   Q_ASSERT(inputDocument.is_object());
   boost::json::object documentRoot = inputDocument.as_object();
   Q_ASSERT(documentRoot.contains("beerjson"));
   Q_ASSERT(documentRoot["beerjson"].is_object());
   boost::json::object beerJson = documentRoot["beerjson"].as_object();

   //
   // See comments in JsonRecord.h for more on the structure of a JSON file
   //
   // For each type of object T that we want to read from a JSON file (eg T is Recipe, Hop, etc) we need to provide
   // an implementation of the following function:
   //    T tag_invoke(value_to_tag<T>, boost::json::value const & jv);
   // Note:
   //    (1) The value_to_tag<T> type is empty and has no members.  It is just a trick used by the library to ensure
   //        the right overload of tag_invoke is called.
   //    (2) We do not call tag_invoke() ourselves.  Instead, we call
   //        template<class T> T boost::json::value_to(boost::json::value const & jv).
   // Inside tag_invoke(), nothing hugely clever is happening.  We just extract each of the fields we care about from
   // jv into a new object of type T, for each field calling the relevant specialisation of boost::json::value_to(),
   // eg something along the following lines:
   //    T newObject;
   //    newObject.id = value_to<int>(jv.as_object().at("id"));
   //    newObject.name = value_to<std::string>(jv.as_object().at("name"));
   //    ...
   //    return newObject;
   // Note that value_to<std::string> will throw an exception if its parameter is not actually a string, etc.
   // Of course we would like to do all these field mappings in data rather than in code, so we take a slightly more
   // elaborate approach.
   //

   /// TEMPORARY DIAGNOSTIC
   boost::json::value const * recs = beerJson.if_contains("recipes");
   if (recs) {
      qDebug() << Q_FUNC_INFO << "Recipes" << *recs;
      if (recs->is_array()) {
         boost::json::array const & recipeList = recs->get_array();
         qDebug() << Q_FUNC_INFO << recipeList.size() << "recipes";
         for (auto rr : recipeList) {
            qDebug() << Q_FUNC_INFO << rr;
         }
      }
   }

   // Version is a JSON number (in JavaScript’s double-precision floating-point format)
   boost::json::string * bjVersion = beerJson["version"].if_string();
   if (bjVersion) {
      qDebug() << Q_FUNC_INFO << "Version" << bjVersion->c_str();
   }
/*      std::string bjv2 = boost::json::value_to<std::string>(beerJson["version"]);
   qDebug() << Q_FUNC_INFO << "Version" << bjv2.c_str();
*/
   for (auto ii : beerJson) {
      // .:TODO:. This gives keys but not values...
      boost::json::value const & val = ii.value();
      qDebug() << Q_FUNC_INFO << "Key" << ii.key().data() << "(" << val.kind() << ")" << val;
      if (val.is_string()) {
         qDebug() << Q_FUNC_INFO << "Value" << val.as_string().c_str();
      } else if (val.is_double()) {
         qDebug() << Q_FUNC_INFO << "Value" << val.as_double();
      }
   }

   /////

   userMessage << "BeerJSON support is not yet complete!";
   return false;

   /// TODO THE REST!
   return false;
}
