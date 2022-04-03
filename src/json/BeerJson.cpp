/*======================================================================================================================
 * json/BeerJson.cpp is part of Brewken, and is copyright the following authors 2021-2022:
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

#include "json/JsonRecord.h"
#include "json/JsonSchema.h"
#include "json/JsonUtils.h"
#include "model/Recipe.h"

namespace {

   template<class NE> QString BEER_JSON_RECORD_NAME;
   template<class NE> JsonRecord::FieldDefinitions const BEER_JSON_RECORD_FIELDS;

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for hop_varieties BeerJSON records
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> QString const BEER_JSON_RECORD_NAME<Hop>{"hop_varieties"};
/* This isn't used with BeerJSON
 * EnumStringMapping const BEER_JSON_HOP_USE_MAPPER {
      {"Boil",       Hop::Boil},
      {"Dry Hop",    Hop::Dry_Hop},
      {"Mash",       Hop::Mash},
      {"First Wort", Hop::First_Wort},
      {"Aroma",      Hop::UseAroma}
   };*/
   EnumStringMapping const BEER_JSON_HOP_TYPE_MAPPER {
      // .:TODO.JSON:.  Add missing values here to Hop::Type and/or combine with Hop::Use
      {"aroma",                  Hop::Type::Aroma},
      {"bittering",              Hop::Type::Bittering},
//      {"flavor",                 Hop::Type::},
      {"aroma/bittering",        Hop::Type::Both},
//      {"bittering/flavor",       Hop::Type::},
//      {"aroma/flavor",           Hop::Type::},
//      {"aroma/bittering/flavor", Hop::Type::},


      {"Bittering", Hop::Bittering},
      {"Aroma",     Hop::Aroma},
      {"Both",      Hop::Both}
   };
   EnumStringMapping const BEER_JSON_HOP_FORM_MAPPER {
      // .:TODO.JSON:.  Add missing values here to Hop::Form
//      {"extract",    Hop::Form::},
      {"leaf",       Hop::Form::Leaf},
//      {"leaf (wet)", Hop::Form::},
      {"pellet",     Hop::Form::Pellet},
//      {"powder",     Hop::Form::},
      {"plug",       Hop::Form::Plug}
   };
   // .:TODO:. Need JsonRecord::Percent.  BeerJSON defines PercentType as an object with unit = "%" and value = number
   template<> JsonRecord::FieldDefinitions const BEER_JSON_RECORD_FIELDS<Hop> {
      // Type                    XPath                                Q_PROPERTY                             Enum Mapper
      {JsonRecord::String,       "name",                              PropertyNames::NamedEntity::name,      nullptr},
      {JsonRecord::String,       "producer",                          BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::String,       "product_id",                        BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::String,       "origin",                            PropertyNames::Hop::origin,            nullptr},
      {JsonRecord::String,       "year",                              BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::Enum,         "form",                              PropertyNames::Hop::form,              &BEER_JSON_HOP_FORM_MAPPER},
      {JsonRecord::Percent,      "alpha_acid",                        PropertyNames::Hop::alpha_pct,         nullptr},
      {JsonRecord::Percent,      "beta_acid",                         PropertyNames::Hop::beta_pct,          nullptr},
      {JsonRecord::Enum,         "type",                              PropertyNames::Hop::type,              &BEER_JSON_HOP_TYPE_MAPPER},
      {JsonRecord::String,       "notes",                             PropertyNames::Hop::notes,             nullptr},
      {JsonRecord::Percent,      "percent_lost",                      PropertyNames::Hop::hsi_pct,           nullptr},
      {JsonRecord::String,       "substitutes",                       PropertyNames::Hop::substitutes,       nullptr},
      {JsonRecord::Double,       "oil_content/total_oil_ml_per_100g", BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::Percent,      "oil_content/humulene",              PropertyNames::Hop::humulene_pct,      nullptr},
      {JsonRecord::Percent,      "oil_content/caryophyllene",         PropertyNames::Hop::caryophyllene_pct, nullptr},
      {JsonRecord::Percent,      "oil_content/cohumulone",            PropertyNames::Hop::cohumulone_pct,    nullptr},
      {JsonRecord::Percent,      "oil_content/myrcene",               PropertyNames::Hop::myrcene_pct,       nullptr},
      {JsonRecord::Percent,      "oil_content/farnesene",             BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::Percent,      "oil_content/geraniol",              BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::Percent,      "oil_content/b_pinene",              BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::Percent,      "oil_content/linalool",              BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::Percent,      "oil_content/limonene",              BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::Percent,      "oil_content/nerol",                 BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::Percent,      "oil_content/pinene",                BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::Percent,      "oil_content/polyphenols",           BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::Percent,      "oil_content/xanthohumol",           BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecord::MassOrVolume, "inventory/amount",                  BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Extend Hop::amount_kg so we can cope with volumes for extract etc

      // .:TODO.JSON:. Note that we'll need to look at HopAdditionType, IBUEstimateType, IBUMethodType when we use Hops in Recipes
   };

   //=-=-=-=-=-=-=-=-

   // This function first validates the input file against a JSON schema (https://json-schema.org/)
   bool validateAndLoad(QString const & fileName, QTextStream & userMessage) {
      boost::json::value inputDocument;
      try {
         inputDocument = JsonUtils::loadJsonDocument(fileName);

         static JsonSchema schema{":/schemas/beerjson/1.0", "beer.json"};

         if (!schema.validate(inputDocument, userMessage)) {
            qWarning() << Q_FUNC_INFO << "Schema validation failed";
            return false;
         }

      } catch (std::exception const & exception) {
         qWarning() <<
            Q_FUNC_INFO << "Caught exception while reading and validating " << fileName << ":" << exception.what();
         userMessage << exception.what();
         return false;
      }

      // Now we've loaded the JSON document into memory and determined that it's valid BeerJSON, we need to extract the
      // data from it
      // .:TODO:. IMPLEMENT THIS!

      // Per https://www.json.org/json-en.html, a JSON object is an unordered set of name/value pairs, so there's no
      // constraint about what order we parse things

      // We're expecting the root of the JSON document to be an object named "beerjson".  This should have been
      // established by the validation above.
      Q_ASSERT(inputDocument.is_object());
      boost::json::object documentRoot = inputDocument.as_object();
      Q_ASSERT(documentRoot.contains("beerjson"));
      Q_ASSERT(documentRoot["beerjson"].is_object());
      boost::json::object beerJson = documentRoot["beerjson"].as_object();

      //
      // Per https://www.json.org/json-en.html, in JSON, a value is one of the following:
      //    object
      //    array
      //    string
      //    number
      //    "true"
      //    "false"
      //    "null"
      // A JSON Schema also offers "integer" as a specialisation of number (integer being a JSON type used in the
      // definition of number).
      // Correspondingly (more or less), if you have a boost::json::value then its kind() member function will return
      // one of the following values:
      //    json::kind::object
      //    json::kind::array
      //    json::kind::string
      //    json::kind::uint64
      //    json::kind::int64
      //    json::kind::double_
      //    json::kind::bool_
      //    json::kind::null
      //
      // For each type of object T that we want to read from a JSON file (eg T is Recipe, Hop, etc) we need to provide
      // an implementation of the following function:
      //    T tag_invoke(value_to_tag<T>, value const & jv);
      // Note:
      //    (1) The value_to_tag<T> type is empty and has no members.  It is just a trick used by the library to ensure
      //        the right overload of tag_invoke is called.
      //    (2) We do not call tag_invoke() ourselves.  Instead, we call
      //        template<class T> T boost::json::value_to(value const & jv).
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
      //
      // At top level, a BeerJSON document consists of the following objects (where "[]" means "array of"):
      //   version:                   VersionType                  required
      //   fermentables:              FermentableType[]            optional
      //   miscellaneous_ingredients: MiscellaneousType[]          optional
      //   hop_varieties:             VarietyInformation[]         optional
      //   cultures:                  CultureInformation[]         optional
      //   profiles:                  WaterBase[]                  optional
      //   styles:                    StyleType[]                  optional
      //   mashes:                    MashProcedureType[]          optional
      //   fermentations:             FermentationProcedureType[]  optional
      //   recipes:                   RecipeType[]                 optional
      //   equipments:                EquipmentType[]              optional
      //   boil:                      BoilProcedureType[]          optional
      //   packaging:                 PackagingProcedureType[]     optional
      //
      // In contrast with BeerXML and our database store, where we specify a canonical unit of measure for each field
      // (eg temperatures are always stored as degrees celcius), BeerJSON allows lots of different units of measure.
      // Thus a lot of the base types in BeerJSON consist of unit & value, where unit is an enum (ie string with
      // restricted set of values) and value is a decimal or integer number.  This is a more universal approach in
      // allowing multiple units to be used for temperature, time, color, etc, but it also means we have a lot more
      // "base" types than for BeerXML or ObjectStore.  (It also means that it's harder for the schema to do bounds
      // validation on such values.)
      //
      // In some cases, BeerJSON only allows one unit of measurement, but the same structure of {unit, value} is
      // maintained, presumably for consistency and extensibility.
      //
      // The main BeerJSON base types are:
      //
      //    VolumeType:         unit ∈ {"ml", "l", "tsp", "tbsp", "floz", "cup", "pt", "qt", "gal", "bbl", "ifloz", "ipt", "iqt", "igal", "ibbl"}
      //                        value : decimal
      //    MassType:           unit ∈ {"mg", "g", "kg", "lb", "oz"}
      //                        value : decimal
      //    DiastaticPowerType: unit ∈ {"Lintner", "WK"}
      //                        value : decimal
      //    TemperatureType:    unit ∈ {"C", "F"}
      //                        value : decimal
      //    PressureType:       unit ∈ {"kPa", "psi", "bar" }
      //                        value : decimal
      //    AcidityType:        unit ∈ {"pH"} (NB: one-element set)
      //                        value : decimal
      //    TimeType:           unit ∈ {"sec", "min", "hr", "day", "week"}
      //                        value : integer
      //    ColorType:          unit ∈ {"EBC", "Lovi", "SRM"}
      //                        value : decimal
      //    CarbonationType:    unit ∈ {"vols", "g/l"}
      //                        value : decimal
      //    BitternessType:     unit ∈ {"IBUs"} (NB: one-element set)
      //                        value : decimal
      //    GravityType:        unit ∈ {"sg", "plato", "brix" }
      //                        value : decimal
      //    SpecificHeatType:   unit ∈ {"Cal/(g C)", "J/(kg K)", "BTU/(lb F)" }
      //                        value : decimal
      //    ConcentrationType:  unit ∈ {"ppm", "ppb", "mg/l"}
      //                        value : decimal
      //    SpecificVolumeType: unit ∈ {"qt/lb", "gal/lb", "gal/oz", "l/g", "l/kg", "floz/oz", "m^3/kg", "ft^3/lb"}
      //                        value : decimal
      //    UnitType:           unit ∈ {"1", "unit", "each", "dimensionless", "pkg"}
      //                        value : decimal
      //    ViscosityType:      unit ∈ {"cP", "mPa-s"}
      //                        value : decimal
      //
      // Furthermore, for many of these types, an additional "range" type is defined - eg GravityRangeType,
      // BitternessRangeType, etc are used in beer styles.  The range type is just an object with two required elements,
      // minimum and maximum, of the underlying type (eg GravityType for the members of GravityRangeType, BitternessType
      // for the members of BitternessRangeType, etc).
      //

      boost::json::value & bjv = beerJson["version"];
      qDebug() << Q_FUNC_INFO << "Version" << bjv;

      boost::json::value const * bjVer = beerJson.if_contains("version");
      if (bjVer) {
         qDebug() << Q_FUNC_INFO << "Version" << bjVer;
      }

      boost::json::value const * recs = beerJson.if_contains("recipes");
      if (recs) {
         qDebug() << Q_FUNC_INFO << "Recipes" << recs;
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
