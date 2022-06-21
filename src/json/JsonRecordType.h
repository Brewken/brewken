/*======================================================================================================================
 * json/JsonRecordType.h is part of Brewken, and is copyright the following authors 2020-2022:
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
#ifndef JSON_JSONRECORDTYPE_H
#define JSON_JSONRECORDTYPE_H
#pragma once

#include <QVector>

#include "utils/BtStringConst.h"
#include "utils/EnumStringMapping.h"

// .:TODO:. Change name of this to JsonRecordDefinition

///class JsonCoding;

/**
 * \brief This class and its derived classes represent a type of data record in a JSON document.  Each instance of this
 *        class (including of it sub-classes) is a constant entity that tells us how to map between a particular JSON
 *        record type and our internal data structures.
 *
 *        The related \c JsonRecord class holds data about a specific individual record that we are reading from or
 *        writing to a JSON document.
 *
 *        NB: In theory we should separate out BeerJSON specifics from more generic JSON capabilities, in case there is
 *        ever some other format of JSON that we want to use, or in case future versions of BeerJSON change radically.
 *        In practice, these things seem sufficiently unlikely that we can cross that bridge if and when we come to it.
 *
 *        NB: For XML processing, \c XmlRecord corresponds to a combination of \c JsonRecord and \c JsonRecordType
 */
class JsonRecordType {
public:
   /**
    * \brief The types of fields that we know how to process.  Used in \b FieldDefinition records
    *
    *        JSON
    *        ----
    *        Per https://www.json.org/json-en.html, in JSON, a value is one of the following:
    *           object
    *           array
    *           string
    *           number
    *           "true"
    *           "false"
    *           "null"
    *
    *        JSON also offers "integer" as a specialisation of number (integer being a JSON type used in the definition
    *        of number).
    *
    *        Correspondingly (more or less), if you have a boost::json::value (see comment in json/JsonSchema.cpp for
    *        why we are using Boost.JSON) then its kind() member function will return one of the following values:
    *           boost::json::kind::object
    *           boost::json::kind::array
    *           boost::json::kind::string
    *           boost::json::kind::uint64
    *           boost::json::kind::int64
    *           boost::json::kind::double_
    *           boost::json::kind::bool_
    *           boost::json::kind::null
    *
    *       JSON Schemas Generally
    *       ----------------------
    *       JSON itself doesn't have an enum type, but a JSON schema (see https://json-schema.org/) can achieve the same
    *       effect by restricting the values a string can take to those in a fixed list.
    *
    *       Similarly, a JSON schema can enforce restrictions on string values via regular expressions (see
    *       https://json-schema.org/understanding-json-schema/reference/regular_expressions.html).  This is used in
    *       BeerJSON for its DateType -- see below.
    *
    *       BeerJSON Specifically
    *       ---------------------
    *       In contrast with BeerXML and our database store, where we specify a canonical unit of measure for each field
    *       (eg temperatures are always stored as degrees celcius), BeerJSON allows lots of different units of measure.
    *       Thus a lot of the base types in BeerJSON consist of unit & value, where unit is an enum (ie string with
    *       restricted set of values) and value is a decimal or integer number.  This is a more universal approach in
    *       allowing multiple units to be used for temperature, time, color, etc, but it also means we have a lot more
    *       "base" types than for BeerXML or ObjectStore.  (It also means that it's harder for the schema to do bounds
    *       validation on such values.)
    *
    *       In some cases, BeerJSON only allows one unit of measurement, but the same structure of {unit, value} is
    *       maintained, presumably for extensibility.
    *
    *       The main BeerJSON base types are:
    *          AcidityType:        unit ∈ {"pH"} (NB: one-element set)
    *                              value : decimal
    *          BitternessType:     unit ∈ {"IBUs"} (NB: one-element set)
    *                              value : decimal
    *          CarbonationType:    unit ∈ {"vols", "g/l"}
    *                              value : decimal
    *          ColorType:          unit ∈ {"EBC", "Lovi", "SRM"}
    *                              value : decimal
    *          ConcentrationType:  unit ∈ {"ppm", "ppb", "mg/l"}
    *                              value : decimal
    *          DiastaticPowerType: unit ∈ {"Lintner", "WK"}
    *                              value : decimal
    *          GravityType:        unit ∈ {"sg", "plato", "brix" }
    *                              value : decimal
    *          MassType:           unit ∈ {"mg", "g", "kg", "lb", "oz"}
    *                              value : decimal
    *          PercentType:        unit ∈ {"%"} (NB: one-element set)
    *                              value : decimal
    *          PressureType:       unit ∈ {"kPa", "psi", "bar" }
    *                              value : decimal
    *          SpecificHeatType:   unit ∈ {"Cal/(g C)", "J/(kg K)", "BTU/(lb F)" }
    *                              value : decimal
    *          SpecificVolumeType: unit ∈ {"qt/lb", "gal/lb", "gal/oz", "l/g", "l/kg", "floz/oz", "m^3/kg", "ft^3/lb"}
    *                              value : decimal
    *          TemperatureType:    unit ∈ {"C", "F"}
    *                              value : decimal
    *          TimeType:           unit ∈ {"sec", "min", "hr", "day", "week"}
    *                              value : integer
    *          UnitType:           unit ∈ {"1", "unit", "each", "dimensionless", "pkg"}
    *                              value : decimal
    *          ViscosityType:      unit ∈ {"cP", "mPa-s"}
    *                              value : decimal
    *          VolumeType:         unit ∈ {"ml", "l", "tsp", "tbsp", "floz", "cup", "pt", "qt", "gal", "bbl", "ifloz", "ipt", "iqt", "igal", "ibbl"}
    *                              value : decimal
    *
    *       Furthermore, for many of these types, an additional "range" type is defined - eg GravityRangeType,
    *       BitternessRangeType, etc are used in beer styles.  The range type is just an object with two required elements,
    *       minimum and maximum, of the underlying type (eg GravityType for the members of GravityRangeType, BitternessType
    *       for the members of BitternessRangeType, etc).
    *
    *       BeerJSON also has DateType which is a regexp restriction on a string.  The regexp is a bit cumbersome, but
    *       it boils down to allowing either of the following formats where 'd' is a digit (ie 0-9):
    *          dddd-dd-dd
    *          dddd-dd-ddTdd:dd:dd
    *       We take this to mean ISO 8601 is used for date fields.  (Hurrah!)
    */
   enum class FieldType {
      //
      // These values correspond with base JSON types
      //
      Bool,             // boost::json::kind::bool_
      Int,              // boost::json::kind::int64
      UInt,             // boost::json::kind::uint64
      Double,           // boost::json::kind::double_
      String,           // boost::json::kind::string
      Enum,             // A string that we need to map to/from our own enum
      Array,            // Zero, one or more contained records
      //
      // These values correspond with BeerJSON types
      //
      Date,             // DateType
      Acidity,          // .:TODO.JSON:. Implement!
      Bitterness,       // .:TODO.JSON:. Implement!
      Carbonation,      // .:TODO.JSON:. Implement!
      Color,            // .:TODO.JSON:. Implement!
      Concentration,    // .:TODO.JSON:. Implement! Examples for concentration include ppm, ppb, and mg/l
      DiastaticPower,   // .:TODO.JSON:. Implement!
      Gravity,          // .:TODO.JSON:. Implement!
      Percent,          // .:TODO.JSON:. Implement!
      Temperature,      // .:TODO.JSON:. Implement!
      TimeElapsed,      // .:TODO.JSON:. Implement!  We use a slightly different name from BeerJSON to make clear this is not time of day
      Viscosity,        // .:TODO.JSON:. Implement!
      //
      // Other
      //
      MassOrVolume,     // This isn't an explicit BeerJSON type, but a lot of fields are allowed to be Mass or Volume,
                        // so it's a useful concept for us .:TODO.JSON:. Implement!
      RequiredConstant  // A fixed value we have to write out in the record (used for BeerJSON VERSION tag)
   };

   /**
    * \brief How to parse every field that we want to be able to read out of the JSON file.  See class description for
    *        more details.
    */
   struct FieldDefinition {
      FieldType            fieldType;
      char const *         xPath;
      BtStringConst const *     propertyName;  // If fieldType == RequiredConstant, then this is actually the constant value
      EnumStringMapping const * enumMapping;
   };

   typedef QVector<FieldDefinition> FieldDefinitions;

   /**
    * \brief Constructor
    * \param recordName The name of the JSON object for this type of record, eg "fermentables" for a list of
    *                   fermentables in BeerJSON.
    * \param namedEntityClassName The class name of the \c NamedEntity to which this record relates, eg "Fermentable",
    *                             or empty string if there is none
    * \param fieldDefinitions A list of fields we expect to find in this record (other fields will be ignored) and how
    *                         to parse them.
    * \param jsonCoding An \b JsonCoding object representing the JSON Coding we are using (eg BeerJSON 2.1).  This is what
    *                   we'll need to look up how to handle nested records inside this one.
    */
   JsonRecordType(char const * const recordName,
                  char const * const namedEntityClassName,
///                  JsonCoding const & jsonCoding,
                  FieldDefinitions const & fieldDefinitions);

   /**
    * \brief Get the record name (in this coding)
    */
   BtStringConst const & getRecordName() const;

protected:
public:
   BtStringConst const       recordName;

   // The name of the class of object contained in this type of record, eg "Hop", "Yeast", etc.
   // Blank for the root record (which is just a container and doesn't have a NamedEntity).
   BtStringConst const namedEntityClassName;

   FieldDefinitions const & fieldDefinitions;
};

#endif
