/*======================================================================================================================
 * FermentableSortFilterProxyModel.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Jamie Daws <jdelectronics1@gmail.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
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
#include "FermentableSortFilterProxyModel.h"

#include <iostream>

#include <QDebug>

#include "Localization.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "model/Fermentable.h"
#include "tableModels/FermentableTableModel.h"

FermentableSortFilterProxyModel::FermentableSortFilterProxyModel(QObject *parent, bool filt) :
   QSortFilterProxyModel{parent},
   filter{filt} {
   return;
}

bool FermentableSortFilterProxyModel::lessThan(QModelIndex const & left,
                                               QModelIndex const & right) const {
   QVariant leftFermentable = sourceModel()->data(left);
   QVariant rightFermentable = sourceModel()->data(right);

   switch (left.column()) {
      case FERMINVENTORYCOL:
         // If the numbers are equal, compare the names and be done with it
         if (Measurement::qStringToSI(leftFermentable.toString(), Measurement::PhysicalQuantity::Mass) ==
             Measurement::qStringToSI(rightFermentable.toString(), Measurement::PhysicalQuantity::Mass)) {
            return getName(right) < getName(left);
         } else if (Measurement::qStringToSI(leftFermentable.toString(), Measurement::PhysicalQuantity::Mass) == 0.0 &&
                    this->sortOrder() == Qt::AscendingOrder) {
            // Show non-zero entries first.
            return false;
         }
         return Measurement::qStringToSI(leftFermentable.toString(), Measurement::PhysicalQuantity::Mass) <
                Measurement::qStringToSI(rightFermentable.toString(), Measurement::PhysicalQuantity::Mass);

      case FERMAMOUNTCOL:
         // If the numbers are equal, compare the names and be done with it
         if (Measurement::qStringToSI(leftFermentable.toString(), Measurement::PhysicalQuantity::Mass) ==
             Measurement::qStringToSI(rightFermentable.toString(), Measurement::PhysicalQuantity::Mass)) {
            return getName(right) < getName(left);
         }
         return Measurement::qStringToSI(leftFermentable.toString(), Measurement::PhysicalQuantity::Mass) <
                Measurement::qStringToSI(rightFermentable.toString(), Measurement::PhysicalQuantity::Mass);

      case FERMYIELDCOL:
         {
            double leftDouble = toDouble(leftFermentable);
            double rightDouble = toDouble(rightFermentable);

            if (leftDouble == rightDouble) {
               return getName(right) < getName(left);
            }
            return leftDouble < rightDouble;
         }

      case FERMCOLORCOL:
         {
            double leftDouble = Measurement::qStringToSI(leftFermentable.toString(),
                                                         Measurement::PhysicalQuantity::Color);
            double rightDouble = Measurement::qStringToSI(rightFermentable.toString(),
                                                          Measurement::PhysicalQuantity::Color);
            if (leftDouble == rightDouble) {
               return getName(right) < getName(left);
            }
            return leftDouble < rightDouble;
         }
   }

   return leftFermentable.toString() < rightFermentable.toString();
}

double FermentableSortFilterProxyModel::toDouble(QVariant side) const {
   return Localization::toDouble(side.toString(), Q_FUNC_INFO);
}

QString FermentableSortFilterProxyModel::getName( const QModelIndex &index ) const
{
   QVariant info = sourceModel()->data(QAbstractItemModel::createIndex(index.row(),FERMNAMECOL));
   return info.toString();
}

bool FermentableSortFilterProxyModel::filterAcceptsRow( int source_row, const QModelIndex &source_parent) const
{
   FermentableTableModel* model = qobject_cast<FermentableTableModel*>(sourceModel());
   QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

   return !filter
          ||
          (  sourceModel()->data(index).toString().contains(filterRegExp())
             && model->getFermentable(source_row)->display()
          );
}
