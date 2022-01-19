/*======================================================================================================================
 * tableModels/FermentableTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2022:
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
#include "tableModels/FermentableTableModel.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractTableModel>
#include <QComboBox>
#include <QDebug>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QLineEdit>
#include <QListWidget>
#include <QModelIndex>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStyle>
#include <QVariant>
#include <QVector>
#include <QWidget>

#include "database/ObjectStoreWrapper.h"
#include "MainWindow.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "model/Fermentable.h"
#include "model/Inventory.h"
#include "model/Recipe.h"
#include "PersistentSettings.h"
#include "utils/BtStringConst.h"

//=====================CLASS FermentableTableModel==============================
FermentableTableModel::FermentableTableModel(QTableView* parent, bool editable) :
   BtTableModel{
      parent,
      editable,
      {{FERMNAMECOL,      {tr("Name"),      NonPhysicalQuantity::String,          ""            }},
       {FERMTYPECOL,      {tr("Type"),      NonPhysicalQuantity::String,          ""            }},
       {FERMAMOUNTCOL,    {tr("Amount"),    Measurement::PhysicalQuantity::Mass,  "amount_kg"   }},
       {FERMINVENTORYCOL, {tr("Inventory"), Measurement::PhysicalQuantity::Mass,  "inventory_kg"}},
       {FERMISMASHEDCOL,  {tr("Method"),    NonPhysicalQuantity::String,          ""            }},
       {FERMAFTERBOIL,    {tr("Addition"),  NonPhysicalQuantity::String,          ""            }},
       {FERMYIELDCOL,     {tr("Yield %"),   NonPhysicalQuantity::Percentage,      ""            }},
       {FERMCOLORCOL,     {tr("Color"),     Measurement::PhysicalQuantity::Color, "color_srm"   }}}
   },
   _inventoryEditable(false),
   recObs(nullptr),
   displayPercentages(false),
   totalFermMass_kg(0) {

   fermObs.clear();
   // for units and scales
   setObjectName("fermentableTable");

   // Will this work here? Yes. Yes it will. Bwahahahahahaha
   QHeaderView* headerView = parentTableWidget->horizontalHeader();
   headerView->setContextMenuPolicy(Qt::CustomContextMenu);
   parentTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   parentTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   parentTableWidget->setWordWrap(false);
   connect(headerView, &QWidget::customContextMenuRequested, this, &FermentableTableModel::contextMenu);
   connect(&ObjectStoreTyped<InventoryFermentable>::getInstance(), &ObjectStoreTyped<InventoryFermentable>::signalPropertyChanged, this, &FermentableTableModel::changedInventory);
   return;
}

FermentableTableModel::~FermentableTableModel() = default;

void FermentableTableModel::observeRecipe(Recipe* rec) {
   if (this->recObs) {
      qDebug() << Q_FUNC_INFO << "Unobserve Recipe #" << this->recObs->key() << "(" << this->recObs->name() << ")";
      disconnect(this->recObs, nullptr, this, nullptr);
      this->removeAll();
   }

   this->recObs = rec;
   if (this->recObs) {
      qDebug() << Q_FUNC_INFO << "Observe Recipe #" << this->recObs->key() << "(" << this->recObs->name() << ")";

      connect(this->recObs, &NamedEntity::changed, this, &FermentableTableModel::changed);
      this->addFermentables( recObs->fermentables() );
   }
   return;
}

void FermentableTableModel::observeDatabase(bool val) {
   if ( val ) {
      // Observing a database and a recipe are mutually exclusive.
      this->observeRecipe(nullptr);

      this->removeAll();
      connect(&ObjectStoreTyped<Fermentable>::getInstance(), &ObjectStoreTyped<Fermentable>::signalObjectInserted, this, &FermentableTableModel::addFermentable);
      connect(&ObjectStoreTyped<Fermentable>::getInstance(), &ObjectStoreTyped<Fermentable>::signalObjectDeleted,  this, &FermentableTableModel::removeFermentable);
      addFermentables( ObjectStoreTyped<Fermentable>::getInstance().getAllRaw() );
   } else {
      disconnect(&ObjectStoreTyped<Fermentable>::getInstance(), nullptr, this, nullptr);
      this->removeAll();
   }
}

void FermentableTableModel::addFermentable(int fermId) {
   Fermentable * ferm = ObjectStoreWrapper::getByIdRaw<Fermentable>(fermId);
   qDebug() << Q_FUNC_INFO << ferm->name();

   //Check to see if it's already in the list
   if( fermObs.contains(ferm) )
      return;
   // If we are observing the database, ensure that the ferm is undeleted and
   // fit to display.
   if( recObs == nullptr && ( ferm->deleted() || !ferm->display() ) )
      return;

   int size = fermObs.size();
   beginInsertRows( QModelIndex(), size, size );
   fermObs.append(ferm);
   connect( ferm, &NamedEntity::changed, this, &FermentableTableModel::changed );
   totalFermMass_kg += ferm->amount_kg();
   //reset(); // Tell everybody that the table has changed.
   endInsertRows();
   return;
}

void FermentableTableModel::addFermentables(QList<Fermentable*> ferms) {
   qDebug() << Q_FUNC_INFO << QString("Add up to %1 fermentables to existing list of %2").arg(ferms.size()).arg(this->fermObs.size());

   QList<Fermentable*> tmp;

   for (auto ii : ferms) {
//      qDebug() <<
//         Q_FUNC_INFO << "Fermentable #" << ii->key() << (ii->deleted() ? "" : "not") << "deleted, display=" << (ii->display() ? "on" : "off");
      if ( recObs == nullptr  && ( ii->deleted() || !ii->display() ) ) {

            continue;
      }
      if( !fermObs.contains(ii) ) {
         tmp.append(ii);
      }
   }

   qDebug() << Q_FUNC_INFO << QString("After de-duping, adding %1 fermentables").arg(tmp.size());

   int size = fermObs.size();
   if (size+tmp.size()) {
      beginInsertRows( QModelIndex(), size, size+tmp.size()-1 );
      fermObs.append(tmp);

      for(QList<Fermentable*>::iterator i = tmp.begin(); i != tmp.end(); i++ )
      {
         connect( *i, &NamedEntity::changed, this, &FermentableTableModel::changed );
         totalFermMass_kg += (*i)->amount_kg();
      }

      endInsertRows();
   }
}

void FermentableTableModel::removeFermentable(int fermId, std::shared_ptr<QObject> object) {
   this->remove(std::static_pointer_cast<Fermentable>(object).get());
   return;
}

bool FermentableTableModel::remove(Fermentable * ferm) {
   int i = fermObs.indexOf(ferm);
   if( i >= 0 )
   {
      beginRemoveRows( QModelIndex(), i, i );
      disconnect(ferm, nullptr, this, nullptr);
      fermObs.removeAt(i);

      totalFermMass_kg -= ferm->amount_kg();
      //reset(); // Tell everybody the table has changed.
      endRemoveRows();

      return true;
   }

   return false;
}

void FermentableTableModel::removeAll()
{
   if (fermObs.size())
   {
      beginRemoveRows( QModelIndex(), 0, fermObs.size()-1 );
      while( !fermObs.isEmpty() )
      {
         disconnect( fermObs.takeLast(), nullptr, this, nullptr );
      }
      endRemoveRows();
   }
   // I think we need to zero this out
   totalFermMass_kg = 0;
}

void FermentableTableModel::updateTotalGrains()
{
   int i, size;

   totalFermMass_kg = 0;

   size = fermObs.size();
   for( i = 0; i < size; ++i )
      totalFermMass_kg += fermObs[i]->amount_kg();
}

void FermentableTableModel::setDisplayPercentages(bool var)
{
   displayPercentages = var;
}

void FermentableTableModel::changedInventory(int invKey, BtStringConst const & propertyName) {

   if (propertyName == PropertyNames::Inventory::amount) {
      for( int i = 0; i < fermObs.size(); ++i ) {
         Fermentable* holdmybeer = fermObs.at(i);
         if ( invKey == holdmybeer->inventoryId() ) {
            emit dataChanged( QAbstractItemModel::createIndex(i,FERMINVENTORYCOL),
                              QAbstractItemModel::createIndex(i,FERMINVENTORYCOL) );
         }
      }
   }
   return;
}

void FermentableTableModel::changed(QMetaProperty prop, QVariant /*val*/)
{
   qDebug() << QString("FermentableTableModel::changed() %1").arg(prop.name());

   int i;

   // Is sender one of our fermentables?
   Fermentable* fermSender = qobject_cast<Fermentable*>(sender());
   if( fermSender )
   {
      i = fermObs.indexOf(fermSender);
      if( i < 0 )
         return;

      updateTotalGrains();
      emit dataChanged( QAbstractItemModel::createIndex(i, 0),
                        QAbstractItemModel::createIndex(i, FERMNUMCOLS-1));
      if( displayPercentages && rowCount() > 0 )
         emit headerDataChanged( Qt::Vertical, 0, rowCount()-1 );
      return;
   }

   // See if our recipe gained or lost fermentables.
   Recipe* recSender = qobject_cast<Recipe*>(sender());
   if( recSender && recSender == recObs && prop.name() == PropertyNames::Recipe::fermentableIds )
   {
      removeAll();
      addFermentables( recObs->fermentables() );
      return;
   }
}

int FermentableTableModel::rowCount(QModelIndex const & /*parent*/) const
{
   return fermObs.size();
}

QVariant FermentableTableModel::data(QModelIndex const & index, int role) const {
   // Ensure the row is OK
   if (index.row() >= static_cast<int>(fermObs.size() )) {
      qCritical() << Q_FUNC_INFO << "Bad model index. row = " << index.row();
      return QVariant();
   }

   Fermentable* row = this->fermObs[index.row()];
   if (row == nullptr) {
      // This is probably a coding error
      qCritical() << Q_FUNC_INFO << "Null pointer at row" << index.row();
      return QVariant();
   }

   int const column = index.column();
   switch (column) {
      case FERMNAMECOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->name());
         }
         break;
      case FERMTYPECOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->typeStringTr());
         }
         if (role == Qt::UserRole) {
            return QVariant(row->type());
         }
         break;
      case FERMINVENTORYCOL:
         if (role == Qt::DisplayRole) {
            // So just query the columns
            return QVariant(
               Measurement::displayAmount(row->inventory(),
                                          &Measurement::Units::kilograms,
                                          3,
                                          this->getForcedSystemOfMeasurementForColumn(column),
                                          this->getForcedRelativeScaleForColumn(column))
            );
         }
         break;
      case FERMAMOUNTCOL:
         if (role == Qt::DisplayRole) {
            // So just query the columns
            return QVariant(
               Measurement::displayAmount(row->amount_kg(),
                                          &Measurement::Units::kilograms,
                                          3,
                                          this->getForcedSystemOfMeasurementForColumn(column),
                                          this->getForcedRelativeScaleForColumn(column))
            );
         }
         break;
      case FERMISMASHEDCOL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->additionMethodStringTr());
         }
         if (role == Qt::UserRole) {
            return QVariant(row->additionMethod());
         }
         break;
      case FERMAFTERBOIL:
         if (role == Qt::DisplayRole) {
            return QVariant(row->additionTimeStringTr());
         }
         if (role == Qt::UserRole) {
            return QVariant(row->additionTime());
         }
         break;
      case FERMYIELDCOL:
         if (role == Qt::DisplayRole) {
            return QVariant(Measurement::displayAmount(row->yield_pct(), nullptr));
         }
         break;
      case FERMCOLORCOL:
         if (role == Qt::DisplayRole) {
            return QVariant(
               Measurement::displayAmount(row->color_srm(),
                                          &Measurement::Units::srm,
                                          0,
                                          this->getForcedSystemOfMeasurementForColumn(column),
                                          std::nullopt)
            );
         }
         break;
      default :
         qCritical() << Q_FUNC_INFO << "Bad column: " << column;
         break;
   }
   return QVariant();
}

QVariant FermentableTableModel::headerData( int section, Qt::Orientation orientation, int role ) const {
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
      return this->getColumName(section);
   }

   if (displayPercentages && orientation == Qt::Vertical && role == Qt::DisplayRole) {
      double perMass = 0.0;
      if (totalFermMass_kg > 0.0 ) {
         perMass = fermObs[section]->amount_kg()/totalFermMass_kg;
      }
      return QVariant( QString("%1%").arg( static_cast<double>(100.0) * perMass, 0, 'f', 0 ) );
   }

   return QVariant();
}

Qt::ItemFlags FermentableTableModel::flags(const QModelIndex& index ) const
{
   Qt::ItemFlags defaults = Qt::ItemIsEnabled;
   int col = index.column();
   Fermentable* row = fermObs[index.row()];

   switch(col)
   {
      case FERMISMASHEDCOL:
         // Ensure that being mashed and being a late addition are mutually exclusive.
         if( !row->addAfterBoil() )
            return (defaults | Qt::ItemIsSelectable | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled);
         else
            return Qt::ItemIsSelectable | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled;
      case FERMAFTERBOIL:
         // Ensure that being mashed and being a late addition are mutually exclusive.
         if( !row->isMashed() )
            return (defaults | Qt::ItemIsSelectable | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled);
         else
            return Qt::ItemIsSelectable | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled;
      case FERMNAMECOL:
         return (defaults | Qt::ItemIsSelectable);
      case FERMINVENTORYCOL:
         return (defaults | (_inventoryEditable ? Qt::ItemIsEditable : Qt::NoItemFlags));
      default:
         return (defaults | Qt::ItemIsSelectable | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags) );
   }
}


bool FermentableTableModel::setData(QModelIndex const & index, QVariant const & value, int role) {
   if (index.row() >= static_cast<int>(this->fermObs.size())) {
      return false;
   }

   bool retVal = false;
   Fermentable* row = fermObs[index.row()];

   int const column = index.column();
   switch (column) {
      case FERMNAMECOL:
         retVal = value.canConvert(QVariant::String);
         if (retVal) {
            MainWindow::instance().doOrRedoUpdate(*row,
                                                  PropertyNames::NamedEntity::name,
                                                  value.toString(),
                                                  tr("Change Fermentable Name"));
         }
         break;
      case FERMTYPECOL:
         retVal = value.canConvert(QVariant::Int);
         if (retVal) {
            // Doing the set via doOrRedoUpdate() saves us from doing a static_cast<Fermentable::Type>() here (as the
            // Q_PROPERTY system will do the casting for us).
            MainWindow::instance().doOrRedoUpdate(*row,
                                                  PropertyNames::Fermentable::type,
                                                  value.toInt(),
                                                  tr("Change Fermentable Type"));
         }
         break;
      case FERMINVENTORYCOL:
         retVal = value.canConvert(QVariant::String);
         if (retVal) {
            // Inventory amount is in kg, but is just called "inventory" rather than "inventory_kg" in the Q_PROPERTY declaration in the Fermentable class
            MainWindow::instance().doOrRedoUpdate(
               *row,
               PropertyNames::NamedEntityWithInventory::inventory,
               Measurement::qStringToSI(value.toString(),
                                        Measurement::PhysicalQuantity::Mass,
                                        this->getForcedSystemOfMeasurementForColumn(column),
                                        this->getForcedRelativeScaleForColumn(column)),
               tr("Change Inventory Amount")
            );
         }
         break;
      case FERMAMOUNTCOL:
         retVal = value.canConvert(QVariant::String);
         if (retVal) {
            // This is where the amount of a fermentable in a recipe gets updated
            // We need to refer back to the MainWindow to make this an undoable operation
            MainWindow::instance().doOrRedoUpdate(
               *row,
               PropertyNames::Fermentable::amount_kg,
               Measurement::qStringToSI(value.toString(),
                                        Measurement::PhysicalQuantity::Mass,
                                        this->getForcedSystemOfMeasurementForColumn(column),
                                        this->getForcedRelativeScaleForColumn(column)),
               tr("Change Fermentable Amount")
            );
            if (rowCount() > 0) {
               headerDataChanged( Qt::Vertical, 0, rowCount()-1 ); // Need to re-show header (grain percent).
            }
         }
         break;
      case FERMISMASHEDCOL:
         retVal = value.canConvert(QVariant::Int);
         if (retVal) {
            // Doing the set via doOrRedoUpdate() saves us from doing a static_cast<Fermentable::AdditionMethod>() here
            // (as the Q_PROPERTY system will do the casting for us).
            MainWindow::instance().doOrRedoUpdate(*row,
                                                  PropertyNames::Fermentable::additionMethod,
                                                  value.toInt(),
                                                  tr("Change Addition Method"));
         }
         break;
      case FERMAFTERBOIL:
         retVal = value.canConvert(QVariant::Int);
         if (retVal) {
            // Doing the set via doOrRedoUpdate() saves us from doing a static_cast<Fermentable::AdditionTime>() here
            // (as the Q_PROPERTY system will do the casting for us).
            MainWindow::instance().doOrRedoUpdate(*row,
                                                  PropertyNames::Fermentable::additionTime,
                                                  value.toInt(),
                                                  tr("Change Addition Time"));
         }
         break;
      case FERMYIELDCOL:
         retVal = value.canConvert(QVariant::Double);
         if (retVal) {
            MainWindow::instance().doOrRedoUpdate(*row,
                                                  PropertyNames::Fermentable::yield_pct,
                                                  value.toDouble(),
                                                  tr("Change Yield"));
         }
         break;
      case FERMCOLORCOL:
         retVal = value.canConvert(QVariant::Double);
         if (retVal) {
            MainWindow::instance().doOrRedoUpdate(
               *row,
               PropertyNames::Fermentable::color_srm,
               Measurement::qStringToSI(value.toString(),
                                        Measurement::PhysicalQuantity::Color,
                                        this->getForcedSystemOfMeasurementForColumn(column),
                                        this->getForcedRelativeScaleForColumn(column)),
               tr("Change Color")
            );
         }
         break;
      default:
         qWarning() << Q_FUNC_INFO << "Bad column: " << index.column();
         return false;
   }
   return retVal;
}

Fermentable* FermentableTableModel::getFermentable(unsigned int i) {
   return fermObs.at(static_cast<int>(i));
}

//======================CLASS FermentableItemDelegate===========================

FermentableItemDelegate::FermentableItemDelegate(QObject* parent) : QItemDelegate(parent) {
   return;
}

QWidget* FermentableItemDelegate::createEditor(QWidget *parent,
                                               QStyleOptionViewItem const & option,
                                               QModelIndex const & index) const {
   if( index.column() == FERMTYPECOL )
   {
      QComboBox *box = new QComboBox(parent);

      box->addItem(tr("Grain"));
      box->addItem(tr("Sugar"));
      box->addItem(tr("Extract"));
      box->addItem(tr("Dry Extract"));
      box->addItem(tr("Adjunct"));

      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      box->setFocusPolicy(Qt::StrongFocus);

      return box;
   }

   if( index.column() == FERMISMASHEDCOL )
   {
      QComboBox* box = new QComboBox(parent);
      QListWidget* list = new QListWidget(parent);
      list->setResizeMode(QListWidget::Adjust);

      list->addItem(tr("Mashed"));
      list->addItem(tr("Steeped"));
      list->addItem(tr("Not mashed"));
      box->setModel(list->model());
      box->setView(list);

      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      box->setFocusPolicy(Qt::StrongFocus);

      // Can we access to the data model into FermentableItemDelegate ? Yes we can !
      int type = index.model()->index(index.row(), FERMTYPECOL).data(Qt::UserRole).toInt();

      // Hide the unsuitable item keeping the same enumeration
      if(type == Fermentable::Grain)
      {
         list->item(Fermentable::Not_Mashed)->setHidden(true);
      }
      else
      {
         list->item(Fermentable::Steeped)->setHidden(true);
      }

      return box;
   }

   if( index.column() == FERMAFTERBOIL )
   {
      QComboBox* box = new QComboBox(parent);

      box->addItem(tr("Normal"));
      box->addItem(tr("Late"));

      box->setMinimumWidth(box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      box->setFocusPolicy(Qt::StrongFocus);

      return box;
   }

   return new QLineEdit(parent);
}

void FermentableItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
   int col = index.column();

   if( col == FERMTYPECOL || col == FERMISMASHEDCOL || col == FERMAFTERBOIL)
   {
      QComboBox* box = static_cast<QComboBox*>(editor);
      int ndx = index.model()->data(index, Qt::UserRole).toInt();

      box->setCurrentIndex(ndx);
   }
   else
   {
      QLineEdit* line = static_cast<QLineEdit*>(editor);

      line->setText(index.model()->data(index, Qt::DisplayRole).toString());
   }
}

void FermentableItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
   int col = index.column();

   if( col == FERMTYPECOL || col == FERMISMASHEDCOL || col == FERMAFTERBOIL )
   {
      QComboBox* box = qobject_cast<QComboBox*>(editor);
      int value = box->currentIndex();
      int ndx = model->data(index, Qt::UserRole).toInt();

     // Only do something when something needs to be done
      if ( value != ndx )
         model->setData(index, value, Qt::EditRole);
   }
   else if( col == FERMISMASHEDCOL || col == FERMAFTERBOIL )
   {
      QComboBox* box = qobject_cast<QComboBox*>(editor);
      int value = box->currentIndex();
      int ndx = model->data(index, Qt::UserRole).toInt();

     // Only do something when something needs to be done
      if ( value != ndx )
         model->setData(index, value, Qt::EditRole);
   }
   else
   {
      QLineEdit* line = qobject_cast<QLineEdit*>(editor);

      if ( line->isModified() )
          model->setData(index, line->text(), Qt::EditRole);
   }
}

void FermentableItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
   editor->setGeometry(option.rect);
}
