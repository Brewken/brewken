/**
 * WaterTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Tim Payne <swstim@gmail.com>
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
#include "WaterTableModel.h"

#include <QAbstractItemModel>
#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QLineEdit>
#include <QList>
#include <QModelIndex>
#include <QString>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QWidget>

#include "Brewken.h"
#include "database/Database.h"
#include "model/Recipe.h"
#include "model/Water.h"
#include "PersistentSettings.h"
#include "Unit.h"
#include "WaterTableWidget.h"

WaterTableModel::WaterTableModel(WaterTableWidget* parent)
   : QAbstractTableModel(parent), recObs(nullptr), parentTableWidget(parent)
{
}

void WaterTableModel::observeRecipe(Recipe* rec)
{
   if( recObs )
   {
      disconnect( recObs, nullptr, this, nullptr );
      removeAll();
   }

   recObs = rec;
   if( recObs )
   {
      connect( recObs, &NamedEntity::changed, this, &WaterTableModel::changed );
      addWaters( recObs->waters() );
   }
}

void WaterTableModel::observeDatabase(bool val) {
   if( val ) {
      observeRecipe(nullptr);
      removeAll();
      connect(&DbNamedEntityRecords<Water>::getInstance(), &DbNamedEntityRecords<Water>::signalObjectInserted, this, &WaterTableModel::addWater);
      connect(&DbNamedEntityRecords<Water>::getInstance(), &DbNamedEntityRecords<Water>::signalObjectDeleted,  this, &WaterTableModel::removeWater);
      this->addWaters( DbNamedEntityRecords<Water>::getInstance().getAllRaw() );
   } else {
      removeAll();
      disconnect(&DbNamedEntityRecords<Water>::getInstance(), nullptr, this, nullptr);
   }
   return;
}

void WaterTableModel::addWater(int waterId) {
   Water* water = ObjectStoreWrapper::getByIdRaw<Water>(waterId);
   if( waterObs.contains(water) )
      return;
   // If we are observing the database, ensure that the item is undeleted and
   // fit to display.
   if(
      recObs == nullptr &&
      (
         water->deleted() ||
         !water->display()
      )
   )
      return;

   beginInsertRows( QModelIndex(), waterObs.size(), waterObs.size() );
   waterObs.append(water);
   connect( water, &NamedEntity::changed, this, &WaterTableModel::changed );
   endInsertRows();

   if(parentTableWidget)
   {
      parentTableWidget->resizeColumnsToContents();
      parentTableWidget->resizeRowsToContents();
   }
}

void WaterTableModel::addWaters(QList<Water*> waters)
{
   QList<Water*>::iterator i;
   QList<Water*> tmp;

   for( i = waters.begin(); i != waters.end(); i++ )
   {
      if( !waterObs.contains(*i) )
         tmp.append(*i);
   }

   int size = waterObs.size();
   if (size+tmp.size())
   {
      beginInsertRows( QModelIndex(), size, size+tmp.size()-1 );
      waterObs.append(tmp);

      for( i = tmp.begin(); i != tmp.end(); i++ )
         connect( *i, &NamedEntity::changed, this, &WaterTableModel::changed );

      endInsertRows();
   }

   if( parentTableWidget )
   {
      parentTableWidget->resizeColumnsToContents();
      parentTableWidget->resizeRowsToContents();
   }

}

void WaterTableModel::removeWater(int waterId, std::shared_ptr<QObject> object) {
   Water* water = std::static_pointer_cast<Water>(object).get();
   int i = waterObs.indexOf(water);
   if( i >= 0 )
   {
      beginRemoveRows( QModelIndex(), i, i );
      disconnect( water, nullptr, this, nullptr );
      waterObs.removeAt(i);
      endRemoveRows();

      if(parentTableWidget)
      {
         parentTableWidget->resizeColumnsToContents();
         parentTableWidget->resizeRowsToContents();
      }
   }
}

void WaterTableModel::removeAll()
{
   beginRemoveRows( QModelIndex(), 0, waterObs.size()-1 );
   while( !waterObs.isEmpty() )
   {
      disconnect( waterObs.takeLast(), nullptr, this, nullptr );
   }
   endRemoveRows();
}

void WaterTableModel::changed(QMetaProperty prop, QVariant val)
{
   int i;

   Q_UNUSED(prop)
   Q_UNUSED(val)
   // Find the notifier in the list
   Water* waterSender = qobject_cast<Water*>(sender());
   if( waterSender )
   {
      i = waterObs.indexOf(waterSender);
      if( i >= 0 )
         emit dataChanged( QAbstractItemModel::createIndex(i, 0),
                           QAbstractItemModel::createIndex(i, WATERNUMCOLS-1));
      return;
   }
}

int WaterTableModel::rowCount(const QModelIndex& /*parent*/) const
{
   return waterObs.size();
}

int WaterTableModel::columnCount(const QModelIndex& /*parent*/) const
{
   return WATERNUMCOLS;
}

QVariant WaterTableModel::data( const QModelIndex& index, int role ) const
{
   Water* row;

   // Ensure the row is ok.
   if( index.row() >= waterObs.size() )
   {
      qWarning() << tr("Bad model index. row = %1").arg(index.row());
      return QVariant();
   }
   else
      row = waterObs[index.row()];

   // Make sure we only respond to the DisplayRole role.
   if( role != Qt::DisplayRole )
      return QVariant();

   switch( index.column() )
   {
      case WATERNAMECOL:
         return QVariant(row->name());
      case WATERAMOUNTCOL:
         return QVariant( Brewken::displayAmount(row->amount(), &Units::liters) );
      case WATERCALCIUMCOL:
         return QVariant( Brewken::displayAmount(row->calcium_ppm(), nullptr) );
      case WATERBICARBONATECOL:
         return QVariant( Brewken::displayAmount(row->bicarbonate_ppm(), nullptr) );
      case WATERSULFATECOL:
         return QVariant( Brewken::displayAmount(row->sulfate_ppm(), nullptr) );
      case WATERCHLORIDECOL:
         return QVariant( Brewken::displayAmount(row->chloride_ppm(), nullptr) );
      case WATERSODIUMCOL:
         return QVariant( Brewken::displayAmount(row->sodium_ppm(), nullptr) );
      case WATERMAGNESIUMCOL:
         return QVariant( Brewken::displayAmount(row->magnesium_ppm(), nullptr) );
      default :
         qWarning() << tr("Bad column: %1").arg(index.column());
         return QVariant();
   }
}

QVariant WaterTableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
   if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
   {
      switch( section )
      {
         case WATERNAMECOL:
            return QVariant(tr("Name"));
         case WATERAMOUNTCOL:
            return QVariant(tr("Amount"));
         case WATERCALCIUMCOL:
            return QVariant(tr("Calcium (ppm)"));
         case WATERBICARBONATECOL:
            return QVariant(tr("Bicarbonate (ppm)"));
         case WATERSULFATECOL:
            return QVariant(tr("Sulfate (ppm)"));
         case WATERCHLORIDECOL:
            return QVariant(tr("Chloride (ppm)"));
         case WATERSODIUMCOL:
            return QVariant(tr("Sodium (ppm)"));
         case WATERMAGNESIUMCOL:
            return QVariant(tr("Magnesium (ppm)"));
         default:
            qWarning() << tr("Bad column: %1").arg(section);
            return QVariant();
      }
   }
   else
      return QVariant();
}

Qt::ItemFlags WaterTableModel::flags(const QModelIndex& index ) const
{
   int col = index.column();
   switch(col)
   {
      case WATERNAMECOL:
         return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
      default:
         return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled |
            Qt::ItemIsEnabled;
   }
}

bool WaterTableModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
   Water *row;
   bool retval = false;

   if( index.row() >= waterObs.size() || role != Qt::EditRole )
      return false;
   else
      row = waterObs[index.row()];

   retval = value.canConvert(QVariant::String);
   if ( ! retval )
      return retval;

   Unit::unitDisplay dspUnit = displayUnit(index.column());
   Unit::unitScale   dspScl  = displayScale(index.column());

   switch( index.column() )
   {
      case WATERNAMECOL:
         row->setName(value.toString());
         break;
      case WATERAMOUNTCOL:
         row->setAmount( Brewken::qStringToSI(value.toString(), &Units::liters, dspUnit, dspScl) );
         break;
      case WATERCALCIUMCOL:
         row->setCalcium_ppm( Brewken::toDouble(value.toString(), "WaterTableModel::setData()"));
         break;
      case WATERBICARBONATECOL:
         row->setBicarbonate_ppm(Brewken::toDouble(value.toString(), "WaterTableModel::setData()"));
         break;
      case WATERSULFATECOL:
         row->setSulfate_ppm( Brewken::toDouble(value.toString(), "WaterTableModel::setData()"));
         break;
      case WATERCHLORIDECOL:
         row->setChloride_ppm( Brewken::toDouble(value.toString(), "WaterTableModel::setData()"));
         break;
      case WATERSODIUMCOL:
         row->setSodium_ppm( Brewken::toDouble(value.toString(), "WaterTableModel::setData()"));
         break;
      case WATERMAGNESIUMCOL:
         row->setMagnesium_ppm( Brewken::toDouble(value.toString(), "WaterTableModel::setData()"));
         break;
      default:
         retval = false;
         qWarning() << tr("Bad column: %1").arg(index.column());
   }

   return retval;
}

Unit::unitDisplay WaterTableModel::displayUnit(int column) const
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return Unit::noUnit;

   return static_cast<Unit::unitDisplay>(PersistentSettings::value(attribute, QVariant(-1), this->objectName(), PersistentSettings::UNIT).toInt());
}

Unit::unitScale WaterTableModel::displayScale(int column) const
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return Unit::noScale;

   return static_cast<Unit::unitScale>(PersistentSettings::value(attribute, QVariant(-1), this->objectName(), PersistentSettings::SCALE).toInt());
}

// We need to:
//   o clear the custom scale if set
//   o clear any custom unit from the rows
//      o which should have the side effect of clearing any scale
void WaterTableModel::setDisplayUnit(int column, Unit::unitDisplay displayUnit)
{
   // Yeast* row; // disabled per-cell magic
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return;

   PersistentSettings::insert(attribute, displayUnit, this->objectName(), PersistentSettings::UNIT);
   PersistentSettings::insert(attribute, Unit::noScale, this->objectName(), PersistentSettings::SCALE);

   /* Disabled cell-specific code
   for (int i = 0; i < rowCount(); ++i )
   {
      row = getYeast(i);
      row->setDisplayUnit(Unit::noUnit);
   }
   */
}

// Setting the scale should clear any cell-level scaling options
void WaterTableModel::setDisplayScale(int column, Unit::unitScale displayScale)
{
   // Yeast* row; //disabled per-cell magic

   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return;

   PersistentSettings::insert(attribute,displayScale,this->objectName(),PersistentSettings::SCALE);

   /* disabled cell-specific code
   for (int i = 0; i < rowCount(); ++i )
   {
      row = getYeast(i);
      row->setDisplayScale(Unit::noScale);
   }
   */
}

QString WaterTableModel::generateName(int column) const
{
   QString attribute;

   switch(column)
   {
      case WATERAMOUNTCOL:
         attribute = "amount";
         break;
      default:
         attribute = "";
   }
   return attribute;
}

//==========================CLASS HopItemDelegate===============================

WaterItemDelegate::WaterItemDelegate(QObject* parent)
        : QItemDelegate(parent)
{
}

QWidget* WaterItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
{
   return new QLineEdit(parent);
}

void WaterItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
   QLineEdit* line = qobject_cast<QLineEdit*>(editor);
   line->setText(index.model()->data(index, Qt::DisplayRole).toString());
}

void WaterItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
   QLineEdit* line = qobject_cast<QLineEdit*>(editor);

   if ( line->isModified() )
      model->setData(index, line->text(), Qt::EditRole);
}

void WaterItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex& /*index*/) const
{
   editor->setGeometry(option.rect);
}
