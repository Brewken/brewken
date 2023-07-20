/*======================================================================================================================
 * model/RecipeAdditionHop.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "model/RecipeAdditionHop.h"

#include "database/ObjectStoreTyped.h"
#include "database/ObjectStoreWrapper.h"
#include "model/NamedParameterBundle.h"

ObjectStore & RecipeAdditionHop::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<RecipeAdditionHop>::getInstance();
}

RecipeAdditionHop::RecipeAdditionHop(QString name) :
   RecipeAdditionMassOrVolume{name},
   InRecipeBase<RecipeAdditionHop, Hop>{} {
   return;
}

RecipeAdditionHop::RecipeAdditionHop(NamedParameterBundle const & namedParameterBundle) :
   RecipeAdditionMassOrVolume{namedParameterBundle} {
   //
   // If the addition stage is not specified then we assume it is boil, as this is the first stage at which it is usual
   // to add hops.
   //
   m_stage = namedParameterBundle.val<RecipeAddition::Stage>(PropertyNames::RecipeAddition::stage,
                                                                  RecipeAddition::Stage::Boil);
   return;
}

RecipeAdditionHop::RecipeAdditionHop(RecipeAdditionHop const & other) :
   RecipeAdditionMassOrVolume{other} {
   return;
}

RecipeAdditionHop::~RecipeAdditionHop() = default;

Recipe * RecipeAdditionHop::getOwningRecipe() const {
   return ObjectStoreWrapper::findFirstMatching<Recipe>( [this](Recipe * rec) {return rec->uses(*this);} );
}
