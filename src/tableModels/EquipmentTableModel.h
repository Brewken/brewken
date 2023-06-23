/*======================================================================================================================
 * tableModels/EquipmentTableModel.h is part of Brewken, and is copyright the following authors 2023:
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
 =====================================================================================================================*/
#ifndef TABLEMODELS_EQUIPMENTTABLEMODEL_H
#define TABLEMODELS_EQUIPMENTTABLEMODEL_H
#pragma once

#include <QItemDelegate>
#include <QMetaProperty>
#include <QModelIndex>
#include <QVariant>
#include <QWidget>

#include "model/Equipment.h"
#include "tableModels/ItemDelegate.h"
#include "tableModels/TableModelBase.h"

class BtStringConst;
class Recipe;

// You have to get the order of everything right with traits classes, but the end result is that we can refer to
// EquipmentTableModel::ColumnIndex::Type etc.
class EquipmentTableModel;
template <> struct TableModelTraits<EquipmentTableModel> {
   enum class ColumnIndex {
      Name           ,
      MashTunVolume  ,
      KettleVolume   ,
      FermenterVolume,
   };
};

/*!
 * \class EquipmentTableModel
 *
 * \brief Table model for a list of equipment records.
 */
class EquipmentTableModel : public BtTableModel, public TableModelBase<EquipmentTableModel, Equipment> {
   Q_OBJECT

   TABLE_MODEL_COMMON_DECL(Equipment)
};

//============================================== CLASS EquipmentItemDelegate ===============================================
/*!
 * \class EquipmentItemDelegate
 *
 * \brief Item delegate for equipment tables.
 * \sa EquipmentTableModel
 */
class EquipmentItemDelegate : public QItemDelegate, public ItemDelegate<EquipmentItemDelegate, EquipmentTableModel> {
   Q_OBJECT

   ITEM_DELEGATE_COMMON_DECL(Equipment)
};

#endif
