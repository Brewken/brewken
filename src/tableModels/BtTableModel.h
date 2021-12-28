/*======================================================================================================================
 * tableModels/BtTableModel.h is part of Brewken, and is copyright the following authors 2021:
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
   BtTableModel(QTableView * parent = nullptr, bool editable = true);
   virtual ~BtTableModel();

   // Stuff for setting display units and scales -- per cell first, then by
   // column
   Measurement::UnitSystem const * displayUnitSystem(int column) const;
   Measurement::UnitSystem::RelativeScale displayScale(int column) const;
   void setDisplayUnitSystem(int column, Measurement::UnitSystem const * displayUnitSystem);
   void setDisplayScale(int column, Measurement::UnitSystem::RelativeScale displayScale);

protected:
   bool editable;
   virtual QString generateName(int column) const = 0;
   void doContextMenu(QPoint const & point, QHeaderView * hView, QMenu * menu, int selected);

};

#endif
