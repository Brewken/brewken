/*======================================================================================================================
 * ingredientDialogs/HopDialog.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Luke Vincent <luke.r.vincent@gmail.com>
 *   • Markus Mårtensson <mackan.90@gmail.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
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
#include "ingredientDialogs/HopDialog.h"

#include "editors/HopEditor.h"
#include "HopSortFilterProxyModel.h"
#include "MainWindow.h"
#include "tableModels/HopTableModel.h"

HopDialog::HopDialog(MainWindow* parent) :
   QDialog(parent),
   IngredientDialog<Hop,
                    HopDialog,
                    HopTableModel,
                    HopSortFilterProxyModel,
                    HopEditor>(parent) {
   return;
}

HopDialog::~HopDialog() = default;

// Insert the boiler-plate stuff that we cannot do in IngredientDialog
INGREDIENT_DIALOG_COMMON_CODE(HopDialog)
