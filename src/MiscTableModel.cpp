/**
 * MiscTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2021:
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
 */
#include "MiscTableModel.h"

#include <QComboBox>
#include <QHeaderView>
#include <QLineEdit>

#include "Brewken.h"
#include "database/Database.h"
#include "MainWindow.h"
#include "model/Misc.h"
#include "model/Recipe.h"
#include "PersistentSettings.h"
#include "unit.h"

MiscTableModel::MiscTableModel(QTableView* parent, bool editable)
   : QAbstractTableModel(parent),
     editable(editable),
     _inventoryEditable(false),
     recObs(nullptr),
     parentTableWidget(parent)
{
   miscObs.clear();
   setObjectName("miscTableModel");

   QHeaderView* headerView = parentTableWidget->horizontalHeader();
   headerView->setContextMenuPolicy(Qt::CustomContextMenu);
   parentTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   parentTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   parentTableWidget->setWordWrap(false);

   connect(headerView, &QWidget::customContextMenuRequested, this, &MiscTableModel::contextMenu);
   connect( &(Database::instance()), &Database::changedInventory, this, &MiscTableModel::changedInventory );
}

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

void MiscTableModel::observeDatabase(bool val)
{
   if( val )
   {
      observeRecipe(nullptr);
      removeAll();
      connect( &(Database::instance()), &Database::newMiscSignal, this, &MiscTableModel::addMisc );
      connect( &(Database::instance()), SIGNAL(deletedSignal(Misc*)), this, SLOT(removeMisc(Misc*)) );
      addMiscs( Database::instance().miscs() );
   }
   else
   {
      removeAll();
      disconnect( &(Database::instance()), nullptr, this, nullptr );
   }
}

void MiscTableModel::addMisc(Misc* misc)
{
   if( miscObs.contains(misc) )
      return;
   // If we are observing the database, ensure that the item is undeleted and
   // fit to display.
   if(
      recObs == nullptr &&
      (
         misc->deleted() ||
         !misc->display()
      )
   )
      return;

   int size = miscObs.size();
   beginInsertRows( QModelIndex(), size, size );
   miscObs.append(misc);
   connect( misc, &NamedEntity::changed, this, &MiscTableModel::changed );
   //reset(); // Tell everybody that the table has changed.
   endInsertRows();
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
bool MiscTableModel::removeMisc(Misc* misc)
{
   int i;

   i = miscObs.indexOf(misc);
   if( i >= 0 )
   {
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

QVariant MiscTableModel::data( const QModelIndex& index, int role ) const
{
   Misc* row;
   Unit::unitDisplay unit;
   Unit::unitScale scale;

   // Ensure the row is ok.
   if( index.row() >= static_cast<int>(miscObs.size() ))
   {
      qWarning() << QString("Bad model index. row = %1").arg(index.row());
      return QVariant();
   }
   else
      row = miscObs[index.row()];

   // Deal with the column and return the right data.
   switch( index.column() )
   {
      case MISCNAMECOL:
         if( role == Qt::DisplayRole )
            return QVariant(row->name());
         else
            return QVariant();
      case MISCTYPECOL:
         if( role == Qt::DisplayRole )
            return QVariant(row->typeStringTr());
         else if( role == Qt::UserRole )
            return QVariant(row->type());
         else
            return QVariant();
      case MISCUSECOL:
         if( role == Qt::DisplayRole )
            return QVariant(row->useStringTr());
         else if( role == Qt::UserRole )
            return QVariant(row->use());
         else
            return QVariant();
      case MISCTIMECOL:
         if( role != Qt::DisplayRole )
            return QVariant();

         scale = displayScale(MISCTIMECOL);

         return QVariant( Brewken::displayAmount(row->time(), Units::minutes, 3, Unit::noUnit, scale) );
      case MISCINVENTORYCOL:
         if( role != Qt::DisplayRole )
            return QVariant();

         unit = displayUnit(index.column());
         return QVariant( Brewken::displayAmount(row->inventory(), row->amountIsWeight()? static_cast<Unit*>(Units::kilograms) : static_cast<Unit*>(Units::liters), 3, unit, Unit::noScale ) );
      case MISCAMOUNTCOL:
         if( role != Qt::DisplayRole )
            return QVariant();

         unit = displayUnit(index.column());
         return QVariant( Brewken::displayAmount(row->amount(), row->amountIsWeight()? static_cast<Unit*>(Units::kilograms) : static_cast<Unit*>(Units::liters), 3, unit, Unit::noScale ) );

      case MISCISWEIGHT:
         if( role == Qt::DisplayRole )
            return QVariant(row->amountTypeStringTr());
         else if( role == Qt::UserRole )
            return QVariant(row->amountType());
         else
            return QVariant();
      default:
         qWarning() << QString("Bad model index. column = %1").arg(index.column());
   }
   return QVariant();
}

QVariant MiscTableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
   if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
   {
      switch( section )
      {
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
   else
      return QVariant();
}

Qt::ItemFlags MiscTableModel::flags(const QModelIndex& index ) const
{
   int col = index.column();
   Qt::ItemFlags defaults = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
   switch( col )
   {
      case MISCNAMECOL:
         return defaults;
      case MISCINVENTORYCOL:
         return (defaults | (_inventoryEditable ? Qt::ItemIsEditable : Qt::NoItemFlags));
      default:
         return defaults | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags);
   }
}

bool MiscTableModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
   Misc *row;
   int col;
   Unit* unit;

   if( index.row() >= static_cast<int>(miscObs.size()) )
      return false;
   else
      row = miscObs[index.row()];

   col = index.column();
   unit = row->amountIsWeight() ? static_cast<Unit*>(Units::kilograms): static_cast<Unit*>(Units::liters);

   Unit::unitDisplay dspUnit = displayUnit(index.column());
   Unit::unitScale   dspScl  = displayScale(index.column());

   switch (col )
   {
      case MISCNAMECOL:
         if( value.canConvert(QVariant::String) )
         {
            Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                     PropertyNames::NamedEntity::name,
                                                     value.toString(),
                                                     tr("Change Misc Name"));
         }
         else
            return false;
         break;
      case MISCTYPECOL:
         if( ! value.canConvert(QVariant::Int) )
            return false;
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                   "type",
                                                  static_cast<Misc::Type>(value.toInt()),
                                                  tr("Change Misc Type"));
         break;
      case MISCUSECOL:
         if( ! value.canConvert(QVariant::Int) )
            return false;
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                   "use",
                                                   static_cast<Misc::Use>(value.toInt()),
                                                   tr("Change Misc Use"));
         break;
      case MISCTIMECOL:
         if( ! value.canConvert(QVariant::String) )
            return false;

         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                   PropertyNames::Misc::time,
                                                   Brewken::qStringToSI(value.toString(), Units::minutes, dspUnit, dspScl),
                                                   tr("Change Misc Time"));
         break;
      case MISCINVENTORYCOL:
         if( ! value.canConvert(QVariant::String) )
            return false;

         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                   "inventoryAmount",
                                                   Brewken::qStringToSI(value.toString(), unit, dspUnit,dspScl),
                                                   tr("Change Misc Inventory Amount"));
         break;
      case MISCAMOUNTCOL:
         if( ! value.canConvert(QVariant::String) )
            return false;

         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                   "amount",
                                                   Brewken::qStringToSI(value.toString(), unit, dspUnit,dspScl),
                                                   tr("Change Misc Amount"));
         break;
      case MISCISWEIGHT:
         if( ! value.canConvert(QVariant::Int) )
            return false;

         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                                   "amountType",
                                                   static_cast<Misc::AmountType>(value.toInt()),
                                                   tr("Change Misc Amount Type"));
         break;
      default:
         return false;
   }

   emit dataChanged( index, index );
   return true;
}

void MiscTableModel::changedInventory(DatabaseConstants::DbTableId table, int invKey, QVariant val)
{
   if ( table == DatabaseConstants::MISCTABLE ) {
      for( int i = 0; i < miscObs.size(); ++i ) {
         Misc* holdmybeer = miscObs.at(i);

         if ( invKey == holdmybeer->inventoryId() ) {
            holdmybeer->setCacheOnly(true);
            holdmybeer->setInventoryAmount(val.toDouble());
            holdmybeer->setCacheOnly(false);
            emit dataChanged( QAbstractItemModel::createIndex(i,MISCINVENTORYCOL),
                              QAbstractItemModel::createIndex(i,MISCINVENTORYCOL) );
         }
      }
   }
}
void MiscTableModel::changed(QMetaProperty prop, QVariant /*val*/)
{
   int i;

   Misc* miscSender = qobject_cast<Misc*>(sender());
   if( miscSender )
   {
      i = miscObs.indexOf(miscSender);
      if( i < 0 )
         return;

      emit dataChanged( QAbstractItemModel::createIndex(i, 0),
                        QAbstractItemModel::createIndex(i, MISCNUMCOLS-1) );
      return;
   }

   // See if sender is our recipe.
   Recipe* recSender = qobject_cast<Recipe*>(sender());
   if( recSender && recSender == recObs )
   {
      if( QString(prop.name()) == "miscs" )
      {
         removeAll();
         addMiscs( recObs->miscs() );
      }
      if( rowCount() > 0 )
         emit headerDataChanged( Qt::Vertical, 0, rowCount()-1 );
      return;
   }

   // See if sender is the database.
   if( sender() == &(Database::instance()) && QString(prop.name()) == "miscs" )
   {
      removeAll();
      addMiscs( Database::instance().miscs() );
      return;
   }
}

Misc* MiscTableModel::getMisc(unsigned int i)
{
   return miscObs[static_cast<int>(i)];
}

Unit::unitDisplay MiscTableModel::displayUnit(int column) const
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return Unit::noUnit;

   return static_cast<Unit::unitDisplay>(PersistentSettings::value(attribute, Unit::noUnit, this->objectName(), PersistentSettings::UNIT).toInt());
}

Unit::unitScale MiscTableModel::displayScale(int column) const
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return Unit::noScale;

   return static_cast<Unit::unitScale>(PersistentSettings::value(attribute, Unit::noScale, this->objectName(), PersistentSettings::SCALE).toInt());
}

// We need to:
//   o clear the custom scale if set
//   o clear any custom unit from the rows
//      o which should have the side effect of clearing any scale
void MiscTableModel::setDisplayUnit(int column, Unit::unitDisplay displayUnit)
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return;

   PersistentSettings::insert(attribute,displayUnit,this->objectName(),PersistentSettings::UNIT);
   PersistentSettings::insert(attribute,Unit::noScale,this->objectName(),PersistentSettings::SCALE);

}

// Setting the scale should clear any cell-level scaling options
void MiscTableModel::setDisplayScale(int column, Unit::unitScale displayScale)
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return;

   PersistentSettings::insert(attribute,displayScale,this->objectName(),PersistentSettings::SCALE);

}

QString MiscTableModel::generateName(int column) const
{
   QString attribute;

   switch(column)
   {
      case MISCINVENTORYCOL:
         attribute = "inventory";
         break;
      case MISCAMOUNTCOL:
         attribute = "amount";
         break;
      case MISCTIMECOL:
         attribute = PropertyNames::Misc::time;
         break;
      default:
         attribute = "";
   }
   return attribute;
}

void MiscTableModel::contextMenu(const QPoint &point)
{
   QObject* calledBy = sender();
   QHeaderView* hView = qobject_cast<QHeaderView*>(calledBy);

   int selected = hView->logicalIndexAt(point);
   Unit::unitDisplay currentUnit;
   Unit::unitScale  currentScale;

   // Since we need to call generateVolumeMenu() two different ways, we need
   // to figure out the currentUnit and Scale here

   currentUnit  = displayUnit(selected);
   currentScale = displayScale(selected);

   QMenu* menu;
   QAction* invoked;

   switch(selected)
   {
      case MISCINVENTORYCOL:
      case MISCAMOUNTCOL:
         menu = Brewken::setupMassMenu(parentTableWidget,currentUnit, currentScale, false);
         break;
      case MISCTIMECOL:
         menu = Brewken::setupTimeMenu(parentTableWidget,currentScale);
         break;
      default:
         return;
   }

   invoked = menu->exec(hView->mapToGlobal(point));
   if ( invoked == nullptr )
      return;

   if ( selected == MISCTIMECOL )
      setDisplayScale(selected,static_cast<Unit::unitScale>(invoked->data().toInt()));
   else
      setDisplayUnit(selected,static_cast<Unit::unitDisplay>(invoked->data().toInt()));
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
