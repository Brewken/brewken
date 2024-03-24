/*======================================================================================================================
 * tableModels/SaltTableModel.h is part of Brewken, and is copyright the following authors 2009-2024:
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
#ifndef TABLEMODELS_SALTTABLEMODEL_H
#define TABLEMODELS_SALTTABLEMODEL_H
#pragma once

#include <QItemDelegate>
#include <QList>
#include <QMetaProperty>
#include <QModelIndex>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QWidget>

#include "measurement/Unit.h"
#include "model/InventorySalt.h"
#include "model/Salt.h"
#include "model/Water.h"
#include "tableModels/BtTableModel.h"
#include "tableModels/ItemDelegate.h"
#include "tableModels/TableModelBase.h"

// Forward declarations.
class Mash;
class Recipe;

// You have to get the order of everything right with traits classes, but the end result is that we can refer to
// SaltTableModel::ColumnIndex::AddTo etc.
class SaltTableModel;
template <> struct TableModelTraits<SaltTableModel> {
   enum class ColumnIndex {
      Name   ,
///      Amount ,
///      AddTo  ,
      PctAcid,
      TotalInventory    ,
      TotalInventoryType,
   };
};

/*!
 * \class SaltTableModel
 *
 * \brief Table model for salts.
 */
class SaltTableModel : public BtTableModel, public TableModelBase<SaltTableModel, Salt> {
   Q_OBJECT

   TABLE_MODEL_COMMON_DECL(Salt)

///public:
///   double total_Ca() const;
///   double total_Cl() const;
///   double total_CO3() const;
///   double total_HCO3() const;
///   double total_Mg() const;
///   double total_Na() const;
///   double total_SO4() const;
///
///   double total(Water::Ion ion) const;
///   double total( Salt::Type type ) const;
///   double totalAcidWeight(Salt::Type type) const;
///
///   void saveAndClose();
///
///public slots:
///   void catchSalt();
///
///signals:
///   void newTotals();
///
///private:
//////   double spargePct;
///   double multiplier(Salt & salt) const;
};

//=============================================== CLASS SaltItemDelegate ===============================================

/*!
 * \brief An item delegate for Salt tables.
 * \sa SaltTableModel.
 */
class SaltItemDelegate : public QItemDelegate,
                         public ItemDelegate<SaltItemDelegate, SaltTableModel> {
   Q_OBJECT

   ITEM_DELEGATE_COMMON_DECL(Salt)
};

#endif
