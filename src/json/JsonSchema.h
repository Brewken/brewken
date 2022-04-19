/*======================================================================================================================
 * json/JsonSchema.h is part of Brewken, and is copyright the following authors 2021-2022:
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
 * \class JsonSchema holds all the files for a single JSON schema (which we give to Valijson for it to validate a JSON
 *        document)
 *
 *        Note that this class ONLY wraps the JSON schema (see https://json-schema.org/).  It does not hold any of the
 *        info needed for us to process the file.  For that, see \c JsonCoding.  (Each \c JsonCoding has a corresponding
 *        \c JsonSchema.)
 */
class JsonSchema {
public:

   // .:TODO:. Each JsonSchema is a const (after construction) singleton for the schema it represents (eg BeerJSON 2.1),
   //          so we should have a registry of them.

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
   bool validate(boost::json::value const & document, QTextStream & userMessage) const;

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;

   /**
    * \brief This is the callback we give to Valijson, which then forwards it on to whatever the last JsonSchema object
    *        we were dealing with on this thread was (which should be the one that gave the callback to Valijson).
    */
   static boost::json::value const * fetchReferencedDocument(std::string const & uri);
};

#endif
