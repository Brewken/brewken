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
#include "model/Hop.h"
#include "model/Inventory.h"
#include "model/Recipe.h"
#include "PersistentSettings.h"
#include "utils/BtStringConst.h"

HopTableModel::HopTableModel(QTableView * parent, bool editable) :
   BtTableModelInventory{
      parent,
      editable,
      {
         SMART_COLUMN_HEADER_DEFN(HopTableModel, Name     , tr("Name"     ), Hop, PropertyNames::NamedEntity::name),
         SMART_COLUMN_HEADER_DEFN(HopTableModel, Alpha    , tr("Alpha %"  ), Hop, PropertyNames::Hop::alpha_pct   , PrecisionInfo{1}),
         SMART_COLUMN_HEADER_DEFN(HopTableModel, Amount   , tr("Amount"   ), Hop, PropertyNames::Hop::amount_kg   ),
         SMART_COLUMN_HEADER_DEFN(HopTableModel, Inventory, tr("Inventory"), Hop, PropertyNames::Hop::amount_kg   ), // No inventory property name
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
void HopTableModel::removedAll()                                        { return; }

void HopTableModel::observeRecipe(Recipe * rec) {
   if (this->recObs) {
      disconnect(this->recObs, nullptr, this, nullptr);
      this->removeAll();
   }

   this->recObs = rec;
   if (this->recObs) {
      connect(this->recObs, &NamedEntity::changed, this, &HopTableModel::changed);
      this->addHops(this->recObs->getAll<Hop>());
   }
   return;
}

void HopTableModel::observeDatabase(bool val) {
   if (val) {
      observeRecipe(nullptr);
      removeAll();
      connect(&ObjectStoreTyped<Hop>::getInstance(), &ObjectStoreTyped<Hop>::signalObjectInserted, this,
              &HopTableModel::addHop);
      connect(&ObjectStoreTyped<Hop>::getInstance(),
              &ObjectStoreTyped<Hop>::signalObjectDeleted,
              this,
              &HopTableModel::removeHop);
      this->addHops(ObjectStoreWrapper::getAll<Hop>());
   } else {
      removeAll();
      disconnect(&ObjectStoreTyped<Hop>::getInstance(), nullptr, this, nullptr);
   }
   return;
}

void HopTableModel::addHop(int hopId) {
   auto hopAdded = ObjectStoreTyped<Hop>::getInstance().getById(hopId);
   if (!hopAdded) {
      // Not sure this should ever happen in practice, but, if there ever is no hop with the specified ID, there's not
      // a lot we can do.
      qWarning() << Q_FUNC_INFO << "Received signal that Hop ID" << hopId << "added, but unable to retrieve the Hop";
      return;
   }

   if (this->rows.contains(hopAdded)) {
      return;
   }

   // If we are observing the database, ensure that the item is undeleted and
   // fit to display.
   if (this->recObs == nullptr && (hopAdded->deleted() || !hopAdded->display())) {
      return;
   }

   // If we are watching a Recipe and the new Hop does not belong to it then there is nothing for us to do
   if (this->recObs) {
      Recipe * recipeOfNewHop = hopAdded->getOwningRecipe();
      if (recipeOfNewHop && this->recObs->key() != recipeOfNewHop->key()) {
         qDebug() <<
            Q_FUNC_INFO << "Ignoring signal about new Hop #" << hopAdded->key() << "as it belongs to Recipe #" <<
            recipeOfNewHop->key() << "and we are watching Recipe #" << this->recObs->key();
         return;
      }
   }

   int size = this->rows.size();
   beginInsertRows(QModelIndex(), size, size);
   this->rows.append(hopAdded);
   connect(hopAdded.get(), &NamedEntity::changed, this, &HopTableModel::changed);
   endInsertRows();
   return;
}

void HopTableModel::addHops(QList< std::shared_ptr<Hop> > hops) {
   decltype(hops) tmp;

   for (auto hop : hops) {
      if (recObs == nullptr && (hop->deleted() || !hop->display())) {
         continue;
      }
      if (!this->rows.contains(hop)) {
         tmp.append(hop);
      }
   }

   int size = this->rows.size();
   if (size + tmp.size()) {
      beginInsertRows(QModelIndex(), size, size + tmp.size() - 1);
      this->rows.append(tmp);

      for (auto hop : tmp) {
         connect(hop.get(), &NamedEntity::changed, this, &HopTableModel::changed);
      }

      endInsertRows();
   }
   return;
}

void HopTableModel::removeHop([[maybe_unused]] int hopId,
                              std::shared_ptr<QObject> object) {
   this->remove(std::static_pointer_cast<Hop>(object));
   return;
}

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

void HopTableModel::changed(QMetaProperty prop, [[maybe_unused]] QVariant val) {

   // Find the notifier in the list
   Hop * hopSender = qobject_cast<Hop *>(sender());
   if (hopSender) {
      int ii = this->findIndexOf(hopSender);
      if (ii < 0) {
         return;
      }

      emit dataChanged(QAbstractItemModel::createIndex(ii, 0),
                       QAbstractItemModel::createIndex(ii, this->columnCount() - 1));
      emit headerDataChanged(Qt::Vertical, ii, ii);
      return;
   }

   // See if sender is our recipe.
   Recipe * recSender = qobject_cast<Recipe *>(sender());
   if (recSender && recSender == recObs) {
      if (QString(prop.name()) == PropertyNames::Recipe::hopIds) {
         removeAll();
         this->addHops(recObs->getAll<Hop>());
      }
      if (rowCount() > 0) {
         emit headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
      }
      return;
   }
}

int HopTableModel::rowCount(const QModelIndex & /*parent*/) const {
   return this->rows.size();
}

QVariant HopTableModel::data(const QModelIndex & index, int role) const {
   if (!this->isIndexOk(index)) {
      return QVariant();
   }

   auto row = this->rows[index.row()];
   auto const columnIndex = static_cast<HopTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case HopTableModel::ColumnIndex::Name:
      case HopTableModel::ColumnIndex::Alpha:
      case HopTableModel::ColumnIndex::Use:
      case HopTableModel::ColumnIndex::Time:
      case HopTableModel::ColumnIndex::Form:
      case HopTableModel::ColumnIndex::Amount:
         return this->readDataFromModel(index, role);

      case HopTableModel::ColumnIndex::Inventory:
         if (role == Qt::DisplayRole) {
            return QVariant(Measurement::displayAmount(Measurement::Amount{row->inventory(), Measurement::Units::kilograms},
                                                       3,
                                                       this->get_ColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
                                                       this->get_ColumnInfo(columnIndex).getForcedRelativeScale()));
         }
         break;
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
   auto row = this->rows[index.row()];
///   double amt;

   auto const columnIndex = static_cast<HopTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case HopTableModel::ColumnIndex::Name:
      case HopTableModel::ColumnIndex::Alpha:
      case HopTableModel::ColumnIndex::Use:
      case HopTableModel::ColumnIndex::Form:
      case HopTableModel::ColumnIndex::Time:
         return this->writeDataToModel(index, value, role);

      case HopTableModel::ColumnIndex::Inventory:
         retVal = value.canConvert(QVariant::String);
         if (retVal) {
            MainWindow::instance().doOrRedoUpdate(
               *row,
               TYPE_INFO(Hop, NamedEntityWithInventory, inventory),
               Measurement::qStringToSI(value.toString(),
                                        Measurement::PhysicalQuantity::Mass,
                                        this->get_ColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
                                        this->get_ColumnInfo(columnIndex).getForcedRelativeScale()).quantity(),
               tr("Change Hop Inventory Amount")
            );
         }
         break;

      case HopTableModel::ColumnIndex::Amount:
         retVal = value.canConvert(QVariant::String);
         if (retVal) {
            MainWindow::instance().doOrRedoUpdate(
               *row,
               TYPE_INFO(Hop, amount_kg),
               Measurement::qStringToSI(value.toString(),
                                        Measurement::PhysicalQuantity::Mass,
                                        this->get_ColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
                                        this->get_ColumnInfo(columnIndex).getForcedRelativeScale()).quantity(),
               tr("Change Hop Amount")
            );
         }
         break;

      // No default case as we want the compiler to warn us if we missed one
   }

   if (retVal) {
      headerDataChanged(Qt::Vertical, index.row(), index.row());   // Need to re-show header (IBUs).
   }

   return retVal;
}

//=============================================== CLASS HopItemDelegate ================================================

// Insert the boiler-plate stuff that we cannot do in ItemDelegate
ITEM_DELEGATE_COMMON_CODE(Hop)
