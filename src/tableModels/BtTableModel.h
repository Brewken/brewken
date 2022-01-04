/*======================================================================================================================
 * tableModels/BtTableModel.h is part of Brewken, and is copyright the following authors 2021-2022:
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
#ifndef TABLEMODELS_BTTABLEMODEL_H
#define TABLEMODELS_BTTABLEMODEL_H
#pragma once

#include <QAbstractTableModel>
#include <QHeaderView>
#include <QMap>
#include <QMenu>
#include <QPoint>
#include <QTableView>

#include "measurement/UnitSystem.h"


/*!
 * \class BtTableModel
 *
 * \brief Shared interface & code for all the table models we use
 */
class BtTableModel : public QAbstractTableModel {
   Q_OBJECT
public:
   struct ColumnInfo {
      //headerName
      Measurement::PhysicalQuantity physicalQuantity;
      QString attribute;
   };

   BtTableModel(QTableView * parent,
                bool editable,
                std::initializer_list<std::pair<int const, ColumnInfo> > columnIdToInfo);
   virtual ~BtTableModel();

   // Stuff for setting display units and scales -- per cell first, then by
   // column
   Measurement::UnitSystem const * displayUnitSystem(int column) const;
   Measurement::UnitSystem::RelativeScale displayScale(int column) const;
   void setDisplayUnitSystem(int column, Measurement::UnitSystem const * displayUnitSystem);
   void setDisplayScale(int column, Measurement::UnitSystem::RelativeScale displayScale);

private:
   QString                       columnGetAttribute       (int column) const;
   Measurement::PhysicalQuantity columnGetPhysicalQuantity(int column) const;
   void doContextMenu(QPoint const & point, QHeaderView * hView, QMenu * menu, int selected);

public slots:
   //! \brief pops the context menu for changing units and scales
   void contextMenu(QPoint const & point);

protected:
   QTableView* parentTableWidget;
   bool editable;

private:
   QMap<int, ColumnInfo> columnIdToInfo;
};

#endif
