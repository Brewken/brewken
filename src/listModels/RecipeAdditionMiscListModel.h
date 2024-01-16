/*======================================================================================================================
 * listModels/RecipeAdditionMiscListModel.h is part of Brewken, and is copyright the following authors 2023-2024:
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
#ifndef LISTMODELS_RECIPEADDITIONMISCLISTMODEL_H
#define LISTMODELS_RECIPEADDITIONMISCLISTMODEL_H
#pragma once

#include <QAbstractListModel>

#include "listModels/ListModelBase.h"
#include "model/RecipeAdditionMisc.h"

/*!
 * \class RecipeAdditionMiscListModel
 *
 * \brief Model for a list of RecipeAdditionMiscs.
 */
class RecipeAdditionMiscListModel : public QAbstractListModel,
                                    public ListModelBase<RecipeAdditionMiscListModel, RecipeAdditionMisc> {
   Q_OBJECT
   LIST_MODEL_COMMON_DECL(RecipeAdditionMisc)
};

#endif
