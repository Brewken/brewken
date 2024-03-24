/*======================================================================================================================
 * listModels/RecipeAdditionFermentableListModel.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef LISTMODELS_RECIPEADDITIONFERMENTABLELISTMODEL_H
#define LISTMODELS_RECIPEADDITIONFERMENTABLELISTMODEL_H
#pragma once

#include <QAbstractListModel>

#include "listModels/ListModelBase.h"
#include "model/RecipeAdditionFermentable.h"

/*!
 * \class RecipeAdditionFermentableListModel
 *
 * \brief Model for a list of RecipeAdditionFermentables.
 */
class RecipeAdditionFermentableListModel :
   public QAbstractListModel, public ListModelBase<RecipeAdditionFermentableListModel, RecipeAdditionFermentable> {
   Q_OBJECT
   LIST_MODEL_COMMON_DECL(RecipeAdditionFermentable)
};

#endif