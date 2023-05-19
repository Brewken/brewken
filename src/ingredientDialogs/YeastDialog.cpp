/*======================================================================================================================
 * ingredientDialogs/YeastDialog.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Daniel Pettersson <pettson81@gmail.com>
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
#include "ingredientDialogs/YeastDialog.h"

#include "editors/YeastEditor.h"
#include "YeastSortFilterProxyModel.h"
#include "MainWindow.h"
#include "tableModels/YeastTableModel.h"

YeastDialog::YeastDialog(MainWindow* parent) :
   QDialog(parent),
   IngredientDialog<Yeast,
                    YeastDialog,
                    YeastTableModel,
                    YeastSortFilterProxyModel,
                    YeastEditor>(parent) {
   return;
}

YeastDialog::~YeastDialog() = default;

// Insert the boiler-plate stuff that we cannot do in IngredientDialog
INGREDIENT_DIALOG_COMMON_CODE(YeastDialog)
