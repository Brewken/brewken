/*======================================================================================================================
 * HopTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2021:
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
#include "HopTableModel.h"

#include <QAbstractItemModel>
#include <QAbstractTableModel>
#include <QComboBox>
#include <QHeaderView>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>
#include <QString>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QVector>
#include <QWidget>

#include "Brewken.h"
#include "database/ObjectStoreWrapper.h"
#include "MainWindow.h"
#include "model/Hop.h"
#include "model/Inventory.h"
#include "PersistentSettings.h"
#include "units/Unit.h"
#include "utils/BtStringConst.h"

HopTableModel::HopTableModel(QTableView * parent, bool editable) :
   QAbstractTableModel(parent),
   colFlags(HOPNUMCOLS),
   _inventoryEditable(false),
   recObs(nullptr),
   parentTableWidget(parent),
   showIBUs(false) {
   this->hopObs.clear();
   this->setObjectName("hopTable");

   for (int i = 0; i < HOPNUMCOLS; ++i) {
      if (i == HOPNAMECOL) {
         colFlags[i] = Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
      } else if (i == HOPINVENTORYCOL) {
         colFlags[i] = Qt::ItemIsEnabled;
      } else
         colFlags[i] = Qt::ItemIsSelectable | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled |
                       Qt::ItemIsEnabled;
   }

   QHeaderView * headerView = parentTableWidget->horizontalHeader();
   headerView->setContextMenuPolicy(Qt::CustomContextMenu);
   parentTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   parentTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   parentTableWidget->setWordWrap(false);

   connect(headerView, &QWidget::customContextMenuRequested, this, &HopTableModel::contextMenu);
   connect(&ObjectStoreTyped<InventoryHop>::getInstance(), &ObjectStoreTyped<InventoryHop>::signalPropertyChanged, this,
           &HopTableModel::changedInventory);
   return;
}

HopTableModel::~HopTableModel() {
   this->hopObs.clear();
}

void HopTableModel::observeRecipe(Recipe * rec) {
   if (this->recObs) {
      disconnect(this->recObs, nullptr, this, nullptr);
      removeAll();
   }

   this->recObs = rec;
   if (this->recObs) {
      connect(this->recObs, &NamedEntity::changed, this, &HopTableModel::changed);
      this->addHops(this->recObs->hops());
   }
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
              static_cast<bool (HopTableModel::*)(int)>
              (&HopTableModel::removeHop)); // static_cast is needed here because removeHop is overloaded
      this->addHops(ObjectStoreTyped<Hop>::getInstance().getAllRaw());
   } else {
      removeAll();
      disconnect(&ObjectStoreTyped<Hop>::getInstance(), nullptr, this, nullptr);

   }
}

/*void HopTableModel::addHop(Hop * hop) {
   if (hop == nullptr || hopObs.contains(hop)) {
      return;
   }

   // If we are observing the database, ensure that the item is undeleted and
   // fit to display.
   if (recObs == nullptr && (hop->deleted() || !hop->display())) {
      return;
   }

   int size = hopObs.size();
   beginInsertRows(QModelIndex(), size, size);
   hopObs.append(hop);
   connect(hop, SIGNAL(changed(QMetaProperty, QVariant)), this, SLOT(changed(QMetaProperty, QVariant)));
   //reset(); // Tell everybody that the table has changed.
   endInsertRows();
}*/
void HopTableModel::addHop(int hopId) {
   auto hopAdded = ObjectStoreTyped<Hop>::getInstance().getById(hopId);
   if (!hopAdded) {
      // Not sure this should ever happen in practice, but, if there ever is no hop with the specified ID, there's not
      // a lot we can do.
      qWarning() << Q_FUNC_INFO << "Received signal that Hop ID" << hopId << "added, but unable to retrieve the Hop";
      return;
   }

   // .:TODO:. For the moment at least, the rest of this class uses raw pointers, but would be good to refactor to use
   // shared pointers.
   Hop * hop = hopAdded.get();
   if (this->hopObs.contains(hop)) {
      return;
   }

   // If we are observing the database, ensure that the item is undeleted and
   // fit to display.
   if (recObs == nullptr && (hop->deleted() || !hop->display())) {
      return;
   }

   int size = hopObs.size();
   beginInsertRows(QModelIndex(), size, size);
   hopObs.append(hop);
   connect(hop, &NamedEntity::changed, this, &HopTableModel::changed);
   endInsertRows();
   return;
}

void HopTableModel::addHops(QList<Hop *> hops) {
   QList<Hop *> tmp;

   for (auto hop : hops) {
      if (recObs == nullptr && (hop->deleted() || !hop->display())) {
         continue;
      }
      if (!hopObs.contains(hop)) {
         tmp.append(hop);
      }
   }

   int size = hopObs.size();
   if (size + tmp.size()) {
      beginInsertRows(QModelIndex(), size, size + tmp.size() - 1);
      hopObs.append(tmp);

      for (auto hop : tmp) {
         connect(hop, &NamedEntity::changed, this, &HopTableModel::changed);
      }

      endInsertRows();
   }
   return;
}

bool HopTableModel::removeHop(Hop * hop) {
   int i = hopObs.indexOf(hop);
   if (i >= 0) {
      beginRemoveRows(QModelIndex(), i, i);
      disconnect(hop, nullptr, this, nullptr);
      hopObs.removeAt(i);
      //reset(); // Tell everybody the table has changed.
      endRemoveRows();

      return true;
   }

   return false;
}

bool HopTableModel::removeHop(int hopId) {
   auto match = std::find_if(this->hopObs.begin(),
                             this->hopObs.end(),
   [hopId](Hop * current) {
      return hopId == current->key();
   });
   if (match == this->hopObs.cend()) {
      // We didn't find the deleted Hop in our list
      return false;
   }
   return this->removeHop(*match);
}

void HopTableModel::setShowIBUs(bool var) {
   showIBUs = var;
}

void HopTableModel::removeAll() {
   if (hopObs.size()) {
      beginRemoveRows(QModelIndex(), 0, hopObs.size() - 1);
      while (!hopObs.isEmpty()) {
         disconnect(hopObs.takeLast(), nullptr, this, nullptr);
      }
      endRemoveRows();
   }
}

void HopTableModel::changedInventory(int invKey, BtStringConst const & propertyName) {
   if (propertyName == PropertyNames::Inventory::amount) {
///      double newAmount = ObjectStoreWrapper::getById<InventoryHop>()->getAmount();
      for (int i = 0; i < hopObs.size(); ++i) {
         Hop * holdmybeer = hopObs.at(i);

         if (invKey == holdmybeer->inventoryId()) {
/// No need to update amount as it's only stored in one place (the inventory object) now
///            holdmybeer->setInventoryAmount(newAmount);
            emit dataChanged(QAbstractItemModel::createIndex(i, HOPINVENTORYCOL),
                             QAbstractItemModel::createIndex(i, HOPINVENTORYCOL));
         }
      }
   }
   return;
}

void HopTableModel::changed(QMetaProperty prop, QVariant /*val*/) {
   int i;

   // Find the notifier in the list
   Hop * hopSender = qobject_cast<Hop *>(sender());
   if (hopSender) {
      i = hopObs.indexOf(hopSender);
      if (i < 0) {
         return;
      }

      emit dataChanged(QAbstractItemModel::createIndex(i, 0),
                       QAbstractItemModel::createIndex(i, HOPNUMCOLS - 1));
      emit headerDataChanged(Qt::Vertical, i, i);
      return;
   }

   // See if sender is our recipe.
   Recipe * recSender = qobject_cast<Recipe *>(sender());
   if (recSender && recSender == recObs) {
      if (QString(prop.name()) == PropertyNames::Recipe::hopIds) {
         removeAll();
         addHops(recObs->hops());
      }
      if (rowCount() > 0) {
         emit headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
      }
      return;
   }
}

int HopTableModel::rowCount(const QModelIndex & /*parent*/) const {
   return hopObs.size();
}

int HopTableModel::columnCount(const QModelIndex & /*parent*/) const {
   return HOPNUMCOLS;
}

QVariant HopTableModel::data(const QModelIndex & index, int role) const {
   Hop * row;
   int col = index.column();
   Unit::RelativeScale scale;
   Unit::unitDisplay unit;

   // Ensure the row is ok.
   if (index.row() >= static_cast<int>(hopObs.size())) {
      qWarning() << QString("Bad model index. row = %1").arg(index.row());
      return QVariant();
   } else {
      row = hopObs[index.row()];
   }

   switch (index.column()) {
      case HOPNAMECOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->name());
         } else {
            return QVariant();
         }
      case HOPALPHACOL:
         if (role == Qt::DisplayRole) {
            return QVariant(Brewken::displayAmount(row->alpha_pct(), nullptr));
         } else {
            return QVariant();
         }
      case HOPINVENTORYCOL:
         if (role != Qt::DisplayRole) {
            return QVariant();
         }
         unit = displayUnit(col);
         scale = displayScale(col);

         return QVariant(Brewken::displayAmount(row->inventory(), &Units::kilograms, 3, unit, scale));

      case HOPAMOUNTCOL:
         if (role != Qt::DisplayRole) {
            return QVariant();
         }
         unit = displayUnit(col);
         scale = displayScale(col);

         return QVariant(Brewken::displayAmount(row->amount_kg(), &Units::kilograms, 3, unit, scale));

      case HOPUSECOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->useStringTr());
         } else if (role == Qt::UserRole) {
            return QVariant(row->use());
         } else {
            return QVariant();
         }
      case HOPTIMECOL:
         if (role != Qt::DisplayRole) {
            return QVariant();
         }

         scale = displayScale(col);

         return QVariant(Brewken::displayAmount(row->time_min(), &Units::minutes, 3, Unit::noUnit, scale));
      case HOPFORMCOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->formStringTr());
         } else if (role == Qt::UserRole) {
            return QVariant(row->form());
         } else {
            return QVariant();
         }
      default :
         qWarning() << QString("HopTableModel::data Bad column: %1").arg(index.column());
         return QVariant();
   }
}

QVariant HopTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
      switch (section) {
         case HOPNAMECOL:
            return QVariant(tr("Name"));
         case HOPALPHACOL:
            return QVariant(tr("Alpha %"));
         case HOPINVENTORYCOL:
            return QVariant(tr("Inventory"));
         case HOPAMOUNTCOL:
            return QVariant(tr("Amount"));
         case HOPUSECOL:
            return QVariant(tr("Use"));
         case HOPTIMECOL:
            return QVariant(tr("Time"));
         case HOPFORMCOL:
            return QVariant(tr("Form"));
         default:
            qWarning() << QString("HopTableModel::headerdata Bad column: %1").arg(section);
            return QVariant();
      }
   } else if (showIBUs && recObs && orientation == Qt::Vertical && role == Qt::DisplayRole) {
      QList<double> ibus = recObs->IBUs();

      if (ibus.size() > section) {
         return QVariant(QString("%L1 IBU").arg(ibus.at(section), 0, 'f', 1));
      }
   }
   return QVariant();
}

Qt::ItemFlags HopTableModel::flags(const QModelIndex & index) const {
   int col = index.column();

   return colFlags[col];
}

bool HopTableModel::setData(const QModelIndex & index, const QVariant & value, int role) {
   Hop * row;
   bool retVal = false;
   double amt;

   if (index.row() >= static_cast<int>(hopObs.size()) || role != Qt::EditRole) {
      return false;
   }

   row = hopObs[index.row()];

   Unit::unitDisplay dspUnit = displayUnit(index.column());
   Unit::RelativeScale   dspScl  = displayScale(index.column());
   switch (index.column()) {
      case HOPNAMECOL:
         retVal = value.canConvert(QVariant::String);
         if (retVal) {
            Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                  PropertyNames::NamedEntity::name,
                                                  value.toString(),
                                                  tr("Change Hop Name"));
         }
         break;
      case HOPALPHACOL:
         retVal = value.canConvert(QVariant::Double);
         if (retVal) {
            amt = Brewken::toDouble(value.toString(), &retVal);
            if (! retVal) {
               qWarning() << QString("HopTableModel::setData() could not convert %1 to double").arg(value.toString());
            }
            Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                  PropertyNames::Hop::alpha_pct,
                                                  amt,
                                                  tr("Change Hop Alpha %"));
         }
         break;

      case HOPINVENTORYCOL:
         retVal = value.canConvert(QVariant::String);
         if (retVal) {
            Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                  PropertyNames::NamedEntityWithInventory::inventory,
                                                  Brewken::qStringToSI(value.toString(), &Units::kilograms, displayUnit(HOPINVENTORYCOL)),
                                                  tr("Change Hop Inventory Amount"));
         }
         break;
      case HOPAMOUNTCOL:
         retVal = value.canConvert(QVariant::String);
         if (retVal) {
            Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                  PropertyNames::Hop::amount_kg,
                                                  Brewken::qStringToSI(value.toString(), &Units::kilograms, dspUnit, dspScl),
                                                  tr("Change Hop Amount"));
         }
         break;
      case HOPUSECOL:
         retVal = value.canConvert(QVariant::Int);
         if (retVal) {
            Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                  PropertyNames::Hop::use,
                                                  static_cast<Hop::Use>(value.toInt()),
                                                  tr("Change Hop Use"));
         }
         break;
      case HOPFORMCOL:
         retVal = value.canConvert(QVariant::Int);
         if (retVal) {
            Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                  PropertyNames::Hop::form,
                                                  static_cast<Hop::Form>(value.toInt()),
                                                  tr("Change Hop Form"));
         }
         break;
      case HOPTIMECOL:
         retVal = value.canConvert(QVariant::String);
         if (retVal) {
            Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                  PropertyNames::Hop::time_min,
                                                  Brewken::qStringToSI(value.toString(), &Units::minutes, dspUnit, dspScl),
                                                  tr("Change Hop Time"));
         }
         break;
      default:
         qWarning() << QString("HopTableModel::setdata Bad column: %1").arg(index.column());
         return false;
   }
   if (retVal) {
      headerDataChanged(Qt::Vertical, index.row(), index.row());   // Need to re-show header (IBUs).
   }

   return retVal;
}

Unit::unitDisplay HopTableModel::displayUnit(int column) const {
   QString attribute = generateName(column);

   if (attribute.isEmpty()) {
      return Unit::noUnit;
   }

   return static_cast<Unit::unitDisplay>(PersistentSettings::value(attribute, QVariant(-1), this->objectName(),
                                                                   PersistentSettings::UNIT).toInt());
}

Unit::RelativeScale HopTableModel::displayScale(int column) const {
   QString attribute = generateName(column);

   if (attribute.isEmpty()) {
      return Unit::noScale;
   }

   return static_cast<Unit::RelativeScale>(PersistentSettings::value(attribute, QVariant(-1), this->objectName(),
                                                                 PersistentSettings::SCALE).toInt());
}

// We need to:
//   o clear the custom scale if set
//   o clear any custom unit from the rows
//      o which should have the side effect of clearing any scale
void HopTableModel::setDisplayUnit(int column, Unit::unitDisplay displayUnit) {
   // Hop* row; // disabled per-cell magic
   QString attribute = generateName(column);

   if (attribute.isEmpty()) {
      return;
   }

   PersistentSettings::insert(attribute, displayUnit, this->objectName(), PersistentSettings::UNIT);
   PersistentSettings::insert(attribute, Unit::noScale, this->objectName(), PersistentSettings::SCALE);

}

// Setting the scale should clear any cell-level scaling options
void HopTableModel::setDisplayScale(int column, Unit::RelativeScale displayScale) {
   // Fermentable* row; //disabled per-cell magic

   QString attribute = generateName(column);

   if (attribute.isEmpty()) {
      return;
   }

   PersistentSettings::insert(attribute, displayScale, this->objectName(), PersistentSettings::SCALE);

}

QString HopTableModel::generateName(int column) const {
   QString attribute;

   switch (column) {
      case HOPINVENTORYCOL:
         attribute = *PropertyNames::NamedEntityWithInventory::inventory;
         break;
      case HOPAMOUNTCOL:
         attribute = *PropertyNames::Hop::amount_kg;
         break;
      case HOPTIMECOL:
         attribute = *PropertyNames::Hop::time_min;
         break;
      default:
         attribute = "";
   }
   return attribute;
}

void HopTableModel::contextMenu(const QPoint & point) {
   QObject * calledBy = sender();
   QHeaderView * hView = qobject_cast<QHeaderView *>(calledBy);

   int selected = hView->logicalIndexAt(point);
   Unit::unitDisplay currentUnit;
   Unit::RelativeScale  currentScale;

   // Since we need to call generateVolumeMenu() two different ways, we need
   // to figure out the currentUnit and Scale here
   currentUnit  = displayUnit(selected);
   currentScale = displayScale(selected);

   QMenu * menu;
   QAction * invoked;

   switch (selected) {
      case HOPINVENTORYCOL:
      case HOPAMOUNTCOL:
         menu = Brewken::setupMassMenu(parentTableWidget, currentUnit, currentScale);
         break;
      case HOPTIMECOL:
         menu = Brewken::setupTimeMenu(parentTableWidget, currentScale);
         break;
      default:
         return;
   }

   invoked = menu->exec(hView->mapToGlobal(point));
   if (invoked == nullptr) {
      return;
   }

   QWidget * pMenu = invoked->parentWidget();
   if (selected != HOPTIMECOL && pMenu == menu) {
      setDisplayUnit(selected, static_cast<Unit::unitDisplay>(invoked->data().toInt()));
   } else {
      setDisplayScale(selected, static_cast<Unit::RelativeScale>(invoked->data().toInt()));
   }

}

// Returns null on failure.
Hop * HopTableModel::getHop(int i) {
   if (!(hopObs.isEmpty())) {
      if (i >= 0 && i < hopObs.size()) {
         return hopObs[i];
      }
   } else {
      qWarning() << QString("HopTableModel::getHop( %1/%2 )").arg(i).arg(hopObs.size());
   }
   return nullptr;
}

//==========================CLASS HopItemDelegate===============================

HopItemDelegate::HopItemDelegate(QObject * parent)
   : QItemDelegate(parent) {
}

QWidget * HopItemDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & /*option*/,
                                        const QModelIndex & index) const {
   if (index.column() == HOPUSECOL) {
      QComboBox * box = new QComboBox(parent);

      // NOTE: these need to be in the same order as the Hop::Use enum.
      box->addItem(tr("Mash"));
      box->addItem(tr("First Wort"));
      box->addItem(tr("Boil"));
      box->addItem(tr("Aroma"));
      box->addItem(tr("Dry Hop"));
      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);

      return box;
   } else if (index.column() == HOPFORMCOL) {
      QComboBox * box = new QComboBox(parent);

      box->addItem(tr("Leaf"));
      box->addItem(tr("Pellet"));
      box->addItem(tr("Plug"));
      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);

      return box;
   } else {
      return new QLineEdit(parent);
   }
}

void HopItemDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const {
   if (index.column() == HOPUSECOL) {
      QComboBox * box = static_cast<QComboBox *>(editor);
      int ndx = index.model()->data(index, Qt::UserRole).toInt();

      box->setCurrentIndex(ndx);
   } else if (index.column() == HOPFORMCOL) {
      QComboBox * box = static_cast<QComboBox *>(editor);
      int ndx = index.model()->data(index, Qt::UserRole).toInt();

      box->setCurrentIndex(ndx);
   } else {
      QLineEdit * line = static_cast<QLineEdit *>(editor);
      line->setText(index.model()->data(index, Qt::DisplayRole).toString());
   }
}

void HopItemDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const {
   if (index.column() == HOPUSECOL) {
      QComboBox * box = static_cast<QComboBox *>(editor);
      int value = box->currentIndex();
      int ndx = model->data(index, Qt::UserRole).toInt();

      if (value != ndx) {
         model->setData(index, value, Qt::EditRole);
      }
   } else if (index.column() == HOPFORMCOL) {
      QComboBox * box = static_cast<QComboBox *>(editor);
      int value = box->currentIndex();
      int ndx = model->data(index, Qt::UserRole).toInt();

      if (value != ndx) {
         model->setData(index, value, Qt::EditRole);
      }
   } else {
      QLineEdit * line = static_cast<QLineEdit *>(editor);
      if (line->isModified()) {
         model->setData(index, line->text(), Qt::EditRole);
      }
   }
}

void HopItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                           const QModelIndex & /*index*/) const {
   editor->setGeometry(option.rect);
}
