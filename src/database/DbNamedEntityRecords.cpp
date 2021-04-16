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
#include "model/Style.h"
#include "model/Water.h"
#include "model/Yeast.h"

// .:TODO:. Create tables

namespace {

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for BrewNote
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::FieldDefinitions const BREWNOTE_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",                      PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",                    PropertyNames::NamedEntity::name},
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
      {DbRecords::FieldType::Double, "volume_into_fermenter",   PropertyNames::BrewNote::volumeIntoFerm_l} //,
      //{DbRecords::FieldType::Int   , "recipe_id",               PropertyNames::BrewNote::recipeId}  <<<<<<<<<<<<<<<<<<<<<<<<<<<<TODO
   };
   DbRecords::AssociativeEntities const BREWNOTE_MULTI_FIELDS{};

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Equipment
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::FieldDefinitions const EQUIPMENT_SIMPLE_FIELDS {
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
   DbRecords::AssociativeEntities const EQUIPMENT_MULTI_FIELDS {
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
   DbRecords::FieldDefinitions const FERMENTABLE_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",               PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",             PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "display",          PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::Bool,   "deleted",          PropertyNames::NamedEntity::deleted},
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
   DbRecords::AssociativeEntities const FERMENTABLE_MULTI_FIELDS {
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
   DbRecords::FieldDefinitions const HOP_SIMPLE_FIELDS {
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
   DbRecords::AssociativeEntities const HOP_MULTI_FIELDS {
      {"hop_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, true}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Instruction
   // NB: instructions aren't displayed in trees, and get no folder
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::FieldDefinitions const INSTRUCTION_SIMPLE_FIELDS {
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
   DbRecords::AssociativeEntities const INSTRUCTION_MULTI_FIELDS {
      // Instructions don't have children
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Mash
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for MashStep
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Misc
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Recipe
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Salt
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
   DbRecords::FieldDefinitions const STYLE_SIMPLE_FIELDS {
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
   DbRecords::AssociativeEntities const STYLE_MULTI_FIELDS {
      {"style_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, true}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Database field mappings for Water
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   DbRecords::FieldDefinitions const WATER_SIMPLE_FIELDS {
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
   DbRecords::AssociativeEntities const WATER_MULTI_FIELDS {
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
   DbRecords::FieldDefinitions const YEAST_SIMPLE_FIELDS {
      {DbRecords::FieldType::Int,    "id",          PropertyNames::NamedEntity::key},
      {DbRecords::FieldType::String, "name",        PropertyNames::NamedEntity::name},
      {DbRecords::FieldType::Bool,   "display",     PropertyNames::NamedEntity::display},
      {DbRecords::FieldType::Bool,   "deleted",     PropertyNames::NamedEntity::deleted},
      {DbRecords::FieldType::String, "folder",      PropertyNames::NamedEntity::folder},
      {DbRecords::FieldType::String, "notes",       PropertyNames::Yeast::notes},
      {DbRecords::FieldType::Enum,   "ytype",       PropertyNames::Yeast::type, &DB_YEAST_TYPE_ENUM},
      {DbRecords::FieldType::Enum,   "form",        PropertyNames::Yeast::form, &DB_YEAST_FORM_ENUM},
      {DbRecords::FieldType::Double,   "amount",      PropertyNames::Yeast::amount},
      {DbRecords::FieldType::Bool,   "amount_is_weight",           PropertyNames::Yeast::amountIsWeight},
      {DbRecords::FieldType::String,   "laboratory",           PropertyNames::Yeast::laboratory},
      {DbRecords::FieldType::String,   "product_id",           PropertyNames::Yeast::productID},
      {DbRecords::FieldType::Double,   "min_temperature",           PropertyNames::Yeast::minTemperature_c},
      {DbRecords::FieldType::Double,   "max_temperature",           PropertyNames::Yeast::maxTemperature_c},
      {DbRecords::FieldType::Enum,   "flocculation",           PropertyNames::Yeast::flocculation, &DB_YEAST_FLOCCULATION_ENUM},
      {DbRecords::FieldType::Double,   "attenuation",           PropertyNames::Yeast::attenuation_pct},
      {DbRecords::FieldType::String,   "notes",           PropertyNames::Yeast::notes},
      {DbRecords::FieldType::String,   "best_for",           PropertyNames::Yeast::bestFor},
      {DbRecords::FieldType::Int,   "times_cultured",           PropertyNames::Yeast::timesCultured},
      {DbRecords::FieldType::Int,   "max_reuse",           PropertyNames::Yeast::maxReuse},
      {DbRecords::FieldType::Bool,   "add_to_secondary",           PropertyNames::Yeast::addToSecondary}
   };
   DbRecords::AssociativeEntities const YEAST_MULTI_FIELDS {
      {"yeast_children", "child_id", "parent_id", PropertyNames::NamedEntity::parentKey, true}
   };
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

template<> DbNamedEntityRecords<Style> & DbNamedEntityRecords<Style>::getInstance() {
   static DbNamedEntityRecords<Style> singleton{"style", STYLE_SIMPLE_FIELDS, STYLE_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Water> & DbNamedEntityRecords<Water>::getInstance() {
   static DbNamedEntityRecords<Water> singleton{"water", WATER_SIMPLE_FIELDS, WATER_MULTI_FIELDS};
   return singleton;
}

template<> DbNamedEntityRecords<Yeast> & DbNamedEntityRecords<Yeast>::getInstance() {
   static DbNamedEntityRecords<Yeast> singleton{"yeast", HOP_SIMPLE_FIELDS, HOP_MULTI_FIELDS};
   return singleton;
}


//
// Functions to extract from Database class:
//
//   Water* newWater(Water* other = nullptr);
//   int    insertWater(Water* ins);
//   Water* water(int key); AKA getWater(int key)
//      Water * addToRecipe( Recipe* rec, Water* w, bool noCopy = false, bool transact = true);
//   Q_PROPERTY( QList<Water*> waters READ waters /*WRITE*/ NOTIFY changed STORED false )
//   QList<Water*> waters();
//   //! Return a list of all the waters in a recipe.
//   QList<Water*> waters( Recipe const* parent );
// signals:
//   void changed(QMetaProperty prop, QVariant value);
//   void newWaterSignal(Water*);
//   void deletedSignal(Water*);
//   QHash< int, Water* > allWaters;
//
//   // This is called only from NamedEntity::setEasy() and from Database::remove<T>()
//   void updateEntry( NamedEntity* object, QString propName, QVariant value, bool notify = true, bool transact = false );
//
//   template<class T> T* newNamedEntity(QHash<int,T*>* all) {...}
//   template<class T> T* newNamedEntity(QString name, QHash<int,T*>* all) {...}
//
//   // Mostly called from Database::insertWater() and corresponding functions
//   int    insertElement(NamedEntity* ins);
//
//   // Called only from Database::insertElement() to check if the element is already stored
//   //! \returns true if this ingredient is stored in the DB, false otherwise
//   bool isStored(NamedEntity const & ingredient);
//
//
//   void setInventory(NamedEntity* ins, QVariant value, int invKey = 0, bool notify=true );
//
//
//   Short term we should just load this in when the object is loaded in
//   /**
//   * \brief  This function is intended to be called by an ingredient that has not already cached its parent's key
//   * \return Key of parent ingredient if there is one, 0 otherwise
//   */
//   int getParentNamedEntityKey(NamedEntity const & ingredient);
//
//   /*! \brief Removes the specified ingredient from the recipe, then calls the changed()
//    *         signal corresponding to the appropriate QList
//    *         of ingredients in rec.
//    *  \param rec
//    *  \param ing
//    *  \returns the parent of the ingredient deleted (which is needed to be able to undo the removal)
//    */
//   NamedEntity * removeNamedEntityFromRecipe( Recipe* rec, NamedEntity* ing );
//
//   template <class T> void populateElements( QHash<int,T*>& hash, DatabaseConstants::DbTableId table );
//
//   template <class T> bool getElementsByName( QList<T*>& list, DatabaseConstants::DbTableId table, QString name, QHash<int,T*> allElements, QString id=QString("") )
//   void deleteRecord( NamedEntity* object );
//   template<class T> T* addNamedEntityToRecipe(
//      Recipe* rec,
//      NamedEntity* ing,
//      bool noCopy = false,
//      QHash<int,T*>* keyHash = 0,
//      bool doNotDisplay = true,
//      bool transact = true
//   );
//   /*!
//    * \brief Create a deep copy of the \b object.
//    * \em T must be a subclass of \em NamedEntity.
//    * \returns a pointer to the new copy. You must manually emit the changed()
//    * signal after a copy() call. Also, does not insert things magically into
//    * allHop or allInstructions etc. hashes. This just simply duplicates a
//    * row in a table, unless you provide \em keyHash.
//    * \param object is the thing you want to copy.
//    * \param displayed is true if you want the \em displayed column set to true.
//    * \param keyHash if nonzero, inserts the new (key,T*) pair into the hash.
//    */
//   template<class T> T* copy( NamedEntity const* object, QHash<int,T*>* keyHash, bool displayed = true );
//   // Do an sql update.
//   void sqlUpdate( DatabaseConstants::DbTableId table, QString const& setClause, QString const& whereClause );
//
//   // Do an sql delete.
//   void sqlDelete( DatabaseConstants::DbTableId table, QString const& whereClause );
//   QMap<QString, std::function<NamedEntity*(QString name)> > makeTableParams();
//

//


//
// Water table name "water"
// Int "id" "key"
/* CREATE TABLE water(
   id integer PRIMARY KEY autoincrement,
   -- BeerXML properties
   name varchar(256) not null DEFAULT '',
   amount real DEFAULT 0.0,
   calcium real DEFAULT 0.0,
   bicarbonate real DEFAULT 0.0,
   sulfate real DEFAULT 0.0,
   chloride real DEFAULT 0.0,
   sodium real DEFAULT 0.0,
   magnesium real DEFAULT 0.0,
   ph real DEFAULT 7.0,
   notes text DEFAULT '',
   -- metadata
   deleted boolean DEFAULT 0,
   display boolean DEFAULT 1,
   folder varchar(256) DEFAULT ''
, wtype int DEFAULT 0, alkalinity real DEFAULT 0, as_hco3 boolean DEFAULT true, sparge_ro real DEFAULT 0, mash_ro real DEFAULT 0)
*/
//
