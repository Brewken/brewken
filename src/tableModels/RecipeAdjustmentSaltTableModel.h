/*======================================================================================================================
 * tableModels/RecipeAdjustmentSaltTableModel.h is part of Brewken, and is copyright the following authors 2009-2024:
 *   • Jeff Bailey <skydvr38@verizon.net>
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
#ifndef TABLEMODELS_RECIPEADJUSTMENTSALTTABLEMODEL_H
#define TABLEMODELS_RECIPEADJUSTMENTSALTTABLEMODEL_H
#pragma once

#include <QItemDelegate>
#include <QList>
#include <QMetaProperty>
#include <QModelIndex>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QWidget>

#include "measurement/Unit.h"
#include "model/RecipeAdjustmentSalt.h"
#include "model/Water.h"
#include "tableModels/BtTableModel.h"
#include "tableModels/ItemDelegate.h"
#include "tableModels/TableModelBase.h"

// Forward declarations.
class Mash;
class Recipe;

// You have to get the order of everything right with traits classes, but the end result is that we can refer to
// RecipeAdjustmentSaltTableModel::ColumnIndex::AddTo etc.
class RecipeAdjustmentSaltTableModel;
template <> struct TableModelTraits<RecipeAdjustmentSaltTableModel> {
   enum class ColumnIndex {
      Name   ,
      Type   ,
      Amount ,
      AmountType    ,
      TotalInventory    ,
      AddTo  ,
      PctAcid,
///      TotalInventoryType,
   };
};

/*!
 * \class RecipeAdjustmentSaltTableModel
 *
 * \brief Table model for salts.
 */
class RecipeAdjustmentSaltTableModel :
   public BtTableModelRecipeObserver,
   public TableModelBase<RecipeAdjustmentSaltTableModel, RecipeAdjustmentSalt> {
   Q_OBJECT

   TABLE_MODEL_COMMON_DECL(RecipeAdjustmentSalt)

public:
   double total_Ca()   const;
   double total_Cl()   const;
   double total_CO3()  const;
   double total_HCO3() const;
   double total_Mg()   const;
   double total_Na()   const;
   double total_SO4()  const;

   double total(Water::Ion ion) const;
   double total(Salt::Type type) const;
   double totalAcidWeight(Salt::Type type) const;

   void saveAndClose();

public slots:
   void catchSalt();

signals:
   void newTotals();

private:
///   double spargePct;
   double multiplier(RecipeAdjustmentSalt & salt) const;
};

//======================================= CLASS RecipeAdjustmentSaltItemDelegate =======================================

/*!
 * \brief An item delegate for RecipeAdjustmentSalt tables.
 * \sa RecipeAdjustmentSaltTableModel.
 */
class RecipeAdjustmentSaltItemDelegate :
   public QItemDelegate,
   public ItemDelegate<RecipeAdjustmentSaltItemDelegate, RecipeAdjustmentSaltTableModel> {
   Q_OBJECT

   ITEM_DELEGATE_COMMON_DECL(RecipeAdjustmentSalt)
};

#endif
