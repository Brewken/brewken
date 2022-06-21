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

#include "json/JsonCoding.h"
#include "json/JsonRecord.h"
#include "json/JsonSchema.h"
#include "json/JsonUtils.h"
#include "model/Fermentable.h"
#include "model/Hop.h"
#include "model/Misc.h"
#include "model/Recipe.h"
#include "model/Style.h"
#include "model/Water.h"
#include "model/Yeast.h"

namespace {

   template<class NE> JsonRecordType const BEER_JSON_RECORD_DEFINITION;

   // Field mappings below are in the same order as in schemas/beerjson/1.0/beer.json
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Top-level field mappings for BeerJSON files
   //
   // The root of a BeerJSON document is an object named "beerjson".  Inside this are some or all of the following
   // objects (where "[]" means "array of"):
   //
   //   Object Name                BeerJSON Type                Required or Optional
   //   -----------                -------------                --------------------
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
   // Note that the way ingredients are included inside recipes is more nuanced than in BeerXML.  In BeerXML, you can
   // have eg a Hop record both as an element inside a top-level list of Hops (ie hop varieties) and as an ingredient
   // inside a Recipe.  In BeerJSON, the distinction is made between records in a top-level list of hops, which are
   // stored in VarietyInformation objects, and hop additions in a recipe, which are stored in HopAdditionType records.
   // (We might wish that VarietyInformation were named HopVariety, and HopAdditionType simply HopAddition, but such
   // naming oddities are considerably less egregious than a number of the problems with BeerXML, so it's still
   // progress.)
   //
   // VarietyInformation and HopAdditionType share some common fields, specifically those in HopVarietyBase.  (Although
   // it's NOT actually what's going on, it's helpful in some respects to think of HopVarietyBase as an abstract base
   // class from which VarietyInformation and HopAdditionType both inherit, because that's APPROXIMATELY the effect we
   // get.  Actually, JSON schemas do not actually support inheritance and, strictly speaking, what's happening is
   // schema combination which is not quite the same thing, but this is more something we would need to worry about if
   // we were designing our own schema.)  Anyway, we try to avoid duplicating definitions by having a similar structure.
   //
   // There are similar distinctions for fermentables, miscellaneous ingredients and so on.
   //
   // For historical reasons, we use Hop objects both for "hop variety" (when the object has no parent) and "use of a
   // hop in a recipe" (when the object has a parent Hop object, which should be its hop variety).  (And, again, the
   // same applies to Fermentable, Misc, Yeast and so on.)
   //
   // One day perhaps we should split Hop up into HopBase, HopVariety and HopAddition, and do likewise for Fermentable,
   // Misc, Yeast, etc.  But that's quite a big change so, for now, we'll stick with our existing object structure.
   //
   // It would be nice to be able to make the JsonRecordType::FieldDefinitions constants constexpr rather than just
   // const, but this is not yet easy because QVector cannot be constexpr, std::vector cannot yet be constexpr,
   // std::array (which can be constexpr) cannot deduce its own length when used with non-trivial types, and the
   // proposed std::make_array is still experimental and not yet actually part of std.  (There are various workarounds
   // with template metaprogramming but it's all a bit painful compared with the marginal benefit we would get.)
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> JsonRecordType const BEER_JSON_RECORD_DEFINITION<void> {
      "beerjson",
      "",
      {
         // Type                                   XPath                        Q_PROPERTY          Enum Mapper
         {JsonRecordType::FieldType::RequiredConstant, "version",                   &BtString::NULL_STR, nullptr},
         {JsonRecordType::FieldType::Array,            "fermentables",              &BtString::NULL_STR, nullptr},
         {JsonRecordType::FieldType::Array,            "miscellaneous_ingredients", &BtString::NULL_STR, nullptr},
         {JsonRecordType::FieldType::Array,            "hop_varieties",             &BtString::NULL_STR, nullptr},
         {JsonRecordType::FieldType::Array,            "cultures",                  &BtString::NULL_STR, nullptr},
         {JsonRecordType::FieldType::Array,            "profiles",                  &BtString::NULL_STR, nullptr},
         {JsonRecordType::FieldType::Array,            "styles",                    &BtString::NULL_STR, nullptr},
         {JsonRecordType::FieldType::Array,            "mashes",                    &BtString::NULL_STR, nullptr},
         {JsonRecordType::FieldType::Array,            "fermentations",             &BtString::NULL_STR, nullptr},
         {JsonRecordType::FieldType::Array,            "recipes",                   &BtString::NULL_STR, nullptr},
         {JsonRecordType::FieldType::Array,            "equipments",                &BtString::NULL_STR, nullptr},
         {JsonRecordType::FieldType::Array,            "boil",                      &BtString::NULL_STR, nullptr},
         {JsonRecordType::FieldType::Array,            "packaging",                 &BtString::NULL_STR, nullptr}
      }
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for fermentables BeerJSON records - see schemas/beerjson/1.0/fermentable.json
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   EnumStringMapping const BEER_JSON_FERMENTABLE_TYPE_MAPPER {
      // .:TODO.JSON:.  Add missing values here to Fermentable::Type
      {"dry extract", Fermentable::Type::Dry_Extract},
      {"extract",     Fermentable::Type::Extract},
      {"grain",       Fermentable::Type::Grain},
      {"sugar",       Fermentable::Type::Sugar},
//      {"fruit",       Fermentable::Type::},
//      {"juice",       Fermentable::Type::},
//      {"honey",       Fermentable::Type::},
      {"other",       Fermentable::Type::Adjunct}
   };
   // .:TODO.JSON:.  Create Fermentable::GrainGroup enum class
   EnumStringMapping const BEER_JSON_FERMENTABLE_GRAIN_GROUP_MAPPER {
//      {"base",       Fermentable::GrainGroup::},
//      {"caramel",    Fermentable::GrainGroup::},
//      {"flaked",     Fermentable::GrainGroup::},
//      {"roasted",    Fermentable::GrainGroup::},
//      {"specialty",  Fermentable::GrainGroup::},
//      {"smoked",     Fermentable::GrainGroup::},
//      {"adjunct",    Fermentable::GrainGroup::}
   };
   JsonRecordType::FieldDefinitions const BeerJson_FermentableBase {
      // Type                                 XPath                           Q_PROPERTY                                          Enum Mapper
      {JsonRecordType::FieldType::String,         "name",                         &PropertyNames::NamedEntity::name,                   nullptr},
      {JsonRecordType::FieldType::Enum,           "type",                         &PropertyNames::Fermentable::type,                   &BEER_JSON_FERMENTABLE_TYPE_MAPPER},
      {JsonRecordType::FieldType::String,         "origin",                       &PropertyNames::Fermentable::origin,                 nullptr},
      {JsonRecordType::FieldType::String,         "producer",                     &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable or look at PropertyNames::Fermentable::supplier
      {JsonRecordType::FieldType::String,         "product_id",                   &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Enum,           "grain_group",                  &PropertyNames::Fermentable::type,                   &BEER_JSON_FERMENTABLE_GRAIN_GROUP_MAPPER},
      {JsonRecordType::FieldType::Percent,        "yield/fine_grind",             &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Percent,        "yield/coarse_grind",           &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Percent,        "yield/fine_coarse_difference", &PropertyNames::Fermentable::coarseFineDiff_pct,     nullptr},
      {JsonRecordType::FieldType::Gravity,        "yield/potential",              &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Color,          "color",                        &PropertyNames::Fermentable::color_srm,              nullptr},
   };
   JsonRecordType::FieldDefinitions const BeerJson_FermentableType_ExclBase {
      // Type                                 XPath                           Q_PROPERTY                                          Enum Mapper
      {JsonRecordType::FieldType::String,         "notes",                        &PropertyNames::Fermentable::notes,                  nullptr},
      {JsonRecordType::FieldType::Percent,        "moisture",                     &PropertyNames::Fermentable::moisture_pct,           nullptr},
      {JsonRecordType::FieldType::Double,         "alpha_amylase",                &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::DiastaticPower, "diastatic_power",              &PropertyNames::Fermentable::diastaticPower_lintner, nullptr},
      {JsonRecordType::FieldType::Percent,        "protein",                      &PropertyNames::Fermentable::protein_pct,            nullptr},
      {JsonRecordType::FieldType::Double,         "kolbach_index",                &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Percent,        "max_in_batch",                 &PropertyNames::Fermentable::maxInBatch_pct,         nullptr},
      {JsonRecordType::FieldType::Bool,           "recommend_mash",               &PropertyNames::Fermentable::recommendMash,          nullptr}, // .:TODO.JSON:. What is the difference between PropertyNames::Fermentable::recommendMash and PropertyNames::Fermentable::isMashed
      {JsonRecordType::FieldType::MassOrVolume,   "inventory/amount",             &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Extend Fermentable::amount_kg so we can cope with volumes
      {JsonRecordType::FieldType::Percent,        "glassy",                       &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Percent,        "plump",                        &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Percent,        "half",                         &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Percent,        "mealy",                        &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Percent,        "thru",                         &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Percent,        "friability",                   &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Acidity,        "di_ph",                        &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Viscosity,      "viscosity",                    &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Concentration,  "dms_p",                        &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Concentration,  "fan",                          &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Percent,        "fermentability",               &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordType::FieldType::Concentration,  "beta_glucan",                  &BtString::NULL_STR,                                 nullptr}, // .:TODO.JSON:. Add this to Fermentable
   };
   // .:TODO.JSON:.  Extend Recipe to have an enum for this
   EnumStringMapping const BEER_JSON_RECIPE_ADDITION_POINT_MAPPER {
///      {"add_to_mash",         Recipe::},
///      {"add_to_boil",         Recipe::},
///      {"add_to_fermentation", Recipe::},
///      {"add_to_package",      Recipe::},
    };
   // This is the same across Fermentable, Hop, Misc
   JsonRecordType::FieldDefinitions const BeerJson_IngredientAdditionType_ExclBase {
      // Type                                 XPath                           Q_PROPERTY                            Enum Mapper
      {JsonRecordType::FieldType::TimeElapsed,    "timing/time",                  &BtString::NULL_STR,                  nullptr}, // .:TODO.JSON:.
      {JsonRecordType::FieldType::TimeElapsed,    "timing/duration",              &BtString::NULL_STR,                  nullptr}, // .:TODO.JSON:.
      {JsonRecordType::FieldType::Bool,           "timing/continuous",            &BtString::NULL_STR,                  nullptr}, // .:TODO.JSON:.
      {JsonRecordType::FieldType::Gravity,        "timing/specific_gravity",      &BtString::NULL_STR,                  nullptr}, // .:TODO.JSON:.
      {JsonRecordType::FieldType::Acidity,        "timing/pH",                    &BtString::NULL_STR,                  nullptr}, // .:TODO.JSON:.
      {JsonRecordType::FieldType::Int,            "timing/step",                  &BtString::NULL_STR,                  nullptr}, // .:TODO.JSON:.
      {JsonRecordType::FieldType::Enum,           "timing/use",                   &BtString::NULL_STR,                  &BEER_JSON_RECIPE_ADDITION_POINT_MAPPER}, // .:TODO.JSON:.
      {JsonRecordType::FieldType::MassOrVolume,   "amount",                       &BtString::NULL_STR,                  nullptr}, // .:TODO.JSON:.
   };
   // As mentioned above, it would be really nice to do this at compile time, but haven't yet found a nice way to do so
///   template<> JsonRecordType::FieldDefinitions const BEER_JSON_RECORD_FIELDS<Fermentable> = BeerJson_FermentableBase + BeerJson_FermentableType_ExclBase;
   template<> JsonRecordType const BEER_JSON_RECORD_DEFINITION<Fermentable> {
      "fermentables",
      "Fermentable",
      BeerJson_FermentableBase + BeerJson_FermentableType_ExclBase
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for miscellaneous_ingredients BeerJSON records - see schemas/beerjson/1.0/misc.json TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   EnumStringMapping const BEER_JSON_MISC_TYPE_MAPPER {
      // .:TODO.JSON:.  Add missing values here to Misc::Type
      {"spice",       Misc::Type::Spice},
      {"fining",      Misc::Type::Fining},
      {"water agent", Misc::Type::Water_Agent},
      {"herb",        Misc::Type::Herb},
      {"flavor",      Misc::Type::Flavor},
///      {"wood",        Misc::Type::Wood},
      {"other",       Misc::Type::Other}
   };
   // .:TBD.JSON:. There is no equivalent of the Misc::Use enum in BeerJSON, just the use_for string
//   EnumStringMapping const BEER_JSON_MISC_USE_MAPPER {
//      {"Boil",      Misc::Use::Boil},
//      {"Mash",      Misc::Use::Mash},
//      {"Primary",   Misc::Use::Primary},
//      {"Secondary", Misc::Use::Secondary},
//      {"Bottling",  Misc::Use::Bottling}
//   };
   JsonRecordType::FieldDefinitions const BeerJson_MiscellaneousBase {
      // Type                               XPath               Q_PROPERTY                        Enum Mapper
      {JsonRecordType::FieldType::String,       "name",             &PropertyNames::NamedEntity::name, nullptr},
      {JsonRecordType::FieldType::String,       "producer",         &BtString::NULL_STR,               nullptr}, // .:TODO.JSON:. Add this to Misc
      {JsonRecordType::FieldType::String,       "product_id",       &BtString::NULL_STR,               nullptr}, // .:TODO.JSON:. Add this to Misc
      {JsonRecordType::FieldType::Enum,         "type",             &PropertyNames::Fermentable::type, &BEER_JSON_MISC_TYPE_MAPPER},
   };
   JsonRecordType::FieldDefinitions const BeerJson_MiscellaneousType_ExclBase {
      // Type                               XPath               Q_PROPERTY                        Enum Mapper
      {JsonRecordType::FieldType::String,       "use_for",          &PropertyNames::Misc::useFor,      nullptr},
      {JsonRecordType::FieldType::String,       "notes",            &PropertyNames::Misc::notes,       nullptr},
      {JsonRecordType::FieldType::MassOrVolume, "inventory/amount", &PropertyNames::Misc::amount,      nullptr}, // .:TODO.JSON:. Also need to reference Misc::amountIsWeight PLUS we need to cope with UnitType
   };
///   template<> JsonRecordType::FieldDefinitions const BEER_JSON_RECORD_FIELDS<Misc> = BeerJson_MiscellaneousBase + BeerJson_MiscellaneousType_ExclBase;
   template<> JsonRecordType const BEER_JSON_RECORD_DEFINITION<Misc> {
      "miscellaneous_ingredients",
      "Misc",
      BeerJson_MiscellaneousBase + BeerJson_MiscellaneousType_ExclBase
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for hop_varieties BeerJSON records - see schemas/beerjson/1.0/hop.json
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* This isn't used with BeerJSON
 * EnumStringMapping const BEER_JSON_HOP_USE_MAPPER {
      {"Boil",       Hop::Use::Boil},
      {"Dry Hop",    Hop::Use::Dry_Hop},
      {"Mash",       Hop::Use::Mash},
      {"First Wort", Hop::Use::First_Wort},
      {"Aroma",      Hop::Use::UseAroma}
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


      {"Bittering", Hop::Type::Bittering},
      {"Aroma",     Hop::Type::Aroma},
      {"Both",      Hop::Type::Both}
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
   JsonRecordType::FieldDefinitions const BeerJson_HopBase {
      // Type                    XPath                                Q_PROPERTY                             Enum Mapper
      {JsonRecordType::FieldType::String,       "name",                              &PropertyNames::NamedEntity::name,      nullptr},
      {JsonRecordType::FieldType::String,       "producer",                          &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::String,       "product_id",                        &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::String,       "origin",                            &PropertyNames::Hop::origin,            nullptr},
      {JsonRecordType::FieldType::String,       "year",                              &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::Enum,         "form",                              &PropertyNames::Hop::form,              &BEER_JSON_HOP_FORM_MAPPER},
   };
   JsonRecordType::FieldDefinitions const BeerJson_HopType_ExclBase {
      // Type                    XPath                                Q_PROPERTY                             Enum Mapper
      {JsonRecordType::FieldType::Percent,      "alpha_acid",                        &PropertyNames::Hop::alpha_pct,         nullptr},
      {JsonRecordType::FieldType::Percent,      "beta_acid",                         &PropertyNames::Hop::beta_pct,          nullptr},
      {JsonRecordType::FieldType::Enum,         "type",                              &PropertyNames::Hop::type,              &BEER_JSON_HOP_TYPE_MAPPER},
      {JsonRecordType::FieldType::String,       "notes",                             &PropertyNames::Hop::notes,             nullptr},
      {JsonRecordType::FieldType::Percent,      "percent_lost",                      &PropertyNames::Hop::hsi_pct,           nullptr},
      {JsonRecordType::FieldType::String,       "substitutes",                       &PropertyNames::Hop::substitutes,       nullptr},
      {JsonRecordType::FieldType::Double,       "oil_content/total_oil_ml_per_100g", &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::Percent,      "oil_content/humulene",              &PropertyNames::Hop::humulene_pct,      nullptr},
      {JsonRecordType::FieldType::Percent,      "oil_content/caryophyllene",         &PropertyNames::Hop::caryophyllene_pct, nullptr},
      {JsonRecordType::FieldType::Percent,      "oil_content/cohumulone",            &PropertyNames::Hop::cohumulone_pct,    nullptr},
      {JsonRecordType::FieldType::Percent,      "oil_content/myrcene",               &PropertyNames::Hop::myrcene_pct,       nullptr},
      {JsonRecordType::FieldType::Percent,      "oil_content/farnesene",             &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::Percent,      "oil_content/geraniol",              &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::Percent,      "oil_content/b_pinene",              &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::Percent,      "oil_content/linalool",              &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::Percent,      "oil_content/limonene",              &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::Percent,      "oil_content/nerol",                 &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::Percent,      "oil_content/pinene",                &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::Percent,      "oil_content/polyphenols",           &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::Percent,      "oil_content/xanthohumol",           &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Hop
      {JsonRecordType::FieldType::MassOrVolume, "inventory/amount",                  &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Extend Hop::amount_kg so we can cope with volumes for extract etc

      // .:TODO.JSON:. Note that we'll need to look at HopAdditionType, IBUEstimateType, IBUMethodType when we use Hops in Recipes
   };
   template<> JsonRecordType const BEER_JSON_RECORD_DEFINITION<Hop> {
      "hop_varieties",
      "Hop",
      BeerJson_HopBase + BeerJson_HopType_ExclBase
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for cultures BeerJSON records - see schemas/beerjson/1.0/culture.json
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   EnumStringMapping const BEER_JSON_YEAST_TYPE_MAPPER {
      // .:TODO.JSON:.  Add missing values here to Yeast::Type, and decide what to do about Yeast::Type::Wheat - maybe it becomes Other?
//      {"??",     Yeast::Type::Wheat}, BeerJSON doesn't have a type corresponding to this
      {"ale",           Yeast::Type::Ale},
//      {"bacteria",      Yeast::Type::},
//      {"brett",         Yeast::Type::},
      {"champagne",     Yeast::Type::Champagne},
//      {"kveik",         Yeast::Type::},
//      {"lacto",         Yeast::Type::},
      {"lager",         Yeast::Type::Lager},
//      {"malolactic",    Yeast::Type::},
//      {"mixed-culture", Yeast::Type::},
//      {"other",         Yeast::Type::},
//      {"pedio",         Yeast::Type::},
//      {"spontaneous",   Yeast::Type::},
      {"wine",          Yeast::Type::Wine},
   };
   EnumStringMapping const BEER_JSON_YEAST_FORM_MAPPER {
      // .:TODO.JSON:.  Add missing value here to Yeast::Form
      {"liquid",  Yeast::Form::Liquid},
      {"dry",     Yeast::Form::Dry},
      {"slant",   Yeast::Form::Slant},
      {"culture", Yeast::Form::Culture}
//      {"dregs",   Yeast::Form::}
   };
   EnumStringMapping const BEER_JSON_YEAST_FLOCCULATION_MAPPER {
      // BeerJSON has an entire type called QualitativeRangeType, but it's only used for this field, so, for now, we
      // treat it as an enum
      // .:TODO.JSON:.  Add missing value here to Yeast::Flocculation
//      {"very low",    Yeast::Flocculation::},
      {"low",         Yeast::Flocculation::Low},
//      {"medium low",  Yeast::Flocculation::},
      {"medium",      Yeast::Flocculation::Medium},
//      {"medium high", Yeast::Flocculation::},
      {"high",        Yeast::Flocculation::High},
      {"very high",   Yeast::Flocculation::Very_High},
   };
   template<> JsonRecordType const BEER_JSON_RECORD_DEFINITION<Yeast> {
      "cultures",
      "Yeast",
      {
         // Type                              XPath                        Q_PROPERTY                              Enum Mapper
         {JsonRecordType::FieldType::String,      "name",                      &PropertyNames::NamedEntity::name,       nullptr},
         {JsonRecordType::FieldType::Enum,        "type",                      &PropertyNames::Yeast::type,             &BEER_JSON_YEAST_TYPE_MAPPER},
         {JsonRecordType::FieldType::Enum,        "form",                      &PropertyNames::Yeast::form,             &BEER_JSON_YEAST_FORM_MAPPER},
         {JsonRecordType::FieldType::String,      "producer",                  &PropertyNames::Yeast::laboratory,       nullptr},
         {JsonRecordType::FieldType::String,      "product_id",                &PropertyNames::Yeast::productID,        nullptr},
         {JsonRecordType::FieldType::Temperature, "temperature_range/minimum", &PropertyNames::Yeast::minTemperature_c, nullptr},
         {JsonRecordType::FieldType::Temperature, "temperature_range/maximum", &PropertyNames::Yeast::maxTemperature_c, nullptr},
         {JsonRecordType::FieldType::Percent,     "alcohol_tolerance",         &BtString::NULL_STR,                     nullptr}, // .:TODO.JSON:. Add this to Yeast
         {JsonRecordType::FieldType::Enum,        "flocculation",              &PropertyNames::Yeast::flocculation,     &BEER_JSON_YEAST_FLOCCULATION_MAPPER},
         {JsonRecordType::FieldType::Percent,     "attenuation_range/minimum", &BtString::NULL_STR,                     nullptr}, // .:TODO.JSON:. Convert/extend PropertyNames::Yeast::attenuation_pct to a range
         {JsonRecordType::FieldType::Percent,     "attenuation_range/maximum", &BtString::NULL_STR,                     nullptr}, // .:TODO.JSON:. Convert/extend PropertyNames::Yeast::attenuation_pct to a range
         {JsonRecordType::FieldType::String,      "notes",                     &PropertyNames::Yeast::notes,            nullptr},
         {JsonRecordType::FieldType::String,      "best_for",                  &PropertyNames::Yeast::bestFor,          nullptr},
         {JsonRecordType::FieldType::Int,         "max_reuse",                 &PropertyNames::Yeast::maxReuse,         nullptr},
         {JsonRecordType::FieldType::Bool,        "pof",                       &BtString::NULL_STR,                     nullptr}, // .:TODO.JSON:. Add isPhenolicOffFlavorPositive (aka POF+) to Yeast
         {JsonRecordType::FieldType::Bool,        "glucoamylase",              &BtString::NULL_STR,                     nullptr}, // .:TODO.JSON:. Add isGlucoamylasePositive to Yeast
         // .:TODO.JSON:. I think this one is a bit more commplicated as inventory/dry/amount is Mass but
         // inventory/liquid/amount, inventory/slant/amount, inventory/culture/amount are all volume
         {JsonRecordType::FieldType::MassOrVolume,     "inventory/amount",     &BtString::NULL_STR,                    nullptr},
         // .:TBD.JSON:. Not sure how important it is for us to support the following fields.
         // See http://www.milkthefunk.com/wiki/Saccharomyces#Killer_Wine_Yeast for a bit more info
         {JsonRecordType::FieldType::Bool,        "zymocide/no1",              &BtString::NULL_STR,                     nullptr},
         {JsonRecordType::FieldType::Bool,        "zymocide/no2",              &BtString::NULL_STR,                     nullptr},
         {JsonRecordType::FieldType::Bool,        "zymocide/no28",             &BtString::NULL_STR,                     nullptr},
         {JsonRecordType::FieldType::Bool,        "zymocide/klus",             &BtString::NULL_STR,                     nullptr},
         {JsonRecordType::FieldType::Bool,        "zymocide/neutral",          &BtString::NULL_STR,                     nullptr},
         // Note that there is, AFAICT, no equivalent in BeerJSON to the following BeerXML properties:
         //  • Int:  TIMES_CULTURED   / PropertyNames::Yeast::timesCultured
         //  • Bool: ADD_TO_SECONDARY / PropertyNames::Yeast::addToSecondary
      }
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for profiles BeerJSON records - see schemas/beerjson/1.0/water.json
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> JsonRecordType const BEER_JSON_RECORD_DEFINITION<Water> {
      "profiles",
      "Water",
      {
         // Type                                XPath             Q_PROPERTY                             Enum Mapper
         {JsonRecordType::FieldType::String,        "name",           &PropertyNames::NamedEntity::name,      nullptr},
         {JsonRecordType::FieldType::String,        "producer",       &BtString::NULL_STR,                    nullptr}, // Not sure what this means for water...
         {JsonRecordType::FieldType::Concentration, "calcium",        &PropertyNames::Water::calcium_ppm,     nullptr},
         {JsonRecordType::FieldType::Concentration, "bicarbonate",    &PropertyNames::Water::bicarbonate_ppm, nullptr},
         {JsonRecordType::FieldType::Concentration, "potassium",      &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Water
         {JsonRecordType::FieldType::Concentration, "iron",           &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Water
         {JsonRecordType::FieldType::Concentration, "nitrate",        &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Water
         {JsonRecordType::FieldType::Concentration, "nitrite",        &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Water
         {JsonRecordType::FieldType::Concentration, "flouride",       &BtString::NULL_STR,                    nullptr}, // .:TODO.JSON:. Add this to Water
         {JsonRecordType::FieldType::Concentration, "sulfate",        &PropertyNames::Water::sulfate_ppm,     nullptr},
         {JsonRecordType::FieldType::Concentration, "chloride",       &PropertyNames::Water::chloride_ppm,    nullptr},
         {JsonRecordType::FieldType::Concentration, "sodium",         &PropertyNames::Water::sodium_ppm,      nullptr},
         {JsonRecordType::FieldType::Concentration, "magnesium",      &PropertyNames::Water::magnesium_ppm,   nullptr},
         {JsonRecordType::FieldType::Double,        "ph",             &PropertyNames::Water::ph,              nullptr},
         {JsonRecordType::FieldType::String,        "notes",          &PropertyNames::Water::notes,           nullptr},

         // .:TODO.JSON:. Note that we'll need to look at WaterAdditionType at some point...
      }
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for styles BeerJSON records - see schemas/beerjson/1.0/style.json TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   EnumStringMapping const BEER_JSON_STYLE_TYPE_MAPPER {
      // .:TBD.JSON:. BeerJSON doesn't have style types matching Style::Type::Lager, Style::Type::Ale, Style::Type::Wheat, Style::Type::Mixed
      // .:TODO.JSON:.  Add missing values here to Style::Type
//      {"beer",     Style::Type::},
      {"cider",    Style::Type::Cider},
//      {"kombucha", Style::Type::},
      {"mead",     Style::Type::Mead},
//      {"other",    Style::Type::},
//      {"soda",     Style::Type::},
//      {"wine",     Style::Type::}
   };
   template<> JsonRecordType const BEER_JSON_RECORD_DEFINITION<Style> {
      "styles",
      "Style",
      {
         // Type                              XPath                                     Q_PROPERTY                            Enum Mapper
         {JsonRecordType::FieldType::String,      "name",                                   &PropertyNames::NamedEntity::name,     nullptr},
         {JsonRecordType::FieldType::String,      "category",                               &PropertyNames::Style::category,       nullptr},
         {JsonRecordType::FieldType::Int,         "category_number",                        &PropertyNames::Style::categoryNumber, nullptr},
         {JsonRecordType::FieldType::String,      "style_letter",                           &PropertyNames::Style::styleLetter,    nullptr},
         {JsonRecordType::FieldType::String,      "style_guide",                            &PropertyNames::Style::styleGuide,     nullptr},
         {JsonRecordType::FieldType::Enum,        "type",                                   &PropertyNames::Style::type,           &BEER_JSON_STYLE_TYPE_MAPPER},
         {JsonRecordType::FieldType::Gravity,     "original_gravity/minimum",               &PropertyNames::Style::ogMin,          nullptr},
         {JsonRecordType::FieldType::Gravity,     "original_gravity/maximum",               &PropertyNames::Style::ogMax,          nullptr},
         {JsonRecordType::FieldType::Gravity,     "final_gravity/minimum",                  &PropertyNames::Style::fgMin,          nullptr},
         {JsonRecordType::FieldType::Gravity,     "final_gravity/maximum",                  &PropertyNames::Style::fgMax,          nullptr},
         {JsonRecordType::FieldType::Double,      "international_bitterness_units/minimum", &PropertyNames::Style::ibuMin,         nullptr},
         {JsonRecordType::FieldType::Double,      "international_bitterness_units/maximum", &PropertyNames::Style::ibuMax,         nullptr},
         {JsonRecordType::FieldType::Color,       "color/minimum",                          &PropertyNames::Style::colorMin_srm,   nullptr},
         {JsonRecordType::FieldType::Color,       "color/maximum",                          &PropertyNames::Style::colorMax_srm,   nullptr},
         {JsonRecordType::FieldType::Carbonation, "carbonation/minimum",                    &PropertyNames::Style::carbMin_vol,    nullptr},
         {JsonRecordType::FieldType::Carbonation, "carbonation/maximum",                    &PropertyNames::Style::carbMax_vol,    nullptr},
         {JsonRecordType::FieldType::Percent,     "alcohol_by_volume/minimum",              &PropertyNames::Style::abvMin_pct,     nullptr},
         {JsonRecordType::FieldType::Percent,     "alcohol_by_volume/maximum",              &PropertyNames::Style::abvMax_pct,     nullptr},
         {JsonRecordType::FieldType::String,      "notes",                                  &PropertyNames::Style::notes,          nullptr},
         {JsonRecordType::FieldType::String,      "aroma",                                  &BtString::NULL_STR,                   nullptr}, // .:TODO.JSON:. Add this to Style
         {JsonRecordType::FieldType::String,      "appearance",                             &BtString::NULL_STR,                   nullptr}, // .:TODO.JSON:. Add this to Style
         {JsonRecordType::FieldType::String,      "flavor",                                 &BtString::NULL_STR,                   nullptr}, // .:TODO.JSON:. Add this to Style
         {JsonRecordType::FieldType::String,      "mouthfeel",                              &BtString::NULL_STR,                   nullptr}, // .:TODO.JSON:. Add this to Style
         {JsonRecordType::FieldType::String,      "overall_impression",                     &BtString::NULL_STR,                   nullptr}, // .:TODO.JSON:. Add this to Style
         {JsonRecordType::FieldType::String,      "ingredients",                            &PropertyNames::Style::ingredients,    nullptr},
         {JsonRecordType::FieldType::String,      "examples",                               &PropertyNames::Style::examples,       nullptr},
         // .:TBD.JSON:. Nothing in BeerJSON directly maps to PropertyNames::Style::profile
      }
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for mashes BeerJSON records TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for fermentations BeerJSON records TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for recipes BeerJSON records TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for equipments BeerJSON records TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for boil BeerJSON records TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for packaging BeerJSON records TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   //
   // The mapping we use between BeerJSON structure and our own object structure
   //
   JsonCoding const BEER_JSON_1_CODING{
      // Yes, it is odd that BeerJSON 1.0 uses version number 2.06.  AFAICT this is because BeerJSON 1.0 was took its
      // starting point as the unfinished BeerXML 2.01 specification.
      "BeerJSON 1.0",
      "2.06",
      JsonSchema::Id::BEER_JSON_2_1,
      {
         BEER_JSON_RECORD_DEFINITION<void>       , //Root
         BEER_JSON_RECORD_DEFINITION<Hop>        ,
         BEER_JSON_RECORD_DEFINITION<Fermentable>,
         BEER_JSON_RECORD_DEFINITION<Yeast>      ,
         BEER_JSON_RECORD_DEFINITION<Misc>       ,
         BEER_JSON_RECORD_DEFINITION<Water>      ,
         BEER_JSON_RECORD_DEFINITION<Style>      ,
//         BEER_JSON_RECORD_DEFINITION<MashStep>   ,
//         BEER_JSON_RECORD_DEFINITION<Mash>       ,
//         BEER_JSON_RECORD_DEFINITION<Equipment>  ,
//         BEER_JSON_RECORD_DEFINITION<Instruction>,
//         BEER_JSON_RECORD_DEFINITION<BrewNote>   ,
//         BEER_JSON_RECORD_DEFINITION<Recipe>
      }
   };

   //=-=-=-=-=-=-=-=-

   // This function first validates the input file against a JSON schema (https://json-schema.org/)
   bool validateAndLoad(QString const & fileName, QTextStream & userMessage) {
      boost::json::value inputDocument;
      try {
         inputDocument = JsonUtils::loadJsonDocument(fileName);
      } catch (std::exception const & exception) {
         qWarning() <<
            Q_FUNC_INFO << "Caught exception while reading" << fileName << ":" << exception.what();
         userMessage << exception.what();
         return false;
      }

      //
      // If there are ever multiple versions of BeerJSON, this is where we'll work out which one to use for reading
      // this file.  For now, we just log some info.
      //
      // Note that, at this point, because we have not yet validated it against a JSON schema, we can't make any
      // assumptions about the input document - hence all the if statements in the block of code here.
      //
      // The root of a JSON document should be an object named "beerjson"
      //
      QString beerJsonVersion = "";
      if (!inputDocument.is_object()) {
         qWarning() << Q_FUNC_INFO << "Root of" << fileName << "is not a JSON object";
      } else {
         boost::json::object const & documentRoot = inputDocument.as_object();
         if (!documentRoot.contains("beerjson")) {
            qWarning() << Q_FUNC_INFO << "No beerjson root object found in" << fileName;
         } else {
            boost::json::value const & beerJsonValue = documentRoot.at("beerjson");
            if (!beerJsonValue.is_object()) {
               qWarning() << Q_FUNC_INFO << "beerjson element in" << fileName << "is not a JSON object";
            } else {
               boost::json::object const & beerJson = beerJsonValue.as_object();
               boost::json::value const * bjVer = beerJson.if_contains("version");
               if (!bjVer) {
                  qWarning() << Q_FUNC_INFO << "No version found in" << fileName;
               } else {
                  //
                  // Version is a JSON number (in JavaScript’s double-precision floating-point format).  It would be
                  // nice if we could get hold of the raw string from the JSON file (because, really, version is
                  // integer-dot-integer so a string would be easier to parse).  However, AFAICT, there isn't a way to
                  // do this with Boost.JSON.
                  //
                  qDebug() << Q_FUNC_INFO << "Version" << *bjVer << "(" << bjVer->kind() << ")";
                  double const * bjVersion = bjVer->if_double();
                  if (!bjVersion) {
                     qDebug() << Q_FUNC_INFO << "Could not parse version" << bjVer << "in" << fileName;
                  } else {
                     qDebug() << Q_FUNC_INFO << "BeerJSON version of" << fileName << "is" << *bjVersion;
                     beerJsonVersion = QString::number(*bjVersion);
                  }
               }
            }
         }
      }

      if (beerJsonVersion.isEmpty()) {
         qWarning() << Q_FUNC_INFO << "Unable to read BeerJSON version from" << fileName;
         userMessage << "Invalid BeerJSON file: could not read version number";
         return false;
      }

      //
      // Per above, for the moment, we assume everything is BeerJSON 1.0 (using version number 2.06 per comment above)
      // and validate against that schema.
      //
      if (!BEER_JSON_1_CODING.validateAgainstSchema(inputDocument, userMessage)) {
         qWarning() << Q_FUNC_INFO << "Schema validation of" << fileName << "failed";
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
