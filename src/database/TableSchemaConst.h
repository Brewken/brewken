/**
 * database/TableSchemaConst.h is part of Brewken, and is copyright the following authors 2019-2021:
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
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
#ifndef TABLESCHEMACONST_H
#define TABLESCHEMACONST_H

#include <QString>
#include <QStringList>

namespace DatabaseConstants {
   extern QStringList const dbTableToName;

   //! \brief The database tables. These are heavily used by the TableSchema and DatabaseSchema classes
   //! NOTE: If you add a table to this list, do NOT forget to update dbTableToName with the
   //! new name(s)
   enum DbTableId {
      //! None of the tables. 0
      NOTABLE,
      // Meta tables first
      SETTINGTABLE,

      // BeerXML tables next
      EQUIPTABLE,
      FERMTABLE,
      HOPTABLE,
      MISCTABLE,
      STYLETABLE,
      YEASTTABLE,
      WATERTABLE,
      MASHTABLE,
      MASHSTEPTABLE,
      RECTABLE,
      BREWNOTETABLE,
      INSTRUCTIONTABLE,
      SALTTABLE,

      // then the bt_* tables
      BT_EQUIPTABLE,
      BT_FERMTABLE,
      BT_HOPTABLE,
      BT_MISCTABLE,
      BT_STYLETABLE,
      BT_YEASTTABLE,
      BT_WATERTABLE,

      // then the *_in_recipe tables
      FERMINRECTABLE,
      HOPINRECTABLE,
      MISCINRECTABLE,
      WATERINRECTABLE,
      YEASTINRECTABLE,
      INSTINRECTABLE,
      SALTINRECTABLE,

      // then the child tables
      EQUIPCHILDTABLE,
      FERMCHILDTABLE,
      HOPCHILDTABLE,
      MISCCHILDTABLE,
      RECIPECHILDTABLE,
      STYLECHILDTABLE,
      WATERCHILDTABLE,
      YEASTCHILDTABLE,

      // finally the inventory tables
      FERMINVTABLE,
      HOPINVTABLE,
      MISCINVTABLE,
      YEASTINVTABLE
   };
}


// Define tables names first, because I need them first
static const QString ktableSettings("settings");
static const QString ktableEquipment("equipment");
static const QString ktableFermentable("fermentable");
static const QString ktableHop("hop");
static const QString ktableMisc("misc");
static const QString ktableStyle("style");
static const QString ktableYeast("yeast");
static const QString ktableWater("water");
static const QString ktableSalt("salt");
static const QString ktableMash("mash");
static const QString ktableMashStep("mashstep");
static const QString ktableBrewnote("brewnote");
static const QString ktableInstruction("instruction");
//static const QString ktableRecipe("recipe");
namespace DatabaseNames::Tables { static char const * const recipe = "recipe"; /* previously ktableRecipe */ }

// BT default tables
static const QString ktableBtEquipment("bt_equipment");
static const QString ktableBtFermentable("bt_fermentable");
static const QString ktableBtHop("bt_hop");
static const QString ktableBtMisc("bt_misc");
static const QString ktableBtStyle("bt_style");
static const QString ktableBtYeast("bt_yeast");
static const QString ktableBtWater("bt_water");

// In recipe tables
static const QString ktableFermInRec("fermentable_in_recipe");
static const QString ktableHopInRec("hop_in_recipe");
static const QString ktableMiscInRec("misc_in_recipe");
static const QString ktableWaterInRec("water_in_recipe");
static const QString ktableSaltInRec("salt_in_recipe");
static const QString ktableYeastInRec("yeast_in_recipe");
static const QString ktableInsInRec("instruction_in_recipe");

// Children tables
static const QString ktableEquipChildren("equipment_children");
static const QString ktableFermChildren("fermentable_children");
static const QString ktableHopChildren("hop_children");
static const QString ktableMiscChildren("misc_children");
static const QString ktableRecChildren("recipe_children");
static const QString ktableStyleChildren("style_children");
static const QString ktableWaterChildren("water_children");
static const QString ktableYeastChildren("yeast_children");

// Inventory tables
static const QString ktableFermInventory("fermentable_in_inventory");
static const QString ktableHopInventory("hop_in_inventory");
static const QString ktableMiscInventory("misc_in_inventory");
static const QString ktableYeastInventory("yeast_in_inventory");

// These properties are pretty consistent over all objects, so defined them
// once and be done with it
 static char const * const kpropLocked = "locked";
 static char const * const kpropInventory = "inventory";
 static char const * const kpropKey = "_key";

// needed by both mash and equipment

// needed by both misc and yeast

/// used by both hops and yeast

// used by recipe and yeast

// used by recipe and equipment

// A rename later, and both brewnotes and recipes use this

// I am not sure this makes sense, but it is consistent
 static char const * const kpropParentId = "parent_id";
 static char const * const kpropChildId = "child_id";

// water and mash

// properties for foreign keys. This is being done for consistency
// later. They are exactly the same as the kcol elements later, but
// it really got annoying trying to remember when to use kprop or kcol.
 static char const * const kpropRecipeId = "recipe_id";
 static char const * const kpropEquipmentId = "equipment_id";
 static char const * const kpropFermentableId = "fermentable_id";
 static char const * const kpropHopId = "hop_id";
 static char const * const kpropInstructionId = "instruction_id";
 static char const * const kpropInventoryId = "inventory_id";
 static char const * const kpropMashId = "mash_id";
 static char const * const kpropMiscId = "misc_id";
 static char const * const kpropStyleId = "style_id";
 static char const * const kpropWaterId = "water_id";
 static char const * const kpropSaltId = "salt_id";
 static char const * const kpropYeastId = "yeast_id";

// Same for these column names. Yes, but I think the consistency is better
// this way
static const QString kcolName("name");
static const QString kcolNotes("notes");
static const QString kcolDeleted("deleted");
static const QString kcolDisplay("display");
static const QString kcolFolder("folder");
static const QString kcolLocked("locked");
static const QString kcolInventory("amount");
static const QString kcolAmount("amount");
static const QString kcolOrigin("origin");
static const QString kcolUse("use");
static const QString kcolSubstitutes("substitutes");
static const QString kcolTime("time");
static const QString kcolKey("id");

static const QString kcolPH("ph");

// Child information. We were really consistent with our naming here, so all the
// *children tables will use these constants. The only thing that changes is
// the table it points at

static const QString kcolParentId("parent_id");
static const QString kcolChildId("child_id");

// _in_recipe key columns
static const QString kcolRecipeId("recipe_id");
static const QString kcolEquipmentId("equipment_id");
static const QString kcolFermentableId("fermentable_id");
static const QString kcolHopId("hop_id");
static const QString kcolInstructionId("instruction_id");
static const QString kcolInventoryId("inventory_id");
static const QString kcolMashId("mash_id");
static const QString kcolMiscId("misc_id");
static const QString kcolStyleId("style_id");
static const QString kcolWaterId("water_id");
static const QString kcolSaltId("salt_id");
static const QString kcolYeastId("yeast_id");

#endif
