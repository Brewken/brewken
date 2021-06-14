/**
 * database/DbNamedEntityRecords.cpp is part of Brewken, and is copyright the following authors 2021:
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
#include "database/DbNamedEntityRecords.h"

#include "model/BrewNote.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Hop.h"
#include "model/Instruction.h"
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
// .:TBD:. What about inventory?
// .:TBD:. What about read-only fields, eg if we want an Instruction to pull its Recipe ID from instruction_in_recipe
//
// .:TBD:. At the moment, each table name is used pretty much once, but if that changes then we might want to add
//        constants along the following lines:
// namespace DatabaseNames::Tables { static char const * const brewnote            = "brewnote"; }
// plus something similar for column names


namespace {

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for BrewNote
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::FieldSimpleDefns const BREWNOTE_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",                      PropertyNames::NamedEntity::key},
      // NB: BrewNotes don't have names in DB {DbRecords::FieldType::String, "name",                    PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::String, "'Dummy name'",                    PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "display",                 PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::Bool,   "deleted",                 PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::String, "folder",                  PropertyNames::NamedEntity::folder},
      {DbRecords::FieldType::Double, "abv",                     PropertyNames::BrewNote::abv},
      {DbRecords::FieldType::Double, "attenuation",             PropertyNames::BrewNote::attenuation},
      {DbRecords::FieldType::Double, "boil_off",                PropertyNames::BrewNote::boilOff_l},
      {DbRecords::FieldType::Date,   "brewdate",                PropertyNames::BrewNote::brewDate},
      {DbRecords::FieldType::Double, "brewhouse_eff",           PropertyNames::BrewNote::brewhouseEff_pct},
      {DbRecords::FieldType::Double, "eff_into_bk",             PropertyNames::BrewNote::effIntoBK_pct},
      {DbRecords::FieldType::Date,   "fermentdate",             PropertyNames::BrewNote::fermentDate},
      {DbRecords::FieldType::Double, "fg",                      PropertyNames::BrewNote::fg},
      {DbRecords::FieldType::Double, "final_volume",            PropertyNames::BrewNote::finalVolume_l},
      // NB: BrewNotes don't have folders, as each one is owned by a Recipe
      {DbRecords::FieldType::Double, "mash_final_temp",         PropertyNames::BrewNote::mashFinTemp_c},
      {DbRecords::FieldType::String, "notes",                   PropertyNames::BrewNote::notes},
      {DbRecords::FieldType::Double, "og",                      PropertyNames::BrewNote::og},
      {DbRecords::FieldType::Double, "pitch_temp",              PropertyNames::BrewNote::pitchTemp_c},
      {DbRecords::FieldType::Double, "post_boil_volume",        PropertyNames::BrewNote::postBoilVolume_l},
      {DbRecords::FieldType::Double, "projected_abv",           PropertyNames::BrewNote::projABV_pct},
      {DbRecords::FieldType::Double, "projected_atten",         PropertyNames::BrewNote::projAtten},
      {DbRecords::FieldType::Double, "projected_boil_grav",     PropertyNames::BrewNote::projBoilGrav},
      {DbRecords::FieldType::Double, "projected_eff",           PropertyNames::BrewNote::projEff_pct},
      {DbRecords::FieldType::Double, "projected_ferm_points",   PropertyNames::BrewNote::projFermPoints},
      {DbRecords::FieldType::Double, "projected_fg",            PropertyNames::BrewNote::projFg},
      {DbRecords::FieldType::Double, "projected_mash_fin_temp", PropertyNames::BrewNote::projMashFinTemp_c},
      {DbRecords::FieldType::Double, "projected_og",            PropertyNames::BrewNote::projOg},
      {DbRecords::FieldType::Double, "projected_points",        PropertyNames::BrewNote::projPoints},
      {DbRecords::FieldType::Double, "projected_strike_temp",   PropertyNames::BrewNote::projStrikeTemp_c},
      {DbRecords::FieldType::Double, "projected_vol_into_bk",   PropertyNames::BrewNote::projVolIntoBK_l},
      {DbRecords::FieldType::Double, "projected_vol_into_ferm", PropertyNames::BrewNote::projVolIntoFerm_l},
      {DbRecords::FieldType::Double, "sg",                      PropertyNames::BrewNote::sg},
      {DbRecords::FieldType::Double, "strike_temp",             PropertyNames::BrewNote::strikeTemp_c},
      {DbRecords::FieldType::Double, "volume_into_bk",          PropertyNames::BrewNote::volumeIntoBK_l},
      {DbRecords::FieldType::Double, "volume_into_fermenter",   PropertyNames::BrewNote::volumeIntoFerm_l},
      {DbRecords::FieldType::Int   , "recipe_id",               PropertyNames::BrewNote::recipeId}
   };
   DbRecords::FieldManyToManyDefns const BREWNOTE_MULTI_FIELDS{};

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Equipment
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::FieldSimpleDefns const EQUIPMENT_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",                PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",              PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "display",           PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::Bool,   "deleted",           PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::String, "folder",            PropertyNames::NamedEntity::folder},
      {DbRecords::FieldType::Double, "batch_size",        PropertyNames::Equipment::batchSize_l},
      {DbRecords::FieldType::Double, "boiling_point",     PropertyNames::Equipment::boilingPoint_c},
      {DbRecords::FieldType::Double, "boil_size",         PropertyNames::Equipment::boilSize_l},
      {DbRecords::FieldType::Double, "boil_time",         PropertyNames::Equipment::boilTime_min},
      {DbRecords::FieldType::Bool,   "calc_boil_volume",  PropertyNames::Equipment::calcBoilVolume},
      {DbRecords::FieldType::Double, "real_evap_rate",    PropertyNames::Equipment::evapRate_lHr},
      {DbRecords::FieldType::Double, "evap_rate",         PropertyNames::Equipment::evapRate_pctHr},
      {DbRecords::FieldType::Double, "absorption",        PropertyNames::Equipment::grainAbsorption_LKg},
      {DbRecords::FieldType::Double, "hop_utilization",   PropertyNames::Equipment::hopUtilization_pct},
      {DbRecords::FieldType::Double, "lauter_deadspace",  PropertyNames::Equipment::lauterDeadspace_l},
      {DbRecords::FieldType::String, "notes",             PropertyNames::Equipment::notes},
      {DbRecords::FieldType::Double, "top_up_kettle",     PropertyNames::Equipment::topUpKettle_l},
      {DbRecords::FieldType::Double, "top_up_water",      PropertyNames::Equipment::topUpWater_l},
      {DbRecords::FieldType::Double, "trub_chiller_loss", PropertyNames::Equipment::trubChillerLoss_l},
      {DbRecords::FieldType::Double, "tun_specific_heat", PropertyNames::Equipment::tunSpecificHeat_calGC},
      {DbRecords::FieldType::Double, "tun_volume",        PropertyNames::Equipment::tunVolume_l},
      {DbRecords::FieldType::Double, "tun_weight",        PropertyNames::Equipment::tunWeight_kg}
   };
   DbRecords::FieldManyToManyDefns const EQUIPMENT_MULTI_FIELDS {
      // Objects store their parents not their children, so this view of the junction table is from the child's point of view
      {"equipment_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, true}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Fermentable
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::EnumStringMapping const DB_FERMENTABLE_TYPE_ENUM {
      {"Grain",       Fermentable::Grain},
      {"Sugar",       Fermentable::Sugar},
      {"Extract",     Fermentable::Extract},
      {"Dry Extract", Fermentable::Dry_Extract},
      {"Adjunct",     Fermentable::Adjunct}
   };
   DbRecords::FieldSimpleDefns const FERMENTABLE_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",               PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",             PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "deleted",          PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::Bool,   "display",          PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::String, "folder",           PropertyNames::NamedEntity::folder},
      {DbRecords::FieldType::Bool,   "add_after_boil",   PropertyNames::Fermentable::addAfterBoil},
      {DbRecords::FieldType::Double, "amount",           PropertyNames::Fermentable::amount_kg},
      {DbRecords::FieldType::Double, "coarse_fine_diff", PropertyNames::Fermentable::coarseFineDiff_pct},
      {DbRecords::FieldType::Double, "color",            PropertyNames::Fermentable::color_srm},
      {DbRecords::FieldType::Double, "diastatic_power",  PropertyNames::Fermentable::diastaticPower_lintner},
      {DbRecords::FieldType::Enum,   "ftype",            PropertyNames::Fermentable::type,                   &DB_FERMENTABLE_TYPE_ENUM},
      {DbRecords::FieldType::Bool,   "is_mashed",        PropertyNames::Fermentable::isMashed},
      {DbRecords::FieldType::Double, "ibu_gal_per_lb",   PropertyNames::Fermentable::ibuGalPerLb},
      {DbRecords::FieldType::Double, "max_in_batch",     PropertyNames::Fermentable::maxInBatch_pct},
      {DbRecords::FieldType::Double, "moisture",         PropertyNames::Fermentable::moisture_pct},
      {DbRecords::FieldType::String, "notes",            PropertyNames::Fermentable::notes},
      {DbRecords::FieldType::String, "origin",           PropertyNames::Fermentable::origin},
      {DbRecords::FieldType::String, "supplier",         PropertyNames::Fermentable::supplier},
      {DbRecords::FieldType::Double, "protein",          PropertyNames::Fermentable::protein_pct},
      {DbRecords::FieldType::Bool,   "recommend_mash",   PropertyNames::Fermentable::recommendMash},
      {DbRecords::FieldType::Double, "yield",            PropertyNames::Fermentable::yield_pct}
      /// inventory_id REFERENCES fermentable_in_inventory (id))      <<< TODO
   };
   DbRecords::FieldManyToManyDefns const FERMENTABLE_MULTI_FIELDS {
      {"fermentable_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, true}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Hop TODO Check the strings!
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::EnumStringMapping const DB_HOP_USE_ENUM {
      {"Boil",       Hop::Boil},
      {"Dry Hop",    Hop::Dry_Hop},
      {"Mash",       Hop::Mash},
      {"First Wort", Hop::First_Wort},
      {"Aroma",      Hop::UseAroma}
   };
   DbRecords::EnumStringMapping const DB_HOP_TYPE_ENUM {
      {"Bittering", Hop::Bittering},
      {"Aroma",     Hop::Aroma},
      {"Both",      Hop::Both}
   };
   DbRecords::EnumStringMapping const DB_HOP_FORM_ENUM {
      {"Pellet", Hop::Pellet},
      {"Plug",   Hop::Plug},
      {"Leaf",   Hop::Leaf}
   };
   DbRecords::FieldSimpleDefns const HOP_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",            PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",          PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "display",       PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::Bool,   "deleted",       PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::String, "folder",        PropertyNames::NamedEntity::folder},
      {DbRecords::FieldType::Double, "alpha",         PropertyNames::Hop::alpha_pct},
      {DbRecords::FieldType::Double, "amount",        PropertyNames::Hop::amount_kg},
      {DbRecords::FieldType::Double, "beta",          PropertyNames::Hop::beta_pct},
      {DbRecords::FieldType::Double, "caryophyllene", PropertyNames::Hop::caryophyllene_pct},
      {DbRecords::FieldType::Double, "cohumulone",    PropertyNames::Hop::cohumulone_pct},
      {DbRecords::FieldType::Enum,   "form",          PropertyNames::Hop::form,              &DB_HOP_FORM_ENUM},
      {DbRecords::FieldType::Double, "hsi",           PropertyNames::Hop::hsi_pct},
      {DbRecords::FieldType::Double, "humulene",      PropertyNames::Hop::humulene_pct},
      //{DbRecords::FieldType::, "", PropertyNames::Hop::inventory},
      {DbRecords::FieldType::Double, "myrcene",       PropertyNames::Hop::myrcene_pct},
      {DbRecords::FieldType::String, "notes",         PropertyNames::Hop::notes},
      {DbRecords::FieldType::String, "origin",        PropertyNames::Hop::origin},
      {DbRecords::FieldType::String, "substitutes",   PropertyNames::Hop::substitutes},
      {DbRecords::FieldType::Double, "time",          PropertyNames::Hop::time_min},
      {DbRecords::FieldType::Enum,   "htype",         PropertyNames::Hop::type,              &DB_HOP_TYPE_ENUM},
      {DbRecords::FieldType::Enum,   "use",           PropertyNames::Hop::use,               &DB_HOP_USE_ENUM}
   };
   DbRecords::FieldManyToManyDefns const HOP_MULTI_FIELDS {
      {"hop_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, true}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Instruction
   // NB: instructions aren't displayed in trees, and get no folder
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::FieldSimpleDefns const INSTRUCTION_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",         PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",       PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "display",    PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::Bool,   "deleted",    PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::String, "directions", PropertyNames::Instruction::directions },
      {DbRecords::FieldType::Bool,   "hasTimer",   PropertyNames::Instruction::hasTimer   },
      {DbRecords::FieldType::String, "timervalue", PropertyNames::Instruction::timerValue },
      {DbRecords::FieldType::Bool,   "completed",  PropertyNames::Instruction::completed  },
      {DbRecords::FieldType::Double, "interval",   PropertyNames::Instruction::interval   }
   };
   DbRecords::FieldManyToManyDefns const INSTRUCTION_MULTI_FIELDS {
      // Instructions don't have children
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Mash
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::FieldSimpleDefns const MASH_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",                PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",              PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "deleted",           PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::Bool,   "display",           PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::String, "folder",            PropertyNames::NamedEntity::folder},
      {DbRecords::FieldType::Bool,   "equip_adjust",      PropertyNames::Mash::equipAdjust},
      {DbRecords::FieldType::Double, "grain_temp",        PropertyNames::Mash::grainTemp_c},
      {DbRecords::FieldType::String, "notes",             PropertyNames::Mash::notes},
      {DbRecords::FieldType::Double, "ph",                PropertyNames::Mash::ph},
      {DbRecords::FieldType::Double, "sparge_temp",       PropertyNames::Mash::spargeTemp_c},
      {DbRecords::FieldType::Double, "tun_specific_heat", PropertyNames::Mash::tunSpecificHeat_calGC},
      {DbRecords::FieldType::Double, "tun_temp",          PropertyNames::Mash::tunTemp_c},
      {DbRecords::FieldType::Double, "tun_weight",        PropertyNames::Mash::tunWeight_kg},
   };
   DbRecords::FieldManyToManyDefns const MASH_MULTI_FIELDS {
      // Mashes don't have children, and the link with their MashSteps is stored in the MashStep (as between Recipe and BrewNotes)
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for MashStep
   // NB: MashSteps don't get folders, because they don't separate from their Mash
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::EnumStringMapping const MASH_STEP_TYPE_ENUM {
      {"Infusion",     MashStep::Infusion},
      {"Temperature",  MashStep::Temperature},
      {"Decoction",    MashStep::Decoction},
      {"FlySparge",    MashStep::flySparge},
      {"BatchSparge",  MashStep::batchSparge}
   };
   DbRecords::FieldSimpleDefns const MASH_STEP_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",                PropertyNames::NamedEntity::key            },
      {DbRecords::FieldType::String, "name",              PropertyNames::NamedEntity::name           },
      {DbRecords::FieldType::Bool,   "deleted",           PropertyNames::NamedEntity::deleted        },
      {DbRecords::FieldType::Bool,   "display",           PropertyNames::NamedEntity::display        },
      // NB: MashSteps don't have folders, as each one is owned by a Mash
      {DbRecords::FieldType::Double,  "decoction_amount", PropertyNames::MashStep::decoctionAmount_l },
      {DbRecords::FieldType::Double,  "end_temp",         PropertyNames::MashStep::endTemp_c         },
      {DbRecords::FieldType::Double,  "infuse_amount",    PropertyNames::MashStep::infuseAmount_l    },
      {DbRecords::FieldType::Double,  "infuse_temp",      PropertyNames::MashStep::infuseTemp_c      },
      {DbRecords::FieldType::Int,     "mash_id",          PropertyNames::MashStep::mashId            },
      {DbRecords::FieldType::Enum,    "mstype",           PropertyNames::MashStep::type,             &MASH_STEP_TYPE_ENUM},
      {DbRecords::FieldType::Double,  "ramp_time",        PropertyNames::MashStep::rampTime_min      },
      {DbRecords::FieldType::Int,     "step_number",      PropertyNames::MashStep::stepNumber        },
      {DbRecords::FieldType::Double,  "step_temp",        PropertyNames::MashStep::stepTemp_c        },
      {DbRecords::FieldType::Double,  "step_time",        PropertyNames::MashStep::stepTime_min      }
   };
   DbRecords::FieldManyToManyDefns const MASH_STEP_MULTI_FIELDS {
      // MashSteps don't have children
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Misc
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::EnumStringMapping const MISC_TYPE_ENUM {
      {"Spice",       Misc::Spice},
      {"Fining",      Misc::Fining},
      {"Water Agent", Misc::Water_Agent},
      {"Herb",        Misc::Herb},
      {"Flavor",      Misc::Flavor},
      {"Other",       Misc::Other}
   };
   DbRecords::EnumStringMapping const MISC_USE_ENUM {
      {"Boil",      Misc::Boil},
      {"Mash",      Misc::Mash},
      {"Primary",   Misc::Primary},
      {"Secondary", Misc::Secondary},
      {"Bottling",  Misc::Bottling}
   };
   DbRecords::FieldSimpleDefns const MISC_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",               PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",             PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "deleted",          PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::Bool,   "display",          PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::String, "folder",           PropertyNames::NamedEntity::folder},
      {DbRecords::FieldType::Enum,   "mtype",            PropertyNames::Misc::type,           &MISC_TYPE_ENUM},
      {DbRecords::FieldType::Enum,   "use",              PropertyNames::Misc::use,            &MISC_USE_ENUM},
      {DbRecords::FieldType::Double, "time",             PropertyNames::Misc::time           },
      {DbRecords::FieldType::Double, "amount",           PropertyNames::Misc::amount         },
      {DbRecords::FieldType::Bool,   "amount_is_weight", PropertyNames::Misc::amountIsWeight },
      {DbRecords::FieldType::String, "use_for",          PropertyNames::Misc::useFor         },
      {DbRecords::FieldType::String, "notes",            PropertyNames::Misc::notes          }
      //, inventory_id REFERENCES misc_in_inventory (id))      <<< TODO
   };
   DbRecords::FieldManyToManyDefns const MISC_MULTI_FIELDS {
      {"misc_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, true}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Recipe
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::EnumStringMapping const RECIPE_STEP_TYPE_ENUM {
      {"Extract",      Recipe::Extract},
      {"Partial Mash", Recipe::PartialMash},
      {"All Grain",    Recipe::AllGrain}
   };
   DbRecords::FieldSimpleDefns const RECIPE_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",                PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",              PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "deleted",           PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::Bool,   "display",           PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::String, "folder",            PropertyNames::NamedEntity::folder},
      {DbRecords::FieldType::Double, "age",                 PropertyNames::Recipe::age,                },
      {DbRecords::FieldType::Double, "age_temp",            PropertyNames::Recipe::ageTemp_c,          },
      {DbRecords::FieldType::String, "assistant_brewer",    PropertyNames::Recipe::asstBrewer,         },
      {DbRecords::FieldType::Double, "batch_size",          PropertyNames::Recipe::batchSize_l,        },
      {DbRecords::FieldType::Double, "boil_size",           PropertyNames::Recipe::boilSize_l,         },
      {DbRecords::FieldType::Double, "boil_time",           PropertyNames::Recipe::boilTime_min,       },
      {DbRecords::FieldType::String, "brewer",              PropertyNames::Recipe::brewer,             },
      {DbRecords::FieldType::Double, "carb_volume",         PropertyNames::Recipe::carbonation_vols,   },
      {DbRecords::FieldType::Double, "carbonationtemp_c",   PropertyNames::Recipe::carbonationTemp_c,  },
      {DbRecords::FieldType::Date,   "date",                PropertyNames::Recipe::date,               },
      {DbRecords::FieldType::Double, "efficiency",          PropertyNames::Recipe::efficiency_pct,     },
      {DbRecords::FieldType::Int,    "equipment_id",        PropertyNames::Recipe::equipmentId,        },
      {DbRecords::FieldType::UInt,   "fermentation_stages", PropertyNames::Recipe::fermentationStages, },
      {DbRecords::FieldType::Double, "fg",                  PropertyNames::Recipe::fg,                 },
      {DbRecords::FieldType::Bool,   "forced_carb",         PropertyNames::Recipe::forcedCarbonation,  },
      {DbRecords::FieldType::Double, "keg_priming_factor",  PropertyNames::Recipe::kegPrimingFactor,   },
      {DbRecords::FieldType::Int,    "mash_id",             PropertyNames::Recipe::mashId,             },
      {DbRecords::FieldType::String, "notes",               PropertyNames::Recipe::notes,              },
      {DbRecords::FieldType::Double, "og",                  PropertyNames::Recipe::og,                 },
      {DbRecords::FieldType::Double, "primary_age",         PropertyNames::Recipe::primaryAge_days,    },
      {DbRecords::FieldType::Double, "primary_temp",        PropertyNames::Recipe::primaryTemp_c,      },
      {DbRecords::FieldType::Double, "priming_sugar_equiv", PropertyNames::Recipe::primingSugarEquiv,  },
      {DbRecords::FieldType::String, "priming_sugar_name",  PropertyNames::Recipe::primingSugarName,   },
      {DbRecords::FieldType::Double, "secondary_age",       PropertyNames::Recipe::secondaryAge_days,  },
      {DbRecords::FieldType::Double, "secondary_temp",      PropertyNames::Recipe::secondaryTemp_c,    },
      {DbRecords::FieldType::Int,    "style_id",            PropertyNames::Recipe::styleId,            },
      {DbRecords::FieldType::String, "taste_notes",         PropertyNames::Recipe::tasteNotes,         },
      {DbRecords::FieldType::Double, "taste_rating",        PropertyNames::Recipe::tasteRating,        },
      {DbRecords::FieldType::Double, "tertiary_age",        PropertyNames::Recipe::tertiaryAge_days,   },
      {DbRecords::FieldType::Double, "tertiary_temp",       PropertyNames::Recipe::tertiaryTemp_c,     },
      {DbRecords::FieldType::Enum,   "type",                PropertyNames::Recipe::recipeType,         &RECIPE_STEP_TYPE_ENUM},
   };
   DbRecords::FieldManyToManyDefns const RECIPE_MULTI_FIELDS {
      // .:TODO:. BrewNote table stores its recipe ID, so there isn't a brewnote junction table
      {"fermentable_in_recipe", "recipe_id", "fermentable_id", PropertyNames::Recipe::fermentableIds},
      {"hop_in_recipe",         "recipe_id", "hop_id",         PropertyNames::Recipe::hopIds},
      {"instruction_in_recipe", "recipe_id", "instruction_id", PropertyNames::Recipe::instructionIds, false, "instruction_number"},
      {"misc_in_recipe",        "recipe_id", "misc_id",        PropertyNames::Recipe::miscIds},
      {"salt_in_recipe",        "recipe_id", "salt_id",        PropertyNames::Recipe::saltIds},
      {"water_in_recipe",       "recipe_id", "water_id",       PropertyNames::Recipe::waterIds},
      {"yeast_in_recipe",       "recipe_id", "yeast_id",       PropertyNames::Recipe::yeastIds}

   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Salt
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::FieldSimpleDefns const SALT_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",               PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",             PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "deleted",          PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::Bool,   "display",          PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::String, "folder",           PropertyNames::NamedEntity::folder},
      {DbRecords::FieldType::Int,    "addTo",            PropertyNames::Salt::addTo          }, // TODO: Really an Enum.  Would be less fragile to store this as text than a number
      {DbRecords::FieldType::Double, "amount",           PropertyNames::Salt::amount         },
      {DbRecords::FieldType::Bool,   "amount_is_weight", PropertyNames::Salt::amountIsWeight },
      {DbRecords::FieldType::Bool,   "is_acid",          PropertyNames::Salt::isAcid         },
      {DbRecords::FieldType::Double, "percent_acid",     PropertyNames::Salt::percentAcid    },
      {DbRecords::FieldType::Int,    "stype",            PropertyNames::Salt::type           }    // TODO: Really an Enum.  Would be less fragile to store this as text than a number
   };
   DbRecords::FieldManyToManyDefns const SALT_MULTI_FIELDS {
      // Salts don't have children
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Style
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::EnumStringMapping const STYLE_TYPE_ENUM {
      {"Lager", Style::Lager},
      {"Ale",   Style::Ale},
      {"Mead",  Style::Mead},
      {"Wheat", Style::Wheat},
      {"Mixed", Style::Mixed},
      {"Cider", Style::Cider}
   };
   DbRecords::FieldSimpleDefns const STYLE_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",              PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",            PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "display",         PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::Bool,   "deleted",         PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::String, "folder",          PropertyNames::NamedEntity::folder},
      {DbRecords::FieldType::Double, "abv_max",         PropertyNames::Style::abvMax_pct},
      {DbRecords::FieldType::Double, "abv_min",         PropertyNames::Style::abvMin_pct},
      {DbRecords::FieldType::Double, "carb_max",        PropertyNames::Style::carbMax_vol},
      {DbRecords::FieldType::Double, "carb_min",        PropertyNames::Style::carbMin_vol},
      {DbRecords::FieldType::String, "category",        PropertyNames::Style::category},
      {DbRecords::FieldType::String, "category_number", PropertyNames::Style::categoryNumber},
      {DbRecords::FieldType::Double, "color_max",       PropertyNames::Style::colorMax_srm},
      {DbRecords::FieldType::Double, "color_min",       PropertyNames::Style::colorMin_srm},
      {DbRecords::FieldType::String, "examples",        PropertyNames::Style::examples},
      {DbRecords::FieldType::Double, "fg_max",          PropertyNames::Style::fgMax},
      {DbRecords::FieldType::Double, "fg_min",          PropertyNames::Style::fgMin},
      {DbRecords::FieldType::Double, "ibu_max",         PropertyNames::Style::ibuMax},
      {DbRecords::FieldType::Double, "ibu_min",         PropertyNames::Style::ibuMin},
      {DbRecords::FieldType::String, "ingredients",     PropertyNames::Style::ingredients},
      {DbRecords::FieldType::String, "notes",           PropertyNames::Style::notes},
      {DbRecords::FieldType::Double, "og_max",          PropertyNames::Style::ogMax},
      {DbRecords::FieldType::Double, "og_min",          PropertyNames::Style::ogMin},
      {DbRecords::FieldType::String, "profile",         PropertyNames::Style::profile},
      {DbRecords::FieldType::String, "style_guide",     PropertyNames::Style::styleGuide},
      {DbRecords::FieldType::String, "style_letter",    PropertyNames::Style::styleLetter},
      {DbRecords::FieldType::Enum,   "s_type",          PropertyNames::Style::type,           &STYLE_TYPE_ENUM}
   };
   DbRecords::FieldManyToManyDefns const STYLE_MULTI_FIELDS {
      {"style_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, true}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Water
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::FieldSimpleDefns const WATER_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",          PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",        PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "display",     PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::Bool,   "deleted",     PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::String, "folder",      PropertyNames::NamedEntity::folder},
      {DbRecords::FieldType::String, "notes",       PropertyNames::Water::notes},
      {DbRecords::FieldType::Double, "amount",      PropertyNames::Water::amount},
      {DbRecords::FieldType::Double, "calcium",     PropertyNames::Water::calcium_ppm},
      {DbRecords::FieldType::Double, "bicarbonate", PropertyNames::Water::bicarbonate_ppm},
      {DbRecords::FieldType::Double, "sulfate",     PropertyNames::Water::sulfate_ppm},
      {DbRecords::FieldType::Double, "sodium",      PropertyNames::Water::sodium_ppm},
      {DbRecords::FieldType::Double, "chloride",    PropertyNames::Water::chloride_ppm},
      {DbRecords::FieldType::Double, "magnesium",   PropertyNames::Water::magnesium_ppm},
      {DbRecords::FieldType::Double, "ph",          PropertyNames::Water::ph},
      {DbRecords::FieldType::Double, "alkalinity",  PropertyNames::Water::alkalinity},
      {DbRecords::FieldType::Int,    "wtype",       PropertyNames::Water::type},             // TODO: Would be less fragile to store this as text than a number
      {DbRecords::FieldType::Double, "mash_ro",     PropertyNames::Water::mashRO},
      {DbRecords::FieldType::Double, "sparge_ro",   PropertyNames::Water::spargeRO},
      {DbRecords::FieldType::Bool,   "as_hco3",     PropertyNames::Water::alkalinityAsHCO3}
   };
   DbRecords::FieldManyToManyDefns const WATER_MULTI_FIELDS {
      {"water_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, true}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Yeast
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::EnumStringMapping const DB_YEAST_TYPE_ENUM {
      {"Ale",       Yeast::Ale},
      {"Lager",     Yeast::Lager},
      {"Wheat",     Yeast::Wheat},
      {"Wine",      Yeast::Wine},
      {"Champagne", Yeast::Champagne}
   };
   DbRecords::EnumStringMapping const DB_YEAST_FORM_ENUM {
      {"Liquid",  Yeast::Liquid},
      {"Dry",     Yeast::Dry},
      {"Slant",   Yeast::Slant},
      {"Culture", Yeast::Culture}
   };
   DbRecords::EnumStringMapping const DB_YEAST_FLOCCULATION_ENUM {
      {"Low",       Yeast::Low},
      {"Medium",    Yeast::Medium},
      {"High",      Yeast::High},
      {"Very High", Yeast::Very_High}
   };
   DbRecords::FieldSimpleDefns const YEAST_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",          PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",        PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "display",     PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::Bool,   "deleted",     PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::String, "folder",      PropertyNames::NamedEntity::folder},
      {DbRecords::FieldType::Bool,   "add_to_secondary", PropertyNames::Yeast::addToSecondary},
      {DbRecords::FieldType::Bool,   "amount_is_weight", PropertyNames::Yeast::amountIsWeight},
      {DbRecords::FieldType::Double, "amount",           PropertyNames::Yeast::amount},
      {DbRecords::FieldType::Double, "attenuation",      PropertyNames::Yeast::attenuation_pct},
      {DbRecords::FieldType::Double, "max_temperature",  PropertyNames::Yeast::maxTemperature_c},
      {DbRecords::FieldType::Double, "min_temperature",  PropertyNames::Yeast::minTemperature_c},
      {DbRecords::FieldType::Enum,   "flocculation",     PropertyNames::Yeast::flocculation,   &DB_YEAST_FLOCCULATION_ENUM},
      {DbRecords::FieldType::Enum,   "form",             PropertyNames::Yeast::form,           &DB_YEAST_FORM_ENUM},
      {DbRecords::FieldType::Enum,   "ytype",            PropertyNames::Yeast::type,           &DB_YEAST_TYPE_ENUM},
      {DbRecords::FieldType::Int,    "max_reuse",        PropertyNames::Yeast::maxReuse},
      {DbRecords::FieldType::Int,    "times_cultured",   PropertyNames::Yeast::timesCultured},
      {DbRecords::FieldType::String, "best_for",         PropertyNames::Yeast::bestFor},
      {DbRecords::FieldType::String, "laboratory",       PropertyNames::Yeast::laboratory},
      {DbRecords::FieldType::String, "notes",            PropertyNames::Yeast::notes},
      {DbRecords::FieldType::String, "product_id",       PropertyNames::Yeast::productID}
   };
   DbRecords::FieldManyToManyDefns const YEAST_MULTI_FIELDS {
      {"yeast_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, true}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Inventory
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*   CREATE TABLE "fermentable_in_inventory" (id INTEGER PRIMARY KEY autoincrement , amount real  DEFAULT 0)
   CREATE TABLE "hop_in_inventory" (id INTEGER PRIMARY KEY autoincrement , amount real  DEFAULT 0)
   CREATE TABLE "misc_in_inventory" (id INTEGER PRIMARY KEY autoincrement , amount real  DEFAULT 0)
   CREATE TABLE water_in_recipe( id integer PRIMARY KEY autoincrement, water_id integer, recipe_id integer, foreign key(water_id) references water(id), foreign key(recipe_id) references recipe(id))
   CREATE TABLE "yeast_in_inventory" (id INTEGER PRIMARY KEY autoincrement , quanta real  DEFAULT 0) */
}

template<> DbNamedEntityRecords<BrewNote> & DbNamedEntityRecords<BrewNote>::getInstance() {
   static DbNamedEntityRecords<BrewNote> singleton{"brewnote", BREWNOTE_SIMPLE_FIELDS, BREWNOTE_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Equipment> & DbNamedEntityRecords<Equipment>::getInstance() {
   static DbNamedEntityRecords<Equipment> singleton{"equipment", EQUIPMENT_SIMPLE_FIELDS, EQUIPMENT_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Fermentable> & DbNamedEntityRecords<Fermentable>::getInstance() {
   static DbNamedEntityRecords<Fermentable> singleton{"fermentable", FERMENTABLE_SIMPLE_FIELDS, FERMENTABLE_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Hop> & DbNamedEntityRecords<Hop>::getInstance() {
   static DbNamedEntityRecords<Hop> singleton{"hop", HOP_SIMPLE_FIELDS, HOP_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Instruction> & DbNamedEntityRecords<Instruction>::getInstance() {
   static DbNamedEntityRecords<Instruction> singleton{"instruction", INSTRUCTION_SIMPLE_FIELDS, INSTRUCTION_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Mash> & DbNamedEntityRecords<Mash>::getInstance() {
   static DbNamedEntityRecords<Mash> singleton{"mash", MASH_SIMPLE_FIELDS, MASH_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<MashStep> & DbNamedEntityRecords<MashStep>::getInstance() {
   static DbNamedEntityRecords<MashStep> singleton{"mashstep", MASH_STEP_SIMPLE_FIELDS, MASH_STEP_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Misc> & DbNamedEntityRecords<Misc>::getInstance() {
   static DbNamedEntityRecords<Misc> singleton{"misc", MISC_SIMPLE_FIELDS, MISC_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Recipe> & DbNamedEntityRecords<Recipe>::getInstance() {
   static DbNamedEntityRecords<Recipe> singleton{"recipe", RECIPE_SIMPLE_FIELDS, RECIPE_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Salt> & DbNamedEntityRecords<Salt>::getInstance() {
   static DbNamedEntityRecords<Salt> singleton{"salt", SALT_SIMPLE_FIELDS, SALT_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Style> & DbNamedEntityRecords<Style>::getInstance() {
   static DbNamedEntityRecords<Style> singleton{"style", STYLE_SIMPLE_FIELDS, STYLE_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Water> & DbNamedEntityRecords<Water>::getInstance() {
   static DbNamedEntityRecords<Water> singleton{"water", WATER_SIMPLE_FIELDS, WATER_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Yeast> & DbNamedEntityRecords<Yeast>::getInstance() {
   static DbNamedEntityRecords<Yeast> singleton{"yeast", YEAST_SIMPLE_FIELDS, YEAST_MULTI_FIELDS};
   return singleton;
}
