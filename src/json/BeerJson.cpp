/*======================================================================================================================
 * json/BeerJson.cpp is part of Brewken, and is copyright the following authors 2021:
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
#include "json/BeerJson.h"

// We could just include <boost/json.hpp> which pulls all the Boost.JSON headers in, but that seems overkill
#include <boost/json/parse_options.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/string.hpp>

#include <valijson/adapters/boost_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

#include <QApplication>
#include <QDebug>
#include <QString>
#include <QTextStream>

#include "json/JsonSchema.h"
#include "json/JsonUtils.h"
#include "model/Recipe.h"

//
//                                 ****************************************************
//                                 * General note about JSON libraries and frameworks *
//                                 ****************************************************
//
// There are several C++ JSON libraries, including some Qt classes (https://doc.qt.io/qt-5/json.html), RapidJSON
// (https://rapidjson.org/) "JSON for Modern C++" AKA "nlohmann JSON" (https://github.com/nlohmann/json) and Boost.JSON
// (https://www.boost.org/doc/libs/1_77_0/libs/json/doc/html/index.html).
//
// I am reluctant to use the Qt classes, even though we are a Qt app, because Qt have a history of dropping support for
// "non-core" features (see comments in xml/XmlCoding.cpp for example).
//
// The Boost library is one of the newer implementations but has some design advantages over other libraries (see
// https://www.boost.org/doc/libs/1_77_0/libs/json/doc/html/json/comparison.html).  Boost libraries in general are seen
// to be high quality and several of them have become the basis for C++ Standard Library features.  So, using the Boost
// library seems like a safe bet.
//
// Schema validation in JSON is also a relatively new thing.  There are several C++ validators (see
// https://json-schema.org/implementations.html#validator-c++) but we like Valijson
// (https://github.com/tristanpenman/valijson) in particular because it is not tied to one underlying JSON library.
//
//

namespace {
   /**
    * \brief Amongst other things, this is the callback Valijson uses to obtain referenced schema documents
    */
/*   boost::json::value const * fetchReferencedDocument(std::string const & uri) {
      // We assume that we're only going to be asked to fetch relative documents, not go out and fetch something over
      // HTTP etc.  This is reasonable, because we're talking about schema documents here which we control and ship with
      // the product.
      QString schemaFilePath = QString(":/schemas/beerjson/1.0/%1").arg(uri.c_str());

      qDebug() << Q_FUNC_INFO << "Reading" << uri.c_str() << "as" << schemaFilePath;

      QFile schemaFile(schemaFilePath);
      if (!schemaFile.open(QIODevice::ReadOnly)) {
         // This should pretty much never happen, as we're loading from a QResource compiled into the binary rather
         // than reading from the file system at run-time.
         qCritical() <<
            Q_FUNC_INFO << "Could not open schema file resource " << schemaFile.fileName() << " for reading";
         throw std::runtime_error("Could not open schema file resource");
      }

      // readAll doesn't report errors, but we really aren't expecting any on reading a QResource
      QByteArray schemaData = schemaFile.readAll();
      qDebug() <<
         Q_FUNC_INFO << "Schema file " << schemaFile.fileName() << ": " << schemaData.length() << " bytes";

      boost::json::string_view schemaStringView{schemaData.data()};
      boost::json::parse_options schemaParseOptions;
      schemaParseOptions.allow_comments = true;
      boost::json::error_code errorCode;
      boost::json::value * schemaDocument = new boost::json::value(boost::json::parse(schemaStringView, errorCode, {}, schemaParseOptions));
      if (errorCode) {
         // This is almost certainly a coding error, since we're the ones creating and shipping the schema file!
         qCritical() <<
            Q_FUNC_INFO << "Parsing schema file" << schemaFile.fileName() << "failed:" <<
            errorCode.message().c_str();
         throw std::runtime_error("Error parsing schema file");
      }
      qDebug() << Q_FUNC_INFO << "Boost.JSON schema document read";

      return schemaDocument;
   }*/

   /**
    * \brief Called from Valijson to free a resource obtained from \c fetchReferencedDocument()
    */
/*   void freeReferencedDocument(boost::json::value const * document) {
      qDebug() << Q_FUNC_INFO;
      delete document;
      return;
   }*/

   // This function first validates the input file against a JSON schema (https://json-schema.org/)
   bool validateAndLoad(QString const & fileName, QTextStream & userMessage) {
      try {
         boost::json::value inputDocument = JsonUtils::loadJsonDocument(fileName);

         static JsonSchema schema{":/schemas/beerjson/1.0", "beer.json"};

         return schema.validate(inputDocument, userMessage);

      } catch (std::exception const & exception) {
         qWarning() << Q_FUNC_INFO << "Caught exception:" << exception.what();
         userMessage << exception.what();
         return false;
      }
   }
}

bool BeerJson::import(QString const & filename, QTextStream & userMessage) {
   // .:TODO:. This wrapper code is about the same as in BeerXML::importFromXML(), so let's try to pull out the common
   //          bits to one place.

   //
   // During importation we do not want automatic versioning turned on because, during the process of reading in a
   // Recipe we'll end up creating load of versions of it.  The magic of RAII means it's a one-liner to suspend
   // automatic versioning, in an exception-safe way, until the end of this function.
   //
   RecipeHelper::SuspendRecipeVersioning suspendRecipeVersioning;

   //
   // Slightly more manually, we also change the cursor to show "busy" while we're doing the import as, for large
   // imports, processing can take a few seconds or so.
   //
   QApplication::setOverrideCursor(Qt::WaitCursor);
   QApplication::processEvents();
   bool result = validateAndLoad(filename, userMessage);
   QApplication::restoreOverrideCursor();
   return result;
}
