/*======================================================================================================================
 * sortFilterProxyModels/StyleSortFilterProxyModel.cpp is part of Brewken, and is copyright the following authors
 * 2009-2023:
 *   • Matt Young <mfsy@yahoo.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
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
#include "sortFilterProxyModels/StyleSortFilterProxyModel.h"

#include "measurement/Measurement.h"
#include "measurement/PhysicalQuantity.h"

bool StyleSortFilterProxyModel::isLessThan(StyleTableModel::ColumnIndex const columnIndex,
                                           QVariant const & leftItem,
                                           QVariant const & rightItem) const {
    switch (columnIndex) {
       case StyleTableModel::ColumnIndex::Name          :
       case StyleTableModel::ColumnIndex::Type          :
       case StyleTableModel::ColumnIndex::Category      :
       case StyleTableModel::ColumnIndex::CategoryNumber:
       case StyleTableModel::ColumnIndex::StyleLetter   :
       case StyleTableModel::ColumnIndex::StyleGuide    :
         return leftItem.toString() < rightItem.toString();
      // No default case as we want the compiler to warn us if we missed one
   }

   // Should be unreachable
   Q_ASSERT(false);
   return true;
}

// Insert the boiler-plate stuff that we cannot do in SortFilterProxyModelBase
SORT_FILTER_PROXY_MODEL_COMMON_CODE(Style)
