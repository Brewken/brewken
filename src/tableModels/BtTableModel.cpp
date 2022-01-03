/*======================================================================================================================
 * tableModels/BtTableModel.cpp is part of Brewken, and is copyright the following authors 2021:
 *   • Matt Young <mfsy@yahoo.com>
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
#include "tableModels/BtTableModel.h"

#include <QAction>
#include <QDebug>
#include <QHeaderView>
#include <QMenu>

#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "measurement/UnitSystem.h"
#include "widgets/UnitAndScalePopUpMenu.h"

BtTableModel::BtTableModel(QTableView * parent,
                           bool editable,
                           std::initializer_list<std::pair<int const, BtTableModel::ColumnInfo> > columnIdToInfo) :
   QAbstractTableModel{parent},
   parentTableWidget{parent},
   editable{editable},
   columnIdToInfo{columnIdToInfo} {
   return;
}

BtTableModel::~BtTableModel() = default;

/* --maf--
   The cell-specific work has been momentarily disabled until I can find a
   better way to implement. PLEASE DO NOT DELETE
*/
Measurement::UnitSystem const * BtTableModel::displayUnitSystem(int column) const {
   QString attribute = this->columnGetAttribute(column);
   if (attribute.isEmpty()) {
      return nullptr;
   }

   return Measurement::getUnitSystemForField(attribute, this->objectName());
}

Measurement::UnitSystem::RelativeScale BtTableModel::displayScale(int column) const {
   QString attribute = columnGetAttribute(column);
   if ( attribute.isEmpty() ) {
      return Measurement::UnitSystem::noScale;
   }

   return Measurement::getRelativeScaleForField(attribute, this->objectName());
}

void BtTableModel::setDisplayUnitSystem(int column, Measurement::UnitSystem const * unitSystem) {
   // Hop* row; // disabled per-cell magic
   QString attribute = this->columnGetAttribute(column);
   if (attribute.isEmpty()) {
      return;
   }

   // If we're changing the UnitSystem then we want to clear the RelativeScale
   Measurement::setUnitSystemForField(attribute, this->objectName(), unitSystem);
   this->setDisplayScale(column, Measurement::UnitSystem::noScale);

   return;
}

// Setting the scale should clear any cell-level scaling options
void BtTableModel::setDisplayScale(int column, Measurement::UnitSystem::RelativeScale displayScale) {
   // Fermentable* row; //disabled per-cell magic
   QString attribute = this->columnGetAttribute(column);
   if (attribute.isEmpty()) {
      return;
   }

   Measurement::setRelativeScaleForField(attribute, this->objectName(), displayScale);

   return;
}

QString BtTableModel::columnGetAttribute(int column) const {
   if (this->columnIdToInfo.contains(column)) {
      return this->columnIdToInfo.value(column).attribute;
   }
   return "";
}

Measurement::PhysicalQuantity BtTableModel::columnGetPhysicalQuantity(int column) const {
   if (this->columnIdToInfo.contains(column)) {
      return this->columnIdToInfo.value(column).physicalQuantity;
   }
   return Measurement::PhysicalQuantity::None;
}


void BtTableModel::doContextMenu(QPoint const & point, QHeaderView * hView, QMenu * menu, int selected) {
   QAction* invoked = menu->exec(hView->mapToGlobal(point));
   if (invoked == nullptr) {
      return;
   }

   QWidget* pMenu = invoked->parentWidget();
   if (pMenu == menu) {
      this->setDisplayUnitSystem(selected,
                                 Measurement::UnitSystem::getInstanceByUniqueName(invoked->data().toString()));
   } else {
      this->setDisplayScale(selected, static_cast<Measurement::UnitSystem::RelativeScale>(invoked->data().toInt()));
   }
   return;
}

// oofrab
void BtTableModel::contextMenu(QPoint const & point) {
   qDebug() << Q_FUNC_INFO;
   QObject* calledBy = sender();
   QHeaderView* hView = qobject_cast<QHeaderView*>(calledBy);
   int selected = hView->logicalIndexAt(point);

   // Since we need to call setupMassMenu() two different ways, we need
   // to figure out the UnitSystem and RelativeScale here
   Measurement::UnitSystem const * forcedUnitSystem  = this->displayUnitSystem(selected);
   Measurement::UnitSystem::RelativeScale forcedScale = this->displayScale(selected);
   Measurement::PhysicalQuantity physicalQuantity = this->columnGetPhysicalQuantity(selected);

   QMenu* menu = new UnitAndScalePopUpMenu(parentTableWidget, physicalQuantity, forcedUnitSystem, forcedScale);
   this->doContextMenu(point, hView, menu, selected);
   return;
}
