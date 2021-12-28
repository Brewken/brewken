/*======================================================================================================================
 * tableModels/YeastTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2021:
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

YeastTableModel::YeastTableModel(QTableView* parent, bool editable) :
   BtTableModel{parent, editable},
   _inventoryEditable(false),
   parentTableWidget(parent),
   recObs(nullptr) {

   yeastObs.clear();
   setObjectName("yeastTableModel");

   QHeaderView* headerView = parentTableWidget->horizontalHeader();
   headerView->setContextMenuPolicy(Qt::CustomContextMenu);
   parentTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   parentTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   parentTableWidget->setWordWrap(false);

   connect(headerView, &QWidget::customContextMenuRequested, this, &YeastTableModel::contextMenu);
   connect(&ObjectStoreTyped<InventoryYeast>::getInstance(), &ObjectStoreTyped<InventoryYeast>::signalPropertyChanged, this, &YeastTableModel::changedInventory);
   return;
}

YeastTableModel::~YeastTableModel() = default;

void YeastTableModel::addYeast(int yeastId) {
   Yeast* yeast = ObjectStoreWrapper::getByIdRaw<Yeast>(yeastId);

   if (this->yeastObs.contains(yeast)) {
      return;
   }

   // If we are observing the database, ensure that the item is undeleted and
   // fit to display.
   if(
      recObs == nullptr &&
      (
         yeast->deleted() ||
         !yeast->display()
      )
   ) {
      return;
   }
   int size = yeastObs.size();
   beginInsertRows( QModelIndex(), size, size );
   yeastObs.append(yeast);
   connect( yeast, &NamedEntity::changed, this, &YeastTableModel::changed );
   //reset(); // Tell everybody that the table has changed.
   endInsertRows();
}

void YeastTableModel::observeRecipe(Recipe* rec)
{
   if( recObs )
   {
      disconnect( recObs, nullptr, this, nullptr );
      removeAll();
   }

   recObs = rec;
   if( recObs )
   {
      connect( recObs, &NamedEntity::changed, this, &YeastTableModel::changed );
      addYeasts( recObs->yeasts() );
   }
}

void YeastTableModel::observeDatabase(bool val) {
   if( val ) {
      observeRecipe(nullptr);

      removeAll();
      connect(&ObjectStoreTyped<Yeast>::getInstance(), &ObjectStoreTyped<Yeast>::signalObjectInserted, this, &YeastTableModel::addYeast);
      connect(&ObjectStoreTyped<Yeast>::getInstance(), &ObjectStoreTyped<Yeast>::signalObjectDeleted,  this, &YeastTableModel::removeYeast);
      addYeasts( ObjectStoreTyped<Yeast>::getInstance().getAllRaw() );
   } else {
      removeAll();
      disconnect(&ObjectStoreTyped<Yeast>::getInstance(), nullptr, this, nullptr);
   }
}

void YeastTableModel::addYeasts(QList<Yeast*> yeasts)
{
   QList<Yeast*>::iterator i;
   QList<Yeast*> tmp;

   for( i = yeasts.begin(); i != yeasts.end(); i++ )
   {
      if( recObs == nullptr && ( (*i)->deleted() || !(*i)->display() ) )
         continue;

      if( !yeastObs.contains(*i) )
         tmp.append(*i);
   }

   int size = yeastObs.size();
   if (size+tmp.size())
   {
      beginInsertRows( QModelIndex(), size, size+tmp.size()-1 );
      yeastObs.append(tmp);

      for( i = tmp.begin(); i != tmp.end(); i++ )
         connect( *i, &NamedEntity::changed, this, &YeastTableModel::changed );

      endInsertRows();
   }
}

void YeastTableModel::removeYeast(int yeastId, std::shared_ptr<QObject> object) {
   this->remove(std::static_pointer_cast<Yeast>(object).get());
   return;
}

void YeastTableModel::remove(Yeast * yeast) {

   int i = yeastObs.indexOf(yeast);

   if( i >= 0 )
   {
      beginRemoveRows( QModelIndex(), i, i );
      disconnect( yeast, nullptr, this, nullptr );
      yeastObs.removeAt(i);
      //reset(); // Tell everybody the table has changed.
      endRemoveRows();
   }
}

void YeastTableModel::removeAll()
{
   if (yeastObs.size())
   {
      beginRemoveRows( QModelIndex(), 0, yeastObs.size()-1 );
      while( !yeastObs.isEmpty() )
      {
         disconnect( yeastObs.takeLast(), nullptr, this, nullptr );
      }
      endRemoveRows();
   }
}

void YeastTableModel::changedInventory(int invKey, BtStringConst const & propertyName) {
   if (propertyName == PropertyNames::Inventory::amount) {
      for( int i = 0; i < yeastObs.size(); ++i ) {
         Yeast* holdmybeer = yeastObs.at(i);
         if ( invKey == holdmybeer->inventoryId() ) {
            emit dataChanged( QAbstractItemModel::createIndex(i,YEASTINVENTORYCOL),
                              QAbstractItemModel::createIndex(i,YEASTINVENTORYCOL) );
         }
      }
   }
   return;
}

void YeastTableModel::changed(QMetaProperty prop, QVariant /*val*/)
{
   int i;

   // Find the notifier in the list
   Yeast* yeastSender = qobject_cast<Yeast*>(sender());
   if( yeastSender )
   {
      i = yeastObs.indexOf(yeastSender);
      if( i < 0 )
         return;

      emit dataChanged( QAbstractItemModel::createIndex(i, 0),
                        QAbstractItemModel::createIndex(i, YEASTNUMCOLS-1));
      return;
   }

   // See if sender is our recipe.
   Recipe* recSender = qobject_cast<Recipe*>(sender());
   if( recSender && recSender == recObs )
   {
      if( QString(prop.name()) == PropertyNames::Recipe::yeastIds )
      {
         removeAll();
         addYeasts( recObs->yeasts() );
      }
      if( rowCount() > 0 )
         emit headerDataChanged( Qt::Vertical, 0, rowCount()-1 );
      return;
   }
}

int YeastTableModel::rowCount(const QModelIndex& /*parent*/) const
{
   return yeastObs.size();
}

int YeastTableModel::columnCount(const QModelIndex& /*parent*/) const
{
   return YEASTNUMCOLS;
}

QVariant YeastTableModel::data(QModelIndex const & index, int role) const {
   // Ensure the row is ok.
   if (index.row() >= static_cast<int>(this->yeastObs.size())) {
      qWarning() << Q_FUNC_INFO << "Bad model index. row = " << index.row();
      return QVariant();
   }

   Measurement::UnitSystem const * unitSystem;

   Yeast* row = yeastObs[index.row()];

   switch (index.column()) {
      case YEASTNAMECOL:
         if (role != Qt::DisplayRole) {
            return QVariant();
         }
         return QVariant(row->name());
      case YEASTTYPECOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->typeStringTr());
         }
         if (role == Qt::UserRole) {
            return QVariant(row->type());
         }
         return QVariant();
      case YEASTLABCOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->laboratory());
         }
         return QVariant();
      case YEASTPRODIDCOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->productID());
         }
         return QVariant();
      case YEASTFORMCOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->formStringTr());
         }
         if (role == Qt::UserRole) {
            return QVariant(row->form());
         }
         return QVariant();
      case YEASTINVENTORYCOL:
         if (role != Qt::DisplayRole) {
            return QVariant();
         }
         return QVariant(row->inventory());
      case YEASTAMOUNTCOL:
         if (role != Qt::DisplayRole) {
            return QVariant();
         }

         unitSystem = this->displayUnitSystem(index.column());
         return QVariant(Measurement::displayAmount(row->amount(),
                                                row->amountIsWeight() ? &Measurement::Units::kilograms : &Measurement::Units::liters,
                                                3,
                                                unitSystem,
                                                Measurement::UnitSystem::noScale));

      default :
         qWarning() << Q_FUNC_INFO << "Bad column: " << index.column();
         return QVariant();
   }
}

QVariant YeastTableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
   if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
   {
      switch( section )
      {
         case YEASTNAMECOL:
            return QVariant(tr("Name"));
         case YEASTTYPECOL:
            return QVariant(tr("Type"));
         case YEASTFORMCOL:
            return QVariant(tr("Form"));
         case YEASTINVENTORYCOL:
            return QVariant(tr("Inventory"));
         case YEASTAMOUNTCOL:
            return QVariant(tr("Amount"));
         case YEASTLABCOL:
             return QVariant(tr("Laboratory"));
         case YEASTPRODIDCOL:
             return QVariant(tr("Product ID"));
         default:
            qWarning() << tr("Bad column: %1").arg(section);
            return QVariant();
      }
   }
   else
      return QVariant();
}

Qt::ItemFlags YeastTableModel::flags(const QModelIndex& index ) const
{
   int col = index.column();
   switch(col)
   {
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
   Measurement::PhysicalQuantity physicalQuantity =
      row->amountIsWeight() ? Measurement::PhysicalQuantity::Mass : Measurement::PhysicalQuantity::Volume;

   Measurement::UnitSystem const * dspUnitSystem = this->displayUnitSystem(index.column());
   Measurement::UnitSystem::RelativeScale   dspScl  = this->displayScale(index.column());

   switch (index.column()) {
      case YEASTNAMECOL:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::NamedEntity::name,
                                               value.toString(),
                                               tr("Change Yeast Name"));
         break;
      case YEASTLABCOL:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::Yeast::laboratory,
                                               value.toString(),
                                               tr("Change Yeast Laboratory"));
         break;
      case YEASTPRODIDCOL:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::Yeast::productID,
                                               value.toString(),
                                               tr("Change Yeast Product ID"));
         break;
      case YEASTTYPECOL:
         if (!value.canConvert(QVariant::Int)) {
            return false;
         }
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::Yeast::type,
                                               static_cast<Yeast::Type>(value.toInt()),
                                               tr("Change Yeast Type"));
         break;
      case YEASTFORMCOL:
         if (!value.canConvert(QVariant::Int)) {
            return false;
         }
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::Yeast::form,
                                               static_cast<Yeast::Form>(value.toInt()),
                                               tr("Change Yeast Form"));
         break;
      case YEASTINVENTORYCOL:
         if (!value.canConvert(QVariant::Int)) {
            return false;
         }
         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::NamedEntityWithInventory::inventory,
                                               value.toInt(),
                                               tr("Change Yeast Inventory Unit Size")); // .:TBD:. MY 2020-12-11 Whilst it's admirably concise, I find "quanta" unclear, and I'm not sure it's that easy to translate either
         break;
      case YEASTAMOUNTCOL:
         if (!value.canConvert(QVariant::String)) {
            return false;
         }


         Brewken::mainWindow()->doOrRedoUpdate(*row,
                                               PropertyNames::Yeast::amount,
                                               Measurement::qStringToSI(value.toString(),
                                                                        physicalQuantity,
                                                                        dspUnitSystem,
                                                                        dspScl),
                                               tr("Change Yeast Amount"));
         break;

      default:
         qWarning() << Q_FUNC_INFO << "Bad column: " << index.column();
         return false;
   }
   return true;
}

Yeast * YeastTableModel::getYeast(unsigned int i) {
   return yeastObs[static_cast<int>(i)];
}
/*
Measurement::Unit::unitDisplay YeastTableModel::displayUnit(int column) const
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return Measurement::Unit::noUnit;

   return static_cast<Measurement::Unit::unitDisplay>(PersistentSettings::value(attribute, QVariant(-1), this->objectName(), PersistentSettings::UNIT).toInt());
}

Measurement::UnitSystem::RelativeScale YeastTableModel::displayScale(int column) const
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return Measurement::UnitSystem::noScale;

   return static_cast<Measurement::UnitSystem::RelativeScale>(PersistentSettings::value(attribute, QVariant(-1), this->objectName(), PersistentSettings::SCALE).toInt());
}*/

///// We need to:
/////   o clear the custom scale if set
/////   o clear any custom unit from the rows
/////      o which should have the side effect of clearing any scale
///void YeastTableModel::setDisplayUnit(int column, Measurement::Unit::unitDisplay displayUnit)
///{
///   // Yeast* row; // disabled per-cell magic
///   QString attribute = generateName(column);
///
///   if ( attribute.isEmpty() )
///      return;
///
///   PersistentSettings::insert(attribute,displayUnit,this->objectName(),PersistentSettings::UNIT);
///   PersistentSettings::insert(attribute,Measurement::UnitSystem::noScale,this->objectName(),PersistentSettings::SCALE);
///
///   /* Disabled cell-specific code
///   for (int i = 0; i < rowCount(); ++i )
///   {
///      row = getYeast(i);
///      row->setDisplayUnit(Measurement::Unit::noUnit);
///   }
///   */
///}
///
///// Setting the scale should clear any cell-level scaling options
///void YeastTableModel::setDisplayScale(int column, Measurement::UnitSystem::RelativeScale displayScale)
///{
///   // Yeast* row; //disabled per-cell magic
///
///   QString attribute = generateName(column);
///
///   if ( attribute.isEmpty() )
///      return;
///
///   PersistentSettings::insert(attribute,displayScale,this->objectName(),PersistentSettings::SCALE);
///
///   /* disabled cell-specific code
///   for (int i = 0; i < rowCount(); ++i )
///   {
///      row = getYeast(i);
///      row->setDisplayScale(Measurement::UnitSystem::noScale);
///   }
///   */
///}

QString YeastTableModel::generateName(int column) const {
   QString attribute;

   switch(column)
   {
      case YEASTAMOUNTCOL:
         attribute = "amount";
         break;
      default:
         attribute = "";
   }
   return attribute;
}

void YeastTableModel::contextMenu(QPoint const & point) {
   QObject* calledBy = sender();
   QHeaderView* hView = qobject_cast<QHeaderView*>(calledBy);
   int selected = hView->logicalIndexAt(point);

   Measurement::UnitSystem const * currentUnitSystem  = this->displayUnitSystem(selected);
   Measurement::UnitSystem::RelativeScale currentScale = this->displayScale(selected);

   QMenu* menu;

   switch (selected) {
      case YEASTAMOUNTCOL:
         menu = currentUnitSystem->createUnitSystemMenu(parentTableWidget, currentScale, false);
         break;
      default:
         return;
   }

   this->doContextMenu(point, hView, menu, selected);
   return;
}


//==========================CLASS YeastItemDelegate===============================

YeastItemDelegate::YeastItemDelegate(QObject* parent)
        : QItemDelegate(parent)
{
}

QWidget* YeastItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& /*option*/, const QModelIndex &index) const
{
   int col = index.column();

   if( col == YEASTTYPECOL )
   {
      QComboBox *box = new QComboBox(parent);

      box->addItem(tr("Ale"));
      box->addItem(tr("Lager"));
      box->addItem(tr("Wheat"));
      box->addItem(tr("Wine"));
      box->addItem(tr("Champagne"));
      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);

      return box;
   }
   else if( col == YEASTFORMCOL )
   {
      QComboBox *box = new QComboBox(parent);

      box->addItem(tr("Liquid"));
      box->addItem(tr("Dry"));
      box->addItem(tr("Slant"));
      box->addItem(tr("Culture"));
      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      return box;
   }
   else
      return new QLineEdit(parent);
}

void YeastItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
   int col = index.column();

   if( col == YEASTTYPECOL || col == YEASTFORMCOL )
   {
      QComboBox* box = qobject_cast<QComboBox*>(editor);
      int ndx = index.model()->data(index, Qt::UserRole).toInt();

      box->setCurrentIndex(ndx);
   }
   else
   {
      QLineEdit* line = qobject_cast<QLineEdit*>(editor);

      line->setText(index.model()->data(index, Qt::DisplayRole).toString());
   }

}

void YeastItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
   int col = index.column();

   if( col == YEASTTYPECOL || col == YEASTFORMCOL )
   {
      QComboBox* box = static_cast<QComboBox*>(editor);
      int ndx = box->currentIndex();
      int curr = model->data(index,Qt::UserRole).toInt();

      if ( ndx != curr )
         model->setData(index, ndx, Qt::EditRole);
   }
   else
   {
      QLineEdit* line = static_cast<QLineEdit*>(editor);

      if ( line->isModified() )
         model->setData(index, line->text(), Qt::EditRole);
   }
}

void YeastItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex& /*index*/) const
{
   editor->setGeometry(option.rect);
}
