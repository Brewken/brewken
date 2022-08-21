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
#include "json/JsonMeasureableUnitsMapping.h"
#include "json/JsonNamedEntityRecord.h"
#include "json/JsonRecord.h"
#include "json/JsonRecordDefinition.h"
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
   //
   // These are mappings we use in multiple places
   //

   // TemperatureType
   JsonMeasureableUnitsMapping const BEER_JSON_TEMPERATURE_UNIT_MAPPER {
      "unit",
      "value",
      // TemperatureUnitType
      {
         {"C", &Measurement::Units::celsius},
         {"F", &Measurement::Units::fahrenheit}
      }
   };

   JsonMeasureableUnitsMapping const BEER_JSON_COLOR_UNIT_MAPPER {
      "unit",
      "value",
      // ColorUnitType
      {
         {"EBC", &Measurement::Units::ebc},
         {"SRM", &Measurement::Units::srm},
         {"Lovi", &Measurement::Units::lovibond}
      }
   };

   JsonMeasureableUnitsMapping const BEER_JSON_DIASTATIC_POWER_UNIT_MAPPER {
      "unit",
      "value",
      // DiastaticPowerUnitType
      {
         {"Lintner", &Measurement::Units::lintner},
         {"WK",      &Measurement::Units::wk}
      }
   };

   template<class NE> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION;

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
   // It would be nice to be able to make the JsonRecordDefinition::FieldDefinitions constants constexpr rather than just
   // const, but this is not yet easy because QVector cannot be constexpr, std::vector cannot yet be constexpr,
   // std::array (which can be constexpr) cannot deduce its own length when used with non-trivial types, and the
   // proposed std::make_array is still experimental and not yet actually part of std.  (There are various workarounds
   // with template metaprogramming but it's all a bit painful compared with the marginal benefit we would get.)
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<void> {
      "beerjson",
      "",
      JsonRecordDefinition::create<JsonRecord>,
      {
         // Type                                             Name                        Q_PROPERTY
         {JsonRecordDefinition::FieldType::RequiredConstant, "version",                   &BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "fermentables",              &BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "miscellaneous_ingredients", &BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "hop_varieties",             &BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "cultures",                  &BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "profiles",                  &BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "styles",                    &BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "mashes",                    &BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "fermentations",             &BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "recipes",                   &BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "equipments",                &BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "boil",                      &BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "packaging",                 &BtString::NULL_STR}
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
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_FermentableBase {
      // Type                                                 XPath                           Q_PROPERTY                                           Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::String,               "name",                         &PropertyNames::NamedEntity::name,                   },
      {JsonRecordDefinition::FieldType::Enum,                 "type",                         &PropertyNames::Fermentable::type,                   &BEER_JSON_FERMENTABLE_TYPE_MAPPER},
      {JsonRecordDefinition::FieldType::String,               "origin",                       &PropertyNames::Fermentable::origin,                 },
      {JsonRecordDefinition::FieldType::String,               "producer",                     &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable or look at PropertyNames::Fermentable::supplier
      {JsonRecordDefinition::FieldType::String,               "product_id",                   &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Enum,                 "grain_group",                  &PropertyNames::Fermentable::type,                   &BEER_JSON_FERMENTABLE_GRAIN_GROUP_MAPPER},
      {JsonRecordDefinition::FieldType::Percent,              "yield/fine_grind",             &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Percent,              "yield/coarse_grind",           &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Percent,              "yield/fine_coarse_difference", &PropertyNames::Fermentable::coarseFineDiff_pct,     },
      {JsonRecordDefinition::FieldType::Gravity,              "yield/potential",              &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::MeasurementWithUnits, "color",                        &PropertyNames::Fermentable::color_srm,              &BEER_JSON_COLOR_UNIT_MAPPER},
   };
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_FermentableType_ExclBase {
      // Type                                                 XPath                           Q_PROPERTY                                           Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::String,               "notes",                        &PropertyNames::Fermentable::notes,                  },
      {JsonRecordDefinition::FieldType::Percent,              "moisture",                     &PropertyNames::Fermentable::moisture_pct,           },
      {JsonRecordDefinition::FieldType::Double,               "alpha_amylase",                &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::MeasurementWithUnits, "diastatic_power",              &PropertyNames::Fermentable::diastaticPower_lintner, &BEER_JSON_DIASTATIC_POWER_UNIT_MAPPER},
      {JsonRecordDefinition::FieldType::Percent,              "protein",                      &PropertyNames::Fermentable::protein_pct,            },
      {JsonRecordDefinition::FieldType::Double,               "kolbach_index",                &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Percent,              "max_in_batch",                 &PropertyNames::Fermentable::maxInBatch_pct,         },
      {JsonRecordDefinition::FieldType::Bool,                 "recommend_mash",               &PropertyNames::Fermentable::recommendMash,          }, // .:TODO.JSON:. What is the difference between PropertyNames::Fermentable::recommendMash and PropertyNames::Fermentable::isMashed
      {JsonRecordDefinition::FieldType::MassOrVolume,         "inventory/amount",             &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Extend Fermentable::amount_kg so we can cope with volumes
      {JsonRecordDefinition::FieldType::Percent,              "glassy",                       &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Percent,              "plump",                        &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Percent,              "half",                         &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Percent,              "mealy",                        &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Percent,              "thru",                         &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Percent,              "friability",                   &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Acidity,              "di_ph",                        &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Viscosity,            "viscosity",                    &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Concentration,        "dms_p",                        &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Concentration,        "fan",                          &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Percent,              "fermentability",               &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
      {JsonRecordDefinition::FieldType::Concentration,        "beta_glucan",                  &BtString::NULL_STR,                                 }, // .:TODO.JSON:. Add this to Fermentable
   };
   // .:TODO.JSON:.  Extend Recipe to have an enum for this
   EnumStringMapping const BEER_JSON_RECIPE_ADDITION_POINT_MAPPER {
///      {"add_to_mash",         Recipe::},
///      {"add_to_boil",         Recipe::},
///      {"add_to_fermentation", Recipe::},
///      {"add_to_package",      Recipe::},
    };
   // This is the same across Fermentable, Hop, Misc
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_IngredientAdditionType_ExclBase {
      // Type                                           XPath                           Q_PROPERTY                            Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::TimeElapsed,    "timing/time",                  &BtString::NULL_STR,                  }, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::TimeElapsed,    "timing/duration",              &BtString::NULL_STR,                  }, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::Bool,           "timing/continuous",            &BtString::NULL_STR,                  }, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::Gravity,        "timing/specific_gravity",      &BtString::NULL_STR,                  }, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::Acidity,        "timing/pH",                    &BtString::NULL_STR,                  }, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::Int,            "timing/step",                  &BtString::NULL_STR,                  }, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::Enum,           "timing/use",                   &BtString::NULL_STR,                  &BEER_JSON_RECIPE_ADDITION_POINT_MAPPER}, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::MassOrVolume,   "amount",                       &BtString::NULL_STR,                  }, // .:TODO.JSON:.
   };
   // As mentioned above, it would be really nice to do this at compile time, but haven't yet found a nice way to do so
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Fermentable> {
      "fermentables",
      "Fermentable",
      JsonRecordDefinition::create< JsonNamedEntityRecord< Fermentable > >,
      {BeerJson_FermentableBase, BeerJson_FermentableType_ExclBase}
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
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_MiscellaneousBase {
      // Type                                         XPath               Q_PROPERTY                         Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::String,       "name",             &PropertyNames::NamedEntity::name, },
      {JsonRecordDefinition::FieldType::String,       "producer",         &BtString::NULL_STR,               }, // .:TODO.JSON:. Add this to Misc
      {JsonRecordDefinition::FieldType::String,       "product_id",       &BtString::NULL_STR,               }, // .:TODO.JSON:. Add this to Misc
      {JsonRecordDefinition::FieldType::Enum,         "type",             &PropertyNames::Fermentable::type, &BEER_JSON_MISC_TYPE_MAPPER},
   };
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_MiscellaneousType_ExclBase {
      // Type                                         XPath               Q_PROPERTY                         Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::String,       "use_for",          &PropertyNames::Misc::useFor,      },
      {JsonRecordDefinition::FieldType::String,       "notes",            &PropertyNames::Misc::notes,       },
      {JsonRecordDefinition::FieldType::MassOrVolume, "inventory/amount", &PropertyNames::Misc::amount,      }, // .:TODO.JSON:. Also need to reference Misc::amountIsWeight PLUS we need to cope with UnitType
   };
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Misc> {
      "miscellaneous_ingredients",
      "Misc",
      JsonRecordDefinition::create< JsonNamedEntityRecord< Misc > >,
      {BeerJson_MiscellaneousBase, BeerJson_MiscellaneousType_ExclBase}
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
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_HopBase {
      // Type                                         XPath                                Q_PROPERTY                              Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::String,       "name",                              &PropertyNames::NamedEntity::name,      },
      {JsonRecordDefinition::FieldType::String,       "producer",                          &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::String,       "product_id",                        &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::String,       "origin",                            &PropertyNames::Hop::origin,            },
      {JsonRecordDefinition::FieldType::String,       "year",                              &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::Enum,         "form",                              &PropertyNames::Hop::form,              &BEER_JSON_HOP_FORM_MAPPER},
   };
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_HopType_ExclBase {
      // Type                                         XPath                                Q_PROPERTY                              Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::Percent,      "alpha_acid",                        &PropertyNames::Hop::alpha_pct,         },
      {JsonRecordDefinition::FieldType::Percent,      "beta_acid",                         &PropertyNames::Hop::beta_pct,          },
      {JsonRecordDefinition::FieldType::Enum,         "type",                              &PropertyNames::Hop::type,              &BEER_JSON_HOP_TYPE_MAPPER},
      {JsonRecordDefinition::FieldType::String,       "notes",                             &PropertyNames::Hop::notes,             },
      {JsonRecordDefinition::FieldType::Percent,      "percent_lost",                      &PropertyNames::Hop::hsi_pct,           },
      {JsonRecordDefinition::FieldType::String,       "substitutes",                       &PropertyNames::Hop::substitutes,       },
      {JsonRecordDefinition::FieldType::Double,       "oil_content/total_oil_ml_per_100g", &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/humulene",              &PropertyNames::Hop::humulene_pct,      },
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/caryophyllene",         &PropertyNames::Hop::caryophyllene_pct, },
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/cohumulone",            &PropertyNames::Hop::cohumulone_pct,    },
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/myrcene",               &PropertyNames::Hop::myrcene_pct,       },
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/farnesene",             &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/geraniol",              &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/b_pinene",              &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/linalool",              &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/limonene",              &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/nerol",                 &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/pinene",                &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/polyphenols",           &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::Percent,      "oil_content/xanthohumol",           &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Hop
      {JsonRecordDefinition::FieldType::MassOrVolume, "inventory/amount",                  &BtString::NULL_STR,                    }, // .:TODO.JSON:. Extend Hop::amount_kg so we can cope with volumes for extract etc

      // .:TODO.JSON:. Note that we'll need to look at HopAdditionType, IBUEstimateType, IBUMethodType when we use Hops in Recipes
   };
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Hop> {
      "hop_varieties",
      "Hop",
      JsonRecordDefinition::create< JsonNamedEntityRecord< Hop > >,
      {BeerJson_HopBase, BeerJson_HopType_ExclBase}
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
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Yeast> {
      "cultures",
      "Yeast",
      JsonRecordDefinition::create< JsonNamedEntityRecord< Yeast > >,
      {
         // Type                                                 XPath                        Q_PROPERTY                               Enum/Unit Mapper
         {JsonRecordDefinition::FieldType::String,               "name",                      &PropertyNames::NamedEntity::name,       },
         {JsonRecordDefinition::FieldType::Enum,                 "type",                      &PropertyNames::Yeast::type,             &BEER_JSON_YEAST_TYPE_MAPPER},
         {JsonRecordDefinition::FieldType::Enum,                 "form",                      &PropertyNames::Yeast::form,             &BEER_JSON_YEAST_FORM_MAPPER},
         {JsonRecordDefinition::FieldType::String,               "producer",                  &PropertyNames::Yeast::laboratory,       },
         {JsonRecordDefinition::FieldType::String,               "product_id",                &PropertyNames::Yeast::productID,        },
         {JsonRecordDefinition::FieldType::MeasurementWithUnits, "temperature_range/minimum", &PropertyNames::Yeast::minTemperature_c, &BEER_JSON_TEMPERATURE_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::MeasurementWithUnits, "temperature_range/maximum", &PropertyNames::Yeast::maxTemperature_c, &BEER_JSON_TEMPERATURE_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::Percent,              "alcohol_tolerance",         &BtString::NULL_STR,                     }, // .:TODO.JSON:. Add this to Yeast
         {JsonRecordDefinition::FieldType::Enum,                 "flocculation",              &PropertyNames::Yeast::flocculation,     &BEER_JSON_YEAST_FLOCCULATION_MAPPER},
         {JsonRecordDefinition::FieldType::Percent,              "attenuation_range/minimum", &BtString::NULL_STR,                     }, // .:TODO.JSON:. Convert/extend PropertyNames::Yeast::attenuation_pct to a range
         {JsonRecordDefinition::FieldType::Percent,              "attenuation_range/maximum", &BtString::NULL_STR,                     }, // .:TODO.JSON:. Convert/extend PropertyNames::Yeast::attenuation_pct to a range
         {JsonRecordDefinition::FieldType::String,               "notes",                     &PropertyNames::Yeast::notes,            },
         {JsonRecordDefinition::FieldType::String,               "best_for",                  &PropertyNames::Yeast::bestFor,          },
         {JsonRecordDefinition::FieldType::Int,                  "max_reuse",                 &PropertyNames::Yeast::maxReuse,         },
         {JsonRecordDefinition::FieldType::Bool,                 "pof",                       &BtString::NULL_STR,                     }, // .:TODO.JSON:. Add isPhenolicOffFlavorPositive (aka POF+) to Yeast
         {JsonRecordDefinition::FieldType::Bool,                 "glucoamylase",              &BtString::NULL_STR,                     }, // .:TODO.JSON:. Add isGlucoamylasePositive to Yeast
         // .:TODO.JSON:. I think this one is a bit more commplicated as inventory/dry/amount is Mass but
         // inventory/liquid/amount, inventory/slant/amount, inventory/culture/amount are all volume
         {JsonRecordDefinition::FieldType::MassOrVolume,         "inventory/amount",          &BtString::NULL_STR,                     },
         // .:TBD.JSON:. Not sure how important it is for us to support the following fields.
         // See http://www.milkthefunk.com/wiki/Saccharomyces#Killer_Wine_Yeast for a bit more info
         {JsonRecordDefinition::FieldType::Bool,                 "zymocide/no1",              &BtString::NULL_STR,                     },
         {JsonRecordDefinition::FieldType::Bool,                 "zymocide/no2",              &BtString::NULL_STR,                     },
         {JsonRecordDefinition::FieldType::Bool,                 "zymocide/no28",             &BtString::NULL_STR,                     },
         {JsonRecordDefinition::FieldType::Bool,                 "zymocide/klus",             &BtString::NULL_STR,                     },
         {JsonRecordDefinition::FieldType::Bool,                 "zymocide/neutral",          &BtString::NULL_STR,                     },
         // Note that there is, AFAICT, no equivalent in BeerJSON to the following BeerXML properties:
         //  • Int:  TIMES_CULTURED   / PropertyNames::Yeast::timesCultured
         //  • Bool: ADD_TO_SECONDARY / PropertyNames::Yeast::addToSecondary
      }
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for profiles BeerJSON records - see schemas/beerjson/1.0/water.json
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Water> {
      "profiles",
      "Water",
      JsonRecordDefinition::create< JsonNamedEntityRecord< Water > >,
      {
         // Type                                          XPath             Q_PROPERTY                              Enum/Unit Mapper
         {JsonRecordDefinition::FieldType::String,        "name",           &PropertyNames::NamedEntity::name,      },
         {JsonRecordDefinition::FieldType::String,        "producer",       &BtString::NULL_STR,                    }, // Not sure what this means for water...
         {JsonRecordDefinition::FieldType::Concentration, "calcium",        &PropertyNames::Water::calcium_ppm,     },
         {JsonRecordDefinition::FieldType::Concentration, "bicarbonate",    &PropertyNames::Water::bicarbonate_ppm, },
         {JsonRecordDefinition::FieldType::Concentration, "potassium",      &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Water
         {JsonRecordDefinition::FieldType::Concentration, "iron",           &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Water
         {JsonRecordDefinition::FieldType::Concentration, "nitrate",        &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Water
         {JsonRecordDefinition::FieldType::Concentration, "nitrite",        &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Water
         {JsonRecordDefinition::FieldType::Concentration, "flouride",       &BtString::NULL_STR,                    }, // .:TODO.JSON:. Add this to Water
         {JsonRecordDefinition::FieldType::Concentration, "sulfate",        &PropertyNames::Water::sulfate_ppm,     },
         {JsonRecordDefinition::FieldType::Concentration, "chloride",       &PropertyNames::Water::chloride_ppm,    },
         {JsonRecordDefinition::FieldType::Concentration, "sodium",         &PropertyNames::Water::sodium_ppm,      },
         {JsonRecordDefinition::FieldType::Concentration, "magnesium",      &PropertyNames::Water::magnesium_ppm,   },
         {JsonRecordDefinition::FieldType::Double,        "ph",             &PropertyNames::Water::ph,              },
         {JsonRecordDefinition::FieldType::String,        "notes",          &PropertyNames::Water::notes,           },

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
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Style> {
      "styles",
      "Style",
      JsonRecordDefinition::create< JsonNamedEntityRecord< Style > >,
      {
         // Type                                                 XPath                                     Q_PROPERTY                             Enum/Unit Mapper
         {JsonRecordDefinition::FieldType::String,               "name",                                   &PropertyNames::NamedEntity::name,     },
         {JsonRecordDefinition::FieldType::String,               "category",                               &PropertyNames::Style::category,       },
         {JsonRecordDefinition::FieldType::Int,                  "category_number",                        &PropertyNames::Style::categoryNumber, },
         {JsonRecordDefinition::FieldType::String,               "style_letter",                           &PropertyNames::Style::styleLetter,    },
         {JsonRecordDefinition::FieldType::String,               "style_guide",                            &PropertyNames::Style::styleGuide,     },
         {JsonRecordDefinition::FieldType::Enum,                 "type",                                   &PropertyNames::Style::type,           &BEER_JSON_STYLE_TYPE_MAPPER},
         {JsonRecordDefinition::FieldType::Gravity,              "original_gravity/minimum",               &PropertyNames::Style::ogMin,          },
         {JsonRecordDefinition::FieldType::Gravity,              "original_gravity/maximum",               &PropertyNames::Style::ogMax,          },
         {JsonRecordDefinition::FieldType::Gravity,              "final_gravity/minimum",                  &PropertyNames::Style::fgMin,          },
         {JsonRecordDefinition::FieldType::Gravity,              "final_gravity/maximum",                  &PropertyNames::Style::fgMax,          },
         {JsonRecordDefinition::FieldType::Double,               "international_bitterness_units/minimum", &PropertyNames::Style::ibuMin,         },
         {JsonRecordDefinition::FieldType::Double,               "international_bitterness_units/maximum", &PropertyNames::Style::ibuMax,         },
         {JsonRecordDefinition::FieldType::MeasurementWithUnits, "color/minimum",                          &PropertyNames::Style::colorMin_srm,   &BEER_JSON_COLOR_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::MeasurementWithUnits, "color/maximum",                          &PropertyNames::Style::colorMax_srm,   &BEER_JSON_COLOR_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::Carbonation,          "carbonation/minimum",                    &PropertyNames::Style::carbMin_vol,    },
         {JsonRecordDefinition::FieldType::Carbonation,          "carbonation/maximum",                    &PropertyNames::Style::carbMax_vol,    },
         {JsonRecordDefinition::FieldType::Percent,              "alcohol_by_volume/minimum",              &PropertyNames::Style::abvMin_pct,     },
         {JsonRecordDefinition::FieldType::Percent,              "alcohol_by_volume/maximum",              &PropertyNames::Style::abvMax_pct,     },
         {JsonRecordDefinition::FieldType::String,               "notes",                                  &PropertyNames::Style::notes,          },
         {JsonRecordDefinition::FieldType::String,               "aroma",                                  &BtString::NULL_STR,                   }, // .:TODO.JSON:. Add this to Style
         {JsonRecordDefinition::FieldType::String,               "appearance",                             &BtString::NULL_STR,                   }, // .:TODO.JSON:. Add this to Style
         {JsonRecordDefinition::FieldType::String,               "flavor",                                 &BtString::NULL_STR,                   }, // .:TODO.JSON:. Add this to Style
         {JsonRecordDefinition::FieldType::String,               "mouthfeel",                              &BtString::NULL_STR,                   }, // .:TODO.JSON:. Add this to Style
         {JsonRecordDefinition::FieldType::String,               "overall_impression",                     &BtString::NULL_STR,                   }, // .:TODO.JSON:. Add this to Style
         {JsonRecordDefinition::FieldType::String,               "ingredients",                            &PropertyNames::Style::ingredients,    },
         {JsonRecordDefinition::FieldType::String,               "examples",                               &PropertyNames::Style::examples,       },
         // .:TBD.JSON:. Nothing in BeerJSON directly maps to PropertyNames::Style::profile
      }
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for mashes BeerJSON records TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      JsonRecordDefinition::create< JsonMashRecord >,

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for fermentations BeerJSON records TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      JsonRecordDefinition::create< JsonNamedEntityRecord< Fermantation > >,

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for recipes BeerJSON records TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      JsonRecordDefinition::create< JsonRecipRecord >,

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for equipments BeerJSON records TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      JsonRecordDefinition::create< JsonNamedEntityRecord< Equipment > >,

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for boil BeerJSON records TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      JsonRecordDefinition::create< JsonNamedEntityRecord< Boil > >,

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for packaging BeerJSON records TODO
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      JsonRecordDefinition::create< JsonNamedEntityRecord< Packaging > >,

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
      // Obviously, in time, if and when BeerJSON evolves, we'll want to do something less hard-coded here!
      //
      if (beerJsonVersion != "2.06") {
         qWarning() <<
            Q_FUNC_INFO << "BeerJSON version " << beerJsonVersion << "differs from what we are expecting (2.06)";
      }
      return BEER_JSON_1_CODING.validateLoadAndStoreInDb(inputDocument, userMessage);
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
