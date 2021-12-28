/*======================================================================================================================
 * tableModels/MiscTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2021:
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
#include "model/Misc.h"
#include "model/Recipe.h"
#include "PersistentSettings.h"
#include "utils/BtStringConst.h"

MiscTableModel::MiscTableModel(QTableView* parent, bool editable) :
   BtTableModel{parent, editable},
   _inventoryEditable(false),
   recObs(nullptr),
   parentTableWidget(parent) {
   miscObs.clear();
   setObjectName("miscTableModel");

   QHeaderView* headerView = parentTableWidget->horizontalHeader();
   headerView->setContextMenuPolicy(Qt::CustomContextMenu);
   parentTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   parentTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   parentTableWidget->setWordWrap(false);

   connect(headerView, &QWidget::customContextMenuRequested, this, &MiscTableModel::contextMenu);
   connect(&ObjectStoreTyped<InventoryMisc>::getInstance(), &ObjectStoreTyped<InventoryMisc>::signalPropertyChanged, this, &MiscTableModel::changedInventory);
   return;
}

MiscTableModel::~MiscTableModel() = default;

void MiscTableModel::observeRecipe(Recipe* rec)
{
   if( recObs )
   {
      disconnect( recObs, nullptr, this, nullptr );
      removeAll();
   }

   recObs = rec;
   if( recObs )
   {
      connect( recObs, &NamedEntity::changed, this, &MiscTableModel::changed );
      addMiscs( recObs->miscs() );
   }
}

void MiscTableModel::observeDatabase(bool val) {
   if (val) {
      observeRecipe(nullptr);
      removeAll();
      connect(&ObjectStoreTyped<Misc>::getInstance(), &ObjectStoreTyped<Misc>::signalObjectInserted,  this, &MiscTableModel::addMisc);
      connect(&ObjectStoreTyped<Misc>::getInstance(), &ObjectStoreTyped<Misc>::signalObjectDeleted,   this, &MiscTableModel::removeMisc);
      addMiscs( ObjectStoreTyped<Misc>::getInstance().getAllRaw() );
   } else {
      removeAll();
      disconnect(&ObjectStoreTyped<Misc>::getInstance(), nullptr, this, nullptr );
   }
   return;
}

void MiscTableModel::addMisc(int miscId) {
   Misc* misc = ObjectStoreWrapper::getByIdRaw<Misc>(miscId);

   if( miscObs.contains(misc) ) {
      return;
   }

   // If we are observing the database, ensure that the item is undeleted and
   // fit to display.
   if(
      recObs == nullptr &&
      (
         misc->deleted() ||
         !misc->display()
      )
   ) {
      return;
   }

   int size = miscObs.size();
   beginInsertRows( QModelIndex(), size, size );
   miscObs.append(misc);
   connect( misc, &NamedEntity::changed, this, &MiscTableModel::changed );
   //reset(); // Tell everybody that the table has changed.
   endInsertRows();
   return;
}

void MiscTableModel::addMiscs(QList<Misc*> miscs)
{
   QList<Misc*>::iterator i;
   QList<Misc*> tmp;

   for( i = miscs.begin(); i != miscs.end(); i++ )
   {
      if( recObs == nullptr && ( (*i)->deleted() || !(*i)->display() ) )
         continue;
      if( !miscObs.contains(*i) )
         tmp.append(*i);
   }

   int size = miscObs.size();
   if (size+tmp.size())
   {
      beginInsertRows( QModelIndex(), size, size+tmp.size()-1 );
      miscObs.append(tmp);

      for( i = tmp.begin(); i != tmp.end(); i++ )
         connect( *i, &NamedEntity::changed, this, &MiscTableModel::changed );

      endInsertRows();
   }
}

// Returns true when misc is successfully found and removed.
void MiscTableModel::removeMisc(int miscId, std::shared_ptr<QObject> object) {
   this->remove(std::static_pointer_cast<Misc>(object).get());
   return;
}

bool MiscTableModel::remove(Misc * misc) {
   int i = miscObs.indexOf(misc);
   if( i >= 0 ) {
      beginRemoveRows( QModelIndex(), i, i );
      disconnect( misc, nullptr, this, nullptr );
      miscObs.removeAt(i);
      //reset(); // Tell everybody the table has changed.
      endRemoveRows();

      return true;
   }

   return false;
}

void MiscTableModel::removeAll()
{
   if (miscObs.size())
   {
      beginRemoveRows( QModelIndex(), 0, miscObs.size()-1 );
      while( !miscObs.isEmpty() )
      {
         disconnect( miscObs.takeLast(), nullptr, this, nullptr );
      }
      endRemoveRows();
   }
}

int MiscTableModel::rowCount(const QModelIndex& /*parent*/) const
{
   return miscObs.size();
}

int MiscTableModel::columnCount(const QModelIndex& /*parent*/) const
{
   return MISCNUMCOLS;
}

QVariant MiscTableModel::data(QModelIndex const & index, int role) const {

   // Ensure the row is ok.
   if (index.row() >= static_cast<int>(this->miscObs.size())) {
      qWarning() << Q_FUNC_INFO << "Bad model index. row = " << index.row();
      return QVariant();
   }

   Misc * row = this->miscObs[index.row()];

   // Deal with the column and return the right data.
   switch (index.column()) {
      case MISCNAMECOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->name());
         }
         break;
      case MISCTYPECOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->typeStringTr());
         }
         if (role == Qt::UserRole) {
            return QVariant(row->type());
         }
         break;
      case MISCUSECOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->useStringTr());
         }
         if (role == Qt::UserRole) {
            return QVariant(row->use());
         }
         return QVariant();
      case MISCTIMECOL:
         if (role == Qt::DisplayRole) {
            Measurement::UnitSystem::RelativeScale scale = this->displayScale(MISCTIMECOL);
            return QVariant( Measurement::displayAmount(row->time(), &Measurement::Units::minutes, 3, nullptr, scale) );
         }
         break;
      case MISCINVENTORYCOL:
         if (role == Qt::DisplayRole) {
            Measurement::UnitSystem const * unitSystem = this->displayUnitSystem(index.column());
            return QVariant( Measurement::displayAmount(row->inventory(), row->amountIsWeight()? &Measurement::Units::kilograms : &Measurement::Units::liters, 3, unitSystem, Measurement::UnitSystem::noScale )

            );
         }
         break;
      case MISCAMOUNTCOL:
         if (role == Qt::DisplayRole) {
            Measurement::UnitSystem const * unitSystem = this->displayUnitSystem(index.column());
            return QVariant(
               Measurement::displayAmount(row->amount(),
                                          row->amountIsWeight() ? &Measurement::Units::kilograms : &Measurement::Units::liters,
                                          3,
                                          unitSystem,
                                          Measurement::UnitSystem::noScale)
            );
         }
         break;
      case MISCISWEIGHT:
         if (role == Qt::DisplayRole) {
            return QVariant(row->amountTypeStringTr());
         }
         if (role == Qt::UserRole) {
            return QVariant(row->amountType());
         }
         break;
      default:
         qWarning() << QString("Bad model index. column = %1").arg(index.column());
         break;
   }
   return QVariant();
}

QVariant MiscTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
      switch (section) {
         case MISCNAMECOL:
            return QVariant(tr("Name"));
         case MISCTYPECOL:
            return QVariant(tr("Type"));
         case MISCUSECOL:
            return QVariant(tr("Use"));
         case MISCTIMECOL:
            return QVariant(tr("Time"));
         case MISCINVENTORYCOL:
            return QVariant(tr("Inventory"));
         case MISCAMOUNTCOL:
            return QVariant(tr("Amount"));
         case MISCISWEIGHT:
            return QVariant(tr("Amount Type"));
         default:
            return QVariant();
      }
   }

   return QVariant();
}

Qt::ItemFlags MiscTableModel::flags(QModelIndex const & index) const {
   int col = index.column();
   Qt::ItemFlags defaults = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
   switch (col) {
      case MISCNAMECOL:
         return defaults;
      case MISCINVENTORYCOL:
         return (defaults | (_inventoryEditable ? Qt::ItemIsEditable : Qt::NoItemFlags));
      default:
         return defaults | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags);
   }
}

bool MiscTableModel::setData(QModelIndex const & index, QVariant const & value, int role) {

   if (index.row() >= static_cast<int>(miscObs.size())) {
      return false;
   }

   Misc *row = miscObs[index.row()];

   int col = index.column();

   Measurement::PhysicalQuantity physicalQuantity =
      row->amountIsWeight() ? Measurement::PhysicalQuantity::Mass: Measurement::PhysicalQuantity::Volume;

   Measurement::UnitSystem const * dspUnitSystem = this->displayUnitSystem(index.column());
   Measurement::UnitSystem::RelativeScale   dspScl  = this->displayScale(index.column());

   switch (col )
   {
      case MISCNAMECOL:
         if (value.canConvert(QVariant::String)) {
            Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                  PropertyNames::NamedEntity::name,
                                                  value.toString(),
                                                  tr("Change Misc Name"));
         } else {
            return false;
         }
         break;
      case MISCTYPECOL:
         if (!value.canConvert(QVariant::Int)) {
            return false;
         }
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::Misc::type,
                                               static_cast<Misc::Type>(value.toInt()),
                                               tr("Change Misc Type"));
         break;
      case MISCUSECOL:
         if (!value.canConvert(QVariant::Int)) {
            return false;
         }
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::Misc::use,
                                               static_cast<Misc::Use>(value.toInt()),
                                               tr("Change Misc Use"));
         break;
      case MISCTIMECOL:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::Misc::time,
                                               Measurement::qStringToSI(value.toString(),
                                                                        Measurement::PhysicalQuantity::Time,
                                                                        dspUnitSystem,
                                                                        dspScl),
                                               tr("Change Misc Time"));
         break;
      case MISCINVENTORYCOL:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::NamedEntityWithInventory::inventory,
                                               Measurement::qStringToSI(value.toString(),
                                                                        physicalQuantity,
                                                                        dspUnitSystem,
                                                                        dspScl),
                                               tr("Change Misc Inventory Amount"));
         break;
      case MISCAMOUNTCOL:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::Misc::amount,
                                               Measurement::qStringToSI(value.toString(),
                                                                        physicalQuantity,
                                                                        dspUnitSystem,
                                                                        dspScl),
                                               tr("Change Misc Amount"));
         break;
      case MISCISWEIGHT:
         if (!value.canConvert(QVariant::Int)) {
            return false;
         }
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::Misc::amountType,
                                               static_cast<Misc::AmountType>(value.toInt()),
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
      for( int i = 0; i < miscObs.size(); ++i ) {
         Misc* holdmybeer = miscObs.at(i);

         if ( invKey == holdmybeer->inventoryId() ) {
            // No need to update amount as it's only stored in one place (the inventory object) now
            emit dataChanged( QAbstractItemModel::createIndex(i,MISCINVENTORYCOL),
                              QAbstractItemModel::createIndex(i,MISCINVENTORYCOL) );
         }
      }
   }
   return;
}

void MiscTableModel::changed(QMetaProperty prop, QVariant /*val*/) {
   Misc* miscSender = qobject_cast<Misc*>(sender());
   if( miscSender )
   {
      int i = miscObs.indexOf(miscSender);
      if( i < 0 )
         return;

      emit dataChanged( QAbstractItemModel::createIndex(i, 0),
                        QAbstractItemModel::createIndex(i, MISCNUMCOLS-1) );
      return;
   }

   // See if sender is our recipe.
   Recipe* recSender = qobject_cast<Recipe*>(sender());
   if( recSender && recSender == recObs ) {
      if( QString(prop.name()) == PropertyNames::Recipe::miscIds ) {
         removeAll();
         addMiscs( recObs->miscs() );
      }
      if( rowCount() > 0 ) {
         emit headerDataChanged( Qt::Vertical, 0, rowCount()-1 );
      }
      return;
   }

   return;
}

Misc * MiscTableModel::getMisc(unsigned int i) {
   return miscObs[static_cast<int>(i)];
}

/*
Measurement::Unit::unitDisplay MiscTableModel::displayUnit(int column) const
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return Measurement::Unit::noUnit;

   return static_cast<Measurement::Unit::unitDisplay>(PersistentSettings::value(attribute, Measurement::Unit::noUnit, this->objectName(), PersistentSettings::UNIT).toInt());
}

Measurement::UnitSystem::RelativeScale MiscTableModel::displayScale(int column) const
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return Measurement::UnitSystem::noScale;

   return static_cast<Measurement::UnitSystem::RelativeScale>(PersistentSettings::value(attribute, Measurement::UnitSystem::noScale, this->objectName(), PersistentSettings::SCALE).toInt());
}

// We need to:
//   o clear the custom scale if set
//   o clear any custom unit from the rows
//      o which should have the side effect of clearing any scale
void MiscTableModel::setDisplayUnit(int column, Measurement::Unit::unitDisplay displayUnit)
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return;

   PersistentSettings::insert(attribute,displayUnit,this->objectName(),PersistentSettings::UNIT);
   PersistentSettings::insert(attribute,Measurement::UnitSystem::noScale,this->objectName(),PersistentSettings::SCALE);

}

// Setting the scale should clear any cell-level scaling options
void MiscTableModel::setDisplayScale(int column, Measurement::UnitSystem::RelativeScale displayScale)
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return;

   PersistentSettings::insert(attribute,displayScale,this->objectName(),PersistentSettings::SCALE);

}*/

QString MiscTableModel::generateName(int column) const
{
   QString attribute;

   switch(column)
   {
      case MISCINVENTORYCOL:
         attribute = *PropertyNames::NamedEntityWithInventory::inventory;
         break;
      case MISCAMOUNTCOL:
         attribute = *PropertyNames::Misc::amount;
         break;
      case MISCTIMECOL:
         attribute = *PropertyNames::Misc::time;
         break;
      default:
         attribute = "";
   }
   return attribute;
}

void MiscTableModel::contextMenu(const QPoint &point) {
   QObject* calledBy = sender();
   QHeaderView* hView = qobject_cast<QHeaderView*>(calledBy);
   int selected = hView->logicalIndexAt(point);

   // Since we need to call generateVolumeMenu() two different ways, we need
   // to figure out the currentUnit and Scale here
   Measurement::UnitSystem const * currentUnitSystem  = this->displayUnitSystem(selected);
   Measurement::UnitSystem::RelativeScale currentScale = this->displayScale(selected);

   QMenu* menu;

   switch (selected) {
      case MISCINVENTORYCOL:
      case MISCAMOUNTCOL:
         menu = currentUnitSystem->createUnitSystemMenu(parentTableWidget, currentScale, false);
         break;
      case MISCTIMECOL:
         menu = currentUnitSystem->createUnitSystemMenu(parentTableWidget, currentScale);
         break;
      default:
         return;
   }

   this->doContextMenu(point, hView, menu, selected);
   return;
}

//======================CLASS MiscItemDelegate===========================

MiscItemDelegate::MiscItemDelegate(QObject* parent)
        : QItemDelegate(parent)
{
}

QWidget* MiscItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
   if( index.column() == MISCTYPECOL )
   {
      QComboBox *box = new QComboBox(parent);
      box->addItem(tr("Spice"));
      box->addItem(tr("Fining"));
      box->addItem(tr("Water Agent"));
      box->addItem(tr("Herb"));
      box->addItem(tr("Flavor"));
      box->addItem(tr("Other"));
      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      return box;
   }
   else if( index.column() == MISCUSECOL )
   {
      QComboBox *box = new QComboBox(parent);

      box->addItem(tr("Boil"));
      box->addItem(tr("Mash"));
      box->addItem(tr("Primary"));
      box->addItem(tr("Secondary"));
      box->addItem(tr("Bottling"));
      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      return box;
   }
   else if ( index.column() == MISCISWEIGHT )
   {
      QComboBox *box = new QComboBox(parent);

      box->addItem(tr("Weight"));
      box->addItem(tr("Volume"));
      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      return box;
   }
   else
      return new QLineEdit(parent);
}

void MiscItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
   int column = index.column();

   if( column == MISCTYPECOL || column == MISCUSECOL || column == MISCISWEIGHT)
   {
      QComboBox* box = qobject_cast<QComboBox*>(editor);
      if( box == nullptr )
         return;
      box->setCurrentIndex(index.model()->data(index, Qt::UserRole).toInt());
   }
   else
   {
      QLineEdit* line = static_cast<QLineEdit*>(editor);

      line->setText(index.model()->data(index, Qt::DisplayRole).toString());
   }
}

void MiscItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
   int column = index.column();
   if( column == MISCTYPECOL || column == MISCUSECOL || column == MISCISWEIGHT)
   {
      QComboBox* box = static_cast<QComboBox*>(editor);
      int ndx = box->currentIndex();
      int curr = model->data(index, Qt::UserRole).toInt();

      if ( curr != ndx )
         model->setData(index, ndx, Qt::EditRole);
   }
   else
   {
      QLineEdit* line = static_cast<QLineEdit*>(editor);

      if ( line->isModified() )
         model->setData(index, line->text(), Qt::EditRole);
   }
}

void MiscItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
   editor->setGeometry(option.rect);
}
