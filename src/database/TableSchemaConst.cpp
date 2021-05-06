/**
 * database/TableSchemaConst.cpp is part of Brewken, and is copyright the following authors 2021:
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
#include "database/TableSchemaConst.h"

namespace DatabaseConstants {
   // These HAVE to be in the same order as they are listed in
   // DatabaseConstants::DbTableId
   QStringList const dbTableToName  = QStringList() <<
      QString("none") <<  // need to handle the NOTABLE index
      ktableSettings <<
      ktableEquipment <<
      ktableFermentable <<
      ktableHop <<
      ktableMisc <<
      ktableStyle <<
      ktableYeast <<
      ktableWater <<
      ktableMash <<
      ktableMashStep <<
//      ktableRecipe <<
      QString(DatabaseNames::Tables::recipe) <<
      ktableBrewnote <<
      ktableInstruction <<
      ktableSalt <<
   // Now for BT internal tables
      ktableBtEquipment <<
      ktableBtFermentable <<
      ktableBtHop <<
      ktableBtMisc <<
      ktableBtStyle <<
      ktableBtYeast <<
      ktableBtWater <<
   // Now the in_recipe tables
      ktableFermInRec <<
      ktableHopInRec <<
      ktableMiscInRec <<
      ktableWaterInRec <<
      ktableYeastInRec <<
      ktableInsInRec <<
      ktableSaltInRec <<
   // child tables next
      ktableEquipChildren <<
      ktableFermChildren <<
      ktableHopChildren <<
      ktableMiscChildren <<
      ktableRecChildren <<
      ktableStyleChildren <<
      ktableWaterChildren <<
      ktableYeastChildren <<
   // inventory tables last
      ktableFermInventory <<
      ktableHopInventory <<
      ktableMiscInventory <<
      ktableYeastInventory;
}
