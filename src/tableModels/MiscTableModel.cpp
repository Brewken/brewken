/*======================================================================================================================
 * tableModels/MiscTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Daniel Pettersson <pettson81@gmail.com>
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
#include "tableModels/MiscTableModel.h"

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

MiscTableModel::MiscTableModel(QTableView* parent, bool editable) :
   BtTableModelInventory{
      parent,
      editable,
      {
       SMART_COLUMN_HEADER_DEFN(MiscTableModel, Name     , tr("Name"       ), Misc, PropertyNames::NamedEntity::name   ),
       SMART_COLUMN_HEADER_DEFN(MiscTableModel, Type     , tr("Type"       ), Misc, PropertyNames::Misc::type          ),
       SMART_COLUMN_HEADER_DEFN(MiscTableModel, Use      , tr("Use"        ), Misc, PropertyNames::Misc::use           ),
       SMART_COLUMN_HEADER_DEFN(MiscTableModel, Time     , tr("Time"       ), Misc, PropertyNames::Misc::time_min      ),
       SMART_COLUMN_HEADER_DEFN(MiscTableModel, Amount   , tr("Amount"     ), Misc, PropertyNames::Misc::amount        ),
       SMART_COLUMN_HEADER_DEFN(MiscTableModel, Inventory, tr("Inventory"  ), Misc, PropertyNames::Misc::amount        ), // No inventory property name
       SMART_COLUMN_HEADER_DEFN(MiscTableModel, IsWeight , tr("Amount Type"), Misc, PropertyNames::Misc::amountIsWeight),
      }
   },
   BtTableModelData<Misc>{} {
   this->rows.clear();
   setObjectName("miscTableModel");

   QHeaderView* headerView = parentTableWidget->horizontalHeader();
   connect(headerView, &QWidget::customContextMenuRequested, this, &MiscTableModel::contextMenu);
   connect(&ObjectStoreTyped<InventoryMisc>::getInstance(),
           &ObjectStoreTyped<InventoryMisc>::signalPropertyChanged,
           this,
           &MiscTableModel::changedInventory);
   return;
}

MiscTableModel::~MiscTableModel() = default;

BtTableModel::ColumnInfo const & MiscTableModel::getColumnInfo(MiscTableModel::ColumnIndex const columnIndex) const {
   return this->BtTableModel::getColumnInfo(static_cast<size_t>(columnIndex));
}

void MiscTableModel::observeRecipe(Recipe* rec) {
   if (this->recObs) {
      qDebug() << Q_FUNC_INFO << "Unwatching Recipe" << this->recObs;
      disconnect(this->recObs, nullptr, this, nullptr);
      removeAll();
   }

   this->recObs = rec;
   if (this->recObs) {
      qDebug() << Q_FUNC_INFO << "Watching Recipe" << this->recObs;
      connect(this->recObs, &NamedEntity::changed, this, &MiscTableModel::changed);
      this->addMiscs(this->recObs->getAll<Misc>());
   }
}

void MiscTableModel::observeDatabase(bool val) {
   if (val) {
      observeRecipe(nullptr);
      removeAll();
      connect(&ObjectStoreTyped<Misc>::getInstance(), &ObjectStoreTyped<Misc>::signalObjectInserted,  this, &MiscTableModel::addMisc);
      connect(&ObjectStoreTyped<Misc>::getInstance(), &ObjectStoreTyped<Misc>::signalObjectDeleted,   this, &MiscTableModel::removeMisc);
      this->addMiscs(ObjectStoreWrapper::getAll<Misc>());
   } else {
      removeAll();
      disconnect(&ObjectStoreTyped<Misc>::getInstance(), nullptr, this, nullptr );
   }
   return;
}

void MiscTableModel::addMisc(int miscId) {
   auto misc = ObjectStoreWrapper::getById<Misc>(miscId);

   if (this->rows.contains(misc) ) {
      return;
   }

   // If we are observing the database, ensure that the item is undeleted and
   // fit to display.
   if (recObs == nullptr && (misc->deleted() || !misc->display())) {
      return;
   }

   // If we are watching a Recipe and the new Misc does not belong to it then there is nothing for us to do
   if (this->recObs) {
      auto recipeOfNewMisc = misc->getOwningRecipe();
      if (recipeOfNewMisc && this->recObs->key() != recipeOfNewMisc->key()) {
         qDebug() <<
            Q_FUNC_INFO << "Ignoring signal about new Misc #" << misc->key() << "as it belongs to Recipe #" <<
            recipeOfNewMisc->key() << "and we are watching Recipe #" << this->recObs->key();
         return;
      }
   }

   int size = this->rows.size();
   beginInsertRows( QModelIndex(), size, size );
   this->rows.append(misc);
   connect(misc.get(), &NamedEntity::changed, this, &MiscTableModel::changed );
   //reset(); // Tell everybody that the table has changed.
   endInsertRows();
   return;
}

void MiscTableModel::addMiscs(QList<std::shared_ptr<Misc> > miscs) {
   auto tmp = this->removeDuplicates(miscs, this->recObs);

   int size = this->rows.size();
   if (size+tmp.size()) {
      beginInsertRows( QModelIndex(), size, size+tmp.size()-1 );
      this->rows.append(tmp);

      for (auto ii : tmp) {
         connect(ii.get(), &NamedEntity::changed, this, &MiscTableModel::changed);
      }

      endInsertRows();
   }
}

// Returns true when misc is successfully found and removed.
void MiscTableModel::removeMisc([[maybe_unused]] int miscId,
                                std::shared_ptr<QObject> object) {
   this->remove(std::static_pointer_cast<Misc>(object));
   return;
}

bool MiscTableModel::remove(std::shared_ptr<Misc> misc) {
   int i = this->rows.indexOf(misc);
   if (i >= 0 ) {
      beginRemoveRows( QModelIndex(), i, i );
      disconnect(misc.get(), nullptr, this, nullptr);
      this->rows.removeAt(i);
      //reset(); // Tell everybody the table has changed.
      endRemoveRows();

      return true;
   }

   return false;
}

void MiscTableModel::removeAll()
{
   if (this->rows.size())
   {
      beginRemoveRows( QModelIndex(), 0, this->rows.size()-1 );
      while( !this->rows.isEmpty() )
      {
         disconnect( this->rows.takeLast().get(), nullptr, this, nullptr );
      }
      endRemoveRows();
   }
}

int MiscTableModel::rowCount(const QModelIndex& /*parent*/) const {
   return this->rows.size();
}

QVariant MiscTableModel::data(QModelIndex const & index, int role) const {

   // Ensure the row is ok.
   if (index.row() >= static_cast<int>(this->rows.size())) {
      qWarning() << Q_FUNC_INFO << "Bad model index. row = " << index.row();
      return QVariant();
   }

   auto row = this->rows[index.row()];

   // Deal with the column and return the right data.
   auto const columnIndex = static_cast<MiscTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case MiscTableModel::ColumnIndex::Name:
         if (role == Qt::DisplayRole) {
            return QVariant(row->name());
         }
         break;
      case MiscTableModel::ColumnIndex::Type:
         if (role == Qt::DisplayRole) {
            return QVariant(Misc::typeDisplayNames[row->type()]);
         }
         if (role == Qt::UserRole) {
            return QVariant(static_cast<int>(row->type()));
         }
         break;
      case MiscTableModel::ColumnIndex::Use:
         if (role == Qt::DisplayRole) {
            return QVariant(Misc::useDisplayNames[row->use()]);
         }
         if (role == Qt::UserRole) {
            return QVariant::fromValue(Optional::toOptInt(row->use()));
         }
         return QVariant();
      case MiscTableModel::ColumnIndex::Time:
         if (role == Qt::DisplayRole) {
            return QVariant(Measurement::displayAmount(Measurement::Amount{row->time_min(), Measurement::Units::minutes},
                                                       3,
                                                       std::nullopt,
                                                       this->getColumnInfo(columnIndex).getForcedRelativeScale()));
         }
         break;
      case MiscTableModel::ColumnIndex::Inventory:
         if (role == Qt::DisplayRole) {
            return QVariant(
               Measurement::displayAmount(Measurement::Amount{
                                             row->inventory(),
                                             row->amountIsWeight() ? Measurement::Units::kilograms :
                                                                     Measurement::Units::liters
                                          },
                                          3,
                                          this->getColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
                                          std::nullopt)
            );
         }
         break;
      case MiscTableModel::ColumnIndex::Amount:
         if (role == Qt::DisplayRole) {
            return QVariant(
               Measurement::displayAmount(Measurement::Amount{
                                             row->amount(),
                                             row->amountIsWeight() ? Measurement::Units::kilograms :
                                                                     Measurement::Units::liters
                                          },
                                          3,
                                          this->getColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
                                          std::nullopt)
            );
         }
         break;
      case MiscTableModel::ColumnIndex::IsWeight:
         if (role == Qt::DisplayRole) {
            return QVariant(Measurement::descAmountIsWeight[static_cast<int>(row->amountIsWeight())]);
         }
         if (role == Qt::UserRole) {
            return QVariant(row->amountIsWeight());
         }
         break;
      default:
         qWarning() << Q_FUNC_INFO << "Bad model index. column = " << index.column();
         break;
   }
   return QVariant();
}

QVariant MiscTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
      return this->getColumnLabel(section);
   }
   return QVariant();
}

Qt::ItemFlags MiscTableModel::flags(QModelIndex const & index) const {
   Qt::ItemFlags const defaults = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
   auto const columnIndex = static_cast<MiscTableModel::ColumnIndex>(index.column());
   if (columnIndex == MiscTableModel::ColumnIndex::Name) {
      return defaults;
   }
   if (columnIndex == MiscTableModel::ColumnIndex::Inventory) {
      return (defaults | (this->isInventoryEditable() ? Qt::ItemIsEditable : Qt::NoItemFlags));
   }
   return defaults | (this->editable ? Qt::ItemIsEditable : Qt::NoItemFlags);
}

bool MiscTableModel::setData(QModelIndex const & index,
                             QVariant const & value,
                             [[maybe_unused]] int role) {

   if (index.row() >= static_cast<int>(this->rows.size())) {
      return false;
   }

   auto row = this->rows[index.row()];

   Measurement::PhysicalQuantity physicalQuantity =
      row->amountIsWeight() ? Measurement::PhysicalQuantity::Mass: Measurement::PhysicalQuantity::Volume;

   auto const columnIndex = static_cast<MiscTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case MiscTableModel::ColumnIndex::Name:
         if (value.canConvert(QVariant::String)) {
            MainWindow::instance().doOrRedoUpdate(*row,
                                                  TYPE_INFO(Misc, NamedEntity, name),
                                                  value.toString(),
                                                  tr("Change Misc Name"));
         } else {
            return false;
         }
         break;
      case MiscTableModel::ColumnIndex::Type:
         if (!value.canConvert(QVariant::Int)) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(
            new SimpleUndoableUpdate(*row,
                                     TYPE_INFO(Misc, type),
                                     value.toInt(),
                                     tr("Change Misc Type"))
         );
         break;
      case MiscTableModel::ColumnIndex::Use:
         if (!value.canConvert<std::optional<int>>()) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(
            new SimpleUndoableUpdate(*row,
                                     TYPE_INFO(Misc, use),
                                     value,
                                     tr("Change Misc Use"))
         );
         break;
      case MiscTableModel::ColumnIndex::Time:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(
            *row,
            TYPE_INFO(Misc, time_min),
            Measurement::qStringToSI(value.toString(),
                                     Measurement::PhysicalQuantity::Time,
                                     this->getColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
                                     this->getColumnInfo(columnIndex).getForcedRelativeScale()).quantity(),
            tr("Change Misc Time")
         );
         break;
      case MiscTableModel::ColumnIndex::Inventory:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(
            *row,
            TYPE_INFO(Misc, NamedEntityWithInventory, inventory),
            Measurement::qStringToSI(value.toString(),
                                     physicalQuantity,
                                     this->getColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
                                     this->getColumnInfo(columnIndex).getForcedRelativeScale()).quantity(),
            tr("Change Misc Inventory Amount")
         );
         break;
      case MiscTableModel::ColumnIndex::Amount:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(
            *row,
            TYPE_INFO(Misc, amount),
            Measurement::qStringToSI(value.toString(),
                                     physicalQuantity,
                                     this->getColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
                                     this->getColumnInfo(columnIndex).getForcedRelativeScale()).quantity(),
            tr("Change Misc Amount")
         );
         break;
      case MiscTableModel::ColumnIndex::IsWeight:
         if (!value.canConvert(QVariant::Bool)) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(*row,
                                               TYPE_INFO(Misc, amountIsWeight),
                                               value.toBool(),
                                               tr("Change Misc Amount Type"));
         break;
      default:
         return false;
   }

   emit dataChanged(index, index);
   return true;
}

void MiscTableModel::changedInventory(int invKey, BtStringConst const & propertyName) {
   if (propertyName == PropertyNames::Inventory::amount) {
      for (int ii = 0; ii < this->rows.size(); ++ii) {
         if (invKey == this->rows.at(ii)->inventoryId()) {
            emit dataChanged(QAbstractItemModel::createIndex(ii, static_cast<int>(MiscTableModel::ColumnIndex::Inventory)),
                             QAbstractItemModel::createIndex(ii, static_cast<int>(MiscTableModel::ColumnIndex::Inventory)));
         }
      }
   }
   return;
}

void MiscTableModel::changed(QMetaProperty prop, [[maybe_unused]] QVariant val) {
   Misc * miscSender = qobject_cast<Misc*>(sender());
   if (miscSender) {
      int ii = this->findIndexOf(miscSender);
      if (ii >= 0) {
         emit dataChanged(QAbstractItemModel::createIndex(ii, 0),
                          QAbstractItemModel::createIndex(ii, this->columnCount() - 1) );
      }
      return;
   }

   // See if sender is our recipe.
   Recipe* recSender = qobject_cast<Recipe*>(sender());
   if (recSender && recSender == this->recObs) {
      if (QString(prop.name()) == PropertyNames::Recipe::miscIds) {
         this->removeAll();
         this->addMiscs(this->recObs->getAll<Misc>());
      }
      if (rowCount() > 0) {
         emit headerDataChanged( Qt::Vertical, 0, rowCount()-1 );
      }
      return;
   }

   return;
}

//======================CLASS MiscItemDelegate===========================

MiscItemDelegate::MiscItemDelegate(QObject* parent) : QItemDelegate(parent) {
   return;
}

QWidget* MiscItemDelegate::createEditor(QWidget *parent,
                                        QStyleOptionViewItem const & /*option*/,
                                        QModelIndex const & index) const {
   auto const columnIndex = static_cast<MiscTableModel::ColumnIndex>(index.column());
   if (columnIndex == MiscTableModel::ColumnIndex::Type) {
      BtComboBox * typeBox = new BtComboBox(parent);
      BT_COMBO_BOX_INIT_NOMV(MiscItemDelegate::createEditor, typeBox, Misc, type);
      typeBox->setMinimumWidth(typeBox->minimumSizeHint().width());
      typeBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      return typeBox;
   }

   if (columnIndex == MiscTableModel::ColumnIndex::Use) {
      BtComboBox * useBox = new BtComboBox(parent);
      BT_COMBO_BOX_INIT_NOMV(MiscItemDelegate::createEditor, useBox, Misc, use);
      useBox->setMinimumWidth(useBox->minimumSizeHint().width());
      useBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      return useBox;
   }

   if (columnIndex == MiscTableModel::ColumnIndex::IsWeight) {
      QComboBox *box = new QComboBox(parent);

      box->addItem(tr("Weight"));
      box->addItem(tr("Volume"));
      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      return box;
   }

   return new QLineEdit(parent);
}

void MiscItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
   auto const columnIndex = static_cast<MiscTableModel::ColumnIndex>(index.column());

   if (columnIndex == MiscTableModel::ColumnIndex::Type ||
       columnIndex == MiscTableModel::ColumnIndex::Use) {
      BtComboBox * box = qobject_cast<BtComboBox *>(editor);
      if (!box) {
         return;
      }
      if (columnIndex == MiscTableModel::ColumnIndex::Type) {
         box->setValue(static_cast<Misc::Type>(index.model()->data(index, Qt::UserRole).toInt()));
      } else {
         box->setValue(Optional::fromOptInt<Misc::Use>(index.model()->data(index, Qt::UserRole).value<std::optional<int> >()));
      }
   } else if (columnIndex == MiscTableModel::ColumnIndex::IsWeight) {
      QComboBox* box = qobject_cast<QComboBox*>(editor);
      if (!box) {
         return;
      }
      box->setCurrentIndex(index.model()->data(index, Qt::UserRole).toInt());
   } else {
      QLineEdit* line = static_cast<QLineEdit*>(editor);

      line->setText(index.model()->data(index, Qt::DisplayRole).toString());
   }

   return;
}

void MiscItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
   auto const columnIndex = static_cast<MiscTableModel::ColumnIndex>(index.column());
///   ××× AND OBVIOUSLY THIS IS WRONG TOO!
   if (columnIndex == MiscTableModel::ColumnIndex::Type ||
       columnIndex == MiscTableModel::ColumnIndex::Use ||
       columnIndex == MiscTableModel::ColumnIndex::IsWeight) {
      QComboBox* box = static_cast<QComboBox*>(editor);
      int ndx = box->currentIndex();
      int curr = model->data(index, Qt::UserRole).toInt();

      if (curr != ndx) {
         model->setData(index, ndx, Qt::EditRole);
      }
   } else {
      QLineEdit* line = static_cast<QLineEdit*>(editor);

      if (line->isModified()) {
         model->setData(index, line->text(), Qt::EditRole);
      }
   }

   return;
}

void MiscItemDelegate::updateEditorGeometry(QWidget * editor,
                                            QStyleOptionViewItem const & option,
                                            [[maybe_unused]] QModelIndex const & index) const {
   editor->setGeometry(option.rect);
   return;
}
