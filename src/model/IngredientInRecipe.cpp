/*======================================================================================================================
 * model/IngredientInRecipe.cpp is part of Brewken, and is copyright the following authors 2024:
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
#include "model/IngredientInRecipe.h"

#include "model/Recipe.h"

QString const IngredientInRecipe::LocalisedName = tr("Ingredient In Recipe");

bool IngredientInRecipe::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   IngredientInRecipe const & rhs = static_cast<IngredientInRecipe const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_ingredientId == rhs.m_ingredientId
   );
}

TypeLookup const IngredientInRecipe::typeLookup {
   "IngredientInRecipe",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::IngredientInRecipe::ingredientId, IngredientInRecipe::m_ingredientId),
   },
   // Parent class lookup
   {&OwnedByRecipe::typeLookup}
};

IngredientInRecipe::IngredientInRecipe(QString name, int const recipeId, int const ingredientId) :
   OwnedByRecipe{name, recipeId},
   m_ingredientId{ingredientId} {
   return;
}

IngredientInRecipe::IngredientInRecipe(NamedParameterBundle const & namedParameterBundle) :
   OwnedByRecipe{namedParameterBundle},
   SET_REGULAR_FROM_NPB(m_ingredientId, namedParameterBundle, PropertyNames::IngredientInRecipe::ingredientId) {
   return;
}

IngredientInRecipe::IngredientInRecipe(IngredientInRecipe const & other) :
   OwnedByRecipe{other},
   m_ingredientId{other.m_ingredientId} {
   return;
}

IngredientInRecipe::~IngredientInRecipe() = default;

int IngredientInRecipe::ingredientId() const { return this->m_ingredientId; }

void IngredientInRecipe::setIngredientId(int const val) { SET_AND_NOTIFY(PropertyNames::IngredientInRecipe::ingredientId, this->m_ingredientId, val); return; }
