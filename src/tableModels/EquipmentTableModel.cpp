/*======================================================================================================================
 * tableModels/EquipmentTableModel.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "tableModels/EquipmentTableModel.h"

#include <QComboBox>
#include <QHeaderView>
#include <QLineEdit>

#include "database/ObjectStoreWrapper.h"
#include "MainWindow.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "model/Inventory.h"
#include "model/Recipe.h"
#include "PersistentSettings.h"
#include "utils/BtStringConst.h"
#include "widgets/BtComboBox.h"

EquipmentTableModel::EquipmentTableModel(QTableView* parent, bool editable) :
   BtTableModel{
      parent,
      editable,
      {
         SMART_COLUMN_HEADER_DEFN(EquipmentTableModel, Name           , tr("Name"            ), Equipment, PropertyNames::NamedEntity::name              ),
         SMART_COLUMN_HEADER_DEFN(EquipmentTableModel, MashTunVolume  , tr("Mash Tun Volume" ), Equipment, PropertyNames::Equipment::mashTunVolume_l     ),
         SMART_COLUMN_HEADER_DEFN(EquipmentTableModel, KettleVolume   , tr("Kettle Volume"   ), Equipment, PropertyNames::Equipment::kettleBoilSize_l    ),
         SMART_COLUMN_HEADER_DEFN(EquipmentTableModel, FermenterVolume, tr("Fermenter Volume"), Equipment, PropertyNames::Equipment::fermenterBatchSize_l),
      }
   },
   TableModelBase<EquipmentTableModel, Equipment>{} {
   this->rows.clear();
   setObjectName("equipmentTableModel");

   QHeaderView* headerView = m_parentTableWidget->horizontalHeader();
   connect(headerView, &QWidget::customContextMenuRequested, this, &EquipmentTableModel::contextMenu);
   return;
}

EquipmentTableModel::~EquipmentTableModel() = default;

void EquipmentTableModel::added  ([[maybe_unused]] std::shared_ptr<Equipment> item) { return; }
void EquipmentTableModel::removed([[maybe_unused]] std::shared_ptr<Equipment> item) { return; }
void EquipmentTableModel::updateTotals()                                            { return; }

QVariant EquipmentTableModel::data(QModelIndex const & index, int role) const {
   if (!this->isIndexOk(index)) {
      return QVariant();
   }

   auto row = this->rows[index.row()];

   // Deal with the column and return the right data.
   auto const columnIndex = static_cast<EquipmentTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case EquipmentTableModel::ColumnIndex::Name           :
      case EquipmentTableModel::ColumnIndex::MashTunVolume  :
      case EquipmentTableModel::ColumnIndex::KettleVolume   :
      case EquipmentTableModel::ColumnIndex::FermenterVolume:
         return this->readDataFromModel(index, role);

      // No default case as we want the compiler to warn us if we missed one
   }
   return QVariant();
}

QVariant EquipmentTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
      return this->getColumnLabel(section);
   }
   return QVariant();
}

Qt::ItemFlags EquipmentTableModel::flags(QModelIndex const & index) const {
   Qt::ItemFlags const defaults = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
   auto const columnIndex = static_cast<EquipmentTableModel::ColumnIndex>(index.column());
   if (columnIndex == EquipmentTableModel::ColumnIndex::Name) {
      return defaults;
   }
   return defaults | (this->m_editable ? Qt::ItemIsEditable : Qt::NoItemFlags);
}

bool EquipmentTableModel::setData(QModelIndex const & index,
                             QVariant const & value,
                             [[maybe_unused]] int role) {
   if (!this->isIndexOk(index)) {
      return false;
   }

   auto row = this->rows[index.row()];

   auto const columnIndex = static_cast<EquipmentTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case EquipmentTableModel::ColumnIndex::Name           :
      case EquipmentTableModel::ColumnIndex::MashTunVolume  :
      case EquipmentTableModel::ColumnIndex::KettleVolume   :
      case EquipmentTableModel::ColumnIndex::FermenterVolume:
         return this->writeDataToModel(index, value, role);

      // No default case as we want the compiler to warn us if we missed one
   }

   // Should be unreachable
   emit dataChanged(index, index);
   return true;
}

// Insert the boiler-plate stuff that we cannot do in TableModelBase
TABLE_MODEL_COMMON_CODE(Equipment, equipment, PropertyNames::Recipe::equipmentId)
//=============================================== CLASS EquipmentItemDelegate ===============================================

// Insert the boiler-plate stuff that we cannot do in ItemDelegate
ITEM_DELEGATE_COMMON_CODE(Equipment)
