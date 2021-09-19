/*======================================================================================================================
 * json/JsonSchema.h is part of Brewken, and is copyright the following authors 2021:
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
#ifndef JSON_JSONSCHEMA_H
#define JSON_JSONSCHEMA_H
#pragma once

#include <memory> // For PImpl

#include <boost/json/value.hpp>

class QTextStream;

/**
 * \class JsonSchema holds all the files for a single schema (which we give to Valijson for it to validate a JSON
 *        document)
 */
class JsonSchema {

public:
   /**
    * \brief Constructor
    *
    *        The only reason there are two parameters (directory and file name) rather than one (fully qualified file
    *        name) is because it makes reusing some code inside the class a little easier.
    *
    * \param baseDir The directory path in which these schema files live.  Usually a resource path, eg
    *                ":/schemas/beerjson/1.0"
    * \param fileName  The file name, inside \c baseDir, of the initial file of the schema, eg "beer.json".  (This may
    *                  reference other files via $ref tags in the schema JSON, these will be loaded automatically from
    *                  \c baseDir.)
    */
   JsonSchema(char const * const baseDir,
              char const * const fileName);

   ~JsonSchema();

   /**
    * \brief Validate a JSON document
    *
    * \param document JSON document loaded with \c JsonUtils::loadJsonDocument()
    * \param userMessage Where to write any (brief!) message we want to be shown to the user after the import.
    *                    Typically this is either the reason the import failed or a summary of what was imported.
    *
    * \return \c true if parsing suceeded, \c false otherwise
    */
   bool validate(boost::json::value const & document, QTextStream & userMessage);

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;

   static boost::json::value const * fetchReferencedDocument(std::string const & uri);

};

#endif
