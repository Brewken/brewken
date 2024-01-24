/*======================================================================================================================
 * model/RecipeUseOfWater.cpp is part of Brewken, and is copyright the following authors 2024:
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
#include "model/RecipeUseOfWater.h"

#include "model/NamedParameterBundle.h"

QString const RecipeUseOfWater::LocalisedName = tr("Recipe Use Of Water");

ObjectStore & RecipeUseOfWater::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<RecipeUseOfWater>::getInstance();
}

TypeLookup const RecipeUseOfWater::typeLookup {
   "RecipeUseOfWater",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::RecipeUseOfWater::recipeId       , RecipeUseOfWater::m_recipeId    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::RecipeUseOfWater::ingredientId   , RecipeUseOfWater::m_ingredientId),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::RecipeUseOfWater::volume_l       , RecipeUseOfWater::m_volume_l    , Measurement::PhysicalQuantity::Volume),
   },
   // Parent classes lookup.
   {&NamedEntity::typeLookup}
};
static_assert(std::is_base_of<NamedEntity, RecipeUseOfWater>::value);

RecipeUseOfWater::RecipeUseOfWater(QString name, int const recipeId, int const ingredientId) :
   NamedEntity{name, true},
   m_recipeId       {recipeId},
   m_ingredientId   {ingredientId},
   m_volume_l       {0.0} {
   return;
}

RecipeUseOfWater::RecipeUseOfWater(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity{namedParameterBundle},
   SET_REGULAR_FROM_NPB (m_recipeId    , namedParameterBundle, PropertyNames::RecipeUseOfWater::recipeId    ),
   SET_REGULAR_FROM_NPB (m_ingredientId, namedParameterBundle, PropertyNames::RecipeUseOfWater::ingredientId),
   SET_REGULAR_FROM_NPB (m_volume_l    , namedParameterBundle, PropertyNames::RecipeUseOfWater::volume_l    ) {
   return;
}

RecipeUseOfWater::RecipeUseOfWater(RecipeUseOfWater const & other) :
   NamedEntity   {other               },
   m_recipeId    {other.m_recipeId    },
   m_ingredientId{other.m_ingredientId},
   m_volume_l    {other.m_volume_l    } {
   return;
}

RecipeUseOfWater::~RecipeUseOfWater() = default;
