/*======================================================================================================================
 * tableModels/FermentableTableModel.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Matt Young <mfsy@yahoo.com>
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
#ifndef TABLEMODELS_FERMENTABLETABLEMODEL_H
#define TABLEMODELS_FERMENTABLETABLEMODEL_H
#pragma once

#include <memory>

#include <QAbstractItemDelegate>
#include <QItemDelegate>
#include <QList>
#include <QMetaProperty>
#include <QModelIndex>
#include <QVariant>
#include <QWidget>

#include "measurement/Unit.h"
#include "model/Fermentable.h"
#include "tableModels/BtTableModelInventory.h"
#include "tableModels/ItemDelegate.h"
#include "tableModels/TableModelBase.h"

// Forward declarations.
class BtStringConst;
class Recipe;

// You have to get the order of everything right with traits classes, but the end result is that we can refer to
// FermentableTableModel::ColumnIndex::Color etc.
class FermentableTableModel;
template <> struct TableModelTraits<FermentableTableModel> {
   enum class ColumnIndex {
      Name     ,
      Type     ,
      Amount   ,
      Inventory,
      IsWeight ,
      IsMashed ,
      AfterBoil,
      Yield    ,
      Color    ,
   };
};

/*!
 * \class FermentableTableModel
 *
 * \brief A table model for a list of fermentables.
 */
class FermentableTableModel : public BtTableModelInventory, public TableModelBase<FermentableTableModel, Fermentable> {
   Q_OBJECT

   TABLE_MODEL_COMMON_DECL(Fermentable)

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

// Check that concepts are working!
static_assert(ObservesRecipe<FermentableTableModel>);
static_assert(HasInventory<FermentableTableModel>);

//=========================================== CLASS FermentableItemDelegate ============================================

/*!
 * \brief An item delegate for Fermentable tables.
 * \sa FermentableTableModel.
 */
class FermentableItemDelegate : public QItemDelegate,
                                public ItemDelegate<FermentableItemDelegate, FermentableTableModel> {
   Q_OBJECT

   ITEM_DELEGATE_COMMON_DECL(Fermentable)
};

#endif
