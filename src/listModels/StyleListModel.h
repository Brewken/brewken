/*======================================================================================================================
 * listModels/StyleListModel.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef LISTMODELS_STYLELISTMODEL_H
#define LISTMODELS_STYLELISTMODEL_H
#pragma once

#include <QAbstractListModel>

#include "listModels/ListModelBase.h"
#include "model/Style.h"

/*!
 * \class StyleListModel
 *
 * \brief Model for a list of styles.
 */
class StyleListModel : public QAbstractListModel, public ListModelBase<StyleListModel, Style> {
   Q_OBJECT
   LIST_MODEL_COMMON_DECL(Style)
};

#endif