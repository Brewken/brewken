/*======================================================================================================================
 * model/RecipeAdditionHop.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef MODEL_HOPINRECIPE_H
#define MODEL_HOPINRECIPE_H
#pragma once

#include "model/Hop.h"
#include "model/InRecipeBase.h"
#include "model/RecipeAddition.h"
#include "model/Recipe.h"

class RecipeAdditionHop : public RecipeAddition, public InRecipeBase<RecipeAdditionHop, Hop> {
   Q_OBJECT
public:
   RecipeAdditionHop(QString name = "");
   RecipeAdditionHop(NamedParameterBundle const & namedParameterBundle);
   RecipeAdditionHop(RecipeAdditionHop const & other);

   virtual ~RecipeAdditionHop();

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

};

#endif
