/*======================================================================================================================
 * sortFilterProxyModels/StyleSortFilterProxyModel.h is part of Brewken, and is copyright the following authors
 * 2009-2023:
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
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
#ifndef SORTFILTERPROXYMODELS_STYLESORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODELS_STYLESORTFILTERPROXYMODEL_H
#pragma once

#include <QSortFilterProxyModel>

#include "sortFilterProxyModels/SortFilterProxyModelBase.h"
#include "tableModels/StyleTableModel.h"
#include "listModels/StyleListModel.h"

/*!
 * \class StyleSortFilterProxyModel
 *
 * \brief Proxy model for sorting/filtering Styles.
 */
class StyleSortFilterProxyModel : public QSortFilterProxyModel,
                                  public SortFilterProxyModelBase<StyleSortFilterProxyModel,
                                                                  StyleTableModel,
                                                                  StyleListModel> {
   Q_OBJECT

   SORT_FILTER_PROXY_MODEL_COMMON_DECL(Style)
};

#endif
