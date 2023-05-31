/*======================================================================================================================
 * tableModels/HopTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Luke Vincent <luke.r.vincent@gmail.com>
 *   • Markus Mårtensson <mackan.90@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
 *   • Tim Payne <swstim@gmail.com>
 *   • Théophane Martin <theophane.m@gmail.com>
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
#include "tableModels/HopTableModel.h"

#include <QAbstractItemModel>
#include <QComboBox>
#include <QHeaderView>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>
#include <QString>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QWidget>

#include "database/ObjectStoreWrapper.h"
#include "Localization.h"
#include "MainWindow.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "model/Inventory.h"
#include "model/Recipe.h"
#include "PersistentSettings.h"
#include "utils/BtStringConst.h"

HopTableModel::HopTableModel(QTableView * parent, bool editable) :
   BtTableModelInventory{
      parent,
      editable,
      {
         // Note that we have to use PropertyNames::NamedEntityWithInventory::inventoryWithUnits because
         // PropertyNames::NamedEntityWithInventory::inventory is not implemented
         SMART_COLUMN_HEADER_DEFN(HopTableModel, Name     , tr("Name"     ), Hop, PropertyNames::NamedEntity::name),
         SMART_COLUMN_HEADER_DEFN(HopTableModel, Alpha    , tr("Alpha %"  ), Hop, PropertyNames::Hop::alpha_pct   , PrecisionInfo{1}),
         SMART_COLUMN_HEADER_DEFN(HopTableModel, Amount   , tr("Amount"   ), Hop, PropertyNames::Hop::amount_kg   ),
         SMART_COLUMN_HEADER_DEFN(HopTableModel, Inventory, tr("Inventory"), Hop, PropertyNames::NamedEntityWithInventory::inventoryWithUnits   ),
         SMART_COLUMN_HEADER_DEFN(HopTableModel, Form     , tr("Form"     ), Hop, PropertyNames::Hop::form        , EnumInfo{Hop::formStringMapping, Hop::formDisplayNames}),
         SMART_COLUMN_HEADER_DEFN(HopTableModel, Use      , tr("Use"      ), Hop, PropertyNames::Hop::use         , EnumInfo{Hop::useStringMapping,  Hop::useDisplayNames }),
         SMART_COLUMN_HEADER_DEFN(HopTableModel, Time     , tr("Time"     ), Hop, PropertyNames::Hop::time_min    ),
      }
   },
   TableModelBase<HopTableModel, Hop>{},
   showIBUs(false) {
   this->rows.clear();
   this->setObjectName("hopTable");

   QHeaderView * headerView = parentTableWidget->horizontalHeader();
   connect(headerView, &QWidget::customContextMenuRequested, this, &HopTableModel::contextMenu);
   connect(&ObjectStoreTyped<InventoryHop>::getInstance(), &ObjectStoreTyped<InventoryHop>::signalPropertyChanged, this,
           &HopTableModel::changedInventory);
   return;
}

HopTableModel::~HopTableModel() = default;

void HopTableModel::added  ([[maybe_unused]] std::shared_ptr<Hop> item) { return; }
void HopTableModel::removed([[maybe_unused]] std::shared_ptr<Hop> item) { return; }
void HopTableModel::updateTotals()                                      { return; }

void HopTableModel::setShowIBUs(bool var) {
   showIBUs = var;
}

void HopTableModel::changedInventory(int invKey, BtStringConst const & propertyName) {
   if (propertyName == PropertyNames::Inventory::amount) {
      for (int ii = 0; ii < this->rows.size(); ++ii) {
         if (invKey == this->rows.at(ii)->inventoryId()) {
            emit dataChanged(QAbstractItemModel::createIndex(ii, static_cast<int>(HopTableModel::ColumnIndex::Inventory)),
                             QAbstractItemModel::createIndex(ii, static_cast<int>(HopTableModel::ColumnIndex::Inventory)));
         }
      }
   }
   return;
}

QVariant HopTableModel::data(const QModelIndex & index, int role) const {
   if (!this->isIndexOk(index)) {
      return QVariant();
   }

///   auto row = this->rows[index.row()];
   auto const columnIndex = static_cast<HopTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case HopTableModel::ColumnIndex::Name:
      case HopTableModel::ColumnIndex::Alpha:
      case HopTableModel::ColumnIndex::Use:
      case HopTableModel::ColumnIndex::Time:
      case HopTableModel::ColumnIndex::Form:
      case HopTableModel::ColumnIndex::Amount:
      case HopTableModel::ColumnIndex::Inventory:
         return this->readDataFromModel(index, role);

      // No default case as we want the compiler to warn us if we missed one
   }
   return QVariant();
}

QVariant HopTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
      return this->getColumnLabel(section);
   }
   if (showIBUs && recObs && orientation == Qt::Vertical && role == Qt::DisplayRole) {
      QList<double> ibus = recObs->IBUs();

      if (ibus.size() > section) {
         return QVariant(QString("%L1 IBU").arg(ibus.at(section), 0, 'f', 1));
      }
   }
   return QVariant();
}

Qt::ItemFlags HopTableModel::flags(const QModelIndex & index) const {
   auto const columnIndex = static_cast<HopTableModel::ColumnIndex>(index.column());
   if (columnIndex == HopTableModel::ColumnIndex::Name) {
      return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
   }
   if (columnIndex == HopTableModel::ColumnIndex::Inventory) {
      return Qt::ItemIsEnabled | (this->isInventoryEditable() ? Qt::ItemIsEditable : Qt::NoItemFlags);
   }
   return Qt::ItemIsSelectable |
          (this->editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
}

bool HopTableModel::setData(const QModelIndex & index, const QVariant & value, int role) {
   if (!this->isIndexOk(index)) {
      return false;
   }

   bool retVal = false;
///   auto row = this->rows[index.row()];
   auto const columnIndex = static_cast<HopTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case HopTableModel::ColumnIndex::Name:
      case HopTableModel::ColumnIndex::Alpha:
      case HopTableModel::ColumnIndex::Use:
      case HopTableModel::ColumnIndex::Form:
      case HopTableModel::ColumnIndex::Time:
      case HopTableModel::ColumnIndex::Amount:
         retVal = this->writeDataToModel(index, value, role);
         break;

      case HopTableModel::ColumnIndex::Inventory:
         retVal = this->writeDataToModel(index, value, role, Measurement::PhysicalQuantity::Mass);
         break;

      // No default case as we want the compiler to warn us if we missed one
   }

   if (retVal) {
      headerDataChanged(Qt::Vertical, index.row(), index.row());   // Need to re-show header (IBUs).
   }

   return retVal;
}

// Insert the boiler-plate stuff that we cannot do in TableModelBase
TABLE_MODEL_COMMON_CODE(Hop, hop)
//=============================================== CLASS HopItemDelegate ================================================

// Insert the boiler-plate stuff that we cannot do in ItemDelegate
ITEM_DELEGATE_COMMON_CODE(Hop)
