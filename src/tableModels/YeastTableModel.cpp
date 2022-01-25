/*======================================================================================================================
 * tableModels/YeastTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2022:
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
#include "tableModels/YeastTableModel.h"

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

#include "database/ObjectStoreWrapper.h"
#include "MainWindow.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "model/Inventory.h"
#include "model/Recipe.h"
#include "model/Yeast.h"
#include "PersistentSettings.h"
#include "utils/BtStringConst.h"

YeastTableModel::YeastTableModel(QTableView * parent, bool editable) :
   BtTableModel{
      parent,
      editable,
      {{YEASTNAMECOL,      {tr("Name"),       NonPhysicalQuantity::String,          ""      }},
       {YEASTLABCOL,       {tr("Laboratory"), NonPhysicalQuantity::String,          ""      }},
       {YEASTPRODIDCOL,    {tr("Product ID"), NonPhysicalQuantity::String,          ""      }},
       {YEASTTYPECOL,      {tr("Type"),       NonPhysicalQuantity::String,          ""      }},
       {YEASTFORMCOL,      {tr("Form"),       NonPhysicalQuantity::String,          ""      }},
       {YEASTAMOUNTCOL,    {tr("Amount"),     Measurement::PhysicalQuantity::Mixed, "amount"}},
       {YEASTINVENTORYCOL, {tr("Inventory"),  NonPhysicalQuantity::Count,           ""      }}}
   },
   _inventoryEditable(false),
   recObs(nullptr) {

   yeastObs.clear();
   setObjectName("yeastTableModel");

   QHeaderView * headerView = parentTableWidget->horizontalHeader();
   headerView->setContextMenuPolicy(Qt::CustomContextMenu);
   parentTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   parentTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   parentTableWidget->setWordWrap(false);

   connect(headerView, &QWidget::customContextMenuRequested, this, &YeastTableModel::contextMenu);
   connect(&ObjectStoreTyped<InventoryYeast>::getInstance(), &ObjectStoreTyped<InventoryYeast>::signalPropertyChanged,
           this, &YeastTableModel::changedInventory);
   return;
}

YeastTableModel::~YeastTableModel() = default;

void YeastTableModel::addYeast(int yeastId) {
   Yeast * yeast = ObjectStoreWrapper::getByIdRaw<Yeast>(yeastId);

   if (this->yeastObs.contains(yeast)) {
      return;
   }

   // If we are observing the database, ensure that the item is undeleted and
   // fit to display.
   if (recObs == nullptr && (yeast->deleted() || !yeast->display())) {
      return;
   }
   int size = yeastObs.size();
   beginInsertRows(QModelIndex(), size, size);
   yeastObs.append(yeast);
   connect(yeast, &NamedEntity::changed, this, &YeastTableModel::changed);
   //reset(); // Tell everybody that the table has changed.
   endInsertRows();
}

void YeastTableModel::observeRecipe(Recipe * rec) {
   if (recObs) {
      disconnect(recObs, nullptr, this, nullptr);
      removeAll();
   }

   recObs = rec;
   if (recObs) {
      connect(recObs, &NamedEntity::changed, this, &YeastTableModel::changed);
      addYeasts(recObs->yeasts());
   }
}

void YeastTableModel::observeDatabase(bool val) {
   if (val) {
      observeRecipe(nullptr);

      removeAll();
      connect(&ObjectStoreTyped<Yeast>::getInstance(),
              &ObjectStoreTyped<Yeast>::signalObjectInserted,
              this,
              &YeastTableModel::addYeast);
      connect(&ObjectStoreTyped<Yeast>::getInstance(),
              &ObjectStoreTyped<Yeast>::signalObjectDeleted,
              this,
              &YeastTableModel::removeYeast);
      addYeasts(ObjectStoreTyped<Yeast>::getInstance().getAllRaw());
   } else {
      removeAll();
      disconnect(&ObjectStoreTyped<Yeast>::getInstance(), nullptr, this, nullptr);
   }
}

void YeastTableModel::addYeasts(QList<Yeast *> yeasts) {
   QList<Yeast *>::iterator i;
   QList<Yeast *> tmp;

   for (i = yeasts.begin(); i != yeasts.end(); i++) {
      if (recObs == nullptr && ((*i)->deleted() || !(*i)->display())) {
         continue;
      }

      if (!yeastObs.contains(*i)) {
         tmp.append(*i);
      }
   }

   int size = yeastObs.size();
   if (size + tmp.size()) {
      beginInsertRows(QModelIndex(), size, size + tmp.size() - 1);
      yeastObs.append(tmp);

      for (i = tmp.begin(); i != tmp.end(); i++) {
         connect(*i, &NamedEntity::changed, this, &YeastTableModel::changed);
      }

      endInsertRows();
   }
}

void YeastTableModel::removeYeast(int yeastId, std::shared_ptr<QObject> object) {
   this->remove(std::static_pointer_cast<Yeast>(object).get());
   return;
}

void YeastTableModel::remove(Yeast * yeast) {

   int i = yeastObs.indexOf(yeast);

   if (i >= 0) {
      beginRemoveRows(QModelIndex(), i, i);
      disconnect(yeast, nullptr, this, nullptr);
      yeastObs.removeAt(i);
      //reset(); // Tell everybody the table has changed.
      endRemoveRows();
   }
}

void YeastTableModel::removeAll() {
   if (yeastObs.size()) {
      beginRemoveRows(QModelIndex(), 0, yeastObs.size() - 1);
      while (!yeastObs.isEmpty()) {
         disconnect(yeastObs.takeLast(), nullptr, this, nullptr);
      }
      endRemoveRows();
   }
}

void YeastTableModel::changedInventory(int invKey, BtStringConst const & propertyName) {
   if (propertyName == PropertyNames::Inventory::amount) {
      for (int i = 0; i < yeastObs.size(); ++i) {
         Yeast * holdmybeer = yeastObs.at(i);
         if (invKey == holdmybeer->inventoryId()) {
            emit dataChanged(QAbstractItemModel::createIndex(i, YEASTINVENTORYCOL),
                             QAbstractItemModel::createIndex(i, YEASTINVENTORYCOL));
         }
      }
   }
   return;
}

void YeastTableModel::changed(QMetaProperty prop, QVariant /*val*/) {
   int i;

   // Find the notifier in the list
   Yeast * yeastSender = qobject_cast<Yeast *>(sender());
   if (yeastSender) {
      i = yeastObs.indexOf(yeastSender);
      if (i < 0) {
         return;
      }

      emit dataChanged(QAbstractItemModel::createIndex(i, 0),
                       QAbstractItemModel::createIndex(i, YEASTNUMCOLS - 1));
      return;
   }

   // See if sender is our recipe.
   Recipe * recSender = qobject_cast<Recipe *>(sender());
   if (recSender && recSender == recObs) {
      if (QString(prop.name()) == PropertyNames::Recipe::yeastIds) {
         removeAll();
         addYeasts(recObs->yeasts());
      }
      if (rowCount() > 0) {
         emit headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
      }
      return;
   }
}

int YeastTableModel::rowCount(const QModelIndex & /*parent*/) const {
   return yeastObs.size();
}

QVariant YeastTableModel::data(QModelIndex const & index, int role) const {
   // Ensure the row is ok.
   if (index.row() >= static_cast<int>(this->yeastObs.size())) {
      qWarning() << Q_FUNC_INFO << "Bad model index. row = " << index.row();
      return QVariant();
   }

   Yeast * row = yeastObs[index.row()];

   int const column = index.column();
   switch (column) {
      case YEASTNAMECOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->name());
         }
         break;
      case YEASTTYPECOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->typeStringTr());
         }
         if (role == Qt::UserRole) {
            return QVariant(row->type());
         }
         break;
      case YEASTLABCOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->laboratory());
         }
         break;
      case YEASTPRODIDCOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->productID());
         }
         break;
      case YEASTFORMCOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->formStringTr());
         }
         if (role == Qt::UserRole) {
            return QVariant(row->form());
         }
         break;
      case YEASTINVENTORYCOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->inventory());
         }
         break;
      case YEASTAMOUNTCOL:
         if (role == Qt::DisplayRole) {
            return QVariant(
               Measurement::displayAmount(
                  row->amount(),
                  row->amountIsWeight() ? &Measurement::Units::kilograms : &Measurement::Units::liters,
                  3,
                  this->getForcedSystemOfMeasurementForColumn(column),
                  std::nullopt
               )
            );
         }
         break;
      default :
         qWarning() << Q_FUNC_INFO << "Bad column: " << column;
         break;
   }
   return QVariant();
}

QVariant YeastTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
      return this->getColumName(section);
   }
   return QVariant();
}

Qt::ItemFlags YeastTableModel::flags(const QModelIndex & index) const {
   int col = index.column();
   switch (col) {
      case YEASTNAMECOL:
         return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
      case YEASTINVENTORYCOL:
         return (Qt::ItemIsEnabled | (_inventoryEditable ? Qt::ItemIsEditable : Qt::NoItemFlags));
      default:
         return Qt::ItemIsSelectable | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled |
                Qt::ItemIsEnabled;
   }
}

bool YeastTableModel::setData(QModelIndex const & index, QVariant const & value, int role) {
   if (index.row() >= static_cast<int>(this->yeastObs.size()) || role != Qt::EditRole) {
      return false;
   }

   Yeast * row = this->yeastObs[index.row()];

   int const column = index.column();
   switch (column) {
      case YEASTNAMECOL:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(*row,
                                               PropertyNames::NamedEntity::name,
                                               value.toString(),
                                               tr("Change Yeast Name"));
         break;
      case YEASTLABCOL:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(*row,
                                               PropertyNames::Yeast::laboratory,
                                               value.toString(),
                                               tr("Change Yeast Laboratory"));
         break;
      case YEASTPRODIDCOL:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(*row,
                                               PropertyNames::Yeast::productID,
                                               value.toString(),
                                               tr("Change Yeast Product ID"));
         break;
      case YEASTTYPECOL:
         if (!value.canConvert(QVariant::Int)) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(*row,
                                               PropertyNames::Yeast::type,
                                               static_cast<Yeast::Type>(value.toInt()),
                                               tr("Change Yeast Type"));
         break;
      case YEASTFORMCOL:
         if (!value.canConvert(QVariant::Int)) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(*row,
                                               PropertyNames::Yeast::form,
                                               static_cast<Yeast::Form>(value.toInt()),
                                               tr("Change Yeast Form"));
         break;
      case YEASTINVENTORYCOL:
         if (!value.canConvert(QVariant::Int)) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(*row,
                                               PropertyNames::NamedEntityWithInventory::inventory,
                                               value.toInt(),
                                               tr("Change Yeast Inventory Unit Size"));
         break;
      case YEASTAMOUNTCOL:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }

         MainWindow::instance().doOrRedoUpdate(
            *row,
            PropertyNames::Yeast::amount,
            Measurement::qStringToSI(
               value.toString(),
               row->amountIsWeight() ? Measurement::PhysicalQuantity::Mass : Measurement::PhysicalQuantity::Volume,
               this->getForcedSystemOfMeasurementForColumn(column),
               this->getForcedRelativeScaleForColumn(column)
            ).quantity,
            tr("Change Yeast Amount")
         );
         break;
      default:
         qWarning() << Q_FUNC_INFO << "Bad column: " << column;
         return false;
   }
   return true;
}

Yeast * YeastTableModel::getYeast(unsigned int i) {
   return yeastObs[static_cast<int>(i)];
}


//==========================CLASS YeastItemDelegate===============================

YeastItemDelegate::YeastItemDelegate(QObject * parent)
   : QItemDelegate(parent) {
}

QWidget * YeastItemDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & /*option*/,
                                          const QModelIndex & index) const {
   int col = index.column();

   if (col == YEASTTYPECOL) {
      QComboBox * box = new QComboBox(parent);

      box->addItem(tr("Ale"));
      box->addItem(tr("Lager"));
      box->addItem(tr("Wheat"));
      box->addItem(tr("Wine"));
      box->addItem(tr("Champagne"));
      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);

      return box;
   } else if (col == YEASTFORMCOL) {
      QComboBox * box = new QComboBox(parent);

      box->addItem(tr("Liquid"));
      box->addItem(tr("Dry"));
      box->addItem(tr("Slant"));
      box->addItem(tr("Culture"));
      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      return box;
   } else {
      return new QLineEdit(parent);
   }
}

void YeastItemDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const {
   int col = index.column();

   if (col == YEASTTYPECOL || col == YEASTFORMCOL) {
      QComboBox * box = qobject_cast<QComboBox *>(editor);
      int ndx = index.model()->data(index, Qt::UserRole).toInt();

      box->setCurrentIndex(ndx);
   } else {
      QLineEdit * line = qobject_cast<QLineEdit *>(editor);

      line->setText(index.model()->data(index, Qt::DisplayRole).toString());
   }

}

void YeastItemDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const {
   int col = index.column();

   if (col == YEASTTYPECOL || col == YEASTFORMCOL) {
      QComboBox * box = static_cast<QComboBox *>(editor);
      int ndx = box->currentIndex();
      int curr = model->data(index, Qt::UserRole).toInt();

      if (ndx != curr) {
         model->setData(index, ndx, Qt::EditRole);
      }
   } else {
      QLineEdit * line = static_cast<QLineEdit *>(editor);

      if (line->isModified()) {
         model->setData(index, line->text(), Qt::EditRole);
      }
   }
}

void YeastItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                             const QModelIndex & /*index*/) const {
   editor->setGeometry(option.rect);
}
