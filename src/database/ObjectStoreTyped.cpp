/**
 * database/ObjectStoreTyped.cpp is part of Brewken, and is copyright the following authors 2021:
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
 */
#include "database/ObjectStoreTyped.h"

#include  <mutex> // for std::once_flag

#include "model/BrewNote.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Hop.h"
#include "model/Instruction.h"
#include "model/Inventory.h"
#include "model/Mash.h"
#include "model/MashStep.h"
#include "model/Misc.h"
#include "model/Recipe.h"
#include "model/Salt.h"
#include "model/Style.h"
#include "model/Water.h"
#include "model/Yeast.h"

// .:TODO:. Create tables
// .:TBD:. Do we care about foreign keys?
// .:TBD:. What about triggers?
// .:TBD:. What about inventory?
//         Currently fermentable_in_inventory has id + amount columns - and similar for all other things that have inventory
//         It's like having a nullable column on the main table EXCEPT the ID is FK  to inventory_id on the main table
//         Well, a bit more complicated actually.  Everything with the same ultimate parent shares the same inventory_id
// select id from hop where inventory_id = 45; => 45 149 151 153 155
// select child_id from hop_children where parent_id = 45; => 149 151 153 155
//
// .:TBD:. What about read-only fields, eg if we want an Instruction to pull its Recipe ID from instruction_in_recipe
//
// .:TBD:. At the moment, each table name is used pretty much once, but if that changes then we might want to add
//        constants along the following lines:
// namespace DatabaseNames::Tables { static char const * const brewnote            = "brewnote"; }
// plus something similar for column names

namespace {

   //
   // By the magic of templated variables and template specialisation, we have below all the constructor parameters for
   // each type of ObjectStoreTyped
   //
   template<class NE> ObjectStore::TableSimpleDefn const PRIMARY_TABLE;
   template<class NE> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS;


   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for BrewNote
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<BrewNote> {
      "brewnote",
      {
         {ObjectStore::FieldType::Int,    "id",                      PropertyNames::NamedEntity::key},
         // NB: BrewNotes don't have names in DB
         {ObjectStore::FieldType::Bool,   "display",                 PropertyNames::NamedEntity::display},
         {ObjectStore::FieldType::Bool,   "deleted",                 PropertyNames::NamedEntity::deleted},
         {ObjectStore::FieldType::String, "folder",                  PropertyNames::NamedEntity::folder},
         {ObjectStore::FieldType::Double, "abv",                     PropertyNames::BrewNote::abv},
         {ObjectStore::FieldType::Double, "attenuation",             PropertyNames::BrewNote::attenuation},
         {ObjectStore::FieldType::Double, "boil_off",                PropertyNames::BrewNote::boilOff_l},
         {ObjectStore::FieldType::Date,   "brewdate",                PropertyNames::BrewNote::brewDate},
         {ObjectStore::FieldType::Double, "brewhouse_eff",           PropertyNames::BrewNote::brewhouseEff_pct},
         {ObjectStore::FieldType::Double, "eff_into_bk",             PropertyNames::BrewNote::effIntoBK_pct},
         {ObjectStore::FieldType::Date,   "fermentdate",             PropertyNames::BrewNote::fermentDate},
         {ObjectStore::FieldType::Double, "fg",                      PropertyNames::BrewNote::fg},
         {ObjectStore::FieldType::Double, "final_volume",            PropertyNames::BrewNote::finalVolume_l},
         // NB: BrewNotes don't have folders, as each one is owned by a Recipe
         {ObjectStore::FieldType::Double, "mash_final_temp",         PropertyNames::BrewNote::mashFinTemp_c},
         {ObjectStore::FieldType::String, "notes",                   PropertyNames::BrewNote::notes},
         {ObjectStore::FieldType::Double, "og",                      PropertyNames::BrewNote::og},
         {ObjectStore::FieldType::Double, "pitch_temp",              PropertyNames::BrewNote::pitchTemp_c},
         {ObjectStore::FieldType::Double, "post_boil_volume",        PropertyNames::BrewNote::postBoilVolume_l},
         {ObjectStore::FieldType::Double, "projected_abv",           PropertyNames::BrewNote::projABV_pct},
         {ObjectStore::FieldType::Double, "projected_atten",         PropertyNames::BrewNote::projAtten},
         {ObjectStore::FieldType::Double, "projected_boil_grav",     PropertyNames::BrewNote::projBoilGrav},
         {ObjectStore::FieldType::Double, "projected_eff",           PropertyNames::BrewNote::projEff_pct},
         {ObjectStore::FieldType::Double, "projected_ferm_points",   PropertyNames::BrewNote::projFermPoints},
         {ObjectStore::FieldType::Double, "projected_fg",            PropertyNames::BrewNote::projFg},
         {ObjectStore::FieldType::Double, "projected_mash_fin_temp", PropertyNames::BrewNote::projMashFinTemp_c},
         {ObjectStore::FieldType::Double, "projected_og",            PropertyNames::BrewNote::projOg},
         {ObjectStore::FieldType::Double, "projected_points",        PropertyNames::BrewNote::projPoints},
         {ObjectStore::FieldType::Double, "projected_strike_temp",   PropertyNames::BrewNote::projStrikeTemp_c},
         {ObjectStore::FieldType::Double, "projected_vol_into_bk",   PropertyNames::BrewNote::projVolIntoBK_l},
         {ObjectStore::FieldType::Double, "projected_vol_into_ferm", PropertyNames::BrewNote::projVolIntoFerm_l},
         {ObjectStore::FieldType::Double, "sg",                      PropertyNames::BrewNote::sg},
         {ObjectStore::FieldType::Double, "strike_temp",             PropertyNames::BrewNote::strikeTemp_c},
         {ObjectStore::FieldType::Double, "volume_into_bk",          PropertyNames::BrewNote::volumeIntoBK_l},
         {ObjectStore::FieldType::Double, "volume_into_fermenter",   PropertyNames::BrewNote::volumeIntoFerm_l},
         {ObjectStore::FieldType::Int   , "recipe_id",               PropertyNames::BrewNote::recipeId}
      }
   };
   // BrewNotes don't have children
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<BrewNote> {};

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Equipment
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<Equipment> {
      "equipment",
      {
         {ObjectStore::FieldType::Int,    "id",                PropertyNames::NamedEntity::key},
         {ObjectStore::FieldType::String, "name",              PropertyNames::NamedEntity::name},
         {ObjectStore::FieldType::Bool,   "display",           PropertyNames::NamedEntity::display},
         {ObjectStore::FieldType::Bool,   "deleted",           PropertyNames::NamedEntity::deleted},
         {ObjectStore::FieldType::String, "folder",            PropertyNames::NamedEntity::folder},
         {ObjectStore::FieldType::Double, "batch_size",        PropertyNames::Equipment::batchSize_l},
         {ObjectStore::FieldType::Double, "boiling_point",     PropertyNames::Equipment::boilingPoint_c},
         {ObjectStore::FieldType::Double, "boil_size",         PropertyNames::Equipment::boilSize_l},
         {ObjectStore::FieldType::Double, "boil_time",         PropertyNames::Equipment::boilTime_min},
         {ObjectStore::FieldType::Bool,   "calc_boil_volume",  PropertyNames::Equipment::calcBoilVolume},
         {ObjectStore::FieldType::Double, "real_evap_rate",    PropertyNames::Equipment::evapRate_lHr},
         {ObjectStore::FieldType::Double, "evap_rate",         PropertyNames::Equipment::evapRate_pctHr},
         {ObjectStore::FieldType::Double, "absorption",        PropertyNames::Equipment::grainAbsorption_LKg},
         {ObjectStore::FieldType::Double, "hop_utilization",   PropertyNames::Equipment::hopUtilization_pct},
         {ObjectStore::FieldType::Double, "lauter_deadspace",  PropertyNames::Equipment::lauterDeadspace_l},
         {ObjectStore::FieldType::String, "notes",             PropertyNames::Equipment::notes},
         {ObjectStore::FieldType::Double, "top_up_kettle",     PropertyNames::Equipment::topUpKettle_l},
         {ObjectStore::FieldType::Double, "top_up_water",      PropertyNames::Equipment::topUpWater_l},
         {ObjectStore::FieldType::Double, "trub_chiller_loss", PropertyNames::Equipment::trubChillerLoss_l},
         {ObjectStore::FieldType::Double, "tun_specific_heat", PropertyNames::Equipment::tunSpecificHeat_calGC},
         {ObjectStore::FieldType::Double, "tun_volume",        PropertyNames::Equipment::tunVolume_l},
         {ObjectStore::FieldType::Double, "tun_weight",        PropertyNames::Equipment::tunWeight_kg}
      }
   };
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<Equipment> {
      // Objects store their parents not their children, so this view of the junction table is from the child's point of view
      {"equipment_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, ObjectStore::MAX_ONE_ENTRY}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Fermentable
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   ObjectStore::EnumStringMapping const DB_FERMENTABLE_TYPE_ENUM {
      {"Grain",       Fermentable::Grain},
      {"Sugar",       Fermentable::Sugar},
      {"Extract",     Fermentable::Extract},
      {"Dry Extract", Fermentable::Dry_Extract},
      {"Adjunct",     Fermentable::Adjunct}
   };
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<Fermentable> {
      "fermentable",
      {
         {ObjectStore::FieldType::Int,    "id",               PropertyNames::NamedEntity::key},
         {ObjectStore::FieldType::String, "name",             PropertyNames::NamedEntity::name},
         {ObjectStore::FieldType::Bool,   "deleted",          PropertyNames::NamedEntity::deleted},
         {ObjectStore::FieldType::Bool,   "display",          PropertyNames::NamedEntity::display},
         {ObjectStore::FieldType::String, "folder",           PropertyNames::NamedEntity::folder},
         {ObjectStore::FieldType::Bool,   "add_after_boil",   PropertyNames::Fermentable::addAfterBoil},
         {ObjectStore::FieldType::Double, "amount",           PropertyNames::Fermentable::amount_kg},
         {ObjectStore::FieldType::Double, "coarse_fine_diff", PropertyNames::Fermentable::coarseFineDiff_pct},
         {ObjectStore::FieldType::Double, "color",            PropertyNames::Fermentable::color_srm},
         {ObjectStore::FieldType::Double, "diastatic_power",  PropertyNames::Fermentable::diastaticPower_lintner},
         {ObjectStore::FieldType::Enum,   "ftype",            PropertyNames::Fermentable::type,                   &DB_FERMENTABLE_TYPE_ENUM},
         {ObjectStore::FieldType::Bool,   "is_mashed",        PropertyNames::Fermentable::isMashed},
         {ObjectStore::FieldType::Double, "ibu_gal_per_lb",   PropertyNames::Fermentable::ibuGalPerLb},
         {ObjectStore::FieldType::Double, "max_in_batch",     PropertyNames::Fermentable::maxInBatch_pct},
         {ObjectStore::FieldType::Double, "moisture",         PropertyNames::Fermentable::moisture_pct},
         {ObjectStore::FieldType::String, "notes",            PropertyNames::Fermentable::notes},
         {ObjectStore::FieldType::String, "origin",           PropertyNames::Fermentable::origin},
         {ObjectStore::FieldType::String, "supplier",         PropertyNames::Fermentable::supplier},
         {ObjectStore::FieldType::Double, "protein",          PropertyNames::Fermentable::protein_pct},
         {ObjectStore::FieldType::Bool,   "recommend_mash",   PropertyNames::Fermentable::recommendMash},
         {ObjectStore::FieldType::Double, "yield",            PropertyNames::Fermentable::yield_pct},
         {ObjectStore::FieldType::Int,    "inventory_id",     PropertyNames::Fermentable::inventoryId} /// inventory_id REFERENCES fermentable_in_inventory (id))      <<< TODO
      }
   };
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<Fermentable> {
      {"fermentable_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, ObjectStore::MAX_ONE_ENTRY}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for InventoryFermentable
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<InventoryFermentable> {
      "fermentable_in_inventory",
      {
         {ObjectStore::FieldType::Int,    "id",               PropertyNames::Inventory::id},
         {ObjectStore::FieldType::Double, "amount",           PropertyNames::Inventory::amount}
      }
   };
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<InventoryFermentable> {};

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Hop TODO Check the strings!
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   ObjectStore::EnumStringMapping const DB_HOP_USE_ENUM {
      {"Boil",       Hop::Boil},
      {"Dry Hop",    Hop::Dry_Hop},
      {"Mash",       Hop::Mash},
      {"First Wort", Hop::First_Wort},
      {"Aroma",      Hop::UseAroma}
   };
   ObjectStore::EnumStringMapping const DB_HOP_TYPE_ENUM {
      {"Bittering", Hop::Bittering},
      {"Aroma",     Hop::Aroma},
      {"Both",      Hop::Both}
   };
   ObjectStore::EnumStringMapping const DB_HOP_FORM_ENUM {
      {"Pellet", Hop::Pellet},
      {"Plug",   Hop::Plug},
      {"Leaf",   Hop::Leaf}
   };
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<Hop> {
      "hop",
      {
         {ObjectStore::FieldType::Int,    "id",            PropertyNames::NamedEntity::key},
         {ObjectStore::FieldType::String, "name",          PropertyNames::NamedEntity::name},
         {ObjectStore::FieldType::Bool,   "display",       PropertyNames::NamedEntity::display},
         {ObjectStore::FieldType::Bool,   "deleted",       PropertyNames::NamedEntity::deleted},
         {ObjectStore::FieldType::String, "folder",        PropertyNames::NamedEntity::folder},
         {ObjectStore::FieldType::Double, "alpha",         PropertyNames::Hop::alpha_pct},
         {ObjectStore::FieldType::Double, "amount",        PropertyNames::Hop::amount_kg},
         {ObjectStore::FieldType::Double, "beta",          PropertyNames::Hop::beta_pct},
         {ObjectStore::FieldType::Double, "caryophyllene", PropertyNames::Hop::caryophyllene_pct},
         {ObjectStore::FieldType::Double, "cohumulone",    PropertyNames::Hop::cohumulone_pct},
         {ObjectStore::FieldType::Enum,   "form",          PropertyNames::Hop::form,              &DB_HOP_FORM_ENUM},
         {ObjectStore::FieldType::Double, "hsi",           PropertyNames::Hop::hsi_pct},
         {ObjectStore::FieldType::Double, "humulene",      PropertyNames::Hop::humulene_pct},
         {ObjectStore::FieldType::Int,    "inventory_id",  PropertyNames::Hop::inventoryId},
         {ObjectStore::FieldType::Double, "myrcene",       PropertyNames::Hop::myrcene_pct},
         {ObjectStore::FieldType::String, "notes",         PropertyNames::Hop::notes},
         {ObjectStore::FieldType::String, "origin",        PropertyNames::Hop::origin},
         {ObjectStore::FieldType::String, "substitutes",   PropertyNames::Hop::substitutes},
         {ObjectStore::FieldType::Double, "time",          PropertyNames::Hop::time_min},
         {ObjectStore::FieldType::Enum,   "htype",         PropertyNames::Hop::type,              &DB_HOP_TYPE_ENUM},
         {ObjectStore::FieldType::Enum,   "use",           PropertyNames::Hop::use,               &DB_HOP_USE_ENUM}
      }
   };
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<Hop> {
      {"hop_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, ObjectStore::MAX_ONE_ENTRY}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for InventoryHop
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<InventoryHop> {
      "hop_in_inventory",
      {
         {ObjectStore::FieldType::Int,    "id",               PropertyNames::Inventory::id},
         {ObjectStore::FieldType::Double, "amount",           PropertyNames::Inventory::amount}
      }
   };
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<InventoryHop> {};

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Instruction
   // NB: instructions aren't displayed in trees, and get no folder
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<Instruction> {
      "instruction",
      {
         {ObjectStore::FieldType::Int,    "id",         PropertyNames::NamedEntity::key},
         {ObjectStore::FieldType::String, "name",       PropertyNames::NamedEntity::name},
         {ObjectStore::FieldType::Bool,   "display",    PropertyNames::NamedEntity::display},
         {ObjectStore::FieldType::Bool,   "deleted",    PropertyNames::NamedEntity::deleted},
         {ObjectStore::FieldType::String, "directions", PropertyNames::Instruction::directions },
         {ObjectStore::FieldType::Bool,   "hasTimer",   PropertyNames::Instruction::hasTimer   },
         {ObjectStore::FieldType::String, "timervalue", PropertyNames::Instruction::timerValue },
         {ObjectStore::FieldType::Bool,   "completed",  PropertyNames::Instruction::completed  },
         {ObjectStore::FieldType::Double, "interval",   PropertyNames::Instruction::interval   }
      }
   };
   // Instructions don't have children
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<Instruction> {};

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Mash
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<Mash> {
      "mash",
      {
         {ObjectStore::FieldType::Int,    "id",                PropertyNames::NamedEntity::key},
         {ObjectStore::FieldType::String, "name",              PropertyNames::NamedEntity::name},
         {ObjectStore::FieldType::Bool,   "deleted",           PropertyNames::NamedEntity::deleted},
         {ObjectStore::FieldType::Bool,   "display",           PropertyNames::NamedEntity::display},
         {ObjectStore::FieldType::String, "folder",            PropertyNames::NamedEntity::folder},
         {ObjectStore::FieldType::Bool,   "equip_adjust",      PropertyNames::Mash::equipAdjust},
         {ObjectStore::FieldType::Double, "grain_temp",        PropertyNames::Mash::grainTemp_c},
         {ObjectStore::FieldType::String, "notes",             PropertyNames::Mash::notes},
         {ObjectStore::FieldType::Double, "ph",                PropertyNames::Mash::ph},
         {ObjectStore::FieldType::Double, "sparge_temp",       PropertyNames::Mash::spargeTemp_c},
         {ObjectStore::FieldType::Double, "tun_specific_heat", PropertyNames::Mash::tunSpecificHeat_calGC},
         {ObjectStore::FieldType::Double, "tun_temp",          PropertyNames::Mash::tunTemp_c},
         {ObjectStore::FieldType::Double, "tun_weight",        PropertyNames::Mash::tunWeight_kg},
      }
   };
   // Mashes don't have children, and the link with their MashSteps is stored in the MashStep (as between Recipe and BrewNotes)
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<Mash> {};

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for MashStep
   // NB: MashSteps don't get folders, because they don't separate from their Mash
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   ObjectStore::EnumStringMapping const MASH_STEP_TYPE_ENUM {
      {"Infusion",     MashStep::Infusion},
      {"Temperature",  MashStep::Temperature},
      {"Decoction",    MashStep::Decoction},
      {"FlySparge",    MashStep::flySparge},
      {"BatchSparge",  MashStep::batchSparge}
   };
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<MashStep> {
      "mashstep",
      {
         {ObjectStore::FieldType::Int,    "id",                PropertyNames::NamedEntity::key            },
         {ObjectStore::FieldType::String, "name",              PropertyNames::NamedEntity::name           },
         {ObjectStore::FieldType::Bool,   "deleted",           PropertyNames::NamedEntity::deleted        },
         {ObjectStore::FieldType::Bool,   "display",           PropertyNames::NamedEntity::display        },
         // NB: MashSteps don't have folders, as each one is owned by a Mash
         {ObjectStore::FieldType::Double,  "decoction_amount", PropertyNames::MashStep::decoctionAmount_l },
         {ObjectStore::FieldType::Double,  "end_temp",         PropertyNames::MashStep::endTemp_c         },
         {ObjectStore::FieldType::Double,  "infuse_amount",    PropertyNames::MashStep::infuseAmount_l    },
         {ObjectStore::FieldType::Double,  "infuse_temp",      PropertyNames::MashStep::infuseTemp_c      },
         {ObjectStore::FieldType::Int,     "mash_id",          PropertyNames::MashStep::mashId            },
         {ObjectStore::FieldType::Enum,    "mstype",           PropertyNames::MashStep::type,             &MASH_STEP_TYPE_ENUM},
         {ObjectStore::FieldType::Double,  "ramp_time",        PropertyNames::MashStep::rampTime_min      },
         {ObjectStore::FieldType::Int,     "step_number",      PropertyNames::MashStep::stepNumber        },
         {ObjectStore::FieldType::Double,  "step_temp",        PropertyNames::MashStep::stepTemp_c        },
         {ObjectStore::FieldType::Double,  "step_time",        PropertyNames::MashStep::stepTime_min      }
      }
   };
   // MashSteps don't have children
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<MashStep> {};

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Misc
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   ObjectStore::EnumStringMapping const MISC_TYPE_ENUM {
      {"Spice",       Misc::Spice},
      {"Fining",      Misc::Fining},
      {"Water Agent", Misc::Water_Agent},
      {"Herb",        Misc::Herb},
      {"Flavor",      Misc::Flavor},
      {"Other",       Misc::Other}
   };
   ObjectStore::EnumStringMapping const MISC_USE_ENUM {
      {"Boil",      Misc::Boil},
      {"Mash",      Misc::Mash},
      {"Primary",   Misc::Primary},
      {"Secondary", Misc::Secondary},
      {"Bottling",  Misc::Bottling}
   };
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<Misc> {
      "misc",
      {
         {ObjectStore::FieldType::Int,    "id",               PropertyNames::NamedEntity::key},
         {ObjectStore::FieldType::String, "name",             PropertyNames::NamedEntity::name},
         {ObjectStore::FieldType::Bool,   "deleted",          PropertyNames::NamedEntity::deleted},
         {ObjectStore::FieldType::Bool,   "display",          PropertyNames::NamedEntity::display},
         {ObjectStore::FieldType::String, "folder",           PropertyNames::NamedEntity::folder},
         {ObjectStore::FieldType::Enum,   "mtype",            PropertyNames::Misc::type,           &MISC_TYPE_ENUM},
         {ObjectStore::FieldType::Enum,   "use",              PropertyNames::Misc::use,            &MISC_USE_ENUM},
         {ObjectStore::FieldType::Double, "time",             PropertyNames::Misc::time           },
         {ObjectStore::FieldType::Double, "amount",           PropertyNames::Misc::amount         },
         {ObjectStore::FieldType::Bool,   "amount_is_weight", PropertyNames::Misc::amountIsWeight },
         {ObjectStore::FieldType::String, "use_for",          PropertyNames::Misc::useFor         },
         {ObjectStore::FieldType::String, "notes",            PropertyNames::Misc::notes          },
         {ObjectStore::FieldType::Int,    "inventory_id",     PropertyNames::Misc::inventoryId    }
      }
   };
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<Misc> {
      {"misc_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, ObjectStore::MAX_ONE_ENTRY}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for InventoryMisc
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<InventoryMisc> {
      "misc_in_inventory",
      {
         {ObjectStore::FieldType::Int,    "id",               PropertyNames::Inventory::id},
         {ObjectStore::FieldType::Double, "amount",           PropertyNames::Inventory::amount}
      }
   };
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<InventoryMisc> {};

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Recipe
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   ObjectStore::EnumStringMapping const RECIPE_STEP_TYPE_ENUM {
      {"Extract",      Recipe::Extract},
      {"Partial Mash", Recipe::PartialMash},
      {"All Grain",    Recipe::AllGrain}
   };
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<Recipe> {
      "recipe",
      {
         {ObjectStore::FieldType::Int,    "id",                PropertyNames::NamedEntity::key},
         {ObjectStore::FieldType::String, "name",              PropertyNames::NamedEntity::name},
         {ObjectStore::FieldType::Bool,   "deleted",           PropertyNames::NamedEntity::deleted},
         {ObjectStore::FieldType::Bool,   "display",           PropertyNames::NamedEntity::display},
         {ObjectStore::FieldType::String, "folder",            PropertyNames::NamedEntity::folder},
         {ObjectStore::FieldType::Double, "age",                 PropertyNames::Recipe::age,                },
         {ObjectStore::FieldType::Double, "age_temp",            PropertyNames::Recipe::ageTemp_c,          },
         {ObjectStore::FieldType::String, "assistant_brewer",    PropertyNames::Recipe::asstBrewer,         },
         {ObjectStore::FieldType::Double, "batch_size",          PropertyNames::Recipe::batchSize_l,        },
         {ObjectStore::FieldType::Double, "boil_size",           PropertyNames::Recipe::boilSize_l,         },
         {ObjectStore::FieldType::Double, "boil_time",           PropertyNames::Recipe::boilTime_min,       },
         {ObjectStore::FieldType::String, "brewer",              PropertyNames::Recipe::brewer,             },
         {ObjectStore::FieldType::Double, "carb_volume",         PropertyNames::Recipe::carbonation_vols,   },
         {ObjectStore::FieldType::Double, "carbonationtemp_c",   PropertyNames::Recipe::carbonationTemp_c,  },
         {ObjectStore::FieldType::Date,   "date",                PropertyNames::Recipe::date,               },
         {ObjectStore::FieldType::Double, "efficiency",          PropertyNames::Recipe::efficiency_pct,     },
         {ObjectStore::FieldType::Int,    "equipment_id",        PropertyNames::Recipe::equipmentId,        },
         {ObjectStore::FieldType::UInt,   "fermentation_stages", PropertyNames::Recipe::fermentationStages, },
         {ObjectStore::FieldType::Double, "fg",                  PropertyNames::Recipe::fg,                 },
         {ObjectStore::FieldType::Bool,   "forced_carb",         PropertyNames::Recipe::forcedCarbonation,  },
         {ObjectStore::FieldType::Double, "keg_priming_factor",  PropertyNames::Recipe::kegPrimingFactor,   },
         {ObjectStore::FieldType::Int,    "mash_id",             PropertyNames::Recipe::mashId,             },
         {ObjectStore::FieldType::String, "notes",               PropertyNames::Recipe::notes,              },
         {ObjectStore::FieldType::Double, "og",                  PropertyNames::Recipe::og,                 },
         {ObjectStore::FieldType::Double, "primary_age",         PropertyNames::Recipe::primaryAge_days,    },
         {ObjectStore::FieldType::Double, "primary_temp",        PropertyNames::Recipe::primaryTemp_c,      },
         {ObjectStore::FieldType::Double, "priming_sugar_equiv", PropertyNames::Recipe::primingSugarEquiv,  },
         {ObjectStore::FieldType::String, "priming_sugar_name",  PropertyNames::Recipe::primingSugarName,   },
         {ObjectStore::FieldType::Double, "secondary_age",       PropertyNames::Recipe::secondaryAge_days,  },
         {ObjectStore::FieldType::Double, "secondary_temp",      PropertyNames::Recipe::secondaryTemp_c,    },
         {ObjectStore::FieldType::Int,    "style_id",            PropertyNames::Recipe::styleId,            },
         {ObjectStore::FieldType::String, "taste_notes",         PropertyNames::Recipe::tasteNotes,         },
         {ObjectStore::FieldType::Double, "taste_rating",        PropertyNames::Recipe::tasteRating,        },
         {ObjectStore::FieldType::Double, "tertiary_age",        PropertyNames::Recipe::tertiaryAge_days,   },
         {ObjectStore::FieldType::Double, "tertiary_temp",       PropertyNames::Recipe::tertiaryTemp_c,     },
         {ObjectStore::FieldType::Enum,   "type",                PropertyNames::Recipe::recipeType,         &RECIPE_STEP_TYPE_ENUM},
      }
   };
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<Recipe> {
      // .:TODO:. BrewNote table stores its recipe ID, so there isn't a brewnote junction table
      {"fermentable_in_recipe", "recipe_id", "fermentable_id", PropertyNames::Recipe::fermentableIds},
      {"hop_in_recipe",         "recipe_id", "hop_id",         PropertyNames::Recipe::hopIds},
      {"instruction_in_recipe", "recipe_id", "instruction_id", PropertyNames::Recipe::instructionIds, ObjectStore::MULTIPLE_ENTRIES_OK, "instruction_number"},
      {"misc_in_recipe",        "recipe_id", "misc_id",        PropertyNames::Recipe::miscIds},
      {"salt_in_recipe",        "recipe_id", "salt_id",        PropertyNames::Recipe::saltIds},
      {"water_in_recipe",       "recipe_id", "water_id",       PropertyNames::Recipe::waterIds},
      {"yeast_in_recipe",       "recipe_id", "yeast_id",       PropertyNames::Recipe::yeastIds}

   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Salt
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<Salt> {
      "salt",
      {
         {ObjectStore::FieldType::Int,    "id",               PropertyNames::NamedEntity::key},
         {ObjectStore::FieldType::String, "name",             PropertyNames::NamedEntity::name},
         {ObjectStore::FieldType::Bool,   "deleted",          PropertyNames::NamedEntity::deleted},
         {ObjectStore::FieldType::Bool,   "display",          PropertyNames::NamedEntity::display},
         {ObjectStore::FieldType::String, "folder",           PropertyNames::NamedEntity::folder},
         {ObjectStore::FieldType::Int,    "addTo",            PropertyNames::Salt::addTo          }, // TODO: Really an Enum.  Would be less fragile to store this as text than a number
         {ObjectStore::FieldType::Double, "amount",           PropertyNames::Salt::amount         },
         {ObjectStore::FieldType::Bool,   "amount_is_weight", PropertyNames::Salt::amountIsWeight },
         {ObjectStore::FieldType::Bool,   "is_acid",          PropertyNames::Salt::isAcid         },
         {ObjectStore::FieldType::Double, "percent_acid",     PropertyNames::Salt::percentAcid    },
         {ObjectStore::FieldType::Int,    "stype",            PropertyNames::Salt::type           }    // TODO: Really an Enum.  Would be less fragile to store this as text than a number
      }
   };
   // Salts don't have children
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<Salt> {};

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Style
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   ObjectStore::EnumStringMapping const STYLE_TYPE_ENUM {
      {"Lager", Style::Lager},
      {"Ale",   Style::Ale},
      {"Mead",  Style::Mead},
      {"Wheat", Style::Wheat},
      {"Mixed", Style::Mixed},
      {"Cider", Style::Cider}
   };
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<Style> {
      "style",
      {
         {ObjectStore::FieldType::Int,    "id",              PropertyNames::NamedEntity::key},
         {ObjectStore::FieldType::String, "name",            PropertyNames::NamedEntity::name},
         {ObjectStore::FieldType::Bool,   "display",         PropertyNames::NamedEntity::display},
         {ObjectStore::FieldType::Bool,   "deleted",         PropertyNames::NamedEntity::deleted},
         {ObjectStore::FieldType::String, "folder",          PropertyNames::NamedEntity::folder},
         {ObjectStore::FieldType::Double, "abv_max",         PropertyNames::Style::abvMax_pct},
         {ObjectStore::FieldType::Double, "abv_min",         PropertyNames::Style::abvMin_pct},
         {ObjectStore::FieldType::Double, "carb_max",        PropertyNames::Style::carbMax_vol},
         {ObjectStore::FieldType::Double, "carb_min",        PropertyNames::Style::carbMin_vol},
         {ObjectStore::FieldType::String, "category",        PropertyNames::Style::category},
         {ObjectStore::FieldType::String, "category_number", PropertyNames::Style::categoryNumber},
         {ObjectStore::FieldType::Double, "color_max",       PropertyNames::Style::colorMax_srm},
         {ObjectStore::FieldType::Double, "color_min",       PropertyNames::Style::colorMin_srm},
         {ObjectStore::FieldType::String, "examples",        PropertyNames::Style::examples},
         {ObjectStore::FieldType::Double, "fg_max",          PropertyNames::Style::fgMax},
         {ObjectStore::FieldType::Double, "fg_min",          PropertyNames::Style::fgMin},
         {ObjectStore::FieldType::Double, "ibu_max",         PropertyNames::Style::ibuMax},
         {ObjectStore::FieldType::Double, "ibu_min",         PropertyNames::Style::ibuMin},
         {ObjectStore::FieldType::String, "ingredients",     PropertyNames::Style::ingredients},
         {ObjectStore::FieldType::String, "notes",           PropertyNames::Style::notes},
         {ObjectStore::FieldType::Double, "og_max",          PropertyNames::Style::ogMax},
         {ObjectStore::FieldType::Double, "og_min",          PropertyNames::Style::ogMin},
         {ObjectStore::FieldType::String, "profile",         PropertyNames::Style::profile},
         {ObjectStore::FieldType::String, "style_guide",     PropertyNames::Style::styleGuide},
         {ObjectStore::FieldType::String, "style_letter",    PropertyNames::Style::styleLetter},
         {ObjectStore::FieldType::Enum,   "s_type",          PropertyNames::Style::type,           &STYLE_TYPE_ENUM}
      }
   };
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<Style> {
      {"style_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, ObjectStore::MAX_ONE_ENTRY}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Water
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<Water> {
      "water",
      {
         {ObjectStore::FieldType::Int,    "id",          PropertyNames::NamedEntity::key},
         {ObjectStore::FieldType::String, "name",        PropertyNames::NamedEntity::name},
         {ObjectStore::FieldType::Bool,   "display",     PropertyNames::NamedEntity::display},
         {ObjectStore::FieldType::Bool,   "deleted",     PropertyNames::NamedEntity::deleted},
         {ObjectStore::FieldType::String, "folder",      PropertyNames::NamedEntity::folder},
         {ObjectStore::FieldType::String, "notes",       PropertyNames::Water::notes},
         {ObjectStore::FieldType::Double, "amount",      PropertyNames::Water::amount},
         {ObjectStore::FieldType::Double, "calcium",     PropertyNames::Water::calcium_ppm},
         {ObjectStore::FieldType::Double, "bicarbonate", PropertyNames::Water::bicarbonate_ppm},
         {ObjectStore::FieldType::Double, "sulfate",     PropertyNames::Water::sulfate_ppm},
         {ObjectStore::FieldType::Double, "sodium",      PropertyNames::Water::sodium_ppm},
         {ObjectStore::FieldType::Double, "chloride",    PropertyNames::Water::chloride_ppm},
         {ObjectStore::FieldType::Double, "magnesium",   PropertyNames::Water::magnesium_ppm},
         {ObjectStore::FieldType::Double, "ph",          PropertyNames::Water::ph},
         {ObjectStore::FieldType::Double, "alkalinity",  PropertyNames::Water::alkalinity},
         {ObjectStore::FieldType::Int,    "wtype",       PropertyNames::Water::type},             // TODO: Would be less fragile to store this as text than a number
         {ObjectStore::FieldType::Double, "mash_ro",     PropertyNames::Water::mashRO},
         {ObjectStore::FieldType::Double, "sparge_ro",   PropertyNames::Water::spargeRO},
         {ObjectStore::FieldType::Bool,   "as_hco3",     PropertyNames::Water::alkalinityAsHCO3}
      }
   };
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<Water> {
      {"water_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, ObjectStore::MAX_ONE_ENTRY}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Yeast
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   ObjectStore::EnumStringMapping const DB_YEAST_TYPE_ENUM {
      {"Ale",       Yeast::Ale},
      {"Lager",     Yeast::Lager},
      {"Wheat",     Yeast::Wheat},
      {"Wine",      Yeast::Wine},
      {"Champagne", Yeast::Champagne}
   };
   ObjectStore::EnumStringMapping const DB_YEAST_FORM_ENUM {
      {"Liquid",  Yeast::Liquid},
      {"Dry",     Yeast::Dry},
      {"Slant",   Yeast::Slant},
      {"Culture", Yeast::Culture}
   };
   ObjectStore::EnumStringMapping const DB_YEAST_FLOCCULATION_ENUM {
      {"Low",       Yeast::Low},
      {"Medium",    Yeast::Medium},
      {"High",      Yeast::High},
      {"Very High", Yeast::Very_High}
   };
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<Yeast> {
      "yeast",
      {
         {ObjectStore::FieldType::Int,    "id",          PropertyNames::NamedEntity::key},
         {ObjectStore::FieldType::String, "name",        PropertyNames::NamedEntity::name},
         {ObjectStore::FieldType::Bool,   "display",     PropertyNames::NamedEntity::display},
         {ObjectStore::FieldType::Bool,   "deleted",     PropertyNames::NamedEntity::deleted},
         {ObjectStore::FieldType::String, "folder",      PropertyNames::NamedEntity::folder},
         {ObjectStore::FieldType::Bool,   "add_to_secondary", PropertyNames::Yeast::addToSecondary},
         {ObjectStore::FieldType::Bool,   "amount_is_weight", PropertyNames::Yeast::amountIsWeight},
         {ObjectStore::FieldType::Double, "amount",           PropertyNames::Yeast::amount},
         {ObjectStore::FieldType::Double, "attenuation",      PropertyNames::Yeast::attenuation_pct},
         {ObjectStore::FieldType::Double, "max_temperature",  PropertyNames::Yeast::maxTemperature_c},
         {ObjectStore::FieldType::Double, "min_temperature",  PropertyNames::Yeast::minTemperature_c},
         {ObjectStore::FieldType::Enum,   "flocculation",     PropertyNames::Yeast::flocculation,   &DB_YEAST_FLOCCULATION_ENUM},
         {ObjectStore::FieldType::Enum,   "form",             PropertyNames::Yeast::form,           &DB_YEAST_FORM_ENUM},
         {ObjectStore::FieldType::Enum,   "ytype",            PropertyNames::Yeast::type,           &DB_YEAST_TYPE_ENUM},
         {ObjectStore::FieldType::Int,    "max_reuse",        PropertyNames::Yeast::maxReuse},
         {ObjectStore::FieldType::Int,    "times_cultured",   PropertyNames::Yeast::timesCultured},
         {ObjectStore::FieldType::String, "best_for",         PropertyNames::Yeast::bestFor},
         {ObjectStore::FieldType::String, "laboratory",       PropertyNames::Yeast::laboratory},
         {ObjectStore::FieldType::String, "notes",            PropertyNames::Yeast::notes},
         {ObjectStore::FieldType::String, "product_id",       PropertyNames::Yeast::productID},
         {ObjectStore::FieldType::Int,    "inventory_id",     PropertyNames::Yeast::inventoryId}
      }
   };
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<Yeast> {
      {"yeast_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, ObjectStore::MAX_ONE_ENTRY}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for InventoryYeast
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   template<> ObjectStore::TableSimpleDefn const PRIMARY_TABLE<InventoryYeast> {
      "yeast_in_inventory",
      {
         {ObjectStore::FieldType::Int,    "id",               PropertyNames::Inventory::id},
         // Yeast inventory amount is called quanta, which I find hard to understand
         {ObjectStore::FieldType::Double, "quanta",           PropertyNames::Inventory::amount}
      }
   };
   template<> ObjectStore::FieldManyToManyDefns const MULTI_FIELDS<InventoryYeast> {};

}


template<class NE>
ObjectStoreTyped<NE> & ObjectStoreTyped<NE>::getInstance() {
   static ObjectStoreTyped<NE> singleton{PRIMARY_TABLE<NE>, MULTI_FIELDS<NE>};

   // C++11 provides a thread-safe way to ensure singleton.loadAll() is called exactly once
   static std::once_flag initFlag;
   std::call_once(initFlag, &ObjectStoreTyped<NE>::loadAll, &singleton);

   return singleton;
}

// We have to make sure that each version of the above function gets instantiated
template ObjectStoreTyped<BrewNote> &             ObjectStoreTyped<BrewNote>::getInstance();
template ObjectStoreTyped<Equipment> &            ObjectStoreTyped<Equipment>::getInstance();
template ObjectStoreTyped<Fermentable> &          ObjectStoreTyped<Fermentable>::getInstance();
template ObjectStoreTyped<InventoryFermentable> & ObjectStoreTyped<InventoryFermentable>::getInstance();
template ObjectStoreTyped<Hop> &                  ObjectStoreTyped<Hop>::getInstance();
template ObjectStoreTyped<InventoryHop> &         ObjectStoreTyped<InventoryHop>::getInstance();
template ObjectStoreTyped<Instruction> &          ObjectStoreTyped<Instruction>::getInstance();
template ObjectStoreTyped<Mash> &                 ObjectStoreTyped<Mash>::getInstance();
template ObjectStoreTyped<MashStep> &             ObjectStoreTyped<MashStep>::getInstance();
template ObjectStoreTyped<Misc> &                 ObjectStoreTyped<Misc>::getInstance();
template ObjectStoreTyped<InventoryMisc> &        ObjectStoreTyped<InventoryMisc>::getInstance();
template ObjectStoreTyped<Recipe> &               ObjectStoreTyped<Recipe>::getInstance();
template ObjectStoreTyped<Salt> &                 ObjectStoreTyped<Salt>::getInstance();
template ObjectStoreTyped<Style> &                ObjectStoreTyped<Style>::getInstance();
template ObjectStoreTyped<Water> &                ObjectStoreTyped<Water>::getInstance();
template ObjectStoreTyped<Yeast> &                ObjectStoreTyped<Yeast>::getInstance();
template ObjectStoreTyped<InventoryYeast> &       ObjectStoreTyped<InventoryYeast>::getInstance();
