/*======================================================================================================================
 * tableModels/YeastTableModel.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef TABLEMODELS_YEASTTABLEMODEL_H
#define TABLEMODELS_YEASTTABLEMODEL_H
#pragma once

#include <memory>

#include <QItemDelegate>
#include <QMetaProperty>
#include <QModelIndex>
#include <QVariant>
#include <QWidget>

#include "model/Yeast.h"
#include "tableModels/BtTableModelInventory.h"
#include "tableModels/ItemDelegate.h"
#include "tableModels/TableModelBase.h"

// Forward declarations.
class BtStringConst;
class Recipe;

// You have to get the order of everything right with traits classes, but the end result is that we can refer to
// YeastTableModel::ColumnIndex::Lab etc.
class YeastTableModel;
template <> struct TableModelTraits<YeastTableModel> {
   enum class ColumnIndex {
      Name     ,
      Lab      ,
      ProdId   ,
      Type     ,
      Form     ,
      Amount   ,
      Inventory,
   };
};

/*!
 * \class YeastTableModel
 *
 * \brief Table model for yeasts.
 */
class YeastTableModel : public BtTableModelInventory, public TableModelBase<YeastTableModel, Yeast> {
   Q_OBJECT

   TABLE_MODEL_COMMON_DECL(Yeast)
};

//============================================== CLASS YeastItemDelegate ===============================================

/*!
 * \class YeastItemDelegate
 *
 * \brief Item delegate for yeast tables.
 * \sa YeastTableModel
 */
class YeastItemDelegate : public QItemDelegate,
                          public ItemDelegate<YeastItemDelegate, YeastTableModel> {
   Q_OBJECT

   ITEM_DELEGATE_COMMON_DECL(Yeast)
};

#endif
