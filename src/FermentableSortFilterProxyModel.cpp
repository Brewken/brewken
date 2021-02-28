/**
 * FermentableSortFilterProxyModel.cpp is part of Brewken, and is copyright the following authors 2009-2014:
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Jamie Daws <jdelectronics1@gmail.com>
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
 */

#include "unit.h"
#include "FermentableSortFilterProxyModel.h"
#include "FermentableTableModel.h"
#include "fermentable.h"
#include "Brewken.h"
#include <iostream>
#include <QDebug>

FermentableSortFilterProxyModel::FermentableSortFilterProxyModel(QObject *parent, bool filt)
: QSortFilterProxyModel(parent)
{
   filter = filt;
}

bool FermentableSortFilterProxyModel::lessThan(const QModelIndex &left,
                                         const QModelIndex &right) const
{
   QVariant leftFermentable = sourceModel()->data(left);
   QVariant rightFermentable = sourceModel()->data(right);
   double leftDouble, rightDouble;

   Unit* unit = Units::kilograms;
   Unit* colorunit = Units::srm;

   switch( left.column() )
   {
      case FERMINVENTORYCOL:
         // If the numbers are equal, compare the names and be done with it
         if (Brewken::qStringToSI(leftFermentable.toString(), unit) == Brewken::qStringToSI(rightFermentable.toString(),unit))
            return getName(right) < getName(left);
         // Show non-zero entries first.
         else if (Brewken::qStringToSI(leftFermentable.toString(), unit) == 0.0 && this->sortOrder() == Qt::AscendingOrder)
            return false;
         else
            return Brewken::qStringToSI(leftFermentable.toString(),unit) < Brewken::qStringToSI(rightFermentable.toString(),unit);
      case FERMAMOUNTCOL:
         // If the numbers are equal, compare the names and be done with it
         if (Brewken::qStringToSI(leftFermentable.toString(), unit) == Brewken::qStringToSI(rightFermentable.toString(),unit))
            return getName(right) < getName(left);
         else
            return Brewken::qStringToSI(leftFermentable.toString(),unit) < Brewken::qStringToSI(rightFermentable.toString(),unit);
      case FERMYIELDCOL:
         leftDouble = toDouble(leftFermentable);
         rightDouble = toDouble(rightFermentable);

         if (leftDouble == rightDouble)
            return getName(right) < getName(left);
         else
            return leftDouble < rightDouble;
      case FERMCOLORCOL:
         leftDouble = Brewken::qStringToSI(leftFermentable.toString(),colorunit);
         rightDouble = Brewken::qStringToSI(rightFermentable.toString(),colorunit);

         if (leftDouble == rightDouble)
            return getName(right) < getName(left);
         else
            return leftDouble < rightDouble;
   }

   return leftFermentable.toString() < rightFermentable.toString();
}

double FermentableSortFilterProxyModel::toDouble(QVariant side) const
{
   double amt;
   bool ok = false;

   amt = Brewken::toDouble(side.toString(), &ok);
   if ( ! ok )
      qWarning() << QString("FermentableSortFilterProxyModel::lessThan could not convert %1 to double").arg(side.toString());
   return amt;
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
