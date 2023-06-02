/*======================================================================================================================
 * tableModels/FermentableTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2023:
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

#include <array>

#include <QAbstractItemModel>
#include <QDebug>
#include <QHeaderView>
#include <QModelIndex>
#include <QString>
#include <QVariant>
#include <QWidget>

#include "database/ObjectStoreWrapper.h"
#include "MainWindow.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "model/Inventory.h"
#include "model/Recipe.h"
#include "tableModels/ItemDelegate.h"
#include "utils/BtStringConst.h"
#include "widgets/BtComboBox.h"

//=====================CLASS FermentableTableModel==============================
FermentableTableModel::FermentableTableModel(QTableView* parent, bool editable) :
   BtTableModelInventory{
      parent,
      editable,
      {
         // NOTE: Need PropertyNames::Fermentable::amountWithUnits not PropertyNames::Fermentable::amount below so we
         //       can handle mass-or-volume generically in TableModelBase.  Same for inventoryWithUnits.
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, Name     , tr("Name"       ), Fermentable, PropertyNames::NamedEntity::name                           ),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, Type     , tr("Type"       ), Fermentable, PropertyNames::Fermentable::type                           , EnumInfo{Fermentable::typeStringMapping, Fermentable::typeDisplayNames}),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, Amount   , tr("Amount"     ), Fermentable, PropertyNames::Fermentable::amountWithUnits                ),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, Inventory, tr("Inventory"  ), Fermentable, PropertyNames::NamedEntityWithInventory::inventoryWithUnits),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, IsWeight , tr("Amount Type"), Fermentable, PropertyNames::Fermentable::amountIsWeight                 , BoolInfo{tr("Volume"    ), tr("Weight")}),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, IsMashed , tr("Method"     ), Fermentable, PropertyNames::Fermentable::isMashed                       , BoolInfo{tr("Not mashed"), tr("Mashed")}),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, AfterBoil, tr("Addition"   ), Fermentable, PropertyNames::Fermentable::addAfterBoil                   , BoolInfo{tr("Normal"    ), tr("Late"  )}),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, Yield    , tr("Yield %"    ), Fermentable, PropertyNames::Fermentable::yield_pct                      , PrecisionInfo{1}),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, Color    , tr("Color"      ), Fermentable, PropertyNames::Fermentable::color_srm                      , PrecisionInfo{1}),
      }
   },
   TableModelBase<FermentableTableModel, Fermentable>{},
   displayPercentages(false),
   totalFermMass_kg(0) {

   // for units and scales
   setObjectName("fermentableTable");

   QHeaderView* headerView = m_parentTableWidget->horizontalHeader();
   connect(headerView, &QWidget::customContextMenuRequested, this, &FermentableTableModel::contextMenu);
   connect(&ObjectStoreTyped<InventoryFermentable>::getInstance(), &ObjectStoreTyped<InventoryFermentable>::signalPropertyChanged, this, &FermentableTableModel::changedInventory);
   return;
}

FermentableTableModel::~FermentableTableModel() = default;

// .:TODO:.:JSON:.  Now that fermentables can also be measured by volume, we might need to rethink this
void FermentableTableModel::added  (std::shared_ptr<Fermentable> item) { if (item->amountIsWeight()) { this->totalFermMass_kg += item->amount(); } return; }
void FermentableTableModel::removed(std::shared_ptr<Fermentable> item) { if (item->amountIsWeight()) { this->totalFermMass_kg -= item->amount(); } return; }
void FermentableTableModel::updateTotals() {
   this->totalFermMass_kg = 0;
   for (auto const & ferm : this->rows) {
      if (ferm->amountIsWeight()) {
         totalFermMass_kg += ferm->amount();
      }
   }
   if (this->displayPercentages && this->rowCount() > 0) {
      emit headerDataChanged(Qt::Vertical, 0, this->rowCount() - 1);
   }
   return;
}

void FermentableTableModel::setDisplayPercentages(bool var) {
   this->displayPercentages = var;
   return;
}

QVariant FermentableTableModel::data(QModelIndex const & index, int role) const {
   if (!this->isIndexOk(index)) {
      return QVariant();
   }

   auto row = this->rows[index.row()];
   auto const columnIndex = static_cast<FermentableTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case FermentableTableModel::ColumnIndex::Name:
      case FermentableTableModel::ColumnIndex::Type:
      case FermentableTableModel::ColumnIndex::IsWeight:
      case FermentableTableModel::ColumnIndex::IsMashed:
      case FermentableTableModel::ColumnIndex::AfterBoil:
      case FermentableTableModel::ColumnIndex::Yield:
      case FermentableTableModel::ColumnIndex::Color:
      case FermentableTableModel::ColumnIndex::Amount:
      case FermentableTableModel::ColumnIndex::Inventory:
         return this->readDataFromModel(index, role);

      // No default case as we want the compiler to warn us if we missed one
   }
   return QVariant();
}

QVariant FermentableTableModel::headerData( int section, Qt::Orientation orientation, int role ) const {
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
      return this->getColumnLabel(section);
   }

   if (displayPercentages && orientation == Qt::Vertical && role == Qt::DisplayRole) {
      double perMass = 0.0;
      if (totalFermMass_kg > 0.0 ) {
         // .:TODO:. Work out what to do for amounts that are volumes
         if (this->rows[section]->amountIsWeight()) {
            perMass = this->rows[section]->amount()/totalFermMass_kg;
         } else {
//            qWarning() << Q_FUNC_INFO << "Unhandled branch for liquid fermentables";
         }
      }
      return QVariant( QString("%1%").arg( static_cast<double>(100.0) * perMass, 0, 'f', 0 ) );
   }

   return QVariant();
}

Qt::ItemFlags FermentableTableModel::flags(QModelIndex const & index) const {
   Qt::ItemFlags constexpr defaults = Qt::ItemIsEnabled;
   auto row = this->rows[index.row()];

   auto const columnIndex = static_cast<FermentableTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case FermentableTableModel::ColumnIndex::IsMashed:
         // Ensure that being mashed and being a late addition are mutually exclusive.
         if (!row->addAfterBoil()) {
            return (defaults | Qt::ItemIsSelectable | (m_editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled);
         }
         return Qt::ItemIsSelectable | (m_editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled;
      case FermentableTableModel::ColumnIndex::AfterBoil:
         // Ensure that being mashed and being a late addition are mutually exclusive.
         if (!row->isMashed()) {
            return (defaults | Qt::ItemIsSelectable | (m_editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled);
         }
         return Qt::ItemIsSelectable | (m_editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled;
      case FermentableTableModel::ColumnIndex::Name:
         return (defaults | Qt::ItemIsSelectable);
      case FermentableTableModel::ColumnIndex::Inventory:
         return (defaults | (this->isInventoryEditable() ? Qt::ItemIsEditable : Qt::NoItemFlags));
      default:
         return (defaults | Qt::ItemIsSelectable | (m_editable ? Qt::ItemIsEditable : Qt::NoItemFlags) );
   }
}


bool FermentableTableModel::setData(QModelIndex const & index,
                                    QVariant const & value,
                                    int role) {
   if (!this->isIndexOk(index)) {
      return false;
   }

   bool retVal = false;

   auto row = this->rows[index.row()];
   Measurement::PhysicalQuantity physicalQuantity =
      row->amountIsWeight() ? Measurement::PhysicalQuantity::Mass : Measurement::PhysicalQuantity::Volume;

   auto const columnIndex = static_cast<FermentableTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case FermentableTableModel::ColumnIndex::Name:
      case FermentableTableModel::ColumnIndex::Type:
      case FermentableTableModel::ColumnIndex::IsWeight:
      case FermentableTableModel::ColumnIndex::IsMashed:
      case FermentableTableModel::ColumnIndex::AfterBoil:
      case FermentableTableModel::ColumnIndex::Yield:
      case FermentableTableModel::ColumnIndex::Color:
         return this->writeDataToModel(index, value, role);

      case FermentableTableModel::ColumnIndex::Inventory:
         return this->writeDataToModel(index, value, role, physicalQuantity);

      case FermentableTableModel::ColumnIndex::Amount:
         retVal = this->writeDataToModel(index, value, role, physicalQuantity);
         if (retVal) {
            if (this->rowCount() > 0) {
               headerDataChanged(Qt::Vertical, 0, this->rowCount() - 1); // Need to re-show header (grain percent).
            }
         }
         break;

      // No default case as we want the compiler to warn us if we missed one
   }
   return retVal;
}

// Insert the boiler-plate stuff that we cannot do in TableModelBase
TABLE_MODEL_COMMON_CODE(Fermentable, fermentable)
//=========================================== CLASS FermentableItemDelegate ============================================

// Insert the boiler-plate stuff that we cannot do in ItemDelegate
ITEM_DELEGATE_COMMON_CODE(Fermentable)
