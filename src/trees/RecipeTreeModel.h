/*======================================================================================================================
 * trees/RecipeTreeModel.h is part of Brewken, and is copyright the following authors 2024:
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
#ifndef TREES_RECIPETREEMODEL_H
#define TREES_RECIPETREEMODEL_H
#pragma once

#include "model/Recipe.h"
#include "trees/TreeModel.h"
#include "trees/TreeModelBase.h"

class RecipeTreeModel : public TreeModel,
                        public TreeModelBase<RecipeTreeModel, Recipe> {
   Q_OBJECT
   TREE_MODEL_COMMON_DECL(Recipe)
};


#endif
