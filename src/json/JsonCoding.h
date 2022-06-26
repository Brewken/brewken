/*======================================================================================================================
 * json/JsonCoding.h is part of Brewken, and is copyright the following authors 2020-2022:
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
#ifndef JSON_JSONCODING_H
#define JSON_JSONCODING_H
#pragma once

#include <memory> // For smart pointers
#include <QHash>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QVariant>

#include "json/JsonRecord.h"
#include "json/JsonSchema.h"

/**
 * \brief An instance of this class holds information about a particular JSON encoding (eg BeerJSON 2.1).  Specifically,
 *        that information includes:
 *          • the corresponding \c JsonSchema that we use to validate a JSON document
 *          • the \b JsonRecordDefinition objects that define how we map BeerJSON objects to our own data structures.
 *
 *        As we are parsing or creating a JSON document, we'll create a \b JsonRecord for each record we are reading /
 *        writing, using the relevant \b JsonRecordDefinition as a template.
 *
 *        Similar to xml/XmlCoding.h
 */
class JsonCoding {

public:
   /**
    * \brief C++ does not permit you to have a function pointer to a class constructor, so this templated wrapper
    *        function is a "trick" that allows us to come close enough for our purposes.  Using a pointer to a
    *        template specialisation of this function for some subclass of JsonRecord effectively gives us a pointer to
    *        a create-on-the-heap constructor for that subclass, (provided it takes the same parameters as this
    *        function).
    *
    *        To make it easier for callers, we also typedef \b JsonCoding::JsonRecordConstructorWrapperto be a pointer to
    *        a function of this type.
    *
    * \param recordName passed into the constructor of T (which should be \b JsonRecord or a subclass thereof)
    * \param xmlCoding passed into the constructor of T (which should be \b JsonRecord or a subclass thereof)
    * \param fieldDefinitions passed into the constructor of T (which should be \b JsonRecord or a subclass thereof)
    * \return Pointer to a new instance, constructed on the heap, of an JsonRecord (or subclass thereof) suitable for
    *         reading in objects of type T (where T ie expected either to be some subclass of NamedEntity or void to
    *         signify the root element). Eg:
    *           JsonCoding::construct<Hop>() will construct an JsonNamedEntityRecord<Hop> object
    *           JsonCoding::construct<Yeast>() will construct an JsonNamedEntityRecord<Yeast> object
    *           JsonCoding::construct<Recipe>() will construct an JsonRecipeRecord object ‡
    *           JsonCoding::construct<void>() will construct an JsonRecipe object ‡
    *         ‡ courtesy of template specialisation below
    *
    *         NB: The caller owns this object and is responsible for its deletion.
    */
/*   template<typename T>
   static JsonRecord * construct(QString const & recordName,
                                JsonCoding const & xmlCoding,
                                JsonRecordDefinition::FieldDefinitions const & fieldDefinitions) {
      return new JsonNamedEntityRecord<T>{recordName, xmlCoding, fieldDefinitions};
   }*/

   /**
    * \brief This is just a convenience typedef representing a pointer to a template specialisation of
    *        \b JsonCoding::construct().
    */
/*   typedef JsonRecord * (*JsonRecordConstructorWrapper)(QString const & recordName,
                                                      JsonCoding const &,
                                                      JsonRecordDefinition::FieldDefinitions const &);
*/
   /**
    * Given an JSON element that corresponds to a record, this is the info we need to construct a \b JsonRecord object
    * for this encoding.
    */
/*   struct JsonRecordDefinition {
      JsonRecordConstructorWrapper constructorWrapper;
      JsonRecordDefinition::FieldDefinitions const * fieldDefinitions;
   };*/

   /**
    * \brief Constructor
    * \param name The name of this encoding (eg "BeerJSON 1.0").  Used primarily for logging.
    * \param version The version to write out to BeerJSON records
    * \param schema The wrapper around the JSON schema that we'll use to validate the input (if we are reading from,
    *               rather than writing to, JSON).
    * \param jsonRecordDefinitions
    *
    *
    *
///    * \param entityNameToJsonRecordDefinition Mapping from JSON object name to the information we need to construct a
///    *                                         suitable \b JsonRecord object.
    */
   JsonCoding(char const * const name,
              char const * const version,
              JsonSchema::Id const schemaId,
              std::initializer_list<JsonRecordDefinition> jsonRecordDefinitions);
///              QHash<QString, JsonRecordDefinition> const & entityNameToJsonRecordDefinition);

   /**
    * \brief Destructor
    */
   ~JsonCoding();

   /**
    * \brief Check whether we know how to process a record of a given (JSON tag) name
    * \param recordName
    * \return \b true if we know how to process (ie we have the address of a function that can create a suitable
    *         \b JsonRecord object), \b false if not
    */
   bool isKnownJsonRecordDefinition(QString recordName) const;

   /**
    * \brief Get the root definition element, ie what we use to start processing a document
    */
   JsonRecordDefinition const & getRoot() const;

   /**
    * \brief For a given record name (eg "HOPS", "HOP", "YEASTS", etc) retrieve a new instance of the corresponding
    *        subclass of \b JsonRecord.  Caller is responsible for ensuring that such a subclass exists, either by
    *        having supplied the \b nameToJsonRecordLookup to our constructor or by calling \b isKnownJsonRecordDefinition().
    * \param recordName
    * \return A shared pointer to a new \b JsonRecord constructed on the heap.  (The caller will be the sole owner of
    *         this pointer.)
    */
///   std::shared_ptr<JsonRecord> getNewJsonRecord(QString recordName) const;

   /**
    * \brief Validate JSON file against schema, load its contents into objects, and store then in the DB
    *
    * \param inputDocument The JSON file to validate and read
    * \param userMessage Any message that we want the top-level caller to display to the user (either about an error
    *                    or, in the event of success, summarising what was read in) should be appended to this string.
    *
    * \return true if file validated OK (including if there were "errors" that we can safely ignore)
    *         false if there was a problem that means it's not worth trying to read in the data from the file
    */
   bool validateLoadAndStoreInDb(boost::json::value & inputDocument,
                                 QTextStream & userMessage) const;

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;
};

#endif