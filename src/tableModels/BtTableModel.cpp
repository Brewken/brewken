/*======================================================================================================================
 * tableModels/BtTableModel.cpp is part of Brewken, and is copyright the following authors 2021-2022:
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
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
#include "utils/OptionalToStream.h"
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

std::optional<Measurement::SystemOfMeasurement> BtTableModel::getForcedSystemOfMeasurementForColumn(int column) const {
   QString attribute = this->columnGetAttribute(column);
   return attribute.isEmpty() ? std::nullopt : Measurement::getForcedSystemOfMeasurementForField(attribute,
                                                                                                 this->objectName());
}

std::optional<Measurement::UnitSystem::RelativeScale> BtTableModel::getForcedRelativeScaleForColumn(int column) const {
   QString attribute = this->columnGetAttribute(column);
   return attribute.isEmpty() ? std::nullopt : Measurement::getForcedRelativeScaleForField(attribute,
                                                                                           this->objectName());
}

void BtTableModel::setForcedSystemOfMeasurementForColumn(int column, Measurement::SystemOfMeasurement systemOfMeasurement) {
   QString attribute = this->columnGetAttribute(column);
   if (!attribute.isEmpty()) {
      Measurement::setForcedSystemOfMeasurementForField(attribute, this->objectName(), systemOfMeasurement);
      // As we're setting/changing the forced SystemOfMeasurement, we want to clear the forced RelativeScale
      this->unsetForcedRelativeScaleForColumn(column);
   }
   return;
}

void BtTableModel::setForcedRelativeScaleForColumn(int column, Measurement::UnitSystem::RelativeScale relativeScale) {
   QString attribute = this->columnGetAttribute(column);
   if (!attribute.isEmpty()) {
      Measurement::setForcedRelativeScaleForField(attribute, this->objectName(), relativeScale);
   }
   return;
}

void BtTableModel::unsetForcedSystemOfMeasurementForColumn(int column) {
   QString attribute = this->columnGetAttribute(column);
   if (attribute.isEmpty()) {
      Measurement::unsetForcedSystemOfMeasurementForField(attribute, this->objectName());
      // As we're removing the forced SystemOfMeasurement, we want to clear the forced RelativeScale
      this->unsetForcedRelativeScaleForColumn(column);
   }
   return;
}

void BtTableModel::unsetForcedRelativeScaleForColumn(int column) {
   QString attribute = this->columnGetAttribute(column);
   if (!attribute.isEmpty()) {
      Measurement::unsetForcedRelativeScaleForField(attribute, this->objectName());
   }
   return;
}

QVariant BtTableModel::getColumName(int column) const {
   if (this->columnIdToInfo.contains(column)) {
      return QVariant(this->columnIdToInfo.value(column).headerName);
   }

   qWarning() << Q_FUNC_INFO << "Bad column:" << column;
   return QVariant();
}

int BtTableModel::columnCount(QModelIndex const & /*parent*/) const {
   return this->columnIdToInfo.size();
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

   // User will either have selected a SystemOfMeasurement or a UnitSystem::RelativeScale.  We can know which based on
   // whether it's the menu or the sub-menu that it came from.
   bool isTopMenu{invoked->parentWidget() == menu};
   if (isTopMenu) {
      // It's the menu, so SystemOfMeasurement
      std::optional<Measurement::SystemOfMeasurement> whatSelected =
         UnitAndScalePopUpMenu::dataFromQAction<Measurement::SystemOfMeasurement>(*invoked);
      qDebug() << Q_FUNC_INFO << "Selected SystemOfMeasurement" << whatSelected;
      if (!whatSelected) {
         // Null means "Default", which means don't set a forced SystemOfMeasurement for this field
         this->unsetForcedSystemOfMeasurementForColumn(selected);
      } else {
         this->setForcedSystemOfMeasurementForColumn(selected, *whatSelected);
      }
      // Choosing a forced SystemOfMeasurement resets any selection of forced RelativeScale, but this is handled by
      // unsetForcedSystemOfMeasurementForColumn() and setForcedSystemOfMeasurementForColumn()
   } else {
      // It's the sub-menu, so UnitSystem::RelativeScale
      std::optional<Measurement::UnitSystem::RelativeScale> whatSelected =
         UnitAndScalePopUpMenu::dataFromQAction<Measurement::UnitSystem::RelativeScale>(*invoked);
      qDebug() << Q_FUNC_INFO << "Selected RelativeScale" << whatSelected;
      if (!whatSelected) {
         // Null means "Default", which means don't set a forced RelativeScale for this field
         this->unsetForcedRelativeScaleForColumn(selected);
      } else {
         this->setForcedRelativeScaleForColumn(selected, *whatSelected);
      }
   }
   return;
}

// oofrab
void BtTableModel::contextMenu(QPoint const & point) {
   qDebug() << Q_FUNC_INFO;
   QHeaderView* hView = qobject_cast<QHeaderView*>(this->sender());
   int selected = hView->logicalIndexAt(point);
   QMenu * menu = UnitAndScalePopUpMenu::create(parentTableWidget,
                                                this->columnGetPhysicalQuantity(selected),
                                                this->getForcedSystemOfMeasurementForColumn(selected),
                                                this->getForcedRelativeScaleForColumn(selected));
   this->doContextMenu(point, hView, menu, selected);
   return;
}
