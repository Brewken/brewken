/*======================================================================================================================
 * tableModels/RecipeAdditionFermentableTableModel.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Markus Mårtensson <mackan.90@gmail.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
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
#ifndef TABLEMODELS_RECIPEADDITIONFERMENTABLETABLEMODEL_H
#define TABLEMODELS_RECIPEADDITIONFERMENTABLETABLEMODEL_H
#pragma once

#include <QItemDelegate>
#include <QMetaProperty>
#include <QModelIndex>
#include <QVariant>
#include <QWidget>

#include "model/RecipeAdditionFermentable.h"
#include "tableModels/BtTableModel.h"
#include "tableModels/ItemDelegate.h"
#include "tableModels/TableModelBase.h"

class BtStringConst;
class Recipe;

// You have to get the order of everything right with traits classes, but the end result is that we can refer to
// RecipeAdditionFermentableTableModel::ColumnIndex::Alpha etc.
class RecipeAdditionFermentableTableModel;
template <> struct TableModelTraits<RecipeAdditionFermentableTableModel> {
   enum class ColumnIndex {
      Name          ,
      Type          ,
      Yield         ,
      Color         ,
      Amount        ,
      AmountType    ,
      TotalInventory,
      Stage         ,
      Time          ,
   };
};

/*!
 * \class RecipeAdditionFermentableTableModel
 *
 * \brief Model class for a list of hop additions.
 *
 *        TBD: Maybe there is a way for this class and \c FermentableTableModel to share more code.
 */
class RecipeAdditionFermentableTableModel :
   public BtTableModelRecipeObserver,
   public TableModelBase<RecipeAdditionFermentableTableModel, RecipeAdditionFermentable> {
   Q_OBJECT

   TABLE_MODEL_COMMON_DECL(RecipeAdditionFermentable)

public:
   //! \brief True if you want to display percent of each grain in the row header.
   void setDisplayPercentages(bool var);

private:
   //! \brief Recalculate the total amount of grains in the model.
   void updateTotalGrains();

private:
   bool displayPercentages;

   // .:TODO:.:JSON:.  Now that fermentables can also be measured by volume, we need to rethink this
   double totalFermMass_kg;
};

//=============================================== CLASS RecipeAdditionFermentableItemDelegate ================================================

/**
 * \class RecipeAdditionFermentableItemDelegate
 *
 * \brief An item delegate for \c RecipeAdditionFermentableTableModel
 */
class RecipeAdditionFermentableItemDelegate :
   public QItemDelegate,
   public ItemDelegate<RecipeAdditionFermentableItemDelegate, RecipeAdditionFermentableTableModel> {
   Q_OBJECT

   ITEM_DELEGATE_COMMON_DECL(RecipeAdditionFermentable)
};

#endif
