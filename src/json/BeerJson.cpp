/*======================================================================================================================
 * json/BeerJson.cpp is part of Brewken, and is copyright the following authors 2021-2023:
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

#include <cstdlib>

// We could just include <boost/json.hpp> which pulls all the Boost.JSON headers in, but that seems overkill
#include <boost/json/kind.hpp>
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
#include "model/BrewNote.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Hop.h"
#include "model/Misc.h"
#include "model/Recipe.h"
#include "model/Style.h"
#include "model/Water.h"
#include "model/Yeast.h"
#include "utils/OStreamWriterForQFile.h"

// TODO: WE should upgrade our copy of the BeerJSON schema to the 1.0.2 release at https://github.com/beerjson/beerjson/releases/tag/v1.0.2

namespace {
   // See below for more comments on this.  If and when BeerJSON evolves then we will want separate constants for
   // min/max versions we can read plus whatever version we write.
   BtStringConst const jsonVersionWeSupport{"2.06"};

   //
   // These are mappings we use in multiple places
   //
   JsonMeasureableUnitsMapping const BEER_JSON_MASS_UNIT_MAPPER {
      // MassUnitType in measurable_units.json in BeerJSON schema
      {{"mg",    &Measurement::Units::milligrams},
       {"mg",    &Measurement::Units::grams     },
       {"kg",    &Measurement::Units::kilograms },
       {"lb",    &Measurement::Units::pounds    },
       {"oz",    &Measurement::Units::ounces    }}
   };

   JsonMeasureableUnitsMapping const BEER_JSON_VOLUME_UNIT_MAPPER {
      // VolumeUnitType in measurable_units.json in BeerJSON schema
      // Note that BeerJSON does not support imperial cups, imperial tablespoons or imperial teaspoons
      {{"ml"   , &Measurement::Units::milliliters         },
       {"l"    , &Measurement::Units::liters              },
       {"tsp"  , &Measurement::Units::us_teaspoons        },
       {"tbsp" , &Measurement::Units::us_tablespoons      },
       {"floz" , &Measurement::Units::us_fluidOunces      },
       {"cup"  , &Measurement::Units::us_cups             },
       {"pt"   , &Measurement::Units::us_pints            },
       {"qt"   , &Measurement::Units::us_quarts           },
       {"gal"  , &Measurement::Units::us_gallons          },
       {"bbl"  , &Measurement::Units::us_barrels          },
       {"ifloz", &Measurement::Units::imperial_fluidOunces},
       {"ipt"  , &Measurement::Units::imperial_pints      },
       {"iqt"  , &Measurement::Units::imperial_quarts     },
       {"igal" , &Measurement::Units::imperial_gallons    },
       {"ibbl" , &Measurement::Units::imperial_barrels    }}
   };

   ListOfJsonMeasureableUnitsMappings const BEER_JSON_MASS_OR_VOLUME_UNIT_MAPPER {
      {&BEER_JSON_MASS_UNIT_MAPPER, &BEER_JSON_VOLUME_UNIT_MAPPER}
   };

   JsonMeasureableUnitsMapping const BEER_JSON_TEMPERATURE_UNIT_MAPPER {
      // TemperatureUnitType in measurable_units.json in BeerJSON schema
      {{"C", &Measurement::Units::celsius   },
       {"F", &Measurement::Units::fahrenheit}}
   };

   JsonMeasureableUnitsMapping const BEER_JSON_COLOR_UNIT_MAPPER {
      // ColorUnitType in measurable_units.json in BeerJSON schema
      {{"EBC" , &Measurement::Units::ebc     },
       {"SRM" , &Measurement::Units::srm     },
       {"Lovi", &Measurement::Units::lovibond}}
   };

   JsonMeasureableUnitsMapping const BEER_JSON_DIASTATIC_POWER_UNIT_MAPPER {
      // DiastaticPowerUnitType in measurable_units.json in BeerJSON schema
      {{"Lintner", &Measurement::Units::lintner},
       {"WK",      &Measurement::Units::wk}}
   };

   // BitternessUnitType in measurable_units.json in BeerJSON schema
   JsonSingleUnitSpecifier const BEER_JSON_BITTERNESS_UNIT{{"IBUs"}};

   JsonMeasureableUnitsMapping const BEER_JSON_CARBONATION_UNIT_MAPPER {
      // CarbonationUnitType in measurable_units.json in BeerJSON schema
      {{"vols", &Measurement::Units::carbonationVolumes      },
       {"g/l" , &Measurement::Units::carbonationGramsPerLiter}}
   };

   JsonMeasureableUnitsMapping const BEER_JSON_VOLUME_CONCENTRATION_UNIT_MAPPER {
      // ConcentrationUnitType in measurable_units.json in BeerJSON schema
      {{"ppm",  &Measurement::Units::partsPerMillion},
       {"ppb",  &Measurement::Units::partsPerBillion}}
   };

   JsonMeasureableUnitsMapping const BEER_JSON_MASS_CONCENTRATION_UNIT_MAPPER {
      // ConcentrationUnitType in measurable_units.json in BeerJSON schema
      {{"ppm" , &Measurement::Units::partsPerMillion   },
       {"ppb" , &Measurement::Units::partsPerBillion   },
       {"mg/l", &Measurement::Units::milligramsPerLiter}}
   };

   ListOfJsonMeasureableUnitsMappings const BEER_JSON_CONCENTRATION_UNIT_MAPPER {
      {&BEER_JSON_VOLUME_CONCENTRATION_UNIT_MAPPER, &BEER_JSON_MASS_CONCENTRATION_UNIT_MAPPER}
   };

   JsonMeasureableUnitsMapping const BEER_JSON_DENSITY_UNIT_MAPPER {
      // GravityUnitType in measurable_units.json in BeerJSON schema
      // (See comments in measurement/Unit.h and measurement/PhysicalQuantity.h for why we stick with "density" in our
      // naming.)
      // Note that DensityUnitType is identically defined in measurable_units.json, but does not appear to be referenced
      // anywhere else.
      {{"sg"   , &Measurement::Units::specificGravity},
       {"plato", &Measurement::Units::plato          },
       {"brix" , &Measurement::Units::brix           }}
   };

   // PercentUnitType in measurable_units.json in BeerJSON schema
   JsonSingleUnitSpecifier const BEER_JSON_PERCENT_UNIT{{"%"}};

   // AcidityUnitType in measurable_units.json in BeerJSON schema
   JsonSingleUnitSpecifier const BEER_JSON_ACIDITY_UNIT{{"pH"}};

   JsonMeasureableUnitsMapping const BEER_JSON_TIME_UNIT_MAPPER {
      // TimeUnitType in measurable_units.json in BeerJSON schema
      {{"sec" , &Measurement::Units::seconds},
       {"min" , &Measurement::Units::minutes},
       {"hr"  , &Measurement::Units::hours  },
       {"day" , &Measurement::Units::days   },
       {"week", &Measurement::Units::weeks  }}
   };

   JsonMeasureableUnitsMapping const BEER_JSON_VISCOSITY_UNIT_MAPPER {
      // ViscosityUnitType in measurable_units.json in BeerJSON schema
      {{"cP",    &Measurement::Units::centipoise       },
       {"mPa-s", &Measurement::Units::millipascalSecond}}
   };

   // We only use specialisations of this template.  GCC doesn't mind not having a definition for the default cases (as
   // it's not used) but other compilers do.
   template<class NE> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION {
      "not_used",
      nullptr,
      "not_used",
      JsonRecordDefinition::create<JsonRecord>,
      std::initializer_list<JsonRecordDefinition::FieldDefinition>{}
   };

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
   // The BeerJSON schema is strict about some things but not about others.  Eg, you can't add in your own top-level
   // object (which, eg, since JSON doesn't allow comments, would be useful to use to record information about the
   // program that wrote the file), but you can add extra fields to individual records (eg we could add a "foobar" field
   // inside each hop record and it would pass validation against the BeerJSON schema.
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
      nullptr,
      "",
      JsonRecordDefinition::create<JsonRecord>,
      {
         // Type                                             Name                         Q_PROPERTY
         {JsonRecordDefinition::FieldType::RequiredConstant, "version",                   jsonVersionWeSupport},
         {JsonRecordDefinition::FieldType::Array,            "fermentables",              BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "miscellaneous_ingredients", BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "hop_varieties",             BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "cultures",                  BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "profiles",                  BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "styles",                    BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "mashes",                    BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "fermentations",             BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "recipes",                   BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "equipments",                BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "boil",                      BtString::NULL_STR},
         {JsonRecordDefinition::FieldType::Array,            "packaging",                 BtString::NULL_STR}
      }
   };

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for fermentables BeerJSON records - see schemas/beerjson/1.0/fermentable.json
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_FermentableBase {
      // Type                                                 XPath                           Q_PROPERTY                                        Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::String              , "name"                        , PropertyNames::NamedEntity::name                ,                                      },
      {JsonRecordDefinition::FieldType::Enum                , "type"                        , PropertyNames::Fermentable::type                , &Fermentable::typeStringMapping      },
      {JsonRecordDefinition::FieldType::String              , "origin"                      , PropertyNames::Fermentable::origin              ,                                      },
      {JsonRecordDefinition::FieldType::String              , "producer"                    , PropertyNames::Fermentable::producer            ,                                      },
      {JsonRecordDefinition::FieldType::String              , "product_id"                  , PropertyNames::Fermentable::productId           ,                                      },
      {JsonRecordDefinition::FieldType::Enum                , "grain_group"                 , PropertyNames::Fermentable::grainGroup          , &Fermentable::grainGroupStringMapping},
      {JsonRecordDefinition::FieldType::SingleUnitValue     , "yield/fine_grind"            , PropertyNames::Fermentable::fineGrindYield_pct  , &BEER_JSON_PERCENT_UNIT              },
      {JsonRecordDefinition::FieldType::SingleUnitValue     , "yield/coarse_grind"          , PropertyNames::Fermentable::coarseGrindYield_pct, &BEER_JSON_PERCENT_UNIT              },
      {JsonRecordDefinition::FieldType::SingleUnitValue     , "yield/fine_coarse_difference", PropertyNames::Fermentable::coarseFineDiff_pct  , &BEER_JSON_PERCENT_UNIT              },
      {JsonRecordDefinition::FieldType::MeasurementWithUnits, "yield/potential"             , PropertyNames::Fermentable::potentialYield_sg   , &BEER_JSON_DENSITY_UNIT_MAPPER       },
      {JsonRecordDefinition::FieldType::MeasurementWithUnits, "color"                       , PropertyNames::Fermentable::color_srm           , &BEER_JSON_COLOR_UNIT_MAPPER         },
   };
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_FermentableType_ExclBase {
      // Type                                                       XPath               Q_PROPERTY                                          Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::String                    , "notes"           , PropertyNames::Fermentable::notes                 ,                                       },
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "moisture"        , PropertyNames::Fermentable::moisture_pct          , &BEER_JSON_PERCENT_UNIT               },
      {JsonRecordDefinition::FieldType::Double                    , "alpha_amylase"   , PropertyNames::Fermentable::alphaAmylase_dextUnits,                                       },
      {JsonRecordDefinition::FieldType::MeasurementWithUnits      , "diastatic_power" , PropertyNames::Fermentable::diastaticPower_lintner, &BEER_JSON_DIASTATIC_POWER_UNIT_MAPPER},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "protein"         , PropertyNames::Fermentable::protein_pct           , &BEER_JSON_PERCENT_UNIT               },
      {JsonRecordDefinition::FieldType::Double                    , "kolbach_index"   , PropertyNames::Fermentable::kolbachIndex_pct      ,                                       },
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "max_in_batch"    , PropertyNames::Fermentable::maxInBatch_pct        , &BEER_JSON_PERCENT_UNIT               },
      {JsonRecordDefinition::FieldType::Bool                      , "recommend_mash"  , PropertyNames::Fermentable::recommendMash         ,                                       },
      {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "inventory/amount", PropertyNames::NamedEntityWithInventory::inventoryWithUnits, &BEER_JSON_MASS_OR_VOLUME_UNIT_MAPPER },
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "glassy"          , PropertyNames::Fermentable::hardnessPrpGlassy_pct , &BEER_JSON_PERCENT_UNIT               },
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "plump"           , PropertyNames::Fermentable::kernelSizePrpPlump_pct, &BEER_JSON_PERCENT_UNIT               },
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "half"            , PropertyNames::Fermentable::hardnessPrpHalf_pct   , &BEER_JSON_PERCENT_UNIT               },
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "mealy"           , PropertyNames::Fermentable::hardnessPrpMealy_pct  , &BEER_JSON_PERCENT_UNIT               },
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "thru"            , PropertyNames::Fermentable::kernelSizePrpThin_pct , &BEER_JSON_PERCENT_UNIT               },
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "friability"      , PropertyNames::Fermentable::friability_pct        , &BEER_JSON_PERCENT_UNIT               },
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "di_ph"           , PropertyNames::Fermentable::di_ph                 , &BEER_JSON_ACIDITY_UNIT               },
      {JsonRecordDefinition::FieldType::MeasurementWithUnits      , "viscosity"       , PropertyNames::Fermentable::viscosity_cP          , &BEER_JSON_VISCOSITY_UNIT_MAPPER      },
      {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "dms_p"           , PropertyNames::Fermentable::dmsPWithUnits         , &BEER_JSON_CONCENTRATION_UNIT_MAPPER  },
      {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "fan"             , PropertyNames::Fermentable::fanWithUnits          , &BEER_JSON_CONCENTRATION_UNIT_MAPPER  },
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "fermentability"  , PropertyNames::Fermentable::fermentability_pct    , &BEER_JSON_PERCENT_UNIT               },
      {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "beta_glucan"     , PropertyNames::Fermentable::betaGlucanWithUnits   , &BEER_JSON_CONCENTRATION_UNIT_MAPPER  },
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
      // Type                                                       XPath                      Q_PROPERTY           Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::MeasurementWithUnits      , "timing/time"            , BtString::NULL_STR,  &BEER_JSON_TIME_UNIT_MAPPER}, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::MeasurementWithUnits      , "timing/duration"        , BtString::NULL_STR,  &BEER_JSON_TIME_UNIT_MAPPER}, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::Bool                      , "timing/continuous"      , BtString::NULL_STR,  }, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::MeasurementWithUnits      , "timing/specific_gravity", BtString::NULL_STR,  &BEER_JSON_DENSITY_UNIT_MAPPER}, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "timing/pH"              , BtString::NULL_STR,  &BEER_JSON_ACIDITY_UNIT}, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::Int                       , "timing/step"            , BtString::NULL_STR,  }, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::Enum                      , "timing/use"             , BtString::NULL_STR,  &BEER_JSON_RECIPE_ADDITION_POINT_MAPPER}, // .:TODO.JSON:.
      {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "amount"                 , BtString::NULL_STR,  &BEER_JSON_MASS_OR_VOLUME_UNIT_MAPPER}, // .:TODO.JSON:.
   };
   // As mentioned above, it would be really nice to do this at compile time, but haven't yet found a nice way to do so
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Fermentable> {
      "fermentables",
      &Fermentable::typeLookup,
      "Fermentable",
      JsonRecordDefinition::create< JsonNamedEntityRecord< Fermentable > >,
      {BeerJson_FermentableBase, BeerJson_FermentableType_ExclBase}
   };

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for miscellaneous_ingredients BeerJSON records - see schemas/beerjson/1.0/misc.json
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_MiscellaneousBase {
      // Type                                         XPath               Q_PROPERTY                        Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::String,       "name"      ,       PropertyNames::NamedEntity::name},
      {JsonRecordDefinition::FieldType::String,       "producer"  ,       PropertyNames::Misc::producer   },
      {JsonRecordDefinition::FieldType::String,       "product_id",       PropertyNames::Misc::productId  },
      {JsonRecordDefinition::FieldType::Enum  ,       "type"      ,       PropertyNames::Fermentable::type, &Misc::typeStringMapping},
   };
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_MiscellaneousType_ExclBase {
      // Type                                                       XPath               Q_PROPERTY                            Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::String                    , "use_for"         , PropertyNames::Misc::useFor         },
      {JsonRecordDefinition::FieldType::String                    , "notes"           , PropertyNames::Misc::notes          },
      {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "inventory/amount", PropertyNames::NamedEntityWithInventory::inventoryWithUnits, &BEER_JSON_MASS_OR_VOLUME_UNIT_MAPPER},
   };
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Misc> {
      "miscellaneous_ingredients",
      &Misc::typeLookup,
      "Misc",
      JsonRecordDefinition::create< JsonNamedEntityRecord< Misc > >,
      {BeerJson_MiscellaneousBase, BeerJson_MiscellaneousType_ExclBase}
   };

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for hop_varieties BeerJSON records - see schemas/beerjson/1.0/hop.json
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_HopBase {
      // Type                                   XPath         Q_PROPERTY                        Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::String, "name"      , PropertyNames::NamedEntity::name},
      {JsonRecordDefinition::FieldType::String, "producer"  , PropertyNames::Hop::producer    },
      {JsonRecordDefinition::FieldType::String, "product_id", PropertyNames::Hop::product_id  },
      {JsonRecordDefinition::FieldType::String, "origin"    , PropertyNames::Hop::origin      },
      {JsonRecordDefinition::FieldType::String, "year"      , PropertyNames::Hop::year        },
      {JsonRecordDefinition::FieldType::Enum  , "form"      , PropertyNames::Hop::form        , &Hop::formStringMapping},
   };
   std::initializer_list<JsonRecordDefinition::FieldDefinition> const BeerJson_HopType_ExclBase {
      // Type                                                       XPath                                Q_PROPERTY                                 Enum/Unit Mapper
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "alpha_acid"                       , PropertyNames::Hop::alpha_pct,             &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "beta_acid"                        , PropertyNames::Hop::beta_pct,              &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::Enum                      , "type"                             , PropertyNames::Hop::type,                  &Hop::typeStringMapping},
      {JsonRecordDefinition::FieldType::String                    , "notes"                            , PropertyNames::Hop::notes,                 },
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "percent_lost"                     , PropertyNames::Hop::hsi_pct,               &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::String                    , "substitutes"                      , PropertyNames::Hop::substitutes,           },
      {JsonRecordDefinition::FieldType::Double                    , "oil_content/total_oil_ml_per_100g", PropertyNames::Hop::total_oil_ml_per_100g, },
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/humulene"             , PropertyNames::Hop::humulene_pct,          &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/caryophyllene"        , PropertyNames::Hop::caryophyllene_pct,     &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/cohumulone"           , PropertyNames::Hop::cohumulone_pct,        &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/myrcene"              , PropertyNames::Hop::myrcene_pct,           &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/farnesene"            , PropertyNames::Hop::farnesene_pct,         &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/geraniol"             , PropertyNames::Hop::geraniol_pct,          &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/b_pinene"             , PropertyNames::Hop::b_pinene_pct,          &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/linalool"             , PropertyNames::Hop::linalool_pct,          &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/limonene"             , PropertyNames::Hop::limonene_pct,          &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/nerol"                , PropertyNames::Hop::nerol_pct,             &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/pinene"               , PropertyNames::Hop::pinene_pct,            &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/polyphenols"          , PropertyNames::Hop::polyphenols_pct,       &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::SingleUnitValue           , "oil_content/xanthohumol"          , PropertyNames::Hop::xanthohumol_pct,       &BEER_JSON_PERCENT_UNIT},
      {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "inventory/amount"                 , PropertyNames::NamedEntityWithInventory::inventoryWithUnits, &BEER_JSON_MASS_OR_VOLUME_UNIT_MAPPER}, // .:TODO.JSON:. Extend Hop::amount_kg so we can cope with volumes for extract etc

      // .:TODO.JSON:. Note that we'll need to look at HopAdditionType, IBUEstimateType, IBUMethodType when we use Hops in Recipes
   };
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Hop> {
      "hop_varieties",
      &Hop::typeLookup,
      "Hop",
      JsonRecordDefinition::create< JsonNamedEntityRecord< Hop > >,
      {BeerJson_HopBase, BeerJson_HopType_ExclBase}
   };

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for cultures BeerJSON records - see schemas/beerjson/1.0/culture.json
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Yeast> {
      "cultures",
      &Yeast::typeLookup,
      "Yeast",
      JsonRecordDefinition::create< JsonNamedEntityRecord< Yeast > >,
      {
         // Type                                                       XPath                        Q_PROPERTY                                      Enum/Unit Mapper
         {JsonRecordDefinition::FieldType::String                    , "name"                     , PropertyNames::NamedEntity::name               },
         {JsonRecordDefinition::FieldType::Enum                      , "type"                     , PropertyNames::Yeast::type                     , &Yeast::typeStringMapping},
         {JsonRecordDefinition::FieldType::Enum                      , "form"                     , PropertyNames::Yeast::form                     , &Yeast::formStringMapping},
         {JsonRecordDefinition::FieldType::String                    , "producer"                 , PropertyNames::Yeast::laboratory               },
         {JsonRecordDefinition::FieldType::String                    , "product_id"               , PropertyNames::Yeast::productID                },
         {JsonRecordDefinition::FieldType::MeasurementWithUnits      , "temperature_range/minimum", PropertyNames::Yeast::minTemperature_c         , &BEER_JSON_TEMPERATURE_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::MeasurementWithUnits      , "temperature_range/maximum", PropertyNames::Yeast::maxTemperature_c         , &BEER_JSON_TEMPERATURE_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::SingleUnitValue           , "alcohol_tolerance"        , PropertyNames::Yeast::alcoholTolerance_pct     , &BEER_JSON_PERCENT_UNIT           },
         {JsonRecordDefinition::FieldType::Enum                      , "flocculation"             , PropertyNames::Yeast::flocculation             , &Yeast::flocculationStringMapping },
         {JsonRecordDefinition::FieldType::SingleUnitValue           , "attenuation_range/minimum", PropertyNames::Yeast::attenuationMin_pct       , &BEER_JSON_PERCENT_UNIT           },
         {JsonRecordDefinition::FieldType::SingleUnitValue           , "attenuation_range/maximum", PropertyNames::Yeast::attenuationMax_pct       , &BEER_JSON_PERCENT_UNIT           },
         {JsonRecordDefinition::FieldType::String                    , "notes"                    , PropertyNames::Yeast::notes                    },
         {JsonRecordDefinition::FieldType::String                    , "best_for"                 , PropertyNames::Yeast::bestFor                  },
         {JsonRecordDefinition::FieldType::Int                       , "max_reuse"                , PropertyNames::Yeast::maxReuse                 },
         {JsonRecordDefinition::FieldType::Bool                      , "pof"                      , PropertyNames::Yeast::phenolicOffFlavorPositive},
         {JsonRecordDefinition::FieldType::Bool                      , "glucoamylase"             , PropertyNames::Yeast::glucoamylasePositive     },
         {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "inventory/amount"         , PropertyNames::NamedEntityWithInventory::inventoryWithUnits, &BEER_JSON_MASS_OR_VOLUME_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::Bool                      , "zymocide/no1",              PropertyNames::Yeast::killerProducingK1Toxin   },
         {JsonRecordDefinition::FieldType::Bool                      , "zymocide/no2",              PropertyNames::Yeast::killerProducingK2Toxin   },
         {JsonRecordDefinition::FieldType::Bool                      , "zymocide/no28",             PropertyNames::Yeast::killerProducingK28Toxin  },
         {JsonRecordDefinition::FieldType::Bool                      , "zymocide/klus",             PropertyNames::Yeast::killerProducingKlusToxin },
         {JsonRecordDefinition::FieldType::Bool                      , "zymocide/neutral",          PropertyNames::Yeast::killerNeutral            },
         // Note that there is, AFAICT, no equivalent in BeerJSON to the following optional BeerXML properties:
         //  • Int:  TIMES_CULTURED   / PropertyNames::Yeast::timesCultured
         //  • Bool: ADD_TO_SECONDARY / PropertyNames::Yeast::addToSecondary
      }
   };

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for profiles BeerJSON records - see schemas/beerjson/1.0/water.json
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Water> {
      "profiles",
      &Water::typeLookup,
      "Water",
      JsonRecordDefinition::create< JsonNamedEntityRecord< Water > >,
      {
         // Type                                                       XPath             Q_PROPERTY                             Enum/Unit Mapper
         {JsonRecordDefinition::FieldType::String,                     "name",           PropertyNames::NamedEntity::name,      },
         {JsonRecordDefinition::FieldType::String,                     "producer",       BtString::NULL_STR,                    }, // Not sure what this means for water...
         {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "calcium",        PropertyNames::Water::calcium_ppm,     &BEER_JSON_CONCENTRATION_UNIT_MAPPER}, // .:TODO.JSON:. Extend water to allow mg/L on this field
         {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "bicarbonate",    PropertyNames::Water::bicarbonate_ppm, &BEER_JSON_CONCENTRATION_UNIT_MAPPER}, // .:TODO.JSON:. Extend water to allow mg/L on this field
         {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "potassium",      BtString::NULL_STR,                    &BEER_JSON_CONCENTRATION_UNIT_MAPPER}, // .:TODO.JSON:. Add this to Water
         {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "iron",           BtString::NULL_STR,                    &BEER_JSON_CONCENTRATION_UNIT_MAPPER}, // .:TODO.JSON:. Add this to Water
         {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "nitrate",        BtString::NULL_STR,                    &BEER_JSON_CONCENTRATION_UNIT_MAPPER}, // .:TODO.JSON:. Add this to Water
         {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "nitrite",        BtString::NULL_STR,                    &BEER_JSON_CONCENTRATION_UNIT_MAPPER}, // .:TODO.JSON:. Add this to Water
         {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "flouride",       BtString::NULL_STR,                    &BEER_JSON_CONCENTRATION_UNIT_MAPPER}, // .:TODO.JSON:. Add this to Water
         {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "sulfate",        PropertyNames::Water::sulfate_ppm,     &BEER_JSON_CONCENTRATION_UNIT_MAPPER}, // .:TODO.JSON:. Extend water to allow mg/L on this field
         {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "chloride",       PropertyNames::Water::chloride_ppm,    &BEER_JSON_CONCENTRATION_UNIT_MAPPER}, // .:TODO.JSON:. Extend water to allow mg/L on this field
         {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "sodium",         PropertyNames::Water::sodium_ppm,      &BEER_JSON_CONCENTRATION_UNIT_MAPPER}, // .:TODO.JSON:. Extend water to allow mg/L on this field
         {JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits, "magnesium",      PropertyNames::Water::magnesium_ppm,   &BEER_JSON_CONCENTRATION_UNIT_MAPPER}, // .:TODO.JSON:. Extend water to allow mg/L on this field
         {JsonRecordDefinition::FieldType::SingleUnitValue,            "ph",             PropertyNames::Water::ph,              &BEER_JSON_ACIDITY_UNIT},
         {JsonRecordDefinition::FieldType::String,                     "notes",          PropertyNames::Water::notes,           },

         // .:TODO.JSON:. Note that we'll need to look at WaterAdditionType at some point...
      }
   };

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for styles BeerJSON records - see schemas/beerjson/1.0/style.json TODO
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Style> {
      "styles",
      &Style::typeLookup,
      "Style",
      JsonRecordDefinition::create< JsonNamedEntityRecord< Style > >,
      {
         // Type                                                 XPath                                     Q_PROPERTY                               Enum/Unit Mapper
         {JsonRecordDefinition::FieldType::String,               "name",                                   PropertyNames::NamedEntity::name       },
         {JsonRecordDefinition::FieldType::String,               "category",                               PropertyNames::Style::category         },
         {JsonRecordDefinition::FieldType::Int,                  "category_number",                        PropertyNames::Style::categoryNumber   },
         {JsonRecordDefinition::FieldType::String,               "style_letter",                           PropertyNames::Style::styleLetter      },
         {JsonRecordDefinition::FieldType::String,               "style_guide",                            PropertyNames::Style::styleGuide       },
         {JsonRecordDefinition::FieldType::Enum,                 "type",                                   PropertyNames::Style::type             , &Style::typeStringMapping},
         {JsonRecordDefinition::FieldType::MeasurementWithUnits, "original_gravity/minimum",               PropertyNames::Style::ogMin            , &BEER_JSON_DENSITY_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::MeasurementWithUnits, "original_gravity/maximum",               PropertyNames::Style::ogMax            , &BEER_JSON_DENSITY_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::MeasurementWithUnits, "final_gravity/minimum",                  PropertyNames::Style::fgMin            , &BEER_JSON_DENSITY_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::MeasurementWithUnits, "final_gravity/maximum",                  PropertyNames::Style::fgMax            , &BEER_JSON_DENSITY_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::SingleUnitValue,      "international_bitterness_units/minimum", PropertyNames::Style::ibuMin           , &BEER_JSON_BITTERNESS_UNIT},
         {JsonRecordDefinition::FieldType::SingleUnitValue,      "international_bitterness_units/maximum", PropertyNames::Style::ibuMax           , &BEER_JSON_BITTERNESS_UNIT},
         {JsonRecordDefinition::FieldType::MeasurementWithUnits, "color/minimum",                          PropertyNames::Style::colorMin_srm     , &BEER_JSON_COLOR_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::MeasurementWithUnits, "color/maximum",                          PropertyNames::Style::colorMax_srm     , &BEER_JSON_COLOR_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::MeasurementWithUnits, "carbonation/minimum",                    PropertyNames::Style::carbMin_vol      , &BEER_JSON_CARBONATION_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::MeasurementWithUnits, "carbonation/maximum",                    PropertyNames::Style::carbMax_vol      , &BEER_JSON_CARBONATION_UNIT_MAPPER},
         {JsonRecordDefinition::FieldType::SingleUnitValue,      "alcohol_by_volume/minimum",              PropertyNames::Style::abvMin_pct       , &BEER_JSON_PERCENT_UNIT},
         {JsonRecordDefinition::FieldType::SingleUnitValue,      "alcohol_by_volume/maximum",              PropertyNames::Style::abvMax_pct       , &BEER_JSON_PERCENT_UNIT},
         {JsonRecordDefinition::FieldType::String,               "notes",                                  PropertyNames::Style::notes            },
         {JsonRecordDefinition::FieldType::String,               "aroma",                                  PropertyNames::Style::aroma            },
         {JsonRecordDefinition::FieldType::String,               "appearance",                             PropertyNames::Style::appearance       },
         {JsonRecordDefinition::FieldType::String,               "flavor",                                 PropertyNames::Style::flavor           },
         {JsonRecordDefinition::FieldType::String,               "mouthfeel",                              PropertyNames::Style::mouthfeel        },
         {JsonRecordDefinition::FieldType::String,               "overall_impression",                     PropertyNames::Style::overallImpression},
         {JsonRecordDefinition::FieldType::String,               "ingredients",                            PropertyNames::Style::ingredients      },
         {JsonRecordDefinition::FieldType::String,               "examples",                               PropertyNames::Style::examples         },
      }
   };

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for mashes BeerJSON records TODO
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      JsonRecordDefinition::create< JsonMashRecord >,

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for fermentations BeerJSON records TODO
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      JsonRecordDefinition::create< JsonNamedEntityRecord< Fermantation > >,

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for recipes BeerJSON records TODO
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      JsonRecordDefinition::create< JsonRecipRecord >,

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for equipments BeerJSON records TODO
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> JsonRecordDefinition const BEER_JSON_RECORD_DEFINITION<Equipment> {
      "equipments",              // JSON recordName
      &Equipment::typeLookup,    // Type Lookup for our corresponding model object
      "Equipment",               // NamedEntity class name
      JsonRecordDefinition::create< JsonNamedEntityRecord< Equipment > >,
      {
         //
         // In theory, to access the "Mash Tun" entry in an "equipment_items" array, we just use JSONPath equipment_items[?(@.form=="Mash Tun")]
         //

         // Type                                                 XPath                                     Q_PROPERTY                               Enum/Unit Mapper
///         {JsonRecordDefinition::FieldType::String,               "name",                                   PropertyNames::NamedEntity::name       },
         {JsonRecordDefinition::FieldType::String,               "test/fish[face=\"squid\"]/weasel",                                   PropertyNames::NamedEntity::name       },
///         {JsonRecordDefinition::FieldType::String,               "equipment_items"
      }
   };

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for boil BeerJSON records TODO
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      JsonRecordDefinition::create< JsonNamedEntityRecord< Boil > >,

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Field mappings for packaging BeerJSON records TODO
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      JsonRecordDefinition::create< JsonNamedEntityRecord< Packaging > >,

   //
   // The mapping we use between BeerJSON structure and our own object structure
   //
   JsonCoding const BEER_JSON_1_CODING{
      // Yes, it is odd that BeerJSON 1.0 uses version number 2.06.  AFAICT this is because BeerJSON 1.0 was took its
      // starting point as the unfinished BeerXML 2.01 specification.
      "BeerJSON 1.0",
      *jsonVersionWeSupport, // "2.06",
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
         BEER_JSON_RECORD_DEFINITION<Equipment>  ,
//         BEER_JSON_RECORD_DEFINITION<Instruction>,
//         BEER_JSON_RECORD_DEFINITION<BrewNote>   ,
//         BEER_JSON_RECORD_DEFINITION<Recipe>
      }
   };

   //=-=-=-=-=-=-=-=-

   /**
    * \brief This function first validates the input file against a JSON schema (https://json-schema.org/)
    */
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
      if (beerJsonVersion != jsonVersionWeSupport) {
         qWarning() <<
            Q_FUNC_INFO << "BeerJSON version " << beerJsonVersion << "differs from what we are expecting (" <<
            jsonVersionWeSupport << ")";
      }

      // If you want to check what Boost.JSON read from the file (eg to debug escaping issues etc), uncomment the next
      // line.
//      qDebug() << Q_FUNC_INFO << "JSON file read in is:" << inputDocument;

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

namespace BeerJson {
   //
   // This private implementation class holds all private non-virtual members of Exporter
   //
   class Exporter::impl {
   public:

      /**
      * Constructor
      */
      impl(Exporter & self,
         QFile & outFile,
         QTextStream & userMessage) : self{self},
                                      outFile{outFile},
                                      userMessage{userMessage},
                                      writtenToFile{false},
                                      outputDocument{} {
         // We have to pass in jsonVersionWeSupport as a double, not a char * or a std::string, otherwise it will get
         // quotes put around it.
         this->outputDocument["beerjson"] = { {"version", std::atof(*jsonVersionWeSupport)} };
         return;
      }

      /**
      * Destructor
      */
      ~impl() = default;

      Exporter & self;
      QFile & outFile;
      QTextStream & userMessage;
      bool writtenToFile;

      boost::json::object outputDocument;

   };

   Exporter::Exporter(QFile & outFile, QTextStream & userMessage) :
      pimpl{std::make_unique<impl>(*this, outFile, userMessage)} {
      return;
   }

   Exporter::~Exporter() {
      this->close();
      return;
   }

   template<class NE> void Exporter::add(QList<NE const *> const & nes) {
      boost::json::array outputArray;
      for (auto ne : nes) {
         // We need the containing entity to be a value of type object.  See comments on JsonRecord constructor in
         // json/JsonRecord.h for why we have to take care about object vs value.
         boost::json::value neJson(boost::json::object_kind); // Can't use braces on this constructor until Boost 1.81!

         std::unique_ptr<JsonRecord> jsonRecord{
            BEER_JSON_RECORD_DEFINITION<NE>.makeRecord(BEER_JSON_1_CODING, neJson)
         };
         jsonRecord->toJson(*ne);

         // boost::json::array::push_back() appends a copy of its argument to the array, so we don't have to worry that
         // neJson and jsonRecord are about to go out of scope
         outputArray.push_back(neJson);
      }
      this->pimpl->outputDocument["beerjson"].get_object()[*BEER_JSON_RECORD_DEFINITION<NE>.recordName] = outputArray;
      return;
   }

   //
   // Instantiate the above template function for the types that are going to use it
   // (This is all just a trick to allow the template definition to be here in the .cpp file and not in the header,
   // which means, amongst other things, that we can reference the pimpl.)
   //
   template void Exporter::add(QList<Hop         const *> const & nes);
   template void Exporter::add(QList<Fermentable const *> const & nes);
   template void Exporter::add(QList<Yeast       const *> const & nes);
   template void Exporter::add(QList<Misc        const *> const & nes);
   template void Exporter::add(QList<Water       const *> const & nes);
   template void Exporter::add(QList<Style       const *> const & nes);
   template void Exporter::add(QList<MashStep    const *> const & nes);
   template void Exporter::add(QList<Mash        const *> const & nes);
   template void Exporter::add(QList<Equipment   const *> const & nes);
   template void Exporter::add(QList<Instruction const *> const & nes);
   template void Exporter::add(QList<BrewNote    const *> const & nes);
   template void Exporter::add(QList<Recipe      const *> const & nes);

   void Exporter::close() {
      if (this->pimpl->writtenToFile) {
         return;
      }

      OStreamWriterForQFile outStream(this->pimpl->outFile);
      JsonUtils::serialize(outStream, this->pimpl->outputDocument, "  ");

      this->pimpl->writtenToFile = true;

      return;
   }

}
